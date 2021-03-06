#include <sys/stat.h>
#if UNIX
#include <X11/extensions/XTest.h>
#endif
#include "gcin.h"
#include "gtab.h"
extern INMD *cur_inmd;

static GtkWidget *gwin_kbm;
static GdkColor red;
int win_kbm_on;
extern gboolean test_mode;

enum {
  K_FILL=1,
  K_HOLD=2,
  K_PRESS=4,
  K_AREA_R=8,
  K_CAPSLOCK=16
};


typedef struct {
  KeySym keysym;
  char *enkey;
  char shift_key;
  char flag;
  GtkWidget *lab, *but, *laben;
} KEY;

#define COLN 19
static KEY keys[][COLN]={
{{XK_Escape,"Esc"},{XK_F1,"F1"},{XK_F2,"F2"},{XK_F3,"F3"},{XK_F4,"F4"},{XK_F5,"F5"},{XK_F6,"F6"},{XK_F7,"F7"},{XK_F8,"F8"},{XK_F9,"F9"},{XK_F10,"F10"},{XK_F11,"F11"},{XK_F12,"F12"},{XK_Print,"Pr",0,8},{XK_Scroll_Lock,"Slk",0,8},{XK_Pause,"Pau",0,8}},

{{'`'," ` ",'~'},{'1'," 1 ", '!'},{'2'," 2 ",'@'},{'3'," 3 ",'#'},{'4'," 4 ",'$'},{'5'," 5 ",'%'},{'6'," 6 ",'^'},{'7'," 7 ",'&'},{'8'," 8 ",'*'},{'9'," 9 ",'('},{'0'," 0 ",')'},{'-'," - ",'_'},{'='," = ",'+'},
{XK_BackSpace,"←",0, K_FILL},
{XK_Insert,"Ins",0,8},{XK_Home,"Ho",0,8}, {XK_Prior,"P↑",0,8}},

{{XK_Tab, "Tab"}, {'q'," q "},{'w'," w "},{'e'," e "},{'r'," r "},{'t'," t "}, {'y'," y "},{'u'," u "},{'i'," i "},{'o'," o "}, {'p'," p "},{'['," [ ",'{'},{']'," ] ",'}'},{'\\'," \\ ",'|',K_FILL},{XK_Delete,"Del",0,8},{XK_End,"En",0,8},
{XK_Next,"P↓",0,8}},

{{XK_Caps_Lock, "Caps", 0, K_CAPSLOCK},{'a'," a "},{'s'," s "},{'d'," d "},{'f', " f "},{'g'," g "},{'h'," h "},{'j'," j "},{'k'," k "},{'l'," l "},{';'," ; ",':'},{'\''," ' ",'"'},{XK_Return," Enter ",0,1},{XK_Num_Lock,"Num",0,8},{XK_KP_Add," + ",0,8}},

{{XK_Shift_L,"  Shift  ",0,K_HOLD},{'z'," z "},{'x'," x "},{'c'," c "},{'v'," v "},{'b'," b "},{'n'," n "},{'m'," m "},{','," , ",'<'},{'.'," . ",'>'},{'/'," / ",'?'},{XK_Shift_R," Shift",0,K_HOLD|K_FILL},{XK_KP_Multiply," * ",0,8},
{XK_Up,"↑",0,8}},
{{XK_Control_L,"Ctrl",0,K_HOLD},{XK_Super_L,"◆"},{XK_Alt_L,"Alt",0,K_HOLD},{' ',"Space",0, K_FILL}, {XK_Alt_R,"Alt",0,K_HOLD},{XK_Super_R,"◆"},{XK_Menu,"■"}, {XK_Control_R,"Ctrl",0,K_HOLD},
{XK_Left, "←",0,8},{XK_Down,"↓",0,8},{XK_Right, "→",0,8}}
};

static int keysN=sizeof(keys)/sizeof(keys[0]);

void update_win_kbm();

void mod_fg_all(GtkWidget *lab, GdkColor *col)
{
#if !GTK_CHECK_VERSION(2,91,6)
  gtk_widget_modify_fg(lab, GTK_STATE_NORMAL, col);
  gtk_widget_modify_fg(lab, GTK_STATE_ACTIVE, col);
  gtk_widget_modify_fg(lab, GTK_STATE_SELECTED, col);
  gtk_widget_modify_fg(lab, GTK_STATE_PRELIGHT, col);
#else
  GdkRGBA rgb, *prgb = NULL;
  if (col) {
    gdk_rgba_parse(&rgb, gdk_color_to_string(col));
    prgb = &rgb;
  }
  gtk_widget_override_color(lab, GTK_STATE_FLAG_NORMAL, prgb);
  gtk_widget_override_color(lab, GTK_STATE_FLAG_ACTIVE, prgb);
  gtk_widget_override_color(lab, GTK_STATE_FLAG_SELECTED, prgb);
  gtk_widget_override_color(lab, GTK_STATE_FLAG_PRELIGHT, prgb);
#endif
}


