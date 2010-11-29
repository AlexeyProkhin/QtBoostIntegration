#include "qtboostintegration.h"
#include "qtboostintegrationbindingobject_p.h"

#include <QtCore/QThread>
#include <QtCore/QThreadStorage>

static QThreadStorage<QtBoostIntegrationBindingObject *> s_bindingObjects;

QtBoostAbstractConnectionAdapter::QtBoostAbstractConnectionAdapter(QtBoostConnectionAdapterVtable *vt)
    : m_vtable(vt)
{
}

QtBoostAbstractConnectionAdapter::~QtBoostAbstractConnectionAdapter()
{
    if (m_vtable) {
        void (*destroy)(QtBoostAbstractConnectionAdapter *) = m_vtable->destroy;
        m_vtable = 0;
        (*destroy)(this);
    }
}

void QtBoostAbstractConnectionAdapter::invoke(void **args)
{
    m_vtable->invoke(this, args);
}

bool QtBoostAbstractConnectionAdapter::connect(QObject *sender, const char *signal,
               QObject *receiver, QtBoostAbstractConnectionAdapter *adapter,
               int nrArguments, int argumentTypeList[],
               Qt::ConnectionType connType)
{
    QtBoostIntegrationBindingObject *bindingObj = s_bindingObjects.localData();
    if (!bindingObj) {
        bindingObj = new QtBoostIntegrationBindingObject(QThread::currentThread());
        s_bindingObjects.setLocalData(bindingObj);
    }

    return bindingObj->bind(sender, signal, receiver, adapter,
                            nrArguments, argumentTypeList, connType);
}

bool qtBoostDisconnect(QObject *sender, const char *signal, QObject *receiver)
{
    QtBoostIntegrationBindingObject *bindingObj = s_bindingObjects.localData();
    if (!bindingObj)
        return false;

    return bindingObj->unbind(sender, signal, receiver);
}
