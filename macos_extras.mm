#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
#include "seaview.h"
#include "pdf_or_ps.h"
#include <FL/x.H>
#include <FL/Fl_Sys_Menu_Bar.H>
#include <FL/Fl_Help_View.H>
#include <FL/filename.H>
#include <FL/fl_draw.H>
#include <sys/stat.h>
#include <unistd.h>

// included functions
char *mac_fname_to_roman(const char *in);
const char *MG_GetBundleResourcesDir(void);
static void file_receive_cb(const char *fname);
static void show_apropos(Fl_Widget *, void *unused);
void MG_apple_inits(void);
void set_seaview_modified(SEA_VIEW *view, int ismodified);
#if 100*FL_MAJOR_VERSION + FL_MINOR_VERSION < 104
  void windowmenuitem_callback(Fl_Widget *o, void *data);
  int find_windowmenuitem(Fl_Window *w);
  int add_windowmenuitem(const char *name, Fl_Window *w);
  void rename_windowmenuitem(const char *name, int rank);
  void delete_windowmenuitem(int rank);
#endif
char *mac_GetOutputFName_Plus(const char *dfault, const char *message, int, const char *directory);

/* extern functions */
extern void hide_window_callback(Fl_Widget *ob, void *data);
extern char *get_res_value(const char *name, const char *def_value);
extern int printout_block, printout_fontsize;
extern int printout_vary, printout_black;
extern Fl_Paged_Device::Page_Format printout_pageformat;
extern Fl_Paged_Device::Page_Layout printout_layout;
extern Fl_Window *use_initial_file(SEA_VIEW *view, char *masename, int doing_dnd);

/* globals */

char *mac_fname_to_roman(const char *in)
/* passage codage pathname vers codage MacRoman qui est necessaire pour PDFlib/PostScript 
 */
{
  static char out[250];
  CFStringRef mycfs;
  Boolean ok;
  /* les pathnames sont codes en UTF8 */
  mycfs = CFStringCreateWithCString(NULL, in, kCFStringEncodingUTF8);
  /* conversion vers MacRoman */
  ok = CFStringGetCString(mycfs, out, sizeof(out), kCFStringEncodingMacRoman);
  CFRelease(mycfs);
  return (ok ? out : (char *)in);
}


const char *MG_GetBundleResourcesDir(void)
{
  static char *path = NULL;
  if (!path) {
    path = strdup([[[NSBundle mainBundle] resourcePath] UTF8String]);
    }
  return path;
}


static void label(int x, int y, int w, int h, const char *text, NSView *view)
{
  NSRect rect = {{x, y}, {w, h}};
  NSTextField *label = [[[NSTextField alloc] initWithFrame:rect] autorelease];
  [label setStringValue:[[[NSString alloc] initWithUTF8String:text] autorelease]];
  [label setEditable:NO];
  [label setSelectable:NO];
  [label setBordered:NO];
  [label setDrawsBackground:NO];
  [view addSubview:label];
}

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 101200
const int radio_button_style = NSButtonTypeRadio;
const int switch_button_style = NSButtonTypeSwitch;
const int file_dialog_OK = NSModalResponseOK;
#else
const int radio_button_style = NSRadioButton;
const int switch_button_style = NSSwitchButton;
const int file_dialog_OK = NSFileHandlingPanelOKButton;
#endif

