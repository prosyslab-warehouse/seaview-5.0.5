#include "treerecs_draw.h"

#include <sys/stat.h>

#include <FL/fl_utf8.h>

#if __APPLE_CC__
#include <xlocale.h>
#endif
#include <iostream>
#include <fstream>
#include <cstring>

using std::string;

// Extern definitions
extern char *argname(int argc, char *argv[], const char *arg);
extern float argval(int argc, char *argv[], const char *arg, float defval);
extern bool isarg(int argc, char *argv[], const char *arg);
extern void interrupt_callback(Fl_Widget *ob, void *data);
extern int run_external_prog_in_pseudoterm(char *cmd, const char *dialogfname, const char *label);
extern char *create_tmp_filename(void);
extern void delete_tmp_filename(const char *base_fname);

int treerecs_run_interrupted;

// Directory separator
#ifdef WIN32
  static const char preferred_separator = '\\';
#else
  static const char preferred_separator = '/';
#endif

// Static variable
static string tmp_file_path;
static string output_file_canonical_path;
static string output_dir;

/// Delete tmp files
void delete_tmp_files(Fl_Widget* widget) {
  delete_tmp_filename(output_file_canonical_path.data());
  fl_rmdir(output_dir.data());
  delete_tmp_filename(tmp_file_path.data());
  widget->hide();
}

/// Check file
bool is_regular_file(const char* path) {
  struct stat buf;
  fl_stat(path, &buf);
  return S_ISREG(buf.st_mode);
}

void update_treerecs_prog_path(const char* str) {
  if(str) {
    if(not is_regular_file(str)) {
      printf("Error: impossible to get Treerecs in %s\n", str);
      exit(1);
    }
    treerecs_prog_path[0] = '\0';
    strcat(treerecs_prog_path, str);
  }
};

string create_treerecs_command_parameters(
    const string& genetree_path
    , const char* speciestree_path
    , const bool genenames_mapping
    , const char* genenames_separator
    , const bool speciesnames_prefix_position
    , const char* smap_filename
    , const char* duplication_cost
    , const char* loss_cost
    , const char* threshold
    , const string& output_dir
    , const bool full_reconciliation
    , const bool find_best_root
) {
  string treerecs_command;

  // LOAD DATA
  // Gene trees
  treerecs_command += " -g ";
  treerecs_command += genetree_path;

  // Species tree
  treerecs_command += " -s ";
  treerecs_command += speciestree_path;

  // Mapping
  if (genenames_mapping) {
    if(genenames_separator != NULL and strcmp(genenames_separator, "") != 0) {
      treerecs_command += " --sep ";
      treerecs_command += genenames_separator;
    }

    treerecs_command += " --prefix ";

    if (speciesnames_prefix_position) {
      treerecs_command += "yes ";
    } else {
      treerecs_command += "no ";
    }

  } else if (smap_filename != NULL and strcmp(smap_filename, "") != 0) {
    treerecs_command += " --smap ";
    treerecs_command += smap_filename;
  }

  // Treerecs settings
  // Costs
  if(duplication_cost != NULL and strcmp(duplication_cost, "") != 0) {
    treerecs_command += " --dupcost ";
    treerecs_command += duplication_cost;
  }

  if(loss_cost != NULL and strcmp(loss_cost, "") != 0 ) {
    treerecs_command += " --losscost ";
    treerecs_command += loss_cost;
  }

  // Branch support threshold
  if(threshold != NULL and strcmp(threshold, "") != 0) {
    treerecs_command += " --threshold ";
    treerecs_command += threshold;
  }

  // Output configuration
  treerecs_command += " --outdir ";
  treerecs_command += output_dir;
  treerecs_command += " -f";

  if(full_reconciliation) {
    treerecs_command += " --output-format recphyloxml:svg ";
  } else {
    treerecs_command += " --output-without-description ";
  }

  // Optional commands
  if (find_best_root) {
    treerecs_command += " --reroot ";
  }

  treerecs_command += " --verbose";

  return treerecs_command;
}

