#include "gcin.h"
#include "pho.h"
#include "win-sym.h"
#include "gst.h"

GtkWidget *gwin0;
extern GtkWidget *gwin1;
Window xwin0;
extern Display *dpy;
static GtkWidget *top_bin;
int current_gcin_inner_frame;

static GtkWidget *hbox_edit;
static PangoAttrList* attr_list, *attr_list_blank;
extern gboolean test_mode;

static void compact_win0();
void move_win0(int x, int y);
void get_win0_geom();
gboolean gcin_edit_display_ap_only();

static struct {
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *line;
} chars[MAX_PH_BF_EXT];


static GtkWidget *button_pho;
static GtkWidget *label_pho;
extern char text_pho[];
extern int text_pho_N;
static GtkWidget *button_eng_ph;
static int max_yl;

static void create_win0_gui();

static void recreate_win0()
{
  bzero(chars, sizeof(chars));
  label_pho = NULL;

  create_win0_gui();
}


#if USE_TSIN
void change_win0_style()
{
  if (!top_bin || current_gcin_inner_frame == gcin_inner_frame)
    return;

  gtk_widget_destroy(top_bin);
  top_bin = NULL;

  current_gcin_inner_frame = gcin_inner_frame;
  recreate_win0();
}
#endif


void set_label_font_size();

/* there is a bug in gtk, if the widget is created and hasn't been processed by
   gtk_main(), the coodinate of the widget is sometimes invalid.
   We use pre-create to overcome this bug.
*/
static void create_char(int index)
{
  int i;

  GdkColor fg;
  gdk_color_parse(gcin_win_color_fg, &fg);
  GdkColor color_bg;
  gdk_color_parse(tsin_phrase_line_color, &color_bg);

  for(i=index; i<=index+1 && i < MAX_PH_BF_EXT; i++) {
    if (chars[i].vbox)
      continue;

    GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox_edit), vbox, FALSE, FALSE, 0);
    chars[i].vbox = vbox;

    GtkWidget *label = gtk_label_new(NULL);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

    set_label_font_size(label, gcin_font_size);

    chars[i].label = label;
    GtkWidget *separator =  gtk_drawing_area_new();
    gtk_widget_modify_bg(separator, GTK_STATE_NORMAL, &color_bg);
    gtk_widget_set_size_request(separator, 8, 2);
    gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, FALSE, 0);
    chars[i].line = separator;

    if (gcin_win_color_use)
      gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &fg);

    gtk_widget_show(vbox);
    gtk_widget_show(label);
  }
}

static void change_tsin_line_color()
{
  int i;

  GdkColor fg;
  gdk_color_parse(gcin_win_color_fg, &fg);
  GdkColor color_bg;
  gdk_color_parse(tsin_phrase_line_color, &color_bg);

  for(i=0; i < MAX_PH_BF_EXT; i++) {
    if (!chars[i].line)
      continue;
    gtk_widget_modify_bg(chars[i].line, GTK_STATE_NORMAL, &color_bg);
    gtk_widget_modify_fg(chars[i].label, GTK_STATE_NORMAL, gcin_win_color_use ? &fg:NULL);
  }
}



extern gboolean b_use_full_space;

void set_label_space();
void set_label_space(GtkWidget *label);

void disp_char(int index, char *ch)
{
  char tt[CH_SZ+1];

  if (gcin_edit_display_ap_only())
    return;

//  dbg("disp_char %d %c%c%c\n", index, ch[0], ch[1], ch[2]);
  create_char(index);
  GtkWidget *label = chars[index].label;

  utf8cpy(tt, ch);

  if (ch[0]==' ' && ch[1]==' ')
      set_label_space(label);
  else {
    gtk_label_set_text(GTK_LABEL(label), tt);
  }

  get_win0_geom();
  if (win_x + win_xl >= dpy_xl)
    move_win0(dpy_xl - win_xl, win_y);

  gtk_widget_show(chars[index].vbox);
  gtk_widget_show(label);
}

void hide_char(int index)
{
  if (test_mode)
    return;

  if (!chars[index].vbox)
    return;
  gtk_label_set_text(GTK_LABEL(chars[index].label), "");
  gtk_widget_hide_all(chars[index].vbox);
}


void clear_chars_all()
{
  int i;

  if (test_mode)
    return;

  for(i=0; i < MAX_PH_BF_EXT; i++) {
    hide_char(i);
  }

  compact_win0();
}


void draw_underline(int index)
{
  if (gcin_edit_display_ap_only())
    return;

  create_char(index);

  gtk_widget_show(chars[index].line);
}

