#include "gcin.h"
#include "pho.h"

static GtkWidget *gwin1, *frame;
Window xwin1;

#define SELEN (12)

static GtkWidget *labels_sele[SELEN], *labels_seleR[SELEN];
static GtkWidget *eve_sele[SELEN], *eve_seleR[SELEN];
static GtkWidget *arrow_up, *arrow_down;

void hide_selections_win();
gboolean tsin_page_up(), tsin_page_down();

static gboolean button_scroll_event_tsin(GtkWidget *widget,GdkEventScroll *event, gpointer user_data)
{
  switch (event->direction) {
    case GDK_SCROLL_UP:
      tsin_page_up();
      break;
    case GDK_SCROLL_DOWN:
      tsin_page_down();
      break;
    default:
      break;
  }

  return TRUE;
}



void create_win1()
{
  if (gwin1)
    return;

  gwin1 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
#if WIN32
  set_no_focus(gwin1);
#endif
  gtk_widget_realize (gwin1);

#if UNIX
  GdkWindow *gdkwin1 = gtk_widget_get_window(gwin1);
  xwin1 = GDK_WINDOW_XWINDOW(gdkwin1);
  set_no_focus(gwin1);
#else
  win32_init_win(gwin1);
#endif

  g_signal_connect (G_OBJECT (gwin1), "scroll-event", G_CALLBACK (button_scroll_event_tsin), NULL);
}

void change_win1_font(), force_preedit_shift();
int tsin_sele_by_idx(int c);

static void mouse_button_callback( GtkWidget *widget,GdkEventButton *event, gpointer data)
{
  int v;
  switch (event->button) {
    case 1:
      v = GPOINTER_TO_INT(data);
      tsin_sele_by_idx(v);
      force_preedit_shift();
      break;
  }
}

void create_win1_gui()
{
  if (frame)
    return;

  frame = gtk_frame_new(NULL);
  gtk_container_add (GTK_CONTAINER(gwin1), frame);

  GtkWidget *vbox_top = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER(frame), vbox_top);

  arrow_up = gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox_top), arrow_up, FALSE, FALSE, 0);

  GtkWidget *table = gtk_table_new(SELEN, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox_top), table, FALSE, FALSE, 0);

  int i;
  for(i=0; i < SELEN; i++) {
    GtkWidget *align = gtk_alignment_new(0,0,0,0);
    gtk_table_attach_defaults(GTK_TABLE(table),align, 0,1,i,i+1);
    GtkWidget *event_box_pho = gtk_event_box_new();
    GtkWidget *label = gtk_label_new(NULL);
    gtk_container_add (GTK_CONTAINER (event_box_pho), label);
    labels_sele[i] = label;
    eve_sele[i] = event_box_pho;
    gtk_container_add (GTK_CONTAINER (align), event_box_pho);
    gtk_label_set_justify(GTK_LABEL(labels_sele[i]),GTK_JUSTIFY_LEFT);
    set_label_font_size(labels_sele[i], gcin_font_size_tsin_presel);
    g_signal_connect(G_OBJECT(event_box_pho),"button-press-event",
                   G_CALLBACK(mouse_button_callback), GINT_TO_POINTER(i));


    GtkWidget *alignR = gtk_alignment_new(0,0,0,0);
    gtk_table_attach_defaults(GTK_TABLE(table), alignR, 1,2,i,i+1);
    GtkWidget *event_box_phoR = gtk_event_box_new();
    GtkWidget *labelR = gtk_label_new(NULL);
    gtk_container_add (GTK_CONTAINER (event_box_phoR), labelR);
    labels_seleR[i] = labelR;
    eve_seleR[i] = event_box_phoR;
    gtk_container_add (GTK_CONTAINER (alignR), event_box_phoR);
    gtk_label_set_justify(GTK_LABEL(labels_sele[i]),GTK_JUSTIFY_LEFT);
    set_label_font_size(labels_seleR[i], gcin_font_size_tsin_presel);
    g_signal_connect(G_OBJECT(event_box_phoR),"button-press-event",
                   G_CALLBACK(mouse_button_callback), GINT_TO_POINTER(i));
  }

  arrow_down = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox_top), arrow_down, FALSE, FALSE, 0);

  gtk_widget_show_all(gwin1);
  gdk_flush();
  gtk_widget_hide(gwin1);

  change_win1_font();
}

