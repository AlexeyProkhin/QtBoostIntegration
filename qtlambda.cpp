#include "qtlambda.h"
#include "bindingobject_p.h"

#include <QtCore/QThread>
#include <QtCore/QThreadStorage>

namespace qtlambda {
namespace detail {

static QThreadStorage<BindingObject *> s_bindingObjects;

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

}; // namespace detail

bool disconnect(QObject *sender, const char *signal, QObject *receiver)
{
    detail::BindingObject *bindingObj = detail::s_bindingObjects.localData();
    if (!bindingObj)
        return false;

    return bindingObj->unbind(sender, signal, receiver);
}

}; // namespace qtlambda