void clear_underline(int index)
{
  if (gcin_edit_display_ap_only())
    return;

  gtk_widget_hide(chars[index].line);
}

void set_cursor_tsin(int index)
{
  GtkWidget *label = chars[index].label;

  if (!label)
    return;

  if (gcin_edit_display_ap_only())
    return;

  gtk_label_set_attributes(GTK_LABEL(label), attr_list);
}

void clr_tsin_cursor(int index)
{
  GtkWidget *label = chars[index].label;

  if (!label)
    return;

  if (test_mode)
    return;

  gtk_label_set_attributes(GTK_LABEL(label), attr_list_blank);
}

void disp_pho_sub(GtkWidget *label, int index, char *pho);
gboolean gcin_on_the_spot_key_is_on();
void hide_win0();

void disp_tsin_pho(int index, char *pho)
{
  if (test_mode)
    return;

  if (gcin_on_the_spot_key) {
    if (gwin0 && GTK_WIDGET_VISIBLE(gwin0))
      hide_win0();
    return;
  }

  if (button_pho && !GTK_WIDGET_VISIBLE(button_pho))
    gtk_widget_show(button_pho);

  disp_pho_sub(label_pho, index, pho);
}

void disp_tsin_pho(int index, char *pho);
void clr_in_area_pho_tsin()
{
  int i;

  if (test_mode)
    return;

  for(i=0; i < text_pho_N; i++)
   disp_tsin_pho(i, " ");
}

void get_widget_xy(GtkWidget *win, GtkWidget *widget, int *rx, int *ry)
{
  if (!win && !widget)
    p_err("get_widget_xy err");

//  gtk_widget_show_all(widget);
  gdk_flush();

  GtkRequisition sz;
  gtk_widget_size_request(widget, &sz);
  int wx, wy;

  wx=wy=0;

  gtk_widget_translate_coordinates(widget, win,
         0, sz.height, &wx, &wy);

  gtk_widget_translate_coordinates(widget, win,
         0, sz.height, &wx, &wy);

//  dbg("%d wx:%d\n", index,  wx);

  int win_x, win_y;

  gtk_window_get_position(GTK_WINDOW(win), &win_x, &win_y);
  int win_xl, win_yl;
  get_win_size(win, &win_xl, &win_yl);

  if (wx > win_xl)
    wx = win_xl;

  *rx = win_x + wx;
  *ry = win_y + wy;
}

void getRootXY(Window win, int wx, int wy, int *tx, int *ty);
void disp_selections(int x, int y);
void disp_tsin_select(int index)
{
  int x,y;

  if (test_mode)
    return;

#if 0
  GtkWidget *widget =chars[index].line;

  if (!GTK_WIDGET_VISIBLE(widget)) {
    widget = chars[index].vbox;
  }
#endif
#if 0
  GtkWidget *widget = chars[index].label;
#else
  GtkWidget *widget = chars[index].vbox;
#endif

  if (gcin_edit_display_ap_only()) {
    getRootXY(current_CS->client_win, current_CS->spot_location.x, current_CS->spot_location.y, &x, &y);
  } else {
    int i;
    for(i=0;i<=index;i++) {
      if (!chars[i].vbox)
        p_err("err found");
      gtk_widget_show(chars[i].vbox);
	  gtk_widget_show(chars[i].label);
    }
    gtk_widget_show(gwin0);
    get_widget_xy(gwin0, widget, &x, &y);
  }
  disp_selections(x, y);
}

#define MIN_X_SIZE 32

static int best_win_x, best_win_y;

static void raw_move(int x, int y)
{
  int xl, yl;

  if (!gwin0)
    return;

  get_win_size(gwin0, &xl, &yl);

  if (x + xl > dpy_xl)
    x = dpy_xl - xl;
  if (y + yl > dpy_yl)
    y = dpy_yl - yl;

  gtk_window_move(GTK_WINDOW(gwin0), x, y);
//  dbg("gwin0:%x raw_move %d,%d\n", gwin0, x, y);
}

void compact_win0_x()
{
  if (test_mode)
    return;
#if 0
  int win_xl, win_yl;
  gtk_window_get_size(GTK_WINDOW(gwin0), &win_xl, &win_yl);
  if (max_yl < win_yl)
    max_yl = win_yl;
  gtk_window_resize(GTK_WINDOW(gwin0), MIN_X_SIZE, max_yl);
#else
  gtk_window_resize(GTK_WINDOW(gwin0), 16, 16);
#endif

  raw_move(best_win_x, best_win_y);
}

static void compact_win0()
{
  max_yl = 0;
  gtk_window_resize(GTK_WINDOW(gwin0), MIN_X_SIZE, 16);
  raw_move(best_win_x, best_win_y);
}

