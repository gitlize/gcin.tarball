// #pragma once

#include <qpa/qplatforminputcontext.h>

class QInputMethodEvent;
struct GCIN_client_handle_S;

class QGcinPlatformInputContext : public QPlatformInputContext
{
    Q_OBJECT
public:
    QGcinPlatformInputContext();
    virtual ~QGcinPlatformInputContext();

    virtual bool filterEvent(const QEvent* event);
    virtual bool isValid() const;
    virtual void invokeAction(QInputMethod::Action , int cursorPosition);
    virtual void reset();
    virtual void commit();
    virtual void update(Qt::InputMethodQueries quries );
    virtual void setFocusObject(QObject* object);

private:
	GCIN_client_handle_S *gcin_ch;
	void send_event(QInputMethodEvent e);
	void update_preedit();
	void cursorMoved();
	bool send_key_press(quint32 keysym, quint32 state);
	void commitPreedit();
	void send_str(char *s);
};
