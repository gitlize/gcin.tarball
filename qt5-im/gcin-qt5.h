#pragma once

#include <qpa/qplatforminputcontextplugin_p.h>
#include <QtCore/QStringList>

#include "qgcinplatforminputcontext.h"

class QGcinPlatformInputContextPlugin : public QPlatformInputContextPlugin
{
    Q_OBJECT
public:
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QPlatformInputContextFactoryInterface" FILE "gcin.json")
    QStringList keys() const;
    QGcinPlatformInputContext *create(const QString& system, const QStringList& paramList);
};
