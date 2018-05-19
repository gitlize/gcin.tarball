gcc -x c -std=gnu99 -DUNIX=1  -g -pthread -I/usr/include/gtk-2.0 -I/usr/lib/x86_64-linux-gnu/gtk-2.0/include -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/pango-1.0 -I/usr/include/gio-unix-2.0/ -I/usr/include/freetype2 -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/pixman-1 -I/usr/include/libpng12 -I/usr/include/harfbuzz   -I./IMdkit/include -I./im-client -DDEBUG="01" -DGCIN_TABLE_DIR=\"/usr/share/gcin/table\" -DGCIN_OGG_DIR=\"/usr/share/gcin/ogg\" -DDOC_DIR=\"/usr/share/doc/gcin\" -DGCIN_ICON_DIR=\"/usr/share/icons/gcin\" -DGCIN_SCRIPT_DIR=\"/usr/share/gcin/script\" -DGCIN_BIN_DIR=\"/usr/bin\" -DSYS_ICON_DIR=\"/usr/share/icons\" -DFREEBSD=0 -DMAC_OS=0 -DG_DISABLE_SINGLE_INCLUDES -DG_DISABLE_DEPRECATED -DGDK_DISABLE_SINGLE_INCLUDES -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_DEPRECATED -DUSE_XIM=1 -DTRAY_ENABLED=1 -DUSE_TSIN=1 -DUSE_GCB=1 -DUSE_INDICATOR=1 -pthread -I/usr/include/libappindicator3-0.1 -I/usr/include/libdbusmenu-glib-0.4 -I/usr/include/gtk-3.0 -I/usr/include/atk-1.0 -I/usr/include/at-spi2-atk/2.0 -I/usr/include/pango-1.0 -I/usr/include/gio-unix-2.0/ -I/usr/include/cairo -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/harfbuzz -I/usr/include/freetype2 -I/usr/include/pixman-1 -I/usr/include/libpng12   -c -fpic -o pinyin.pico pinyin.cpp
gcc -Wl,--as-needed -shared -o gcin1.so win-message.pico win-sym.pico win-inmd-switch.pico pinyin.pico win-pho-near.pico win-kbm.pico gcin-module.pico pho2pinyin.pico -Wl,--as-needed -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgio-2.0 -lpangoft2-1.0 -lpangocairo-1.0 -lgdk_pixbuf-2.0 -lcairo -lpango-1.0 -lfontconfig -lgobject-2.0 -lglib-2.0 -lfreetype   -lX11 -ldl -lm
gcc  -Wl,-rpath,/usr/lib/x86_64-linux-gnu/gcin -o gcin gcin.o eve.o util.o gcin-conf.o gcin-settings.o locale.o gcin-icon.o about.o html-browser.o gcin-exec-script.o gcin1.so gcin2.so pho-play.o cache.o gtk_bug_fix.o phrase-save-menu.o win-pho.o pho.o pho-util.o pho-sym.o table-update.o pho-dbg.o gtab.o win-gtab.o gtab-util.o gtab-list.o gtab-buf.o gcin-common.o phrase.o t2s-lookup.o gtab-use-count.o win-save-phrase.o unix-exec.o pho-kbm-name.o statistic.o tsin-scan.o gcin-module.o lang.o gcin-module-cb.o gtab-init.o fullchar.o gtab-tsin-fname.o win-screen-status.o en-miss-cand.o IC.o tray.o eggtrayicon.o tray-win32.o tsin.o tsin-util.o win0.o win1.o tsin-parse.o gcb.o tray-indicator.o IMdkit/lib/libXimd.a im-addr.o im-dispatch.o im-srv.o gcin-crypt.o -lXtst -Wl,--as-needed -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgio-2.0 -lpangoft2-1.0 -lpangocairo-1.0 -lgdk_pixbuf-2.0 -lcairo -lpango-1.0 -lfontconfig -lgobject-2.0 -lglib-2.0 -lfreetype   -lX11 -ldl -lm -L/usr/X11R6/lib
rm -f core.* vgcore.*
ln -sf gcin gcin.test
make -C data
make[1]: Entering directory `/home/dhliu/gcin/data'
make[1]: Nothing to be done for `all'.
make[1]: Leaving directory `/home/dhliu/gcin/data'
make -C gtk-im
make[1]: Entering directory `/home/dhliu/gcin/gtk-im'
make[1]: Nothing to be done for `all'.
make[1]: Leaving directory `/home/dhliu/gcin/gtk-im'
if [ Y = 'Y' ]; then make -C modules; fi
make[1]: Entering directory `/home/dhliu/gcin/modules'
make[1]: Nothing to be done for `all'.
make[1]: Leaving directory `/home/dhliu/gcin/modules'
if [ N = 'Y' ]; then make -C po; fi
if [ Y = 'Y' ]; then make -C gtk3-im; fi
make[1]: Entering directory `/home/dhliu/gcin/gtk3-im'
make[1]: Nothing to be done for `all'.
make[1]: Leaving directory `/home/dhliu/gcin/gtk3-im'
if [ N = 'Y' ]; then make -C qt-im; fi
if [ Y = 'Y' ]; then make -C qt4-im; fi
make[1]: Entering directory `/home/dhliu/gcin/qt4-im'
make[1]: Nothing to be done for `all'.
make[1]: Leaving directory `/home/dhliu/gcin/qt4-im'
if [ Y = 'Y' ]; then make -C qt5-im; fi
make[1]: Entering directory `/home/dhliu/gcin/qt5-im'
make[1]: Nothing to be done for `all'.
make[1]: Leaving directory `/home/dhliu/gcin/qt5-im'
