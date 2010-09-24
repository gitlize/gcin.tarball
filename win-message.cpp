#include "gcin.h"

static gboolean timeout_destroy_window(GtkWidget *win)
{
  gtk_widget_destroy(win);
  return FALSE;
}

#if !TRAY_ENABLED
GdkWindow *tray_da_win;
#endif
extern GdkWindow *tray_da_win;
extern GtkStatusIcon *icon_main;

static void create_win_message(char *icon, char *text, int duration)
{
  GtkWidget *gwin_message = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width (GTK_CONTAINER (gwin_message), 0);
  gtk_widget_realize (gwin_message);
  GdkWindow *gdkwin = gtk_widget_get_window(gwin_message);
  set_no_focus(gwin_message);

  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (gwin_message), hbox);

  if (icon[0] != '-') {
    GtkWidget *image = gtk_image_new_from_file(icon);
    if (text[0] == '-') {
      GdkBitmap *bitmap = NULL;
      gdk_pixbuf_render_pixmap_and_mask(gdk_pixbuf_new_from_file(icon, NULL), NULL, &bitmap, 128);
      gtk_widget_shape_combine_mask(gwin_message, bitmap, 0, 0);
    }
    gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  }

  if (text[0] != '-') {
    GtkWidget *label = gtk_label_new(text);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  }

  gtk_widget_show_all(gwin_message);

  int width, height;
  get_win_size(gwin_message, &width, &height);

  int ox=-1, oy, szx, szy;
  if (tray_da_win) {
    gdk_window_get_origin  (tray_da_win, &ox, &oy);
    gdk_drawable_get_size(tray_da_win, &szx, &szy);

    if (oy<height) {
      oy = szy;
    } else {
      oy -= height;
      if (oy + height > dpy_yl)
        oy = dpy_yl - height;
      if (oy < 0)
        oy = 0;
    }

    if (ox + width > dpy_xl)
      ox = dpy_xl - width;
    if (ox < 0)
      ox = 0;
  } else
  if (icon_main) {
    GdkRectangle rect;
    GtkOrientation ori;
    if (gtk_status_icon_get_geometry(icon_main, NULL, &rect, &ori)) {
      dbg("rect %d,%d\n", rect.x, rect.y, rect.width, rect.height);
      if (ori==GTK_ORIENTATION_HORIZONTAL) {
        ox=rect.x;
        if (rect.y > 100)
          oy=rect.y - height;
        else
          oy=rect.y + rect.height;
      } else {
        oy=rect.y;
        if (rect.x > 100)
          ox=rect.x - width;
        else
          ox=rect.x + rect.width;
      }
    }
  }

  if (ox < 0) {
    ox = dpy_xl - width;
    oy = dpy_yl - height;
  }

  gtk_window_move(GTK_WINDOW(gwin_message), ox, oy);

  g_timeout_add(duration, (GSourceFunc)timeout_destroy_window, gwin_message);
}

void execute_message(char *message)
{
  char head[32];
  char icon[128];
  char text[128];
  int duration = 3000;

  icon[0] = text[0] = 0;

  sscanf(message, "%s %s %s %d", head, icon, text, &duration);

  create_win_message(icon, text, duration);
}
