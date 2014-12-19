#include <QKeyEvent>
#include <QGuiApplication>
#include <QInputMethod>
#include <QTextCharFormat>
#include <QPalette>
#include <QWindow>

#include <unistd.h>
#include <errno.h>
#include <signal.h>
#if 0
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#else
// confliction of qt & x11
typedef unsigned int KeySym;
struct Display;
typedef unsigned int Window;
typedef struct {
    short x, y;
} XPoint;
#endif
#include "../util.h"
#include "gcin-im-client.h"
#include "qgcinplatforminputcontext.h"

static WId focused_win;

#include <qpa/qplatformnativeinterface.h>

#if DEBUG
FILE *out_fp;
void __gcin_dbg_(const char *fmt,...)
{
  va_list args;

  if (!out_fp) {
#if 0	  
	  out_fp = fopen("/tmp/a.txt", "w");
#else
	out_fp = stdout;
#endif	  
}

  va_start(args, fmt);
  vfprintf(out_fp, fmt, args);
  fflush(out_fp);
  va_end(args);
}
#endif

QGcinPlatformInputContext::QGcinPlatformInputContext() 
{
	dbg("QGcinPlatformInputContext::QGcinPlatformInputContext() \n");
   QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
    if(!native)
        return;
    Display *display = static_cast<Display *>(native->nativeResourceForWindow("display", NULL));	
    printf("display %p\n", display);
    
  if (!(gcin_ch = gcin_im_client_open(display))) {
    perror("cannot open gcin_ch");
    dbg("gcin_im_client_open error\n");
    return;
  }
  
  dbg("QGcinPlatformInputContext succ\n");
}

QGcinPlatformInputContext::~QGcinPlatformInputContext()
{
	if (gcin_ch==NULL)
		return;
	gcin_im_client_close(gcin_ch);
	gcin_ch = NULL;
}


bool QGcinPlatformInputContext::isValid() const
{
	dbg("QGcinPlatformInputContext::isValid()\n");
    return true;
}

void QGcinPlatformInputContext::invokeAction(QInputMethod::Action action, int cursorPosition)
{
	dbg("QGcinPlatformInputContext::invokeAction(n");
#if 0	
    if (action == QInputMethod::Click
        && (cursorPosition <= 0 || cursorPosition >= m_preedit.length())
    )
    {
        // qDebug() << action << cursorPosition;
        commitPreedit();
    }
#endif    
}

void QGcinPlatformInputContext::commitPreedit()
{
  dbg("QGcinPlatformInputContext::commitPreedit\n");
  // use this to flush
  int preedit_cursor_position=0;
  int sub_comp_len;
  char *str=NULL;
  GCIN_PREEDIT_ATTR att[GCIN_PREEDIT_ATTR_MAX_N];
  gcin_im_client_get_preedit(gcin_ch, &str, att, &preedit_cursor_position, &sub_comp_len);	
  if (str) {
	  if (strlen(str) > 0) {
		  dbg("send enter to flush\n");
		  send_key_press(0xff0d, 0); // Enter
      } else
		dbg("empty string\n");		
	  free(str);	  
	 update_preedit();
  }  else
	dbg("no str\n");
}


void QGcinPlatformInputContext::reset()
{
	dbg("QGcinPlatformInputContext::reset()\n");
 if (gcin_ch) {
    gcin_im_client_reset(gcin_ch);
    update_preedit();
}  	
}   

void QGcinPlatformInputContext::update(Qt::InputMethodQueries queries )
{
   dbg("QGcinPlatformInputContext::update\n");
    QObject *input = qApp->focusObject();
    if (!input)
        return;

    QInputMethodQueryEvent query(queries);
    QGuiApplication::sendEvent(input, &query);

    if (queries & Qt::ImCursorRectangle) {
        cursorMoved();
    }

#if 0
    if (queries & Qt::ImHints) {
        Qt::InputMethodHints hints = Qt::InputMethodHints(query.value(Qt::ImHints).toUInt());

       if (hints & Qt::ImhPreferNumbers)       
    }
#endif        
}

// this one is essential
void QGcinPlatformInputContext::commit()
{
	dbg("QGcinPlatformInputContext::commit()\n");
	commitPreedit();
//    QPlatformInputContext::commit();
}


void QGcinPlatformInputContext::setFocusObject(QObject* object)
{	
	dbg("QGcinPlatformInputContext::setFocusObject\n");
    QWindow *window = qApp->focusWindow();
    if (!window) {
		dbg("no window, focus out\n");
		focused_win = 0;
		char *rstr = NULL;
		gcin_im_client_focus_out2(gcin_ch, &rstr);
		if (rstr) {
			send_str(rstr);
		} else 
			dbg("no str in preedit\n");
		return;
	}
    
     WId win = window->winId();
     
     if (focused_win && win != focused_win) {
		if (gcin_ch)
			gcin_im_client_focus_out(gcin_ch);    
	}
	
	focused_win = win;

	if (gcin_ch) {
		gcin_im_client_set_window(gcin_ch, win);
		gcin_im_client_focus_in(gcin_ch);    
	}
}