/// Create and open Treerecs window dialog and launch treerecs.
void treerecs_dialog(Fl_Widget *view, void* data)
{
  static int first = TRUE;

  // All Fl_Widgets
  static Fl_Window *w;
  static Fl_File_Input* speciestree_file;
  static Fl_Button *speciestree_file_button;
  static Fl_Float_Input *contraction_threshold;
  static Fl_Float_Input *duplication_cost;
  static Fl_Float_Input *loss_cost;
  static Fl_Box *box;
  static Fl_Check_Button *auto_mapping;
  static Fl_Round_Button *genenames_mapping;
  static Fl_Round_Button *file_mapping;
  static Fl_Check_Button *find_best_root;

  static Fl_Input* genenames_mapping_separator;
  static Fl_Check_Button* genenames_mapping_species_in_prefix_position;
  static Fl_File_Input* file_mapping_file;
  static Fl_Button *file_mapping_file_button;

  static Fl_Round_Button *rearrangment_mode;
  static Fl_Round_Button *reconciliation_mode;

  static Fl_Button *interrupt;
  static Fl_Return_Button *go;
  int started;

  // Get current directory path.
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    perror("getcwd() error");
  }
  string current_working_directory = cwd;

  if(first) {
    if(strlen(treerecs_prog_path) == 0) {
      fl_alert("Could no find Treerecs in neither\n"
                   "seaview directory nor PATH");
      return;
    }

    first = FALSE;

    int x_max = 300;
    int y_max = 400;

    int widget_space = 3;

    w = new Fl_Window(x_max, y_max);
    w->label("Treerecs configuration");
    w->set_modal();

    // Treerecs global settings
    Fl_Group* treerecs_setting_group = new Fl_Group(0, 20, w->w(), (25 + widget_space) * 5.5, "Treerecs global settings");
    treerecs_setting_group->box(FL_ROUNDED_BOX);

    int fl_float_input_width = w->w()/5;
    int output_format_content_x = fl_float_input_width * 3.75 - 2 * widget_space;
    int output_format_content_y = 20 ;

    // Speciestree filename input.
    speciestree_file = new Fl_File_Input(1.25*w->w()/4, output_format_content_y + widget_space, 1.75*w->w()/4 - widget_space, 20 * 1.5, "Species tree");
    speciestree_file->value(current_working_directory.c_str());
    speciestree_file->align(FL_ALIGN_LEFT);
    speciestree_file->callback(load_file_callback);

    // Speciestree filename input button.
    speciestree_file_button = new Fl_Button(speciestree_file->x() + speciestree_file->w() + widget_space, speciestree_file->y(), speciestree_file->h() * 2, speciestree_file->h(), "Select");
    speciestree_file_button->align(FL_ALIGN_INSIDE);
    speciestree_file_button->callback(load_file_button_callback, speciestree_file);

    // Branch support threshold input field.
    contraction_threshold = new Fl_Float_Input(output_format_content_x + widget_space, speciestree_file->y() + speciestree_file->h() + widget_space, fl_float_input_width, 25, "Branch support threshold");
    contraction_threshold->align(FL_ALIGN_LEFT);
    contraction_threshold->value("-1.0");

    // Duplication cost input.
    duplication_cost = new Fl_Float_Input(contraction_threshold->x(), contraction_threshold->y()  + contraction_threshold->h() + widget_space, fl_float_input_width, 25, "Duplication cost");
    duplication_cost->align(FL_ALIGN_LEFT);
    duplication_cost->value("2.0");

    // Loss cost input.
    loss_cost = new Fl_Float_Input(duplication_cost->x(), duplication_cost->y() + duplication_cost->h() + widget_space, fl_float_input_width, 25, "Loss cost");
    loss_cost->align(FL_ALIGN_LEFT);
    loss_cost->value("1.0");

    // Find best root check button.
    find_best_root = new Fl_Check_Button(loss_cost->x(), loss_cost->y() + loss_cost->h() + widget_space, w->w()/2.0, 25, "Find best root");
    find_best_root->align(FL_ALIGN_LEFT);
    find_best_root->set();

    treerecs_setting_group->end();

    // Mapping settings
    Fl_Group *mapping_group = new Fl_Group(treerecs_setting_group->x(), treerecs_setting_group->y() + treerecs_setting_group->h() + 20, w->w(), 90, "Gene <> Species mapping method");
    mapping_group->box(FL_ROUNDED_BOX);
    mapping_group->align(FL_ALIGN_TOP|FL_ALIGN_CENTER);

    int mapping_content_x = treerecs_setting_group->x();
    int mapping_content_y = treerecs_setting_group->y() + treerecs_setting_group->h() + 20 + widget_space;
    int trinome_space = widget_space;
    int trinome_width = (int)((double)w->w()/3.0) - 2*(trinome_space);

    // Genenames mapping has several options below.
    genenames_mapping = new Fl_Round_Button(mapping_content_x + trinome_space, mapping_content_y + widget_space, trinome_width * 1.75, 20, "Use gene names");
    genenames_mapping->type(FL_RADIO_BUTTON);
    genenames_mapping->set();

    // File mapping.
    file_mapping = new Fl_Round_Button(genenames_mapping->x() + genenames_mapping->w() + trinome_space, genenames_mapping->y(), genenames_mapping->w(), genenames_mapping->h(), "Use file");
    file_mapping->type(FL_RADIO_BUTTON);

    // Automatic mapping using genenames.
    auto_mapping = new Fl_Check_Button(genenames_mapping->x(), genenames_mapping->y() + genenames_mapping->h() + widget_space, trinome_width, 20, "Auto");
    if(genenames_mapping->value()) auto_mapping->activate();
    else auto_mapping->deactivate();

    // Character separator input for mapping using gene names.
    genenames_mapping_separator = new Fl_Input(auto_mapping->x() + trinome_width + trinome_space + 30, auto_mapping->y(), 20, 20, "Separator");
    genenames_mapping_separator->align(FL_ALIGN_LEFT);
    genenames_mapping_separator->value("_");
    if(genenames_mapping->value() and not auto_mapping->value())
      genenames_mapping_separator->deactivate();
    else
      genenames_mapping_separator->activate();

    // Species name position in gene name for mapping using gene names.
    genenames_mapping_species_in_prefix_position = new Fl_Check_Button(genenames_mapping_separator->x() + trinome_width*1.5 - 2*trinome_space, genenames_mapping_separator->y(), trinome_width, genenames_mapping_separator->h(), "Species before");
    genenames_mapping_species_in_prefix_position->align(FL_ALIGN_LEFT);
    genenames_mapping_species_in_prefix_position->set();
    genenames_mapping_species_in_prefix_position->deactivate();
    if(genenames_mapping->value() and not auto_mapping->value())
      genenames_mapping_species_in_prefix_position->deactivate();
    else
      genenames_mapping_species_in_prefix_position->activate();

    // Map file input for mapping using file.
    file_mapping_file = new Fl_File_Input(1.25*w->w()/4, genenames_mapping_species_in_prefix_position->y() + genenames_mapping_species_in_prefix_position->h() + widget_space, 1.75*w->w()/4 - widget_space, 20 * 1.5, "Map file");
    file_mapping_file->align(FL_ALIGN_LEFT);
    file_mapping_file->value(current_working_directory.c_str());
    file_mapping_file->callback(load_file_callback);

    // Map file input button for mapping using file.
    file_mapping_file_button = new Fl_Button(file_mapping_file->x() + file_mapping_file->w() + widget_space, file_mapping_file->y(), file_mapping_file->h()*2, file_mapping_file->h(), "Select");
    file_mapping_file_button->align(FL_ALIGN_INSIDE);
    file_mapping_file_button->callback(load_file_button_callback, file_mapping_file);

    if(not file_mapping->value()) {
      file_mapping_file->deactivate();
      file_mapping_file_button->deactivate();
    }
    else {
      file_mapping_file->activate();
      file_mapping_file_button->activate();
    }

    auto_mapping->set();
    mapping_group->end();

    // Finally the output settings : choose between rearrangement and reconciliation.
    Fl_Group *output_group = new Fl_Group(mapping_group->x(), mapping_group->y() + mapping_group->h() + 20, w->w(), 30, "Output tree");
    output_group->box(FL_ROUNDED_BOX);
    output_group->align(FL_ALIGN_TOP|FL_ALIGN_CENTER);

    rearrangment_mode = new Fl_Round_Button(mapping_group->x() + widget_space + w->w()/8, mapping_group->y() + mapping_group->h() + 20 + widget_space, w->w()/3, 20, "Gene tree");
    rearrangment_mode->type(FL_RADIO_BUTTON);

    reconciliation_mode = new Fl_Round_Button(rearrangment_mode->x() + rearrangment_mode->w() + 2*widget_space, rearrangment_mode->y(), w->w()/3, 20, "Full reconciliation");
    reconciliation_mode->type(FL_RADIO_BUTTON);
    reconciliation_mode->set();

    output_group->end();

    box = new Fl_Box(output_group->x(), output_group->y() + output_group->h() + widget_space, output_group->w(), 25);
    box->label("Note: please configure Treerecs.");

    interrupt = new Fl_Button(3, w->h() - 25, 70, 20, "");
    interrupt->callback(interrupt_callback, &treerecs_run_interrupted);
    go = new Fl_Return_Button(w->w() - 70 - 3, interrupt->y() , 70, 20, "Go");
    go->callback(interrupt_callback, &started);
    w->end();
    w->callback(interrupt_callback, &treerecs_run_interrupted);
  }

  interrupt->label("Cancel");

  started = treerecs_run_interrupted = 0;
  go->show();
  go->deactivate();
  w->show();

  // Run, while treerecs has not been launched or be interrupted.
  while(!started && !treerecs_run_interrupted) {
    Fl_Widget *o = Fl::readqueue();

    if (!o) Fl::wait();
    else if(o == auto_mapping|| o == file_mapping || o == genenames_mapping) {
      if( file_mapping->value() ) {
        file_mapping_file->activate();
        file_mapping_file_button->activate();
      } else {
        file_mapping_file->deactivate();
        file_mapping_file_button->deactivate();
      }

      if( genenames_mapping->value() ) {
        auto_mapping->activate();
        genenames_mapping_separator->activate();
        genenames_mapping_species_in_prefix_position->activate();
      } else {
        auto_mapping->deactivate();
        genenames_mapping_separator->deactivate();
        genenames_mapping_species_in_prefix_position->deactivate();
      }

      if( auto_mapping->value() ){
        genenames_mapping_separator->deactivate();
        genenames_mapping_species_in_prefix_position->deactivate();
      } else if(genenames_mapping->value()) {
        genenames_mapping_separator->activate();
        genenames_mapping_species_in_prefix_position->activate();
      }
    }

    // Check form consistency.
    // NOTE: the order is important: the error message will be that
    //       corresponding to the first detected problem
    //   - Check species tree file
    if(not speciestree_file->value() or
        not is_regular_file(speciestree_file->value()))  {
      go->deactivate();
      const char* error_msg = "Invalid species tree (mandatory).";
      if (strcmp(box->label(), error_msg)) box->label(error_msg);
    } else
    //  - Check contraction threshold definition
    //    (if active and value is either absent or invalid)
    if(contraction_threshold->active() and
        (not contraction_threshold->value() or
        not strcmp(contraction_threshold->value(), ""))) {
      go->deactivate();
      const char* error_msg = "Invalid branch support threshold.";
      if (strcmp(box->label(), error_msg)) box->label(error_msg);
    } else
    //  - Check mapping file
    //    (if "Use file" is checked and either no file has been selected or it
    //    is invalid)
    if (file_mapping->value() and
        (not file_mapping_file->value() or
        not is_regular_file(file_mapping_file->value()))) {
      go->deactivate();
      const char* error_msg = "Invalid mapping file.";
      if (strcmp(box->label(), error_msg)) box->label(error_msg);
    } else
    //  - Everything is ok, user can start Treerecs.
    {
      go->activate();
      box->label("");
    }
  }

  if(!treerecs_run_interrupted) {
    // Load current gene tree and create temporary file which contains this one.
    FD_nj_plot *fd_nj_plot = (FD_nj_plot*)view->user_data();

    char* current_tree = fd_nj_plot->current_tree;

    // Run treerecs. Before we need to build the command line to launch Treerecs with options.
    go->hide();
    Fl::flush();

    // Set input and output file names and paths
    tmp_file_path = create_tmp_filename();
    string input_genetree_path = tmp_file_path + "_genetree";
    string input_speciestree_path = tmp_file_path + "_speciestree";
    output_dir = tmp_file_path + "_treerecs_output";

    /*auto*/size_t pos = input_genetree_path.find_last_of(preferred_separator);
    string tmp_fname = (pos != string::npos) ?
                       input_genetree_path.substr(pos+1) :
                       input_genetree_path;
    output_file_canonical_path =
        output_dir + preferred_separator + tmp_fname + "_recs";

    // Prepare gene tree and species tree for Treerecs
    create_temp_tree_file(input_genetree_path, current_tree);
    copyfile(input_speciestree_path.data(), speciestree_file->value());

    // Create our Treerecs command.
    string treerecs_command =
        treerecs_prog_path +
        create_treerecs_command_parameters(
            input_genetree_path.data(), input_speciestree_path.data(),
            genenames_mapping->value() and
            not auto_mapping->value(),
            genenames_mapping_separator->value(),
            genenames_mapping_species_in_prefix_position->value(),
            (file_mapping->value()
             ? file_mapping_file->value() : ""),
            duplication_cost->value(), loss_cost->value(),
            contraction_threshold->value(),
            output_dir,
            reconciliation_mode->value(),
            find_best_root->value());

    w->hide();

    int status =
        run_external_prog_in_pseudoterm((char*) treerecs_command.c_str(),
                                        NULL,
                                        "Treerecs");

    // TODO<dpa> this test is poor, see status ?
    // If the "main" output file exists, Treerecs success.
    string main_output_file_path = output_file_canonical_path +
                                   (reconciliation_mode->value()
                                    ? ".recphylo.xml" : ".nwk");

    if(is_regular_file(main_output_file_path.c_str())) {
      FILE *res = fl_fopen(main_output_file_path.c_str(), "r");
      fseek(res, 0, SEEK_END);
      long l = ftell(res);
      fseek(res, 0, SEEK_SET);
      char *resulting_tree = (char *) malloc(l + 1);
      char *p = resulting_tree;
      while (l-- > 0) {
        char c = fgetc(res);
        *(p++) = c;
      }
      *p = 0;
      fclose(res);

      if(not reconciliation_mode->value()) {
        treedraw(resulting_tree, (SEA_VIEW *) view->user_data(), main_output_file_path.c_str(), FALSE);
      } else {
        string svg_file_path = output_file_canonical_path + ".svg";

        if(not is_regular_file(svg_file_path.c_str())) {
          perror("Invalid svg filename.\n");
        }

        Fl_Window* recphylowml_window =
            svgdraw(svg_file_path,
                main_output_file_path,
                "Full-reconciliation view");
      }
    } else {
      fl_message("Error with Treerecs: invalid data.\n");
    }
  }
  w->hide();
}

