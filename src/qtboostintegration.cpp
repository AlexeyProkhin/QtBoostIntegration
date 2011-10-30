/*
 * Copyright 2010  Benjamin K. Stuhl <bks24@cornell.edu>
 *           2011  Alexey Prokhin <alexey.prokhin@yandex.ru>
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

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
#ifdef QTBOOSTINTEGRATION_CHECK_SIGNATURE
               int nrArguments, int argumentTypeList[],
#endif
               Qt::ConnectionType connType)
{
    QtBoostIntegrationBindingObject *bindingObj = s_bindingObjects.localData();
    if (!bindingObj) {
        bindingObj = new QtBoostIntegrationBindingObject(QThread::currentThread());
        s_bindingObjects.setLocalData(bindingObj);
    }

    return bindingObj->bind(sender, signal, receiver, adapter,
#ifdef QTBOOSTINTEGRATION_CHECK_SIGNATURE
                            nrArguments, argumentTypeList,
#endif
                            connType);
}

bool ldisconnect(QObject *sender, const char *signal, QObject *receiver)
{
    QtBoostIntegrationBindingObject *bindingObj = s_bindingObjects.localData();
    if (!bindingObj)
        return false;

    return bindingObj->unbind(sender, signal, receiver);
}