void clear_sele()
{
  int i;

  if (!gwin1)
    create_win1();

  for(i=0; i < SELEN; i++) {
    gtk_widget_hide(labels_sele[i]);
    gtk_widget_hide(labels_seleR[i]);
  }

  gtk_widget_hide(arrow_up);
  gtk_widget_hide(arrow_down);
  gtk_window_resize(GTK_WINDOW(gwin1), 16, 20);
#if WIN32
  gdk_flush();
#endif
  hide_selections_win();
}

char *htmlspecialchars(char *s, char out[]);

void set_sele_text(int i, char *text, int len)
{
  char tt[128];
  char utf8[128];

  memcpy(utf8, text, len);
  utf8[len]=0;
  char uu[32], selma[128];

  char cc[2];
  cc[0]=phkbm.selkey[i];
  cc[1]=0;

  sprintf(selma, "<span foreground=\"%s\">%s</span>", gcin_sel_key_color, htmlspecialchars(cc, uu));

  if (tsin_tail_select_key) {
    char vv[128];
    snprintf(tt, sizeof(tt), "%s %s", htmlspecialchars(utf8, vv), selma);
  } else {
    gtk_label_set_text(GTK_LABEL(labels_seleR[i]), utf8);
    gtk_widget_show(labels_seleR[i]);
    snprintf(tt, sizeof(tt), "%s ", selma);
  }

  gtk_widget_show(labels_sele[i]);
  gtk_label_set_markup(GTK_LABEL(labels_sele[i]), tt);
}

#if WIN32
static int timeout_handle;
gboolean timeout_minimize_win1(gpointer data)
{
  gtk_window_resize(GTK_WINDOW(gwin1), 10, 20);
  gtk_window_present(GTK_WINDOW(gwin1));
  timeout_handle = 0;
  return FALSE;
}
#endif


void disp_selections(int x, int y)
{
  if (!gwin1)
    p_err("disp_selections !gwin1");

  if (!GTK_WIDGET_VISIBLE(gwin1)) {
    gtk_widget_show(gwin1);
  }

  int win1_xl, win1_yl;
  get_win_size(gwin1, &win1_xl, &win1_yl);

  if (x + win1_xl > dpy_xl)
    x = dpy_xl - win1_xl;
  if (y + win1_yl > dpy_yl)
    y = win_y - win1_yl;

  gtk_window_move(GTK_WINDOW(gwin1), x, y);
#if WIN32
  timeout_handle = g_timeout_add(50, timeout_minimize_win1, NULL);
#endif
}

void raise_tsin_selection_win()
{
  if (gwin1 && GTK_WIDGET_VISIBLE(gwin1))
    gtk_window_present(GTK_WINDOW(gwin1));
}


void hide_selections_win()
{
  if (!gwin1)
    return;
#if WIN32
  if (timeout_handle) {
	  g_source_remove(timeout_handle);
	  timeout_handle = 0;
  }
#endif

#if WIN32
  gtk_window_resize(GTK_WINDOW(gwin1), 10, 20);
#endif
  gtk_widget_hide(gwin1);
}

void disp_arrow_up()
{
  gtk_widget_show(arrow_up);
}

void disp_arrow_down()
{
  gtk_widget_show(arrow_down);
}

#if USE_TSIN
void destroy_win1()
{
  if (!gwin1)
    return;
  gtk_widget_destroy(gwin1);
  frame=NULL;
  gwin1 = NULL;
}
#endif

void change_win1_font()
{
  int i;

  GdkColor fg;
  gdk_color_parse(gcin_win_color_fg, &fg);

  for(i=0; i < SELEN; i++) {
    set_label_font_size(labels_sele[i], gcin_font_size_tsin_presel);
    set_label_font_size(labels_seleR[i], gcin_font_size_tsin_presel);
    if (labels_sele[i])
      gtk_widget_modify_fg(labels_sele[i], GTK_STATE_NORMAL, gcin_win_color_use?&fg:NULL);
    if (labels_seleR[i])
      gtk_widget_modify_fg(labels_seleR[i], GTK_STATE_NORMAL, gcin_win_color_use?&fg:NULL);
    change_win_bg(eve_sele[i]);
    change_win_bg(eve_seleR[i]);
  }

  change_win_bg(gwin1);
}
