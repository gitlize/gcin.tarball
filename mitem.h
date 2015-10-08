typedef struct {
  char *name;
  char *stock_id;
  void (*cb)(GtkCheckMenuItem *checkmenuitem, gpointer dat);
  int *check_dat;
  GtkWidget *item;
  gulong handler;
} MITEM;

GtkWidget *create_tray_menu(MITEM *mitems);
