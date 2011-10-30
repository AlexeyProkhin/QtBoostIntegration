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

#include "qtboostintegrationbindingobject_p.h"
#include "qtboostintegration.h"

#include <QtCore/QMetaMethod>

class ConnectNotifyObject : public QObject {
public:
    void callConnectNotify(const char *signal)
    { connectNotify(signal); }
    void callDisconnectNotify(const char *signal)
    { disconnectNotify(signal); }
};

QtBoostIntegrationBindingObject::QtBoostIntegrationBindingObject(QObject *parent) :
    QObject(parent)
{
}

QtBoostIntegrationBindingObject::~QtBoostIntegrationBindingObject()
{
    foreach (const Binding& b, m_bindings) {
        delete b.adapter;
    }
}

static const uint qt_meta_data_QtBoostIntegrationBindingObject[] = {

 // content:
       5,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      33,   32,   32,   32, 0x0a,
      53,   32,   32,   32, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_QtBoostIntegrationBindingObject[] = {
    "QtBoostIntegrationBindingObject\0\0"
    "receiverDestroyed()\0senderDestroyed()\0"
};

const QMetaObject QtBoostIntegrationBindingObject::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_QtBoostIntegrationBindingObject,
      qt_meta_data_QtBoostIntegrationBindingObject, 0 }
};

const QMetaObject *QtBoostIntegrationBindingObject::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *QtBoostIntegrationBindingObject::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QtBoostIntegrationBindingObject))
        return static_cast<void*>(const_cast< QtBoostIntegrationBindingObject*>(this));
    return QObject::qt_metacast(_clname);
}

int QtBoostIntegrationBindingObject::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    // handle QObject base metacalls
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;

    // handle our one named slots
    if (_id == 0) {
        receiverDestroyed();
        return -1;
    } else if (_id == 1) {
        senderDestroyed(_a);
        return -1;
    }

    // it must be a binding call, offset it by our methodCount()
    // and try to handle it
    _id -= metaObject()->methodCount();
    if (_c == QMetaObject::InvokeMetaMethod &&
            _id < m_bindings.size() && m_bindings[_id].adapter) {
        // if we have a binding for this index, call it
        m_bindings[_id].adapter->invoke(_a);
        _id -= m_bindings.size();
    }
    return _id;
}

bool QtBoostIntegrationBindingObject::bind(QObject *sender, const char *signal,
                         QObject *receiver, QtBoostAbstractConnectionAdapter *adapter,
#ifdef QTBOOSTINTEGRATION_CHECK_SIGNATURE
                         int nrArguments, int argumentList[],
#endif
                         Qt::ConnectionType connType)
{
    if (!sender || !signal || !adapter)
        return false;

    int signalIndex = sender->metaObject()->indexOfSignal(&signal[1]);
    if (signalIndex < 0) {
        qWarning("qtlambda::connect(): no match for signal '%s'", signal);
        return false;
    }

    Q_ASSERT_X(!receiver || receiver->thread() == thread(),
               "qtlambda::connect", "The receiving QObject must be on the thread connect() is called in");

#ifdef QTBOOSTINTEGRATION_CHECK_SIGNATURE
    QByteArray signalSignature = sender->metaObject()->normalizedSignature(signal);
    QByteArray adapterSignature = buildAdapterSignature(nrArguments, argumentList);

    if (!QMetaObject::checkConnectArgs(signalSignature, adapterSignature))
        return false;
#endif

    // locate a free slot
    int slotIndex;
    bool alreadyKnowAboutThisReceiver = false;
    bool alreadyKnowAboutThisSender   = false;
    for (slotIndex = 0; slotIndex < m_bindings.size(); slotIndex++) {
        if (m_bindings[slotIndex].receiver == receiver)
            alreadyKnowAboutThisReceiver = true;
        if (m_bindings[slotIndex].sender == sender)
            alreadyKnowAboutThisSender = true;
        if (!m_bindings[slotIndex].adapter)
            break;
    }

    // make sure that we will delete the connection if the receiver or sender go away
    if (!alreadyKnowAboutThisReceiver && receiver) {
        QObject::connect(receiver, SIGNAL(destroyed()),
                         this, SLOT(receiverDestroyed()), Qt::UniqueConnection);
    }
    if (!alreadyKnowAboutThisSender) {
        QObject::connect(sender, SIGNAL(destroyed(QObject*)),
                         this, SLOT(senderDestroyed()), Qt::UniqueConnection);
    }

    // wire up the connection from the binding object to the sender
    bool connected = QMetaObject::connect(sender, signalIndex, this, slotIndex + bindingOffset(), connType);
    if (!connected)
        return false;

    // store the binding
    if (slotIndex == m_bindings.size()) {
        m_bindings.append(Binding(sender, signalIndex, receiver, adapter));
    } else {
        m_bindings[slotIndex] = Binding(sender, signalIndex, receiver, adapter);
    }

    static_cast<ConnectNotifyObject *>(sender)->callConnectNotify(signal);
    return true;
}

bool QtBoostIntegrationBindingObject::unbind(QObject *sender, const char *signal, QObject *receiver)
{
    int signalIndex = -1;

    if (signal) {
        Q_ASSERT(sender);
        signalIndex = sender->metaObject()->indexOfSignal(&signal[1]);
        if (signalIndex < 0)
            return false;
    }

    bool found = false;
    for (int i = 0; i < m_bindings.size(); i++) {
        const Binding& b = m_bindings.at(i);
        if ((b.signalIndex == signalIndex || signalIndex == -1)
                && (b.sender == sender || !sender)
                && (b.receiver == receiver || !receiver)) {
            QMetaObject::disconnect(b.sender, b.signalIndex, b.receiver, i + bindingOffset());
            QByteArray sigName = b.sender->metaObject()->method(b.signalIndex).signature();
            sigName.prepend('2');
            static_cast<ConnectNotifyObject *>(b.sender)->callDisconnectNotify(sigName.constData());
            delete b.adapter;
            m_bindings[i] = Binding();
            found = true;
        }
    }
    return found;
}

QByteArray QtBoostIntegrationBindingObject::buildAdapterSignature
    (int nrArguments, int argumentMetaTypeList[])
{
    QByteArray sig("lambda(");
    for (int i = 0; i < nrArguments; i++) {
        sig += QMetaType::typeName(argumentMetaTypeList[i]);
        if (i != nrArguments-1)
            sig += ",";
    }
    sig += ")";
    return sig;
}

void QtBoostIntegrationBindingObject::receiverDestroyed()
{
    // when any object for which we are holding a binding is destroyed,
    // remove all of its bindings
    QObject *receiver = sender();

    if (receiver)
        unbind(0, 0, receiver);
}

void QtBoostIntegrationBindingObject::senderDestroyed(void **_a)
{
    QObject *sender = this->sender();

    // Workaround for an issue when a function connected to the sender's destroyed() is not called
    int deleteLaterIndex = sender->metaObject()->indexOfSignal("destroyed()");
    int deleteLaterIndex2 = sender->metaObject()->indexOfSignal("destroyed(QObject*)");
    Q_ASSERT(deleteLaterIndex != -1 && deleteLaterIndex2 != -1);
    for (int i = 0; i < m_bindings.size(); i++) {
        const Binding &b = m_bindings.at(i);
        if (b.sender == sender &&
            (b.signalIndex == deleteLaterIndex || b.signalIndex == deleteLaterIndex2))
        {
            b.adapter->invoke(_a);
        }
    }

    if (sender)
        unbind(sender, 0, 0);
}