void create_temp_tree_file(const string& path, const char* tree) {
  int tree_begin = 0;
  if(tree[tree_begin] == '[') {
    while (tree[tree_begin] != ']') {
      tree_begin++;
    }
    tree_begin++;
  }

  int tree_end = tree_begin;
  while(tree[tree_end] != ';'){
    tree_end++;
  }
  tree_end++;

  char tmp_tree[tree_end - tree_begin + 1];
  int i = 0;
  while(tree_begin + i < tree_end){
    tmp_tree[i] = tree[tree_begin + i];
    i++;
  }
  tmp_tree[i] = '\0';

  FILE* temp = fl_fopen(path.c_str(), "w");
  fwrite(tmp_tree, 1, strlen(tmp_tree), temp);
  fputs("\n", temp);
  fclose(temp);
}

/*!
 * @brief Handle svg image in a widget as a Fl_Box.
 */
class Fl_SVG_Box : public Fl_Widget {
  typedef struct {
    /*!
     * @struct SVGText
     * @brief Contains all info of a text (position, size and content).
     * @details Fl_SVG_Image cannot use text tags, we need to manage these elements after Fl_SVG_Image creation.
     */
    const char* content;
    double size;
    double x;
    double y;
  } SVGText;

  /*!
   * @brief Read SVG file and store each SVG text tags in a vector of SVGText.
   * @param filename Name of the SVG file.
   * @return
   */
  void extract_texts(const char* filename){
    // First of all get the total number of lines which contains "<text" str.
    FILE* svgfile = fl_fopen(filename, "r");

    char line[1024];
    svgtexts_size = 0;

    while(fgets(line, sizeof(line), svgfile)) {
      char* pch = strstr(line, "<text");
      if(pch != NULL) svgtexts_size++;
    }

    fseek(svgfile, 0, SEEK_SET);

    svgtexts = (SVGText*)malloc(sizeof(SVGText)*svgtexts_size);

    line[0] = '\0';
    unsigned int i = 0;
    while(fgets(line, sizeof(line), svgfile)) { // I changed this, see below
      //unsigned long pos = line.find("<text");
      char* pch = strstr(line, "<text");
      // We assume that we have only one <text> tag per line.
      if (pch != NULL) {
        while(*pch != ' ' and *pch != '\0') ++pch;
        // Create SVGText by extracting attributes and their value.
        SVGText svgtext;
        while(*pch != '\0' and *pch != '>') {
          while(*pch == ' ' and *pch != '\0') ++pch;
          if(*pch == '>') break;
          char attribute_name[1024];
          attribute_name[0] = '\0';
          char* attribute_name_pch = attribute_name;

          while(*pch != '=' and *pch != ' ' and *pch != '\0') {
            *attribute_name_pch = *pch;
            ++attribute_name_pch;
            ++pch;
          }
          *attribute_name_pch = '\0';

          //printf("Find attribute %s\n", attribute_name);


          char attribute_value[1024];
          attribute_value[0] = '\0';
          char* attribute_value_pch = attribute_value;

          while(*pch != '\"' and *pch != '\0') ++pch;
          ++pch;
          while(*pch != '\"' and *pch != '\0'){
            *attribute_value_pch = *pch;
            ++attribute_value_pch;
            ++pch;
          }
          *attribute_value_pch = '\0';

          if(*pch == '\0') break;
          ++pch;

          //printf("Find value %s\n", attribute_value);

          if(strcmp(attribute_name, "x") == 0)
            svgtext.x = atof(attribute_value);
          else if(strcmp(attribute_name, "y") == 0)
            svgtext.y = atof(attribute_value);
          else if(strcmp(attribute_name, "font-size") == 0)
            svgtext.size = atof(attribute_value);
          else {};
        }
        ++pch; // be after the '>' character
        char* text_content = (char*)malloc(sizeof(char) * 1024);
        char* text_content_pch = text_content;
        while(*pch != '<' and *pch != '\0'){
          *text_content_pch = *pch;
          ++text_content_pch;
          ++pch;
        }
        *text_content_pch = '\0';
        //printf("Text content found = %s\n", text_content);
        svgtext.content = text_content;
        //printf("Extracted : %s\n", text_content);
        assert(i < svgtexts_size);
        svgtexts[i] = svgtext;
        i++;
      }
    }

    fclose(svgfile);
  }