void send_fake_key_eve(KeySym key);
#if WIN32
void win32_FakeKey(UINT vk, bool key_press);
#endif

void send_fake_key_eve2(KeySym key, gboolean press)
{
#if WIN32
  win32_FakeKey(key, press);
#else
  KeyCode kc = XKeysymToKeycode(dpy, key);
  XTestFakeKeyEvent(dpy, kc, press, CurrentTime);
#endif
}


static int kbm_timeout_handle;

static gboolean timeout_repeat(gpointer data)
{
  KeySym k = GPOINTER_TO_INT(data);

  send_fake_key_eve2(k, TRUE);
  return TRUE;
}

static gboolean timeout_first_time(gpointer data)
{
  KeySym k = GPOINTER_TO_INT(data);
  dbg("timeout_first_time %c\n", k);
  send_fake_key_eve2(k, TRUE);
  kbm_timeout_handle = g_timeout_add(50, timeout_repeat, data);
  return FALSE;
}

static void clear_hold(KEY *k)
{
  KeySym keysym=k->keysym;
  GtkWidget *laben = k->laben;
  k->flag &= ~K_PRESS;
  mod_fg_all(laben, NULL);
  send_fake_key_eve2(keysym, FALSE);
}

static gboolean timeout_clear_hold(gpointer data)
{
  clear_hold((KEY *)data);
  return FALSE;
}

void clear_kbm_timeout_handle()
{
  if (!kbm_timeout_handle)
    return;
  g_source_remove(kbm_timeout_handle);
  kbm_timeout_handle = 0;
}

static void cb_button_click(GtkWidget *wid, KEY *k)
{
  KeySym keysym=k->keysym;
  GtkWidget *laben = k->laben;

  dbg("cb_button_click keysym %d\n", keysym);

  gtk_window_present(GTK_WINDOW(gwin_kbm));

  if (k->flag & K_HOLD) {
    if (k->flag & K_PRESS) {
      clear_hold(k);
    } else {
      send_fake_key_eve2(keysym, TRUE);
      k->flag |= K_PRESS;
      mod_fg_all(laben, &red);
      g_timeout_add(10000, timeout_clear_hold, GINT_TO_POINTER(k));
    }
  } else {
	clear_kbm_timeout_handle();
    kbm_timeout_handle = g_timeout_add(500, timeout_first_time, GINT_TO_POINTER(keysym));
    send_fake_key_eve2(keysym, TRUE);
  }
}


static void cb_button_release(GtkWidget *wid, KEY *k)
{
    dbg("cb_button_release %d\n", kbm_timeout_handle);
	clear_kbm_timeout_handle();

    send_fake_key_eve2(k->keysym, FALSE);

    int i;
    for(i=0;i<keysN;i++) {
      int j;
      for(j=0; keys[i][j].enkey; j++) {
        if (!(keys[i][j].flag & K_PRESS))
          continue;
        keys[i][j].flag &= ~K_PRESS;
                send_fake_key_eve2(keys[i][j].keysym, FALSE);
        mod_fg_all(keys[i][j].laben, NULL);
      }
    }
}


static void create_win_kbm()
{
  gdk_color_parse("red", &red);

  gwin_kbm = create_no_focus_win();

  gtk_container_set_border_width (GTK_CONTAINER (gwin_kbm), 0);
  GtkWidget *hbox_top = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (gwin_kbm), hbox_top);


  GtkWidget *vbox_l = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_top), vbox_l, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox_l), 0);
  GtkWidget *vbox_r = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_top), vbox_r, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox_r), 0);

  int i;
  for(i=0;i<keysN;i++) {
    GtkWidget *hboxl = gtk_hbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (hboxl), 0);
    gtk_box_pack_start (GTK_BOX (vbox_l), hboxl, TRUE, TRUE, 0);
    GtkWidget *hboxr = gtk_hbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (hboxr), 0);
    gtk_box_pack_start (GTK_BOX (vbox_r), hboxr, FALSE, FALSE, 0);
    KEY *pk = keys[i];

    int j;
    for(j=0; pk[j].enkey; j++) {
      KEY *ppk=&pk[j];
      char flag=ppk->flag;
      if (!ppk->keysym)
        continue;
      GtkWidget *but=pk[j].but=gtk_button_new();
      g_signal_connect (G_OBJECT (but), "pressed", G_CALLBACK (cb_button_click), ppk);
      if (!(ppk->flag & K_HOLD))
        g_signal_connect (G_OBJECT (but), "released", G_CALLBACK (cb_button_release), ppk);

      GtkWidget *hbox = (flag&K_AREA_R)?hboxr:hboxl;

      gtk_container_set_border_width (GTK_CONTAINER (but), 0);

      gboolean fill = (flag & K_FILL) > 0;
      gtk_box_pack_start (GTK_BOX (hbox), but, fill, fill, 0);

      GtkWidget *v = gtk_vbox_new (FALSE, 0);
      gtk_container_set_border_width (GTK_CONTAINER (v), 0);
      gtk_container_add (GTK_CONTAINER (but), v);
      GtkWidget *laben = ppk->laben=gtk_label_new(ppk->enkey);
      set_label_font_size(laben, gcin_font_size_win_kbm_en);

      gtk_box_pack_start (GTK_BOX (v), laben, fill, fill, 0);

      if (i>0&&i<5) {
        GtkWidget *lab = ppk->lab = gtk_label_new("  ");
//        set_label_font_size(lab, gcin_font_size_win_kbm);
        gtk_box_pack_start (GTK_BOX (v), lab, fill, fill, 0);
      }
    }
  }

  gtk_widget_realize (gwin_kbm);
