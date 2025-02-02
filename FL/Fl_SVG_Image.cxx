#include <FL/Enumerations.H>
#if 100*FL_MAJOR_VERSION + 10*FL_MINOR_VERSION + FL_PATCH_VERSION < 140

#define HAVE_LIBZ 1
#define HAVE_LONG_LONG 1

#include <FL/Fl_SVG_Image.H>
#include <FL/fl_utf8.h>
#include <FL/fl_draw.H>
#include <stdio.h>
#include <stdlib.h>
#if defined(HAVE_LIBZ)
#include <zlib.h>
#endif

#if !defined(HAVE_LONG_LONG)
static double strtoll(const char *str, char **endptr, int base) {
  return (double)strtol(str, endptr, base);
}
#endif

#define NANOSVG_ALL_COLOR_KEYWORDS	// Include full list of color keywords.
#define NANOSVG_IMPLEMENTATION		// Expands implementation
#include "FL/nanosvg.h"

#define NANOSVGRAST_IMPLEMENTATION	// Expands implementation
#include "FL/altsvgrast.h"


/** The constructor loads the SVG image from the given .svg/.svgz filename or in-memory data.
 \param filename The full path and name of a .svg or .svgz file, or NULL.
 \param svg_data A pointer to the memory location of the SVG image data.
 This parameter allows to load an SVG image from in-memory data, and is used when \p filename is NULL.
 \note In-memory SVG data is parsed by the object constructor and is not used after construction.
 */
Fl_SVG_Image::Fl_SVG_Image(const char *filename, const char *svg_data) : Fl_RGB_Image(NULL, 0, 0, 4) {
  init_(filename, svg_data, NULL);
}


// private constructor
Fl_SVG_Image::Fl_SVG_Image(Fl_SVG_Image *source) : Fl_RGB_Image(NULL, 0, 0, 4) {
  init_(NULL, NULL, source);
}


/** The destructor frees all memory and server resources that are used by the SVG image. */
Fl_SVG_Image::~Fl_SVG_Image() {
  if ( --counted_svg_image_->ref_count <= 0) {
    nsvgDelete(counted_svg_image_->svg_image);
    delete counted_svg_image_;
  }
}


float Fl_SVG_Image::svg_scaling_(int W, int H) {
  float f1 = float(W) / int(counted_svg_image_->svg_image->width+0.5);
  float f2 = float(H) / int(counted_svg_image_->svg_image->height+0.5);
  return (f1 < f2) ? f1 : f2;
}

#if defined(HAVE_LIBZ)

static char *svg_inflate(const char *fname) {
  FILE *in = fl_fopen(fname, "r");
  if (!in) return NULL;
  unsigned char header[2];
  if (fread(header, 2, 1, in) < 1) {	// FIXME: can't read file header
    header[0] = header[1] = 0;		// FIXME: continuing anyway ?
  }
  int direct = (header[0] != 0x1f || header[1] != 0x8b);
  fseek(in, 0, SEEK_END);
  long size = ftell(in);
  fclose(in);
  int fd = fl_open(fname, 0);
  if (fd < 0) return NULL;
  gzFile gzf =  gzdopen(fd, "r");
  if (!gzf) return NULL;
  int l;
  long out_size = direct ? size + 1 : 3*size + 1;
  char *out = (char*)malloc(out_size);
  char *p = out;
  do {
    if ((!direct) && p + size > out + out_size) {
      out_size += size;
      char *tmp = (char*)realloc(out, out_size + 1);
      p = tmp + (p - out);
      out = tmp;
    }
    l = gzread(gzf, p, size);
    if (l > 0) {
      p += l; *p = 0;
    }
  } while ((!direct) && l >0);
  gzclose(gzf);
  if (!direct) out = (char*)realloc(out, (p-out)+1);
  return out;
}
#endif

void Fl_SVG_Image::init_(const char *filename, const char *in_filedata, Fl_SVG_Image *copy_source) {
  if (copy_source) {
    filename = in_filedata = NULL;
    counted_svg_image_ = copy_source->counted_svg_image_;
    counted_svg_image_->ref_count++;
  } else {
    counted_svg_image_ = new counted_NSVGimage;
    counted_svg_image_->svg_image = NULL;
    counted_svg_image_->ref_count = 1;
  }
  char *filedata = NULL;
  to_desaturate_ = false;
  average_weight_ = 1;
  proportional = true;
  if (filename) {
#if defined(HAVE_LIBZ)
    filedata = svg_inflate(filename);
#else
    FILE *fp = fl_fopen(filename, "rb");
    if (fp) {
      fseek(fp, 0, SEEK_END);
      long size = ftell(fp);
      fseek(fp, 0, SEEK_SET);
      filedata = (char*)malloc(size+1);
      if (filedata) {
        if (fread(filedata, 1, size, fp) == size) {
          filedata[size] = '\0';
        } else {
          free(filedata);
          filedata = NULL;
        }
      }
      fclose(fp);
    }
#endif // HAVE_LIBZ
    if (!filedata) ld(ERR_FILE_ACCESS);
  } else {
    // XXX: Make internal copy -- nsvgParse() modifies filedata during parsing (!)
    filedata = in_filedata ? strdup(in_filedata) : NULL;
  }
  if (filedata) {
    counted_svg_image_->svg_image = nsvgParse(filedata, "px", 96);
    free(filedata);	// made with svg_inflate|malloc|strdup
    if (counted_svg_image_->svg_image->width == 0 || counted_svg_image_->svg_image->height == 0) {
      d(-1);
      ld(ERR_FORMAT);
    } else {
      w(counted_svg_image_->svg_image->width + 0.5);
      h(counted_svg_image_->svg_image->height + 0.5);
    }
  } else if (copy_source) {
    w(copy_source->w());
    h(copy_source->h());
  }
  rasterized_ = false;
}