  /*!
   * @brief Draw the Fl_SVG_Box.
   */
  FL_EXPORT void draw(void) {

    double transformation_ratio_x = (double)svg_image->w()/original_svg_dim_x;
    double transformation_ratio_y = (double)svg_image->h()/original_svg_dim_y;

    svg_image->draw(x(), y(), w(), h(), svg_shift_x, svg_shift_y);
    fl_color(Fl_Color(FL_BLACK));

    int i = 0;

    while(i < svgtexts_size){
      //printf("Place %s in (%d, %d)\n", svgtexts[i].content.c_str(), (int)(svgtexts[i].x * transformation_ratio_x),(int)(svgtexts[i].y * transformation_ratio_y) );
      double text_pos_x = (x() - svg_shift_x) + (svgtexts[i].x * transformation_ratio_x);
      double text_pos_y = (y() - svg_shift_y) + (svgtexts[i].y * transformation_ratio_y);

      if(text_pos_x >= x() and text_pos_x <= x() + w()
         and text_pos_y >= y() and text_pos_y <= y() + h()) {

        Fl_Fontsize oldsize = fl_size();

        Fl_Fontsize newsize = static_cast<Fl_Fontsize>(12);//computed_fontsize);
        fl_font(fl_font(), newsize);

        fl_draw(svgtexts[i].content, text_pos_x, text_pos_y);

        fl_font(fl_font(), oldsize);
      }
      i++;
    }

    // We draw a rectangle to hide texts which are written beyond the limits
    fl_rectf(x() + w(), y() - 12,
            w(), h() + 12, Fl_Color(FL_BACKGROUND_COLOR));
  };

