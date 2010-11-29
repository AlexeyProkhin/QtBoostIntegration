#ifndef BINDINGOBJECT_P_H
#define BINDINGOBJECT_P_H

#include <QtCore/QObject>
#include <QtCore/QVector>

class QtBoostAbstractConnectionAdapter;

class QtBoostIntegrationBindingObject : public QObject
{
    // no Q_OBJECT, since we don't want moc to run
public:
    explicit QtBoostIntegrationBindingObject(QObject *parent = 0);
    virtual ~QtBoostIntegrationBindingObject();

    bool bind(QObject *sender,
              const char *signal,
              QObject *receiver,
              QtBoostAbstractConnectionAdapter *adapter,
              int nrArguments,
              int argumentTypeList[],
              Qt::ConnectionType connType);

    bool unbind(QObject *sender, const char *signal, QObject *receiver);

    int bindingOffset() const
    { return metaObject()->methodOffset() + metaObject()->methodCount(); }

// we _do_ have the QMetaObject data for this, we just don't need moc
public slots:
    void receiverDestroyed();

    // core QObject stuff: we implement this ourselves rather than
    // via moc, since qt_metacall() is the core of the binding
    static const QMetaObject staticMetaObject;
    virtual const QMetaObject *metaObject() const;
    virtual void *qt_metacast(const char *);
    virtual int qt_metacall(QMetaObject::Call, int, void **argv);

private:
    struct Binding {
        QObject *sender;
        QObject *receiver;
        QtBoostAbstractConnectionAdapter* adapter;
        int signalIndex;

        Binding() : sender(0), receiver(0), adapter(0), signalIndex(-1) { }
        Binding(QObject *s, int sIx, QObject *r, QtBoostAbstractConnectionAdapter *a)
            : sender(s), receiver(r), adapter(a), signalIndex(sIx) { }
    };
    static QByteArray buildAdapterSignature(int nrArguments, int argumentMetaTypeList[]);

    QVector<Binding> m_bindings;
};

#endif // BINDINGOBJECT_P_H
