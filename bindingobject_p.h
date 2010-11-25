#ifndef BINDINGOBJECT_P_H
#define BINDINGOBJECT_P_H

#include <QtCore/QObject>
#include <QtCore/QVector>

namespace qtlambda {
namespace detail {

class connection_adapter_base;

class BindingObject : public QObject
{
    // no Q_OBJECT, since we don't want moc to run
    // Q_OBJECT
public:
    explicit BindingObject(QObject *parent = 0);
    virtual ~BindingObject();

    bool bind(QObject *sender,
              const char *signal,
              QObject *receiver,
              connection_adapter_base *adapter,
              Qt::ConnectionType connType);

    bool unbind(QObject *sender, const char *signal, QObject *receiver);

    int bindingOffset() const
    { return metaObject()->methodOffset() + metaObject()->methodCount(); }

// we _do_ have the QMetaObject data for this, we just don't need moc
//public slots:
    void receiverDestroyed();

    // core QObject stuff: we implement this ourselves rather than
    // via moc
    static const QMetaObject staticMetaObject;
    virtual const QMetaObject *metaObject() const;
    virtual void *qt_metacast(const char *);
    virtual int qt_metacall(QMetaObject::Call, int, void **argv);

private:
    struct Binding {
        QObject *sender;
        QObject *receiver;
        connection_adapter_base* adapter;
        int signalIndex;

        Binding() : sender(0), receiver(0), adapter(0), signalIndex(-1) { }
        Binding(QObject *s, int sIx, QObject *r, connection_adapter_base *a)
            : sender(s), receiver(r), adapter(a), signalIndex(sIx) { }
    };
    static QByteArray buildAdapterSignature(connection_adapter_base *adapter);

    QVector<Binding> m_bindings;
};

};
};

#endif // BINDINGOBJECT_P_H
