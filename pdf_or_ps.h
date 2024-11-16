#ifndef PDF_OR_PS_H
#define PDF_OR_PS_H

#include <FL/Fl_Paged_Device.H>

#if defined(WIN32)
#include <windows.h>
#endif

#ifdef NO_PDF
#include <FL/Fl_PostScript.H>
class PDF_or_PS_File_Device : public Fl_PostScript_File_Device {
  FILE *file;
  char *filename;
public:
  PDF_or_PS_File_Device() { file = NULL; }
  int begin_document(const char* fixedfilename, enum Fl_Paged_Device::Page_Format format=Fl_Paged_Device::A4,
		     enum Fl_Paged_Device::Page_Layout layout=Fl_Paged_Device::PORTRAIT);
  int start_job(const char *defaultname, enum Fl_Paged_Device::Page_Format format=Fl_Paged_Device::A4, 
		enum Fl_Paged_Device::Page_Layout layout=Fl_Paged_Device::PORTRAIT);
  Fl_Graphics_Driver *driver() {  return (Fl_Graphics_Driver*)Fl_PostScript_File_Device::driver(); }
  const char *outfname() {return filename; }
  ~PDF_or_PS_File_Device();
};

#else // NO_PDF

#include <pdflib.h>
#include <setjmp.h>
#if 100*FL_MAJOR_VERSION + 10*FL_MINOR_VERSION + FL_PATCH_VERSION >= 140
#  include <FL/Fl_Graphics_Driver.H>
#else
#  include <FL/Fl_Device.H>
#endif

class PDF_Graphics_Driver : public Fl_Graphics_Driver {
private:
  int pdf_font;
  const char *encoding;
  bool in_page;
  int tr_x, tr_y;
public:
  PDF *pdf;
  PDF_Graphics_Driver();
  void rect(int x, int y, int w, int h);
  void rectf(int x, int y, int w, int h);
  void line_style(int style, int width, char *dashes=0);
  void line(int x1, int y1, int x2, int y2);
  void xyline(int x, int y, int x1);
  void xyline(int x, int y, int x1, int y2);
  void yxline(int x, int y, int y1);
  void yxline(int x, int y, int y1, int x2);
  void font(int f, int s);
  void draw(const char *str, int n, int x, int y);
  void draw(const char*, int, float, float) ;
  void draw(int, const char*, int, int, int) ;
  void rtl_draw(const char*, int, int, int) ;
  void color(uchar r, uchar g, uchar b);
  void color(Fl_Color c);
  void push_clip(int x, int y, int w, int h) ;
  void pop_clip();
  void draw_image(const uchar*, int, int, int, int, int, int) ;
  void draw_image_mono(const uchar*, int, int, int, int, int, int) ;
  void draw_image(void (*)(void*, int, int, int, uchar*), void*, int, int, int, int, int) ;
  void draw_image_mono(void (*)(void*, int, int, int, uchar*), void*, int, int, int, int, int) ;
  void draw(Fl_RGB_Image*, int, int, int, int, int, int) ;
  void draw(Fl_Pixmap*, int, int, int, int, int, int) ;
  void draw(Fl_Bitmap*, int, int, int, int, int, int) ;
  double width(const char*, int) ;
  int height() ;
  int descent() ;
  friend class Fl_PDF_File_Device;
  
#if 100*FL_MAJOR_VERSION + 10*FL_MINOR_VERSION + FL_PATCH_VERSION >= 140
  virtual void point(int x, int y) {}
  virtual void line(int x, int y, int x1, int y1, int x2, int y2) {}
  virtual void xyline(int x, int y, int x1, int y2, int x3) {}
  virtual void yxline(int x, int y, int y1, int x2, int y3) {}
  virtual void loop(int x0, int y0, int x1, int y1, int x2, int y2) {}
  virtual void loop(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3) {}
  virtual void polygon(int x0, int y0, int x1, int y1, int x2, int y2) {}
  virtual void polygon(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3) {}
  virtual int clip_box(int x, int y, int w, int h, int &X, int &Y, int &W, int &H) {return 0;}
  virtual int not_clipped(int x, int y, int w, int h) {return 0;}
  virtual void push_no_clip() {}
  virtual void begin_complex_polygon() {}
  virtual void transformed_vertex(double xf, double yf) {}
  virtual void vertex(double x, double y) {}
  virtual void end_points() {}
  virtual void end_line() {}
  virtual void end_loop() {}
  virtual void end_polygon() {}
  virtual void end_complex_polygon() {}
  virtual void gap() {}
  virtual void circle(double x, double y, double r) {}
  virtual void arc(int x, int y, int w, int h, double a1, double a2) {}
  virtual void pie(int x, int y, int w, int h, double a1, double a2) {}
  virtual Fl_Bitmask create_bitmask(int w, int h, const uchar *array) {return 0;}
  virtual void delete_bitmask(Fl_Bitmask bm) {}
#endif
};

class Fl_PDF_File_Device : public Fl_Paged_Device
{
  int width, height;
  Fl_Surface_Device *previous_surface;
  int left_margin, top_margin;
  char *filename;
public:
  Fl_PDF_File_Device();
  int start_job(const char *defaultname, enum Fl_Paged_Device::Page_Format format=Fl_Paged_Device::A4, 
		enum Fl_Paged_Device::Page_Layout layout=Fl_Paged_Device::PORTRAIT);
  int begin_document(const char* fixedfilename, enum Fl_Paged_Device::Page_Format format=Fl_Paged_Device::A4,
		     enum Fl_Paged_Device::Page_Layout layout=Fl_Paged_Device::PORTRAIT);
  int begin_custom(const char* fixedfilename, int pwidth, int pheight);
  int printable_rect(int *w, int *h);
  void margins(int *left, int *top, int *right, int *bottom);
  void origin(int x, int y);
  void origin (int *x, int *y);
  void translate(int x, int y);
  void untranslate();
  int start_page (void);
  int end_page (void);
  void end_job (void);
  void error_catch();
  int surface_catch();
  void surface_try(jmp_buf** b);
  void *pdf();
  const char *outfname() {return filename; }
  ~Fl_PDF_File_Device();
};
typedef Fl_PDF_File_Device PDF_or_PS_File_Device;
#endif // NO_PDF

#if defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
#undef check
#if  (!__LP64__) && MAC_OS_X_VERSION_MAX_ALLOWED < 1070
#define USE_PICT
#endif
#endif

#if defined(__APPLE__) && defined(USE_PICT)
class Copy_Surface : public Fl_Surface_Device {
  PicHandle mypicture;
  void MyCopyPictToClipboard();
  QDPictRef MyPictToQDPict();
  CGContextRef init_PDF_context(int w, int h, CFMutableDataRef &pdfdata);
public:
  Copy_Surface(int w, int h);
  ~Copy_Surface();
};
#endif


#endif // PDF_OR_PS_H