  FL_EXPORT int handle(int) {return 0;};

public:

  /// Original SVG image ratio.
  double original_ratio() const {return original_svg_dim_x/original_svg_dim_y; };

  FL_EXPORT Fl_SVG_Box(int x, int y, int w, int h, const char* filename) :
      Fl_Widget(x, y, w, h), zoom(1), svg_shift_x(0), svg_shift_y(0), svgtexts_size(0) {

    // Create a Fl_SVG_Image.
    svg_image = new Fl_SVG_Image(filename);

    // Check SVG read.
    switch ( svg_image->fail() ) {
      case Fl_Image::ERR_FILE_ACCESS:
        // File couldn't load? show path + os error to user
        fl_alert("%s: %s\n", filename, strerror(errno));
        exit(EXIT_FAILURE);
      case Fl_Image::ERR_FORMAT:
        // Parsing error
        fl_alert("%s: couldn't decode image\n", filename);
        exit(EXIT_FAILURE);
    }

    original_svg_dim_x = svg_image->w();
    original_svg_dim_y = svg_image->h();

    extract_texts(filename); // Because of Fl_SVG_Image cannot read text tags, we need to extract them after.

    if(original_ratio() < 1) {
      svg_image->resize(w, w / original_ratio());
      zoom = (w / original_ratio())/h;
    }
    else {
      svg_image->resize(h * original_ratio(), h);
      zoom = (h * original_ratio())/w;
    }
    //svg_image->proportional = false; // Be able to change shape of the Fl_SVG_Image.
  }

