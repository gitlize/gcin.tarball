#include <stdio.h>
#include "gcin-qt5.h"
#include "../util.h"
#define GCINID "gcin"


QStringList QGcinPlatformInputContextPlugin::keys() const
{
	dbg("QStringList QGcinPlatformInputContextPlugin::keys()\n");
    return QStringList(QStringLiteral(GCINID));

}

QGcinPlatformInputContext *QGcinPlatformInputContextPlugin::create(const QString& system, const QStringList& paramList)
{		
    Q_UNUSED(paramList);
    dbg("QGcinPlatformInputContextPlugin::create()\n");
    
    if (system.compare(system, QStringLiteral(GCINID), Qt::CaseInsensitive) == 0)
        return new QGcinPlatformInputContext;
    return 0;
}