gboolean tsin_has_input();
GtkWidget *gwin_sym;

void move_win0(int x, int y)
{
//  dbg("--- gwin0:%x move_win0 %d,%d\n", gwin0, x,y);
  best_win_x = x;
  best_win_y = y;

  if (gwin0)
    gtk_window_get_size(GTK_WINDOW(gwin0), &win_xl, &win_yl);

  if (x + win_xl > dpy_xl)
    x = dpy_xl - win_xl;
  if (x < 0)
    x = 0;

  if (y + win_yl > dpy_yl)
    y = dpy_yl - win_yl;
  if (y < 0)
    y = 0;

//  dbg("move_win0 %d,%d\n",x, y);

  if (gwin0)
    gtk_window_move(GTK_WINDOW(gwin0), x, y);

  win_x = x;
  win_y = y;

#if WIN32
  if (gwin1 && GTK_WIDGET_VISIBLE(gwin1)) {
    gtk_window_move(GTK_WINDOW(gwin1), x, y);
  }
#endif

  move_win_sym();
}

GtkWidget *gwin1;

void disp_tsin_eng_pho(int eng_pho)
{
  static unich_t *eng_pho_strs[]={_L("英"),_L("注")};

  if (!button_eng_ph)
    return;

  gtk_button_set_label(GTK_BUTTON(button_eng_ph), _(eng_pho_strs[eng_pho]));
}

void clear_tsin_line()
{
  int i;

  if (test_mode)
    return;

  for(i=0; i < MAX_PH_BF_EXT; i++) {
    GtkWidget *line = chars[i].line;
    if (!line)
      continue;
    gtk_widget_hide(line);
  }
}

void exec_gcin_setup();
void toggle_win_sym();

static void mouse_button_callback( GtkWidget *widget,GdkEventButton *event, gpointer data)
{
//  dbg("mouse_button_callback %d\n", event->button);
  switch (event->button) {
    case 1:
      toggle_win_sym();
      break;
    case 2:
      inmd_switch_popup_handler(widget, (GdkEvent *)event);
      break;
    case 3:
      exec_gcin_setup();
      break;
  }

}


void tsin_toggle_eng_ch();

#if 0
static void cb_clicked_eng_ph()
{
  tsin_toggle_eng_ch();
}
#endif

void set_no_focus();


void create_win0()
{
  if (gwin0)
    return;
#if _DEBUG
  dbg("create_win0\n");
#endif
  gwin0 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
#if WIN32
  set_no_focus(gwin0);
#endif
  gtk_container_set_border_width (GTK_CONTAINER (gwin0), 0);
  gtk_widget_realize (gwin0);
#if UNIX
  GdkWindow *gdkwin0 = gtk_widget_get_window(gwin0);
  xwin0 = GDK_WINDOW_XWINDOW(gdkwin0);
  set_no_focus(gwin0);
#else
  win32_init_win(gwin0);
#endif
}


void create_win1();

static void create_cursor_attr()
{
  if (attr_list)
    pango_attr_list_unref(attr_list);

  GdkColor color_bg, color_fg;
  gdk_color_parse(tsin_cursor_color, &color_bg);
  gdk_color_parse("white", &color_fg);

  attr_list = pango_attr_list_new ();
  attr_list_blank = pango_attr_list_new ();

  PangoAttribute *blue_bg = pango_attr_background_new(
    color_bg.red, color_bg.green, color_bg.blue);
  blue_bg->start_index = 0;
  blue_bg->end_index = 128;
  pango_attr_list_insert (attr_list, blue_bg);

  PangoAttribute *white_fg = pango_attr_foreground_new(
    color_fg.red, color_fg.green, color_fg.blue);
  white_fg->start_index = 0;
  white_fg->end_index = 128;
  pango_attr_list_insert (attr_list, white_fg);
}

void create_win1_gui();
static void set_win0_bg()
{
#if 1
  change_win_bg(gwin0);
#endif
}

void change_win1_font();