void Fl_SVG_Image::rasterize_(int W, int H) {
  static NSVGrasterizer *rasterizer = nsvgCreateRasterizer();
  double fx, fy;
  if (proportional) {
    fx = svg_scaling_(W, H);
    fy = fx;
  } else {
    fx = (double)W / counted_svg_image_->svg_image->width;
    fy = (double)H / counted_svg_image_->svg_image->height;
  }
  array = new uchar[W*H*4];
  nsvgAltRasterize(rasterizer, counted_svg_image_->svg_image, 0, 0, fx, fy, (uchar* )array, W, H, W*4);
  alloc_array = 1;
  data((const char * const *)&array, 1);
  d(4);
  if (to_desaturate_) Fl_RGB_Image::desaturate();
  if (average_weight_ < 1) Fl_RGB_Image::color_average(average_color_, average_weight_);
  rasterized_ = true;
  raster_w_ = W;
  raster_h_ = H;
//printf("rasterize to %dx%d\n",W, H);
}


Fl_Image *Fl_SVG_Image::copy(int W, int H) {
  Fl_SVG_Image *svg2 = new Fl_SVG_Image(this);
  svg2->to_desaturate_ = to_desaturate_;
  svg2->average_weight_ = average_weight_;
  svg2->average_color_ = average_color_;
  svg2->proportional = proportional;
  svg2->w(W); svg2->h(H);
  return svg2;
}


/** Have the svg data (re-)rasterized using the given width and height values.
 By default, the resulting image w() and h() will preserve the width/height ratio
 of the SVG data.
 If \ref proportional was set to \c false, the image is rasterized to the given \c width
 and \c height values.*/
void Fl_SVG_Image::resize(int width, int height) {
  if (ld() < 0) {
    return;
  }
  int w1 = width, h1 = height;
  if (proportional) {
    float f = svg_scaling_(width, height);
    w1 = int( int(counted_svg_image_->svg_image->width+0.5)*f + 0.5 );
    h1 = int( int(counted_svg_image_->svg_image->height+0.5)*f + 0.5 );
  }
  w(w1); h(h1);
  if (rasterized_ && w1 == raster_w_ && h1 == raster_h_) return;
  if (array) {
    delete[] array;
    array = NULL;
  }
  uncache();
  rasterize_(w1, h1);
}


void Fl_SVG_Image::draw(int X, int Y, int W, int H, int cx, int cy) {
  float f = 1;
  /*if (Fl_Surface_Device::surface() == Fl_Display_Device::display_device()) {
    f = Fl::screen_driver()->retina_factor() * fl_graphics_driver->scale();
  }*/
  int w1 = w(), h1 = h();
  /* When f > 1, there may be several pixels per drawing unit in an area
   of size w() x h() of the display. This occurs, e.g., with Apple retina displays
   and when the display is rescaled.
   The SVG is rasterized to the area dimension in pixels. The image is then drawn
   scaled to its size expressed in drawing units. With this procedure,
   the SVG image is drawn using the full resolution of the display.
   */
  resize(f*w(), f*h());
  if (f == 1) {
    Fl_RGB_Image::draw(X, Y, W, H, cx, cy);
  } else {
    bool need_clip = (cx || cy || W != w1 || H != h1);
    if (need_clip) fl_push_clip(X, Y, W, H);
    fl_graphics_driver->draw_scaled(this, X-cx, Y-cy, w1, h1);
    if (need_clip) fl_pop_clip();
    w(w1); h(h1); // restore the previous image size
  }
}


void Fl_SVG_Image::desaturate() {
  to_desaturate_ = true;
  Fl_RGB_Image::desaturate();
}


void Fl_SVG_Image::color_average(Fl_Color c, float i) {
  average_color_ = c;
  average_weight_ = i;
  Fl_RGB_Image::color_average(c, i);
}


int Fl_SVG_Image::draw_scaled(int X, int Y, int W, int H) {
  w(W);
  h(H);
  draw(X, Y, W, H, 0, 0);
  return 1;
}

#endif // 100*FL_MAJOR_VERSION + 10*FL_MINOR_VERSION + FL_PATCH_VERSION < 140
