#include "qtboostintegration.h"
#include "bindingobject_p.h"

#include <QtCore/QThread>
#include <QtCore/QThreadStorage>

static QThreadStorage<QtBoostIntegrationInternal::BindingObject *> s_bindingObjects;

namespace QtBoostIntegrationInternal {

bool doConnect(QObject *sender, const char *signal,
               QObject *receiver, connection_adapter_base *adapter,
               Qt::ConnectionType connType)
{
    BindingObject *bindingObj = s_bindingObjects.localData();
    if (!bindingObj) {
        bindingObj = new BindingObject(QThread::currentThread());
        s_bindingObjects.setLocalData(bindingObj);
    }

    return bindingObj->bind(sender, signal, receiver, adapter, connType);
}

}; // namespace QtBoostIntegrationInternal

bool qtBoostDisconnect(QObject *sender, const char *signal, QObject *receiver)
{
    QtBoostIntegrationInternal::BindingObject *bindingObj = s_bindingObjects.localData();
    if (!bindingObj)
        return false;

    return bindingObj->unbind(sender, signal, receiver);
}
