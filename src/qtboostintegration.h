/*
 * Copyright 2010  Benjamin K. Stuhl <bks24@cornell.edu>
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
#ifndef BOOST_PP_IS_ITERATING
#   ifndef QTBOOSTINTEGRATION_H
#   define QTBOOSTINTEGRATION_H

#   include <QtCore/QObject>
#   include <QtCore/QMetaType>

// for building the library itself, we don't need all the template
// magic: omit it to reduce build times and dependencies
#   ifndef QTBOOSTINTEGRATION_LIBRARY_BUILD
#      include <boost/function.hpp>
#      include <boost/preprocessor/iteration.hpp>
#      include <boost/preprocessor/repetition.hpp>
#   else
namespace boost { template <typename Signature> class function; };
#   endif

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
                        int nrArguments, int argumentList[],
                        Qt::ConnectionType connType);

private:
    QtBoostConnectionAdapterVtable *m_vtable;
};

template<typename Signature> class QtBoostConnectionAdapter
        : public QtBoostAbstractConnectionAdapter
{
};

template<typename Signature> class QtBoostArgumentMetaTypeLister
{
};

template<typename Signature> inline
bool qtBoostConnect(QObject *sender, const char *signal,
             QObject *receiver, const boost::function<Signature>& fn,
             Qt::ConnectionType connType = Qt::AutoConnection)
{
    int nrArguments, *argumentList;
    nrArguments = QtBoostArgumentMetaTypeLister<Signature>::metaTypes(&argumentList);
    return QtBoostAbstractConnectionAdapter::connect(
                sender,
                signal,
                receiver,
                new QtBoostConnectionAdapter<Signature>(fn),
                nrArguments,
                argumentList,
                connType);
};

bool qtBoostDisconnect(QObject *sender, const char *signal, QObject *receiver);

// set up the iteration limits and run the iteration
#   ifndef QTBOOSTINTEGRATION_LIBRARY_BUILD
#      ifndef QTBOOSTINTEGRATION_MAX_ARGUMENTS
#      define QTBOOSTINTEGRATION_MAX_ARGUMENTS 5
#      endif

#      define BOOST_PP_ITERATION_LIMITS (0, QTBOOSTINTEGRATION_MAX_ARGUMENTS)
#      define BOOST_PP_FILENAME_1 "qtboostintegration.h"
#      include BOOST_PP_ITERATE()
#   endif

#   endif // QTBOOSTINTEGRATION_H
#else // BOOST_PP_IS_ITERATING

#define n BOOST_PP_ITERATION()

#if n == 0
#define QTBI_PARTIAL_SPEC void (void)
#else
#define QTBI_PARTIAL_SPEC void (BOOST_PP_ENUM_PARAMS(n, T))
#endif

template<BOOST_PP_ENUM_PARAMS(n, typename T)>
class QtBoostConnectionAdapter<QTBI_PARTIAL_SPEC>
        : public QtBoostAbstractConnectionAdapter {
public:
    explicit QtBoostConnectionAdapter(const boost::function<QTBI_PARTIAL_SPEC> &f)
        : QtBoostAbstractConnectionAdapter(makeVtable()), m_function(f)
    {
    }

private:
    static void invokeFn(QtBoostAbstractConnectionAdapter *thiz_, void **args) {
        QtBoostConnectionAdapter<QTBI_PARTIAL_SPEC> *thiz =
                static_cast<QtBoostConnectionAdapter<QTBI_PARTIAL_SPEC> *>(thiz_);
#if n == 0
        Q_UNUSED(args);
#endif

#define QTBI_PARAM(z, N, arg) *reinterpret_cast<BOOST_PP_CAT(T, N) *>(args[N+1])
        thiz->m_function(BOOST_PP_ENUM(n, QTBI_PARAM, ~));
#undef QTBI_PARAM
    }

    static void destroyFn(QtBoostAbstractConnectionAdapter *thiz_) {
        QtBoostConnectionAdapter<QTBI_PARTIAL_SPEC> *thiz =
            static_cast<QtBoostConnectionAdapter<QTBI_PARTIAL_SPEC> *>(thiz_);
        thiz->~QtBoostConnectionAdapter();
    }

    static QtBoostConnectionAdapterVtable *makeVtable() {
        static QtBoostConnectionAdapterVtable vt = { invokeFn, destroyFn };
        return &vt;
    }


    boost::function<QTBI_PARTIAL_SPEC> m_function;
};

template<BOOST_PP_ENUM_PARAMS(n, typename T)>
class QtBoostArgumentMetaTypeLister<QTBI_PARTIAL_SPEC> {
public:
    static int metaTypes(int **typeList) {
        static int types[n+1] = {
#define QTBI_STORE_METATYPE(z, n, arg) qMetaTypeId<BOOST_PP_CAT(T, n)>()
        BOOST_PP_ENUM(n, QTBI_STORE_METATYPE, ~)
#undef QTBI_STORE_METATYPE
        };
        *typeList = types;
        return n;
    }
};

#undef n
#undef QTBI_PARTIAL_SPEC

#endif // BOOST_PP_IS_ITERATING