#if WIN32
  win32_init_win(gwin_kbm);
#else
  GdkWindow *gdkwin_kbm = gtk_widget_get_window(gwin_kbm);
  set_no_focus(gwin_kbm);
#endif
}

extern GdkWindow *tray_da_win;
extern GtkStatusIcon *icon_main;

static void move_win_kbm()
{
  int width, height;
  get_win_size(gwin_kbm, &width, &height);

  int ox, oy;
  GdkRectangle r;
  GtkOrientation ori;

#if UNIX
  int szx, szy;
  if (tray_da_win) {
    gdk_window_get_origin(tray_da_win, &ox, &oy);
#if !GTK_CHECK_VERSION(2,91,0)
    gdk_drawable_get_size(tray_da_win, &szx, &szy);
#else
    szx = gdk_window_get_width(tray_da_win);
    szy = gdk_window_get_height(tray_da_win);
#endif
    if (oy<height) {
      oy = szy;
    } else {
      oy -= height;
      if (oy + height > dpy_yl)
        oy = dpy_yl - height;
      if (oy < 0)
        oy = szy;
    }

    if (ox + width > dpy_xl)
      ox = dpy_xl - width;
    if (ox < 0)
      ox = 0;
  } else
#endif
  if (icon_main && gtk_status_icon_get_geometry(icon_main, NULL, &r,  &ori)) {
//    dbg("rect %d:%d %d:%d\n", r.x, r.y, r.width, r.height);
    ox = r.x;
    if (ox + width > dpy_xl)
      ox = dpy_xl - width;

    if (r.y < 100)
      oy=r.y+r.height;
    else {
      oy = r.y - height;
    }
  } else {
    ox = dpy_xl - width;
    oy = dpy_yl - height - 31;
  }

  gtk_window_move(GTK_WINDOW(gwin_kbm), ox, oy);
}

void show_win_kbm()
{
  if (!gwin_kbm) {
    create_win_kbm();
    update_win_kbm();
  }

  gtk_widget_show_all(gwin_kbm);
  win_kbm_on = 1;
#if WIN32
  gtk_window_present(GTK_WINDOW(gwin_kbm));
#endif
  move_win_kbm();
}

static char   shift_chars[]="~!@#$%^&*()_+{}|:\"<>?";
static char shift_chars_o[]="`1234567890-=[]\\;',./";

#include "pho.h"

static KEY *get_keys_ent(KeySym keysym)
{
  int i;
  for(i=0;i<keysN;i++) {
    int j;
    for(j=0;j<COLN;j++) {
      char *p;
      if (keysym >='A' && keysym<='Z')
        keysym += 0x20;
      else
      if (p=strchr(shift_chars, keysym)) {
        keysym = shift_chars_o[p - shift_chars];
      }

      if (keys[i][j].keysym!=keysym)
        continue;
      return &keys[i][j];
    }
  }

  return NULL;
}

static void set_kbm_key(KeySym keysym, char *str)
{
  if (!gwin_kbm)
    return;
#if 0
  if (strlen(str)==1 && !(str[0] & 0x80))
    return;
#endif

  KEY *p = get_keys_ent(keysym);
  if (!p)
    return;

  GtkWidget *lab = p->lab;
  char *t = (char *)gtk_label_get_text(GTK_LABEL(lab));
  char tt[64];

  if (t && strcmp(t, str)) {
    strcat(strcpy(tt, t), str);
    str = tt;
  }

  if (lab) {
    gtk_label_set_text(GTK_LABEL(lab), str);
    set_label_font_size(lab, gcin_font_size_win_kbm);
  }
}

