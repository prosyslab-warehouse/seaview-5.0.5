#ifndef TREERECS_SEAVIEW_TREERECS_DRAW_H
#define TREERECS_SEAVIEW_TREERECS_DRAW_H

#include "seaview.h"
#include "treedraw.h"
//#include "pseudoterminal.cxx"
// STD includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

// FLTK includes
#include <FL/Fl_Window.H>
#include <FL/Enumerations.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/Fl_Simple_Counter.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_File_Input.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_SVG_Image.H>

#include <string>

static char treerecs_prog_path[PATH_MAX] = "";

/// Check file
bool is_regular_file(const char* path);

void update_treerecs_prog_path(const char* str);

/// Copy file content of "from" to file "to".
int copyfile(const char *to, const char *from);

const char* create_treerecs_command_parameters(
    const char* genetree_filename
    , const char* speciestree_filename
    , const bool genenames_mapping
    , const char* genenames_separator
    , const bool speciesnames_prefix_position
    , const char* smap_filename
    , const char* duplication_cost
    , const char* loss_cost
    , const char* threshold
    , const char* output_filename
    , const bool full_reconciliation
    , const bool find_best_root
    , const bool verbose
    , const bool superverbose);

/// Create a window to load a file.
void load_file_callback(Fl_Widget *ob);

/// Create a window to save a file in SVG format (given by void* data).
void save_svg_file_callback(Fl_Widget *ob, void *data);

/// Create a window to save a file in recphyloxml (given by void* data).
void save_recphyloxml_file_callback(Fl_Widget *ob, void *data);

/// Callback for file loader.
void load_file_button_callback(Fl_Widget *ob, void *data);

/// Callback for treerecs option.
void treerecs_callback(Fl_Widget *obj, void *data);

/// Draw a Treerecs setting dialog and launch Treerecs bin.
void treerecs_dialog(SEA_VIEW* view);

void create_temp_tree_file(const std::string& path, const char* tree);

/// Fl_SVG_Box is Fl_Widget which draws SVG images.
class Fl_SVG_Box;

/// SVG view data contains all infos of an SVG image and allows
/// * update of associated widgets (scrollbars, zoom, ...)
/// * keep original source filename
/// * save reconciliation scores as duplications/losses numbers and costs.
class SVG_View_data;

/// Zoom into an Fl_SVG_image.
void svg_zoom_callback(Fl_Widget* obj, void* data);

/// Scroolbar x axis for SVG image.
void svg_scrollbar_x_callback(Fl_Widget *obj, void *data);

/// Scroolbar y axis for SVG image.
void svg_scrollbar_y_callback(Fl_Widget *obj, void *data);

/// Extract all costs and number of gene events from RecPhyloXML description.
int extract_costs_from_xml_description(const char* filename,
                                       float* cost, int* ndup, int* nlos);

/// Draw the reconciliation in SVG.
Fl_Window * svgdraw(const std::string& svg_file_path,
                    const std::string& data_file_path,
                    const char *label);

#endif //TREERECS_SEAVIEW_TREERECS_DRAW_H
