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
    for (auto d = m_objectDataList; d; d = d->next) {
        auto &storageData = d->storage->data;
        Q_ASSERT(storageData.contains(this));
        storageData.remove(this);
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
    BindingList::add(sender_d->receivers, slotIndex);
    if (receiver_d) {
        BindingList::add(receiver_d->senders, slotIndex);

        // make the sender and the receiver aware of each other
        receiver_d->senders->other = sender_d->receivers;
        sender_d->receivers->other = receiver_d->senders;
    }

    static_cast<ConnectNotifyObject *>(sender)->callConnectNotify(signal);
    return true;
}

template <typename T>
inline void QtBoostIntegrationBindingObject::unbindHelper(BindingList *bindings, const T &checkFn)
{
    for (auto binding = bindings; binding; ) {
        auto next = binding->next;
        auto &b = m_bindings[binding->id];
        int id = binding->id;

        if (checkFn(b, id)) {
            Q_ASSERT(b.signalIndex >= 0);
            delete b.adapter;
            m_bindings[id] = Binding();
            m_freeList.push_back(id);

            if (binding->other)
                binding->other->remove();
            binding->remove();
        }

        binding = next;
    }
}

void QtBoostIntegrationBindingObject::callDisconnectNotify(const Binding &b)
{
   QByteArray sigName = b.sender->metaObject()->method(b.signalIndex).signature();
   sigName.prepend('2');
   static_cast<ConnectNotifyObject *>(b.sender)->callDisconnectNotify(sigName.constData());
}

bool QtBoostIntegrationBindingObject::unbind(QObject *sender, const char *signal, QObject *receiver)
{
    bool found = false;
    if (sender) {
        int signalIndex = -1;
        if (signal) {
            signalIndex = sender->metaObject()->indexOfSignal(&signal[1]);
            if (signalIndex < 0)
                return false;
        }

        auto sender_d = getObjectData(sender);
        unbindHelper(sender_d->receivers, [=, &found](const Binding &b, int i) {
            if ((b.signalIndex == signalIndex || signalIndex == -1) &&
                (b.receiver == receiver || !receiver))
            {
                found = true;
                QMetaObject::disconnectOne(b.sender, b.signalIndex, this, i + bindingOffset());
                callDisconnectNotify(b);
                return true;
            } else {
                return false;
            }
        });
    } else {
        Q_ASSERT(receiver);
        auto receiver_d = getObjectData(receiver);
        if (receiver_d->senders)
            found = true;
        unbindHelper(receiver_d->senders, [=](const Binding &b, int i) {
            QMetaObject::disconnectOne(b.sender, b.signalIndex, this, i + bindingOffset());
            callDisconnectNotify(b);
            return true;
        });
    }

    return found;
}

void QtBoostIntegrationBindingObject::objectDestroyed(ObjectData *d)
{
    // when any object for which we are holding a binding is destroyed,
    // remove all of its bindings
    unbindHelper(d->senders, [=](const Binding &b, int i) {
       QMetaObject::disconnectOne(b.sender, b.signalIndex, this, i + bindingOffset());
       callDisconnectNotify(b);
       return true;
    });
    unbindHelper(d->receivers, [](const Binding &, int) {
       return true;
    });

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
    auto itr = data.constBegin();
    for (auto end = data.constEnd(); itr != end; ++itr)
        itr.key()->objectDestroyed(*itr);
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

    auto data = storage->data.value(this);
    if (!data) {
        Q_ASSERT(create);
        data = new ObjectData;
        storage->data.insert(this, data);
        data->storage = storage;

        data->next = m_objectDataList;
        m_objectDataList = data;
        data->prev = &m_objectDataList;
        if (data->next)
            data->next->prev = &data->next;
    }

    return data;
}