static void clear_kbm()
{
  int i;
  for(i=0;i<keysN;i++) {
    int j;
    for(j=0;j<COLN;j++) {
      GtkWidget *lab = keys[i][j].lab;
      if (lab)
        gtk_label_set_text(GTK_LABEL(lab), NULL);

      if (keys[i][j].laben)
        gtk_label_set_text(GTK_LABEL(keys[i][j].laben), keys[i][j].enkey);
    }
  }
}

static void disp_shift_keys()
{
      int i;
      for(i=127; i > 0; i--) {
        char tt[64];
          KEY *p = get_keys_ent(i);
          if (p && p->shift_key) {
            char *t = (char *)gtk_label_get_text(GTK_LABEL(p->lab));
            if (t && t[0])
              continue;
//            dbg("zzz %c %s\n",i, tt);
            tt[0]=p->shift_key;
            tt[1]=0;
            set_kbm_key(i, tt);
          }
      }
}


void update_win_kbm()
{
  if (!current_CS || !gwin_kbm)
    return;

  clear_kbm();

  if (current_CS->im_state != GCIN_STATE_CHINESE) {
    if (current_CS->im_state == GCIN_STATE_DISABLED) {
      int i;
      for(i=0;i<keysN;i++) {
        int j;
        for(j=0;j<COLN;j++) {
          char kstr[2];
          kstr[1]=0;
          kstr[0] = keys[i][j].shift_key;

          if (keys[i][j].laben) {
            if (kstr[0])
              gtk_label_set_text(GTK_LABEL(keys[i][j].laben), kstr);
            set_label_font_size(keys[i][j].laben, gcin_font_size_win_kbm_en);
          }

          if (keys[i][j].lab) {
            if (kstr[0])
              gtk_label_set_text(GTK_LABEL(keys[i][j].lab), keys[i][j].enkey);
            set_label_font_size(keys[i][j].lab, gcin_font_size_win_kbm_en);
          }
        }
      }
   }
   goto ret;
 }

  int i;
  switch (current_method_type()) {
    case method_type_PHO:
    case method_type_TSIN:
      for(i=0; i < 128; i++) {
        int j;
        char tt[64];
        int ttN=0;

        for(j=0;j<3; j++) {
          int num = phkbm.phokbm[i][j].num;
          int typ = phkbm.phokbm[i][j].typ;
          if (!num)
            continue;
          ttN+= utf8cpy(&tt[ttN], &pho_chars[typ][num * 3]);
        }

        if (!ttN)
         continue;
        set_kbm_key(i, tt);
      }

      disp_shift_keys();

      break;
    case method_type_MODULE:
      break;
    default:
      if (!cur_inmd || !cur_inmd->DefChars)
        return;

      int loop;
      for(loop=0;loop<2;loop++)
      for(i=127; i > 0; i--) {
        char tt[64];
        char k=cur_inmd->keymap[i];
        if (!k)
          continue;

        char *keyname = &cur_inmd->keyname[k * CH_SZ];
        if (!keyname[0])
          continue;

#if 0
        if (loop==0 && !(keyname[0]&0x80))
#else
		if (loop==0 && !(keyname[0]&0x80) && toupper(i)==toupper(keyname[0]))
#endif
          continue;

        if (loop==1) {
          KEY *p = get_keys_ent(i);
          char *t = (char *)gtk_label_get_text(GTK_LABEL(p->lab));
          if (t && t[0]) {
            continue;
          }
        }


        tt[0]=0;
        if (keyname[0] & 128)
          utf8cpy(tt, keyname);
        else {
          tt[1]=0;
          memcpy(tt, keyname, 2);
          tt[2]=0;
        }

//        dbg("%c '%s'\n", i, tt);
        set_kbm_key(i, tt);
      }

      disp_shift_keys();

      break;
  }

ret:
  gtk_window_resize(GTK_WINDOW(gwin_kbm), 1, 1);
  move_win_kbm();
}


void hide_win_kbm()
{
  if (!gwin_kbm)
    return;
#if WIN32
  if (test_mode)
    return;
#endif

  clear_kbm_timeout_handle();
  win_kbm_on = 0;
  gtk_widget_hide(gwin_kbm);
}

extern gboolean old_capslock_on;

void win_kbm_disp_caplock()
{
  KEY *p = get_keys_ent(XK_Caps_Lock);

  if (old_capslock_on) {
 //   dbg("lock...\n");
    mod_fg_all(p->laben, &red);
  } else {
    mod_fg_all(p->laben, NULL);
  }
}
