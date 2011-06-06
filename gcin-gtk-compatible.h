#if !GTK_CHECK_VERSION(2,4,0)
#define GTK_COMBO_BOX GTK_OPTION_MENU
#define gtk_combo_box_get_active gtk_option_menu_get_history
#define gtk_combo_box_set_active gtk_option_menu_set_history
#define gtk_combo_box_new_text gtk_option_menu_new
#endif

#if !GTK_CHECK_VERSION(2,13,4)
#define gtk_widget_get_window(x) (x)->window
#define gtk_color_selection_dialog_get_color_selection(x) (x)->colorsel
#endif

#if !GTK_CHECK_VERSION(2,15,0)
#define gtk_status_icon_set_tooltip_text(x,y) gtk_status_icon_set_tooltip(x,y)
#endif

#if GTK_CHECK_VERSION(2,17,5)
#undef GTK_WIDGET_NO_WINDOW
#define GTK_WIDGET_NO_WINDOW !gtk_widget_get_has_window
#undef GTK_WIDGET_SET_FLAGS
#define GTK_WIDGET_SET_FLAGS(x,y) gtk_widget_set_can_default(x,1)
#endif

#if GTK_CHECK_VERSION(2,17,7)
#undef GTK_WIDGET_VISIBLE
#define GTK_WIDGET_VISIBLE gtk_widget_get_visible
#endif

#if GTK_CHECK_VERSION(2,17,10)
#undef GTK_WIDGET_DRAWABLE
#define GTK_WIDGET_DRAWABLE gtk_widget_is_drawable
#endif

#if GTK_CHECK_VERSION(2,19,5)
#undef GTK_WIDGET_REALIZED
#define GTK_WIDGET_REALIZED gtk_widget_get_realized
#endif

#if GTK_CHECK_VERSION(2,21,8)
#undef GDK_DISPLAY
#define GDK_DISPLAY() GDK_DISPLAY_XDISPLAY(gdk_display_get_default())
#endif

#if GTK_CHECK_VERSION(2,91,0)
#define GTK_OBJECT
#endif

#if !GTK_CHECK_VERSION(2,91,1)
#define gtk_window_set_has_resize_grip(x,y);
#endif

#ifndef GTK_COMBO_BOX_TEXT
#define GTK_COMBO_BOX_TEXT GTK_COMBO_BOX
#endif

#if GTK_CHECK_VERSION(2,24,0)
#define gtk_combo_box_new_text gtk_combo_box_text_new
#define gtk_combo_box_append_text gtk_combo_box_text_append_text
#define gtk_widget_hide_all gtk_widget_hide
#endif

#if GTK_CHECK_VERSION(2,91,6)
#define GDK_WINDOW_XWINDOW GDK_WINDOW_XID
#endif
