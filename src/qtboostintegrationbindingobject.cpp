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
#include <QtCore/QTimerEvent>
#include <QtCore/QThread>
#include <QtCore/QMutexLocker>

class ConnectNotifyObject : public QObject {
public:
    void callConnectNotify(const char *signal)
    { connectNotify(signal); }
    void callDisconnectNotify(const char *signal)
    { disconnectNotify(signal); }
};

static QMutex mutex;

QtBoostIntegrationBindingObject::QtBoostIntegrationBindingObject(QObject *parent) :
    QObject(parent), m_objectDataList(0)
{
}

QtBoostIntegrationBindingObject::~QtBoostIntegrationBindingObject()
{
    foreach (auto &b, m_bindings)
        delete b.adapter;

    QMutexLocker locker(&mutex);
    auto thread = QThread::currentThreadId();
    for (auto d = m_objectDataList; d; d = d->next) {
        auto &storageData = d->storage->data;
        Q_ASSERT(storageData.contains(thread));
        storageData.remove(thread);
        delete d;
    }
}

static const uint qt_meta_data_QtBoostIntegrationBindingObject[] = {

 // content:
       5,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

static const char qt_meta_stringdata_QtBoostIntegrationBindingObject[] = {
    "QtBoostIntegrationBindingObject\0\0"
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
    int slotIndex = m_freeList.isEmpty() ? m_bindings.size() : m_freeList.takeLast();

    // wire up the connection from the binding object to the sender
    bool connected = QMetaObject::connect(sender, signalIndex, this, slotIndex + bindingOffset(), connType);
    if (!connected)
        return false;

    // store the binding
    if (slotIndex == m_bindings.size())
        m_bindings.append(Binding(sender, signalIndex, receiver, adapter));
    else
        m_bindings[slotIndex] = Binding(sender, signalIndex, receiver, adapter);

    auto sender_d = getObjectData(sender, true);
    auto receiver_d = (receiver && receiver != sender) ? getObjectData(receiver, true) : 0;
    Q_ASSERT(sender_d);

    Q_ASSERT(!sender_d->receivers.contains(slotIndex));
    sender_d->receivers.insert(slotIndex);
    if (receiver_d) {
        Q_ASSERT(!receiver_d->senders.contains(slotIndex));
        receiver_d->senders.insert(slotIndex);
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

    auto sender_d = sender ? getObjectData(sender) : 0;
    ObjectData *receiver_d = 0;
    if (receiver)
        receiver_d = (sender == receiver) ? sender_d : getObjectData(receiver);

    bool found = false;
    for (int i = 0; i < m_bindings.size(); i++) {
        auto &b = m_bindings[i];
        if (b.signalIndex >= 0
                && (b.signalIndex == signalIndex || signalIndex == -1)
                && (b.sender == sender || !sender)
                && (b.receiver == receiver || !receiver))
        {
            QMetaObject::disconnectOne(b.sender, b.signalIndex, this, i + bindingOffset());
            unbindHelper(i,
                         sender_d   ? sender_d   : getObjectData(b.sender),
                         receiver_d ? receiver_d : getObjectData(b.receiver));
            found = true;
        }
    }
    return found;
}

inline void QtBoostIntegrationBindingObject::unbindHelper(int i, ObjectData *sender_d, ObjectData *receiver_d)
{
    auto &b = m_bindings[i];
    Q_ASSERT(b.signalIndex >= 0);
    QByteArray sigName = b.sender->metaObject()->method(b.signalIndex).signature();
    sigName.prepend('2');
    static_cast<ConnectNotifyObject *>(b.sender)->callDisconnectNotify(sigName.constData());
    if (sender_d)
        sender_d->receivers.remove(i);
    if (receiver_d)
        receiver_d->senders.remove(i);
    delete b.adapter;
    m_bindings[i] = Binding();
    m_freeList.push_back(i);
}

void QtBoostIntegrationBindingObject::objectDestroyed(ObjectData *d)
{
    // when any object for which we are holding a binding is destroyed,
    // remove all of its bindings

    foreach (int i, d->senders)
        unbindHelper(i, getObjectData(m_bindings[i].sender), 0);

    foreach (int i, d->receivers)
        unbindHelper(i, 0, getObjectData(m_bindings[i].receiver));

    delete d;
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


QtBoostIntegrationBindingObject::ObjectData::~ObjectData()
{
    if (next)
        next->prev = prev;
    *prev = next;
}

QtBoostIntegrationBindingObject::ObjectDataStorage::~ObjectDataStorage()
{
    foreach (auto objectData, data)
        objectData->bindingObj->objectDestroyed(objectData);
}

auto QtBoostIntegrationBindingObject::getObjectData(QObject *obj, bool create) -> ObjectData*
{
    static uint dataIndex = QObject::registerUserData();

    QMutexLocker locker(&mutex);
    auto storage = reinterpret_cast<ObjectDataStorage*>(obj->userData(dataIndex));
    if (!storage) {
        Q_ASSERT(create);
        storage = new ObjectDataStorage;
        obj->setUserData(dataIndex, storage);
    }

    auto thread = QThread::currentThreadId();
    auto data = storage->data.value(thread);
    if (!data) {
        Q_ASSERT(create);
        data = new ObjectData;
        storage->data.insert(thread, data);
        data->bindingObj = this;
        data->storage = storage;

        data->next = m_objectDataList;
        m_objectDataList = data;
        data->prev = &m_objectDataList;
        if (data->next)
            data->next->prev = &data->next;
    }

    return data;
}