static int last_x=-1, last_y=-1;
void QGcinPlatformInputContext::cursorMoved()
{
	dbg(" QGcinPlatformInputContext::cursorMoved()\n");

    QWindow *inputWindow = qApp->focusWindow();
    if (!inputWindow)
        return;

    QRect r = qApp->inputMethod()->cursorRectangle().toRect();
    if(!r.isValid())
        return;
#if 0
    r.moveTopLeft(inputWindow->mapToGlobal(r.topLeft()));
#endif    

   // gcin server will clear the string if the cursor is moved, make sure the x,y is valid
	int x = r.left(),  y = r.bottom();	
	if (x > inputWindow->width() || y > inputWindow->height() || x < 0 || y < 0)
		return;

  if (gcin_ch && (x !=  last_x || y != last_y)   ) {		 
	 last_x = x; last_y = y;
	 dbg("move cursor %d, %d\n", x, y);
    gcin_im_client_set_cursor_location(gcin_ch, x,  y);
  }
}



void QGcinPlatformInputContext::update_preedit()
{
  if (!gcin_ch)
    return;
  QList<QInputMethodEvent::Attribute> attrList;
//  QString preedit_string;
  int preedit_cursor_position=0;
  int sub_comp_len;
  char *str=NULL;
  GCIN_PREEDIT_ATTR att[GCIN_PREEDIT_ATTR_MAX_N];
  int attN = gcin_im_client_get_preedit(gcin_ch, &str, att, &preedit_cursor_position, &sub_comp_len);

  int ret;
  gcin_im_client_set_flags(gcin_ch, FLAG_GCIN_client_handle_use_preedit, &ret);

  QObject *input = qApp->focusObject();  
  
  if (!input || !str) {
    free(str);
    return;
  }
  
 
#if DBG || 0
  dbg"update_preedit attN:%d '%s'\n", attN, str);
#endif
  int i;
  for(i=0; i < attN; i++) {
    int ofs0 = att[i].ofs0;
    int len = att[i].ofs1 - att[i].ofs0;
    QTextCharFormat format;

    switch (att[i].flag) {
      case GCIN_PREEDIT_ATTR_FLAG_REVERSE:
          {
            QBrush brush;
            QPalette palette;
            palette = QGuiApplication::palette();
            format.setBackground(QBrush(QColor(palette.color(QPalette::Active, QPalette::Highlight))));
            format.setForeground(QBrush(QColor(palette.color(QPalette::Active, QPalette::HighlightedText))));                            
          }
          break;
      case GCIN_PREEDIT_ATTR_FLAG_UNDERLINE:
          {
              format.setUnderlineStyle(QTextCharFormat::DashUnderline);
          }
    }

	attrList.append(QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, ofs0,  len, format));    
  }

  attrList.append(QInputMethodEvent::Attribute(QInputMethodEvent::Cursor,  preedit_cursor_position, 1, 0));

  QInputMethodEvent im_event (QString::fromUtf8(str), attrList);
  send_event (im_event);
  free(str);
}

void QGcinPlatformInputContext::send_event(QInputMethodEvent e) {
    QObject *input = qApp->focusObject();
    if (!input)
        return;		
    QCoreApplication::sendEvent(input, &e);
}	

void QGcinPlatformInputContext::send_str(char *rstr) {
	dbg("send_str %s\n",  rstr);
	  QString inputText = QString::fromUtf8(rstr);
	  free(rstr);	
	  QInputMethodEvent commit_event;
	  commit_event.setCommitString (inputText);
	  send_event (commit_event);				  	
}	

bool QGcinPlatformInputContext::send_key_press(quint32 keysym, quint32 state) {
	dbg("send_key_press\n");
		char *rstr  = NULL;	
		int result = gcin_im_client_forward_key_press(gcin_ch, keysym, state, &rstr);

		  if (rstr) {
			  send_str(rstr);
		  }
			  
	return result;
}	

bool QGcinPlatformInputContext::filterEvent(const QEvent* event) 
{
	dbg("QGcinPlatformInputContext::filterEvent\n");
        if (event->type() != QEvent::KeyPress && event->type() != QEvent::KeyRelease) 
           goto ret;

        const QKeyEvent* keyEvent;
        keyEvent = static_cast<const QKeyEvent*>(event);
        quint32 keysym ;
        keysym = keyEvent->nativeVirtualKey();
//        quint32 keycode = keyEvent->nativeScanCode();
        quint32  state;
        state = keyEvent->nativeModifiers();

        if (!inputMethodAccepted())
            goto ret;

        QObject *input;
        input = qApp->focusObject();

        if (!input) 
			goto ret;
			
		int result;
#if 1
		if (event->type() == QEvent::KeyPress) {
			if (send_key_press(keysym, state)) {
				update_preedit();
				return true;
			}
		} else {
			char *rstr = NULL;			
			 result = gcin_im_client_forward_key_release(gcin_ch,   keysym, state, &rstr);
			if (rstr)
				free(rstr);
				
			if (result)
				return true;
		}
#endif

ret:
    return QPlatformInputContext::filterEvent(event);
}