  ~Fl_SVG_Box() {
    delete svg_image;
    for(int i = 0; i < svgtexts_size ; i++) {
      free((char*)svgtexts[i].content);
    }

    free(svgtexts);
  };

  /// SVG image (without text).
  Fl_SVG_Image* svg_image;
  /// svgtexts contains all infos to draw texts on the svg image.
  SVGText* svgtexts;
  unsigned int svgtexts_size;
  double original_svg_dim_x;
  double original_svg_dim_y;
  double svg_shift_x;
  double svg_shift_y;
  double zoom;
};

/// Contains all data for the SVG viewer window. This class is in charge of
/// freeing/deleting svg window data.
class SVG_View_data {
public:
  /// Widget which is drawing the SVG image.
  Fl_SVG_Box* svg_box;
  /// Zoom button.
  Fl_Simple_Counter* zoom;
  /// Scrollbar in x.
  Fl_Scrollbar* bar_x;
  /// Scrollbar in y.
  Fl_Scrollbar* bar_y;
  /// Window which contains all data.
  Fl_Window* w;
  /// Filenames of the SVG and RecPhyloXML sources.
  const char* svg_fname;
  const char* rpx_fname;
  /// Total cost score of the reconciliation
  float total_cost;
  int duplication_number;
  int loss_number;
  const char* reconciliation_score_message;

  SVG_View_data() :
      svg_box(NULL)
      , zoom(NULL)
      , bar_x(NULL)
      , bar_y(NULL)
      , w(NULL)
      , svg_fname(NULL)
      , rpx_fname(NULL)
      , reconciliation_score_message(NULL)
      , total_cost(0.f)
      , duplication_number(0)
      , loss_number(0) {};

  ~SVG_View_data() {
    if (svg_fname != NULL)
      free((char*)svg_fname);
    if (rpx_fname != NULL)
      free((char*)rpx_fname);

    if (reconciliation_score_message != NULL)
      free((char*)reconciliation_score_message);
  };

  // Update values and redraw the window.
  void update() {
    double zoom_value = zoom->value();
    svg_box->svg_image->resize(svg_box->w() * zoom_value, svg_box->h() * zoom_value);

    bar_x->slider_size((float)svg_box->w()/svg_box->svg_image->w());

    bar_y->slider_size((float)svg_box->h()/svg_box->svg_image->h());

    w->redraw();
  };
};

/// Zoom in SGV image.
void svg_zoom_callback(Fl_Widget* obj, void* data) {
  SVG_View_data* view_data = (SVG_View_data*)data;

  view_data->update();
}

