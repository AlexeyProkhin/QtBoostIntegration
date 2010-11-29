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
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      32,   52,   52,   52, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_QtBoostIntegrationBindingObject[] = {
    "QtBoostIntegrationBindingObject\0"
    "receiverDestroyed()\0\0"
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

    // handle our one named slot
    if (_id == 0) {
        receiverDestroyed();
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
                         int nrArguments, int argumentList[],
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

    QByteArray signalSignature = sender->metaObject()->normalizedSignature(signal);
    QByteArray adapterSignature = buildAdapterSignature(nrArguments, argumentList);

    if (!QMetaObject::checkConnectArgs(signalSignature, adapterSignature))
        return false;

    // locate a free slot
    int slotIndex;
    bool alreadyKnowAboutThisReceiver = false;
    for (slotIndex = 0; slotIndex < m_bindings.size(); slotIndex++) {
        if (m_bindings[slotIndex].receiver == receiver)
            alreadyKnowAboutThisReceiver = true;
        if (!m_bindings[slotIndex].adapter)
            break;
    }

    // wire up a connection from the binding object to the sender
    bool connected = QMetaObject::connect(sender, signalIndex, this, slotIndex + bindingOffset(), connType);
    if (!connected)
        return false;

    // store the binding
    if (slotIndex == m_bindings.size()) {
        m_bindings.append(Binding(sender, signalIndex, receiver, adapter));
    } else {
        m_bindings[slotIndex] = Binding(sender, signalIndex, receiver, adapter);
    }

    // and make sure that we will delete it if the receiver goes away
    // ### should we auto-delete if senders disappear, too?
    // ###    it costs memory on connections to possibly save memory later...
    if (!alreadyKnowAboutThisReceiver && receiver) {
        QObject::connect(receiver, SIGNAL(destroyed()), this, SLOT(receiverDestroyed()));
    }

    static_cast<ConnectNotifyObject *>(sender)->callConnectNotify(signal);
    return true;
}

bool QtBoostIntegrationBindingObject::unbind(QObject *sender, const char *signal, QObject *receiver)
{
    int signalIndex = -1;

    if (signal) {
        signalIndex = sender->metaObject()->indexOfSignal(&signal[1]);
        if (signalIndex < 0)
            return false;
    }

    bool found = false;
    for (int i = 0; i < m_bindings.size(); i++) {
        const Binding& b = m_bindings.at(i);
        if ((b.signalIndex == signalIndex || signalIndex == -1)
                && (b.sender == sender || !sender)
                && b.receiver == receiver) {
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