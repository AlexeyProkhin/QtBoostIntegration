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

#ifndef BINDINGOBJECT_P_H
#define BINDINGOBJECT_P_H

#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCore/QBasicTimer>
#include <QtCore/QSet>
#include <QtCore/QMap>

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
#ifdef QTBOOSTINTEGRATION_CHECK_SIGNATURE
              int nrArguments,
              int argumentTypeList[],
#endif
              Qt::ConnectionType connType);

    bool unbind(QObject *sender, const char *signal, QObject *receiver);

    int bindingOffset() const
    { return metaObject()->methodOffset() + metaObject()->methodCount(); }

protected:
    // core QObject stuff: we implement this ourselves rather than
    // via moc, since qt_metacall() is the core of the binding
    static const QMetaObject staticMetaObject;
    virtual const QMetaObject *metaObject() const;
    virtual void *qt_metacast(const char *);
    virtual int qt_metacall(QMetaObject::Call, int, void **argv);

private:
    friend class ObjectDataStorage;
    struct Binding
    {
        QObject *sender;
        QObject *receiver;
        QtBoostAbstractConnectionAdapter* adapter;
        int signalIndex;

        Binding() : sender(0), receiver(0), adapter(0), signalIndex(-1) { }
        Binding(QObject *s, int sIx, QObject *r, QtBoostAbstractConnectionAdapter *a)
            : sender(s), receiver(r), adapter(a), signalIndex(sIx) { }
    };
    struct ObjectDataStorage;
    struct ObjectData
    {
        ~ObjectData();
        QSet<int> senders;
        QSet<int> receivers;
        QtBoostIntegrationBindingObject *bindingObj;
        ObjectDataStorage *storage;
        ObjectData **prev;
        ObjectData *next;
    };
    struct ObjectDataStorage : public QObjectUserData
    {
        ~ObjectDataStorage();
        QMap<Qt::HANDLE, QtBoostIntegrationBindingObject::ObjectData*> data;
    };

    void unbindHelper(int index, ObjectData *sender_d, ObjectData *receiver_d);
    void objectDestroyed(ObjectData *obj);
    static QByteArray buildAdapterSignature(int nrArguments, int argumentMetaTypeList[]);
    ObjectData *getObjectData(QObject *obj, bool create = false);

    QVector<Binding> m_bindings;
    QList<int> m_freeList;
    ObjectData *m_objectDataList;
};

#endif // BINDINGOBJECT_P_H