/// Scroll in x in SVG image.
void svg_scrollbar_x_callback(Fl_Widget *obj, void *data) {
  Fl_Scrollbar* scrollbar_widget = (Fl_Scrollbar*)obj;
  double value = scrollbar_widget->value();

  SVG_View_data* view_data = (SVG_View_data*)data;
  Fl_SVG_Box *svg_box = view_data->svg_box;
  svg_box->svg_shift_x = value/scrollbar_widget->maximum() * ((svg_box->svg_image->w()) - svg_box->w());

  view_data->update();
}

/// Scroll in y in SVG image.
void svg_scrollbar_y_callback(Fl_Widget *obj, void *data) {
  Fl_Scrollbar* scrollbar_widget = (Fl_Scrollbar*)obj;
  double value = scrollbar_widget->value();

  SVG_View_data* view_data = (SVG_View_data*)data;
  Fl_SVG_Box *svg_box = view_data->svg_box;
  svg_box->svg_shift_y = value/scrollbar_widget->maximum() * (svg_box->svg_image->h() - svg_box->h());

  view_data->update();
}

int extract_costs_from_xml_description(const char* filename,
                                       float* cost, int* ndup, int* nlos) {
  // Open file
  std::ifstream xml_file(filename, std::ios::in);
  /*try {
    if (not xml_file.is_open()) {
      throw std::invalid_argument(std::string("could not open file ") + filename);
    }
  } catch(std::exception const& e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }*/
  if (not xml_file.is_open()) {
    fprintf(stderr, "Could not open file: %s\n", filename);
    return 1;
  }

  // Read file (read entire file content into xml_descr
  std::string xml_descr((std::istreambuf_iterator<char>(xml_file)), std::istreambuf_iterator<char>() );

  // Locate and extract (in a substring str) our information of interest
  const string motif("<!-- family");
  /*auto*/size_t pos = xml_descr.find(motif) + motif.size();
  string str =
      xml_descr.substr(pos, xml_descr.find("-->", pos + motif.size()) - pos);

  // Find positions of total cost, number of duplications and of losses
  const string cost_str("total cost = ");
  const string ndup_str("duplications = ");
  const string nlos_str("losses = ");
  /*auto*/size_t cost_pos = str.find(cost_str) + cost_str.size();
  /*auto*/size_t ndup_pos = str.find(ndup_str) + ndup_str.size();
  /*auto*/size_t nlos_pos = str.find(nlos_str) + nlos_str.size();

  // Convert to corresponding types and assign values to out params
  *cost = /*std::*/strtof(str.data() + cost_pos, NULL/*nullptr*/);
  *ndup = /*std::*/strtol(str.data() + ndup_pos, NULL/*nullptr*/, 10);
  *nlos = /*std::*/strtol(str.data() + nlos_pos, NULL/*nullptr*/, 10);

  return 0;
}

