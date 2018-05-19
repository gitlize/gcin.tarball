include ../config.mak
QT=qt5
QTIM=$(QT)/plugins/platforminputcontexts
IMMODULES=$(libdir)/$(DEB_BUILD_MULTIARCH)/$(QTIM)

INCS=-I../im-client -I/usr/include/X11 `pkg-config Qt5Core Qt5Gui --cflags`
MODVERSION=`pkg-config --modversion Qt5Gui`
INCS+=-I`pkg-config --variable=includedir Qt5Core`/QtCore/$(MODVERSION)
INCS+=-I`pkg-config --variable=includedir Qt5Gui`/QtGui/$(MODVERSION)
INCS+=-I`pkg-config --variable=includedir Qt5Gui`/QtGui/$(MODVERSION)/QtGui
#INCS+=-I/usr/include/i386-linux-gnu/qt5/QtGui/$(MODVERSION)/QtGui -I/usr/include/x86_64-linux-gnu/qt5/QtGui/$(MODVERSION)/QtGui

CXXFLAGS=$(OPTFLAGS) $(INCS) -Wall -D_REENTRANT -DUNIX=1 -fPIC  -DQT5 -DQT_SHARED -DQT_IMMODULE -DPIC -DDEBUG="0$(GCIN_DEBUG)"
OBJS= moc_gcin-qt5.o gcin-qt5.o gcin-imcontext-qt5.o moc_qgcinplatforminputcontext.o
.SUFFIXES:	.c .cpp .a .so .E .h
LDFLAGS=-L../im-client -lgcin-im-client `pkg-config Qt5Core Qt5Gui --libs`

GCIN_QT5 = libgcinplatforminputcontextplugin.so

all:    $(GCIN_QT5)

.cpp.E:
	$(CXX) -E $(CFLAGS) $(INCS) $< > $@

moc_gcin-qt5.cpp:	gcin-qt5.h
	$(QT5_MOC) $< -o $@

moc_qgcinplatforminputcontext.cpp:	qgcinplatforminputcontext.h
	$(QT5_MOC) $< -o $@

$(GCIN_QT5): $(OBJS)
	export LD_RUN_PATH=$(gcinlibdir) ;\
	$(CXX) $(gcc_ld_run_path) -fno-exceptions -shared -o $@ $(OBJS) $(LDFLAGS)
	rm -f core.*
install:
	if [ $(prefix) = /usr/local ]; then \
		install -d $(DESTDIR)/$(QT5_IM_DIR); \
		install -m 755 $(GCIN_QT5) $(DESTDIR)/$(QT5_IM_DIR); \
	else \
		install -d $(IMMODULES); \
                install -m 755 $(GCIN_QT5) $(IMMODULES); \
	fi
clean:
	rm -f $(OBJS) $(GCIN_QT5) *~ core.* *moc*.cpp *.E .depend

.depend:
	$(CXX) $(CXXFLAGS) -MM *.cpp > $@

include .depend