static void create_win0_gui()
{
  if (top_bin)
    return;

  GtkWidget *vbox_top = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (gwin0), 0);

  if (gcin_inner_frame) {
    GtkWidget *frame;
    top_bin = frame = gtk_frame_new(NULL);
    gtk_container_set_border_width (GTK_CONTAINER (frame), 0);
    gtk_container_add (GTK_CONTAINER(gwin0), frame);
    gtk_container_add (GTK_CONTAINER (frame), vbox_top);
  } else {
    top_bin = vbox_top;
    gtk_container_add (GTK_CONTAINER (gwin0), vbox_top);
  }

  bzero(chars, sizeof(chars));

  GtkWidget *hbox_row1 = gtk_hbox_new (FALSE, 0);
  /* This packs the button into the gwin0 (a gtk container). */
  gtk_box_pack_start (GTK_BOX (vbox_top), hbox_row1, FALSE, FALSE, 0);

  hbox_edit = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox_edit), 0);
  /* This packs the button into the gwin0 (a gtk container). */
  gtk_box_pack_start (GTK_BOX (hbox_row1), hbox_edit, FALSE, FALSE, 0);

  create_cursor_attr();

  button_pho = gtk_button_new();
  gtk_container_set_border_width (GTK_CONTAINER (button_pho), 0);
  gtk_box_pack_start (GTK_BOX (hbox_row1), button_pho, FALSE, FALSE, 0);

  g_signal_connect(G_OBJECT(button_pho),"button-press-event",
                   G_CALLBACK(mouse_button_callback), NULL);

  if (left_right_button_tips) {
#if GTK_CHECK_VERSION(2,12,0)
    gtk_widget_set_tooltip_text (button_pho, _(_L("左鍵符號，右鍵設定")));
#else
    GtkTooltips *button_pho_tips = gtk_tooltips_new ();
    gtk_tooltips_set_tip (GTK_TOOLTIPS (button_pho_tips), button_pho, _(_L"左鍵符號，右鍵設定"),NULL);
#endif
  }

  label_pho = gtk_label_new("");
  set_label_font_size(label_pho, gcin_font_size_tsin_pho_in);
  gtk_container_add (GTK_CONTAINER (button_pho), label_pho);

  clr_in_area_pho_tsin();

  gtk_widget_show_all (gwin0);
  gdk_flush();
  gtk_widget_hide (gwin0);

//  compact_win0();

  create_win1();
  create_win1_gui();

  set_win0_bg();

  change_win1_font();
}


#if USE_TSIN
void destroy_win0()
{
  if (!gwin0)
    return;
  bzero(chars, sizeof(chars));
  gtk_widget_destroy(gwin0);
  gwin0 = NULL;
  top_bin = NULL;
  label_pho = NULL;
  button_pho = NULL;
  button_eng_ph = NULL;
}
#endif

void get_win0_geom()
{
  gtk_window_get_position(GTK_WINDOW(gwin0), &win_x, &win_y);

  get_win_size(gwin0, &win_xl, &win_yl);
}

gboolean tsin_has_input();
extern gboolean force_show;
void raise_tsin_selection_win();

void show_win0()
{
  if (test_mode)
    return;

#if _DEBUG
	dbg("show_win0 pop:%d in:%d for:%d \n", gcin_pop_up_win, tsin_has_input(), force_show);
#endif
  create_win0();
  create_win0_gui();

  if (gcin_pop_up_win && !tsin_has_input() && !force_show)
    return;
#if UNIX
  if (!GTK_WIDGET_VISIBLE(gwin0))
#endif
  {
    gtk_widget_show(gwin0);
    move_win0(win_x, win_y);
  }

  show_win_sym();

  if (current_CS->b_raise_window)
  {
    gtk_window_present(GTK_WINDOW(gwin0));
    raise_tsin_selection_win();
  }
}

void hide_selections_win();
void hide_win0()
{
  if (test_mode)
    return;
  if (!gwin0)
    return;
#if UNIX
  gtk_widget_hide(gwin0);
#else
  destroy_win0();
#endif
  hide_selections_win();
  hide_win_sym();
}

void bell();


#if USE_TSIN
void change_tsin_font_size()
{
  int i;

  if (!top_bin)
    return;

  GdkColor fg;
  gdk_color_parse(gcin_win_color_fg, &fg);

  set_label_font_size(label_pho, gcin_font_size_tsin_pho_in);

  for(i=0; i < MAX_PH_BF_EXT; i++) {
    GtkWidget *label = chars[i].label;

    set_label_font_size(label, gcin_font_size);

    if (gcin_win_color_use)
      gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &fg);
  }

  compact_win0();

  change_win1_font();

  set_win0_bg();
  change_tsin_line_color();
}
#endif



void show_button_pho(gboolean bshow)
{
  if (bshow)
    gtk_widget_show(button_pho);
  else {
    gtk_widget_hide(button_pho);
    compact_win0();
  }
}

char *get_full_str();

void win_tsin_disp_half_full()
{
  if (test_mode)
    return;
  gtk_label_set_text(GTK_LABEL(label_pho), get_full_str());

  compact_win0();
}


void drawcursor();

#if USE_TSIN
void change_tsin_color()
{
  change_tsin_line_color();
  create_cursor_attr();

  drawcursor();
}
#endif