char *mac_GetOutputFName_Plus(const char *dfault, const char *message, int use_only_button, const char *directory)
{
  static char pathname[FL_PATH_MAX];
  NSSavePanel *_panel =  [NSSavePanel savePanel];
  CFStringRef cfs_title;
  cfs_title = CFStringCreateWithCString(NULL,message,kCFStringEncodingUTF8);
  [_panel setTitle:(NSString*)cfs_title];
  CFRelease(cfs_title);
  [_panel setNameFieldLabel:@"Output To:"];
  NSString *dir = [[NSString alloc] initWithUTF8String:directory];
  NSString *preset = [[NSString alloc] initWithUTF8String:dfault];
  NSString *fname = [preset lastPathComponent];
  NSRect rectview = {{0, 0}, {513, 65} };
  NSView *view = [[[NSView alloc] initWithFrame:rectview] autorelease];
//blocksize  
  label(5, 28, 75, 15, "block size:", view);
  NSRect rectblock = {{68, 25}, {35, 20} };
  NSTextField *blockview = [[[NSTextField alloc] initWithFrame:rectblock] autorelease];
  [view addSubview:blockview];
  [blockview setIntValue:printout_block];
//fontsize
  label(109, 28, 75, 15, "font size:", view);
  NSRect rectfont = {{165, 25}, {35, 20} };
  NSTextField *fontview = [[[NSTextField alloc] initWithFrame:rectfont] autorelease];
  [view addSubview:fontview];
  [fontview setIntValue:printout_fontsize];
//color  
  NSRect pdfrect = {{215, -10}, {80, 70} };
  NSMatrix *pdfmatrix = [[[NSMatrix alloc] initWithFrame:pdfrect mode:NSRadioModeMatrix 
						cellClass:[NSButtonCell class]
					 numberOfRows:3 numberOfColumns:1] autorelease];
  [pdfmatrix setAllowsEmptySelection:NO];
  NSSize spacing= {0, 2};
  [pdfmatrix setIntercellSpacing:spacing];
  NSSize cellsize= {80, 16};
  [pdfmatrix setCellSize:cellsize];
  NSButton *color = [[[NSButton alloc] init] autorelease];
  [color setButtonType:radio_button_style];
  [color setTitle:@"PDF color"];
  [pdfmatrix putCell:[color cell] atRow:0 column:0];
  NSButton *bw = [[[NSButton alloc] init] autorelease];
  [bw setButtonType:radio_button_style];
  [bw setTitle:@"PDF B&W"];
  [pdfmatrix putCell:[bw cell] atRow:1 column:0];
  NSButton *textfile = [[[NSButton alloc] init] autorelease];
  [textfile setButtonType:radio_button_style];
  [textfile setTitle:@"Text File"];
  [pdfmatrix putCell:[textfile cell] atRow:2 column:0];
  [pdfmatrix selectCell:[bw cell]];
  if([pdfmatrix selectedRow] != printout_black) [pdfmatrix selectCellAtRow:printout_black column:0];
  [view addSubview:pdfmatrix];
//paper format  
  NSRect paperrect = {{305, -10}, {80, 70} };
  NSMatrix *papermatrix = [[[NSMatrix alloc] initWithFrame:paperrect mode:NSRadioModeMatrix 
						 cellClass:[NSButtonCell class]
					      numberOfRows:2 numberOfColumns:1] autorelease];
  [papermatrix setAllowsEmptySelection:NO];
  [papermatrix setIntercellSpacing:spacing];
  [papermatrix setCellSize:cellsize];
  NSButton *A4 = [[[NSButton alloc] init] autorelease];
  [A4 setButtonType:radio_button_style];
  [A4 setTitle:@"A4"];
  [papermatrix putCell:[A4 cell] atRow:0 column:0];
  NSButton *LETTER = [[[NSButton alloc] init] autorelease];
  [LETTER setButtonType:radio_button_style];
  [LETTER setTitle:@"LETTER"];
  [papermatrix putCell:[LETTER cell] atRow:1 column:0];
  [papermatrix selectCell:[LETTER cell]];
  int desired_paper_cell = (printout_pageformat == Fl_Paged_Device::A4 ? 0 : 1);
  if ([papermatrix selectedRow] != desired_paper_cell) [papermatrix selectCellAtRow:desired_paper_cell column:0];
  [view addSubview:papermatrix];
// landscape button
  NSRect landscaperect = {{305, 6}, {80, 20} };
  NSButton *landscape = [[[NSButton alloc] initWithFrame:landscaperect] autorelease];
  [landscape setButtonType:switch_button_style];
  [landscape setTitle:@"landscape"];
  [landscape sizeToFit];
  [landscape setIntValue:(printout_layout == Fl_Paged_Device::LANDSCAPE)];
  [landscape setEnabled:YES];
  [view addSubview:landscape];
//variable site button  
  NSRect variablerect = {{385, 25}, {80, 20} };
  NSButton *variable = [[[NSButton alloc] initWithFrame:variablerect] autorelease];
  [variable setButtonType:switch_button_style];
  [variable setTitle:@"variable sites only"];
  [variable sizeToFit];
  [variable setIntValue:printout_vary];
  [variable setEnabled:use_only_button];
  [view addSubview:variable];
//add accessory view to panel
  [_panel setAccessoryView:view];
//run panel  
  NSWindow *key = [NSApp keyWindow];
  int retval;
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1060 // MAC_OS_X_VERSION_10_6
  if (fl_mac_os_version >= 100600) {
    if (dir) [_panel setDirectoryURL:[NSURL fileURLWithPath:dir]];
    if (fname) [_panel setNameFieldStringValue:fname];
    __block NSInteger complete = -1;
    [_panel beginSheetModalForWindow:key completionHandler:^(NSInteger returnCode) {
        complete = returnCode; // this block runs after OK or Cancel was triggered in file dialog
    }]; // this message returns immediately and begins the file dialog as a sheet
    while (complete < 0) Fl::wait(100); // loop until end of file dialog
    retval = complete;
  }
  else
#endif
  {
    retval = [(id)_panel runModalForDirectory:dir file:fname];
  }
  [key makeKeyWindow];
  [preset release];
  [dir release];
  if ( retval == file_dialog_OK ) {//read accessory view state
    printout_block = [blockview intValue];
    printout_fontsize = [fontview intValue];
    printout_black = [pdfmatrix selectedRow];
    printout_pageformat = ([papermatrix selectedRow] == 0 ? Fl_Paged_Device::A4 : Fl_Paged_Device::LETTER);
    printout_vary = [variable intValue];
    printout_layout = [landscape intValue] ? Fl_Paged_Device::LANDSCAPE : Fl_Paged_Device::PORTRAIT;
    strcpy(pathname, [[[_panel URL] path] UTF8String]);
    }
  [_panel setAccessoryView:nil];
  return (retval == file_dialog_OK ? pathname : NULL);
}

