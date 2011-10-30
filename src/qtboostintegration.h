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

#ifndef QTBOOSTINTEGRATION_H
#define QTBOOSTINTEGRATION_H

#include <QtCore/QObject>
#include <QtCore/QMetaType>
#include <functional>
#include <type_traits>

// define the adapter glue we'll need
class QtBoostAbstractConnectionAdapter;
struct QtBoostConnectionAdapterVtable {
    void (*invoke)(QtBoostAbstractConnectionAdapter *thiz, void **args);
    void (*destroy)(QtBoostAbstractConnectionAdapter *thiz);
};

class QtBoostAbstractConnectionAdapter {
public:
    QtBoostAbstractConnectionAdapter(QtBoostConnectionAdapterVtable *vt);
    ~QtBoostAbstractConnectionAdapter();
    void invoke(void **args);

    static bool connect(QObject *sender, const char *signal,
                        QObject *receiver, QtBoostAbstractConnectionAdapter *adapter,
#ifdef QTBOOSTINTEGRATION_CHECK_SIGNATURE
                        int nrArguments, int argumentList[],
#endif
                        Qt::ConnectionType connType);

private:
    QtBoostConnectionAdapterVtable *m_vtable;
};

template <int N, typename... T>
struct QtBoostCallHelper;

template <int N, typename First, typename... Others>
struct QtBoostCallHelper<N, First, Others...>
{
public:
    template <typename FN, typename... Args>
    static inline void call(const FN &fn, void **data, Args... args)
    {
        typedef QtBoostCallHelper<N-1, Others...> T;
        T::call(fn, data + 1, args..., *reinterpret_cast<typename std::remove_reference<First>::type *>(*data));
    }
};

template <>
struct QtBoostCallHelper<0>
{
public:
    template <typename FN, typename... Args>
    static inline void call(const FN &fn, void **data, Args... args)
    {
        fn(args...);
    }
};

template<typename Signature>
class QtBoostConnectionAdapter;

template<typename Res, typename... T>
class QtBoostConnectionAdapter<Res(T...)>
        : public QtBoostAbstractConnectionAdapter {
public:
    explicit QtBoostConnectionAdapter(const std::function<Res(T...)> &f)
        : QtBoostAbstractConnectionAdapter(makeVtable()), m_function(f)
    {
    }

private:
    static void invokeFn(QtBoostAbstractConnectionAdapter *thiz_, void **args) {
        QtBoostConnectionAdapter<Res(T...)> *thiz =
                static_cast<QtBoostConnectionAdapter<Res(T...)> *>(thiz_);
        QtBoostCallHelper<sizeof...(T), T...>::call(thiz->m_function, args + 1);
    }

    static void destroyFn(QtBoostAbstractConnectionAdapter *thiz_) {
        QtBoostConnectionAdapter<Res(T...)> *thiz =
            static_cast<QtBoostConnectionAdapter<Res(T...)> *>(thiz_);
        thiz->~QtBoostConnectionAdapter();
    }

    static QtBoostConnectionAdapterVtable *makeVtable() {
        static QtBoostConnectionAdapterVtable vt = { invokeFn, destroyFn };
        return &vt;
    }


    std::function<Res(T...)> m_function;
};

#if QTBOOSTINTEGRATION_CHECK_SIGNATURE
template<typename Signature>
class QtBoostArgumentMetaTypeLister;

template<typename Res, typename... T>
class QtBoostArgumentMetaTypeLister<Res(T...)>{
public:
    static int metaTypes(int **typeList) {
        static int types[sizeof...(T)] = {
            qMetaTypeId<typename std::remove_cv<typename std::remove_reference<T>::type>::type>()...
        };
        *typeList = types;
        return sizeof...(T);
    }
};
#endif

template<typename Signature> inline
bool lconnect(QObject *sender, const char *signal,
              QObject *receiver, const std::function<Signature>& fn,
              Qt::ConnectionType connType = Qt::AutoConnection)
{
#ifdef QTBOOSTINTEGRATION_CHECK_SIGNATURE
    int nrArguments, *argumentList;
    nrArguments = QtBoostArgumentMetaTypeLister<Signature>::metaTypes(&argumentList);
#endif
    return QtBoostAbstractConnectionAdapter::connect(
                sender,
                signal,
                receiver,
                new QtBoostConnectionAdapter<Signature>(fn),
#ifdef QTBOOSTINTEGRATION_CHECK_SIGNATURE
                nrArguments,
                argumentList,
#endif
                connType);
};

template<typename Signature>
struct LambdaSignatureHelper
{
    typedef typename LambdaSignatureHelper<decltype(&Signature::operator())>::type type;
};

template<typename F, typename R, typename... T>
struct LambdaSignatureHelper<R (F::*)(T...)const>
{
    typedef R type(T...);
};

template<typename F, typename R, typename... T>
struct LambdaSignatureHelper<R (F::*)(T...)>
{
    typedef R type(T...);
};

template<typename R, typename... T>
struct LambdaSignatureHelper<R (*)(T...)>
{
    typedef R type(T...);
};

template<typename R, typename... T>
struct LambdaSignatureHelper<R(T...)>
{
    typedef R type(T...);
};

template<typename FN> inline
bool lconnect(QObject *sender, const char *signal,
              QObject *receiver, FN fn,
              Qt::ConnectionType connType = Qt::AutoConnection)
{
    return lconnect<typename LambdaSignatureHelper<FN>::type>(
                sender,
                signal,
                receiver,
                fn,
                connType);
};


template<typename FN> inline
bool lconnect(QObject *sender, const char *signal,
              QObject *receiver,
              Qt::ConnectionType connType,
              FN fn)
{
    return lconnect<typename LambdaSignatureHelper<FN>::type>(
                sender,
                signal,
                receiver,
                fn,
                connType);
};

bool ldisconnect(QObject *sender, const char *signal, QObject *receiver);

#endif // QTBOOSTINTEGRATION_H