/// Draw a window with a reconciled tree in SVG and its widgets to navigate or save files.
/// Edit/alloc variables in a SVG_View_data.
Fl_Window * svgdraw(const string& svg_file_path,
    const string& data_file_path,
    const char *label) {

  // Window and svg init dimensions.
  double window_width = 800;
  double window_height = 600;
  double svg_width = window_width - 50;
  double svg_height = window_height - 80;

  // SVG_View_data manages all widgets and update of the svg_image.
  SVG_View_data* view_data = new SVG_View_data();
  char* svg_fname_copy = (char*)malloc(sizeof(char) * svg_file_path.size() + 1);
  strcpy(svg_fname_copy, svg_file_path.c_str());
  view_data->svg_fname = (const char*)svg_fname_copy;
  char* rpx_fname_copy = (char*)malloc(sizeof(char) * data_file_path.size() + 1);
  strcpy(rpx_fname_copy, data_file_path.c_str());
  view_data->rpx_fname = (const char*)rpx_fname_copy;

  if ((extract_costs_from_xml_description(data_file_path.c_str(),
      &view_data->total_cost,
      &view_data->duplication_number,
      &view_data->loss_number) != 0)) {
    return NULL;
  }

  // Create window and its widgets
  Fl_Window* w = new Fl_Window(window_width, window_height);
  w->label(label);
  w->color(Fl_Color(FL_BACKGROUND_COLOR));
  view_data->w = w;
  w->callback(delete_tmp_files);

  // Menu bar
  Fl_Menu_Bar *menu = new Fl_Menu_Bar(0, 0, window_width, 25);
  menu->add("Save/SVG",   FL_COMMAND+'s', save_svg_file_callback, view_data);
  menu->add("Save/RecPhyloXML"
      ,   FL_COMMAND+'x'
      , save_recphyloxml_file_callback
      , view_data);

  // Add text box to print a summary of events in the reconciled tree.
  Fl_Box* text_box = new Fl_Box(25, menu->y() + menu->h() + 10, svg_width/3, 20);
  char* text_box_message = (char*)malloc(sizeof(char)*1024);
  sprintf(
      text_box_message
      , "Total cost of the reconciliation = %.1f, duplications = %d, losses = %d"
      , view_data->total_cost
      , view_data->duplication_number
      , view_data->loss_number
  );
  view_data->reconciliation_score_message = text_box_message;

  text_box->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT);
  text_box->label(view_data->reconciliation_score_message);

  // Create SVG image (has no text)
  Fl_SVG_Box* svg_box = new Fl_SVG_Box(25, 55, svg_width, svg_height,
      svg_file_path.c_str());
  view_data->svg_box = svg_box;

  w->resizable(svg_box);

  // Create an Fl_Simple_Counter to zoom into the SVG image.
  Fl_Simple_Counter* zoom = new Fl_Simple_Counter(
      svg_box->x() + svg_box->w() - 50, text_box->y(), 50, 20, "Zoom");
  zoom->value(svg_box->zoom);
  view_data->zoom = zoom;
  zoom->callback(svg_zoom_callback, view_data);

  // Scrollbar to navigate into the svg_image
  Fl_Scrollbar* svg_box_scrollbar_x = new Fl_Scrollbar(
      svg_box->x(), svg_box->y() + svg_box->h(), svg_box->w(), 10);
  svg_box_scrollbar_x->slider_size((float)svg_box->w()/svg_box->svg_image->w());
  svg_box_scrollbar_x->bounds(0, 100);
  svg_box_scrollbar_x->type(FL_HORIZONTAL);
  view_data->bar_x = svg_box_scrollbar_x;
  svg_box_scrollbar_x->callback(svg_scrollbar_x_callback, view_data);

  Fl_Scrollbar* svg_box_scrollbar_y = new Fl_Scrollbar(
      svg_box->x() + svg_box->w(), svg_box->y(), 10, svg_box->h());
  svg_box_scrollbar_y->slider_size((float)svg_box->h()/svg_box->svg_image->h());
  svg_box_scrollbar_y->bounds(0, 100);
  view_data->bar_y = svg_box_scrollbar_y;
  svg_box_scrollbar_y->callback(svg_scrollbar_y_callback, view_data);

  w->show();

  w->end();

  return w;
}

/// Open Treerecs dialog.
void treerecs_callback(Fl_Widget *ob, void *data)
{
  //SEA_VIEW *view = (SEA_VIEW *) ob->user_data();
  treerecs_dialog(ob, data);
}

/// Open a windows with a file chooser.
void load_file_callback(Fl_Widget *ob)
{
  Fl_File_Input* o = (Fl_File_Input*) ob;

  //SEA_VIEW *view = (SEA_VIEW *) ob->user_data();
  Fl_Native_File_Chooser* chooser = new Fl_Native_File_Chooser();
  if (o->label())
    chooser->title(o->label());
  else
    chooser->title("Choose a file");
  chooser->type(Fl_Native_File_Chooser::BROWSE_FILE);
  char* filename = run_and_close_native_file_chooser(chooser);
  if(filename != NULL and strcmp(filename, "") != 0) {
    (o)->value(filename);
  }

}

/// Open a windows with a file chooser. Send value to adj.
void load_file_button_callback(Fl_Widget *ob, void *adj)
{
  //SEA_VIEW *view = (SEA_VIEW *) ob->user_data();
  load_file_callback ((Fl_File_Input*)adj);
}

/// Create a file (to) which is a copy of an other (from).
int copyfile(const char *to, const char *from)
{
  char ch;
  FILE* source = fl_fopen(from, "r");
  FILE* target = fl_fopen(to, "w");

  while( ( ch = fgetc(source) ) != EOF )
    fputc(ch, target);

  fclose(source);
  fclose(target);

  return 0;
}

/// Open and close a window to save a SVG file with SVG_View_data.
void save_svg_file_callback(Fl_Widget *ob, void *data) {
  SVG_View_data* svg_data = (SVG_View_data*)data;

  Fl_Native_File_Chooser* chooser = new Fl_Native_File_Chooser(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
  chooser->title("Save file");
  chooser->options(Fl_Native_File_Chooser::USE_FILTER_EXT | chooser->options());
  chooser->filter("SVG files\t*.svg");
  const char* filename = run_and_close_native_file_chooser(chooser);

  if(filename != NULL and strcmp(filename, "") != 0) {
    copyfile(filename, svg_data->svg_fname);
  }
}

/// Open and close a window to save a RecPhyloXML file with SVG_View_data.
void save_recphyloxml_file_callback(Fl_Widget *ob, void *data) {
  SVG_View_data* svg_data = (SVG_View_data*)data;

  Fl_Native_File_Chooser* chooser = new Fl_Native_File_Chooser();
  chooser->title("Save file");
  chooser->type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
  const char* filename = run_and_close_native_file_chooser(chooser);

  if(filename != NULL and strcmp(filename, "") != 0) {
    copyfile(filename, svg_data->rpx_fname);
  }
}