#if 100*FL_MAJOR_VERSION + FL_MINOR_VERSION < 104

void windowmenuitem_callback(Fl_Widget *o, void *data)
{
  ((Fl_Window *)data)->show();
  const char *c = ((Fl_Window *)data)->xclass();
  if(c == NULL) return;
  if(strcmp(c, SEAVIEW_WINDOW) == 0) {
    SEA_VIEW *view = (SEA_VIEW *)((Fl_Window *)data)->user_data();
    view->DNA_obj->take_focus();
    }
}


int find_windowmenuitem(Fl_Window *w)
{
  const Fl_Menu_Item *items = fl_sys_menu_bar->Fl_Menu_::menu();
  int count = fl_sys_menu_bar->Fl_Menu_::size();
  for(int i = 1; i < count; i++) {
    if(items[i].user_data() == w) return i;
  }
  return -1;
}

int add_windowmenuitem(const char *name, Fl_Window *w)
//returns rank of new menu item or -1 if error
{
  if(fl_sys_menu_bar == NULL) new Fl_Sys_Menu_Bar(0,0,0,0,NULL);
  char line[200];
  sprintf(line, "Window/%s", name ? name : "<empty>");
  int val = fl_sys_menu_bar->add(line, 0, windowmenuitem_callback, w);
  return val;
}


void rename_windowmenuitem(const char *name, int rank)
{
  fl_sys_menu_bar->replace(rank, name);
}

void delete_windowmenuitem(int rank)
{
  fl_sys_menu_bar->remove(rank);
}

#endif


static void file_receive_cb(const char *fname)
{
  SEA_VIEW *view;
  Fl_Window *w = Fl::first_window();
  while(w != NULL) {
    const char *c = w->xclass();
    if(c != NULL && strcmp(c, SEAVIEW_WINDOW) == 0) break;
    w = Fl::next_window(w);
  }
  view = (w != NULL ? (SEA_VIEW *)w->user_data() : NULL);
  use_initial_file(view, (char *)fname, false);
}

static void show_apropos(Fl_Widget *w, void *unused)
{
  static Fl_Window *about = NULL;
  char line[100], *p;
  FILE *data;
  static char text[5000];
  
  if(about == NULL) {
	  about = new Fl_Window(600, 450, "About seaview");
	  Fl_Help_View *br = new Fl_Help_View(1, 1, about->w() - 2, about->h() - 2);
	  Fl_Button *b = new Fl_Button(1, 1, 1, 1, NULL);//only to allow the shortcut
	  b->callback(hide_window_callback, NULL);
	  b->shortcut(FL_COMMAND | 'w');
	  about->end();
	  p = get_full_path(get_res_value("helpfile", "seaview.html"));
	  if(p == NULL) return;
	  data = fopen(p, "r");
	  if(data == NULL) return;
	  p = text;
	  int doit = 0;
	  while(TRUE) {
		  fgets(line, sizeof(line), data);
		  if(strncmp(line, "<a name=", 8) == 0) doit = 1;
		  if(strncmp(line, "<hr>", 4) == 0) break;
		  if(doit) {
			  strcpy(p, line);
			  p += strlen(p);
			  }
	  }
	  fclose(data);
	  br->value(text);
	  about->resizable(br);
  }
  about->show();
}

#if 100*FL_MAJOR_VERSION + FL_MINOR_VERSION >= 104

static Fl_Window *merged;

static bool merge_windows_for_class(Fl_Window *first)
{
  bool retval = false;
  Fl_Window *win = first;
  NSWindow *nsw = (NSWindow*)fl_xid(first), *nsw2;
  while ((win = Fl::next_window(win)) != NULL) {
    int diff = strcmp(win->xclass(), first->xclass());
    if (win->parent() || diff) continue;
    nsw2 = (NSWindow*)fl_xid(win);
    [nsw2 addTabbedWindow:nsw ordered:NSWindowAbove];
    retval = true;
    if (!merged) merged = win;
  }
  return retval;
}

static void merge_windows_by_class_cb(Fl_Widget *, void *)
{
  merged = NULL;
more:
  Fl_Window *win = Fl::first_window();
  NSWindow *nsw;
  while (win) {
    nsw = (NSWindow*)fl_xid(win);
    if (!win->parent() && ![nsw tabbedWindows] && strcmp(win->xclass(), Fl_Window::default_xclass())) {
      if (merge_windows_for_class(win)) goto more;
    }
    win = Fl::next_window(win);
  }
  if (merged) merged->show();
}
#endif // FLTK ≥ 1.4

void MG_apple_inits(void)
{
  fl_open_callback(file_receive_cb);
  fl_mac_set_about(show_apropos, NULL, 0);
#if 100*FL_MAJOR_VERSION + FL_MINOR_VERSION >= 104
  Fl_Sys_Menu_Bar::create_window_menu();
  int merge = fl_sys_menu_bar->find_index("Window/Merge All Windows");
  if (merge >= 0) {
    fl_sys_menu_bar->replace(merge, "Merge Windows by Class");
    Fl_Menu_Item *item = (Fl_Menu_Item*)fl_sys_menu_bar->menu() + merge;
    item->callback(merge_windows_by_class_cb);
  }
#endif
  Fl::set_font(FL_COURIER,"Courier");
  Fl::set_font(FL_COURIER_BOLD, (fl_mac_os_version >= 100500 ? "Courier-Bold" : "Courier Bold") );
  if (fl_mac_os_version >= 100500) { // it seems that Courier Oblique is not present on older OS
    Fl::set_font(FL_COURIER_ITALIC,"Courier-Oblique");
    Fl::set_font(FL_COURIER_BOLD_ITALIC,"Courier-BoldOblique");
    }
}


void set_seaview_modified(SEA_VIEW *view, int ismodified)
{
  view->modif_but_not_saved = ismodified;
  [(NSWindow*)(fl_xid(view->dnawin)) setDocumentEdited:ismodified];
}

#ifdef USE_PICT
class Fl_PICT_Graphics_Driver : public Fl_Graphics_Driver {
protected:
  void rect(int x, int y, int w, int h);
  void rectf(int x, int y, int w, int h);
  void line_style(int style, int width, char *dashes=0);
  void line(int x1, int y1, int x2, int y2);
  void font(int f, int s);
  void draw(const char *str, int n, int x, int y);
  void draw(const char*, int, float, float) ;
  void draw(int, const char*, int, int, int) ;
  void rtl_draw(const char*, int, int, int) ;
  void color(uchar r, uchar g, uchar b);
  void color(Fl_Color c);
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
};

void Fl_PICT_Graphics_Driver::draw(const char *txt, int n, int x, int y)
{
  if(txt == NULL || n == 0) return;
  MoveTo(x, y);
  unsigned char copy[n + 1];
  memcpy(copy+1, txt, n); *copy = n;
  DrawString(copy);
}

void Fl_PICT_Graphics_Driver::line(int x, int y, int x1, int y1)
{
  MoveTo(x, y);
  LineTo(x1, y1);
}

void Fl_PICT_Graphics_Driver::rect(int x, int y, int w, int h)
{
  MoveTo(x, y);
  LineTo(x, y+h-1);
  LineTo(x+w-1, y+h-1);
  LineTo(x+w-1, y);
  LineTo(x, y);
}

void Fl_PICT_Graphics_Driver::rectf(int x, int y, int w, int h)
{
  Rect r;
  r.top = (short)y;
  r.left = (short)x;
  r.bottom = (short)(y+h);
  r.right = (short)(x+w);
  PaintRect(&r);
}

void Fl_PICT_Graphics_Driver::font(int fontrank, int size)
{
  Style mystyle = normal;
  int macfont, fontstyle;
  if(fontrank < FL_SYMBOL) {
    fontstyle = fontrank % 4;
    fontrank -= fontstyle;
  }
  if(fontrank == FL_COURIER)
    macfont=kFontIDCourier/*22*/;
  else if(fontrank == FL_HELVETICA)
    macfont=kFontIDHelvetica/*21*/;
  else if(fontrank == FL_TIMES)
    macfont=kFontIDTimes/*20*/;
  else if(fontrank == FL_SYMBOL)
    macfont=kFontIDSymbol/*23*/;
  else
    macfont=21; 
  if(fontrank < FL_SYMBOL) {
    if(fontstyle == 0) mystyle = normal;
    else if(fontstyle == 1) mystyle = bold;
    else if(fontstyle == 2) mystyle = italic;
    else if(fontstyle == 3) mystyle = bold+italic;
  }
  TextFont(macfont);
  TextSize(size);
  TextFace(mystyle);
  Fl_Display_Device::display_device()->driver()->font(fontrank, size);
}

void Fl_PICT_Graphics_Driver::color(Fl_Color color)
{
  unsigned char r, g, b;
  Fl::get_color(color, r, g, b);
  RGBColor maccolor;
  maccolor.red = r << 8;
  maccolor.green = g << 8;
  maccolor.blue = b << 8;
  RGBForeColor(&maccolor);
}

double Fl_PICT_Graphics_Driver::width(const char *txt, int l)
{
  return (double)TextWidth( txt, 0, l );
}

void Fl_PICT_Graphics_Driver::draw_image(const uchar*, int, int, int, int, int, int) {}
void Fl_PICT_Graphics_Driver::draw_image_mono(const uchar*, int, int, int, int, int, int) {}
void Fl_PICT_Graphics_Driver::draw_image(void (*)(void*, int, int, int, uchar*), void*, int, int, int, int, int) {}
void Fl_PICT_Graphics_Driver::draw_image_mono(void (*)(void*, int, int, int, uchar*), void*, int, int, int, int, int) {}
void Fl_PICT_Graphics_Driver::draw(Fl_RGB_Image*, int, int, int, int, int, int) {}
void Fl_PICT_Graphics_Driver::draw(Fl_Pixmap*, int, int, int, int, int, int) {}
void Fl_PICT_Graphics_Driver::draw(Fl_Bitmap*, int, int, int, int, int, int) {}
void Fl_PICT_Graphics_Driver::line_style(int style, int width, char *dashes) {}
void Fl_PICT_Graphics_Driver::color(uchar r, uchar g, uchar b) {}
void Fl_PICT_Graphics_Driver::rtl_draw(const char*, int, int, int) {}
void Fl_PICT_Graphics_Driver::draw(int angle, const char* str, int n, int x, int y) {}
int Fl_PICT_Graphics_Driver::height() {
  return Fl_Display_Device::display_device()->driver()->height();
}
int Fl_PICT_Graphics_Driver::descent() {
  return Fl_Display_Device::display_device()->driver()->descent();
}
void Fl_PICT_Graphics_Driver::draw(const char* str, int n, float fx, float fy) {
  draw(str, n, (int)fx, (int)fy);
}

Copy_Surface::Copy_Surface(int w, int h) : Fl_Surface_Device(NULL)
{
  Rect myrect;
  myrect.top=0;
  myrect.left=0;
  myrect.right=w;
  myrect.bottom=h;
  mypicture = OpenPicture(&myrect);
  ClipRect(&myrect);
  PenNormal();
  driver(new Fl_PICT_Graphics_Driver());
}

Copy_Surface::~Copy_Surface()
{
  ClosePicture();
  MyCopyPictToClipboard();
  KillPicture(mypicture);
  delete driver();
}

QDPictRef Copy_Surface::MyPictToQDPict()
{
  CFIndex pictlen;
  CGDataProviderRef provider;
  /* conversion picture to QDPict */
  pictlen = GetHandleSize( (Handle)  mypicture);
  HLock( (Handle) mypicture);
  provider = CGDataProviderCreateWithData (NULL, *mypicture, pictlen, NULL);
  HUnlock( (Handle) mypicture);
  if (provider != NULL) {
	  QDPictRef myqdpict = QDPictCreateWithProvider (provider);
	  CFRelease(provider);
	  return myqdpict;
  }
  return NULL;
}


void Copy_Surface::MyCopyPictToClipboard()
{
  CFDataRef  data = NULL;
  PasteboardRef clipboard = NULL;
  CFIndex pictlen;
  
  pictlen = GetHandleSize( (Handle)  mypicture);
  PasteboardCreate (kPasteboardClipboard, &clipboard);
  PasteboardClear(clipboard);
  HLock( (Handle) mypicture);
  data = CFDataCreate( kCFAllocatorDefault, (UInt8*)*mypicture, pictlen );
  HUnlock( (Handle) mypicture);
  PasteboardPutItemFlavor(clipboard, (PasteboardItemID)1, 
				      /* kUTTypePICT */ CFSTR("com.apple.pict"),
				      data,
				      kPasteboardFlavorNoFlags );
  CFRelease (data);
  QDPictRef myqdpict = (fl_mac_os_version >= 100400 ? MyPictToQDPict() : 0); // useful under 10.3 for unclear reason
  if (!myqdpict) {
    CFRelease(clipboard);
    return;
  }
  
  CGRect bounds = QDPictGetBounds (myqdpict);
  CFMutableDataRef pdfdata;
  CGContextRef gc = init_PDF_context(bounds.size.width, bounds.size.height, pdfdata);
  if (gc) {
    /* convert PICT to PDF */
    CGContextBeginPage (gc, &bounds);
    (void) QDPictDrawToCGContext (gc, bounds, myqdpict);
    CGContextEndPage (gc);
    CGContextRelease (gc);
    /* copy PDF to clipboard */
    (void) PasteboardPutItemFlavor (clipboard, (PasteboardItemID)1, 
				     /* kUTTypePDF */ CFSTR("com.adobe.pdf"), 
				     pdfdata, kPasteboardFlavorNoFlags);
    CFRelease (pdfdata);
  }
  CFRelease (clipboard);
  QDPictRelease(myqdpict);
}


CGContextRef Copy_Surface::init_PDF_context(int w, int h, CFMutableDataRef &pdfdata)
{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1040
  CGRect bounds = CGRectMake(0, 0, w, h );
  pdfdata = CFDataCreateMutable(NULL, 0);
  if (&CGDataConsumerCreateWithCFData) { // true from 10.4
    CGDataConsumerRef myconsumer = CGDataConsumerCreateWithCFData (pdfdata);
    CGContextRef gc = CGPDFContextCreate (myconsumer, &bounds, NULL);
    CGDataConsumerRelease (myconsumer);
    return gc;
  }
#endif
  return NULL;
}

#endif // USE_PICT

void mac_tmp_dir_name(char *buffer, int l)
{
  CFStringGetCString((CFStringRef)NSTemporaryDirectory(), buffer, l, kCFStringEncodingUTF8);
}

#ifdef SAVE_WINDOW_AS_EPS

#include <FL/Fl_PostScript.H>

void EPS_cb(Fl_Widget *, void *data) {
  Fl_Native_File_Chooser fnfc;
  fnfc.title("Pick an .eps file");
  fnfc.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
  fnfc.filter("EPS\t*.eps\n");
  if (!fnfc.show() ) {
    Fl_Window *target = Fl::first_window();
    if (target) {
      FILE *file = fl_fopen(fnfc.filename(), "w");
      if (file) {
        Fl_EPS_File_Surface p(target->decorated_w(), target->decorated_h(), file);
        p.draw_decorated_window(target);
      }
    }
  }
}

static Fl_Menu_Item custom[] = {
  { "Save front window as EPS", 0, EPS_cb, 0, 0 },
  { 0 }
};

static int unused = (Fl_Mac_App_Menu::custom_application_menu_items(custom), 0);

#endif // SAVE_WINDOW_AS_EPS

#endif //__APPLE__
