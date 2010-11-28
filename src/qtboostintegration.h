#ifndef BOOST_PP_IS_ITERATING
#   ifndef QTLAMBDA_H
#   define QTLAMBDA_H

#   include <QtCore/QObject>
#   include <QtCore/QMetaType>
#   include <boost/function.hpp>
#   include <boost/preprocessor/iteration.hpp>
#   include <boost/preprocessor/repetition.hpp>

// define the adapter glue we'll need
namespace QtBoostIntegrationInternal {

struct connection_adapter_base {
    explicit connection_adapter_base() { }
    virtual ~connection_adapter_base() { }
    virtual void call(void **args) = 0;
    virtual int types(int maxTypes, int *typeList) = 0;
};

template<typename Signature> struct connection_adapter
        : public connection_adapter_base
{
};

bool doConnect(QObject *sender, const char *signal,
               QObject *receiver, connection_adapter_base *adapter,
               Qt::ConnectionType connType);
}; // namespace QtBoostIntegrationInternal

template<typename Signature> inline
bool qtBoostConnect(QObject *sender, const char *signal,
             QObject *receiver, const boost::function<Signature>& fn,
             Qt::ConnectionType connType = Qt::AutoConnection)
{
    return QtBoostIntegrationInternal::doConnect(
                sender,
                signal,
                receiver,
                new QtBoostIntegrationInternal::connection_adapter<Signature>(fn),
                connType);
};

bool qtBoostDisconnect(QObject *sender, const char *signal, QObject *receiver);

// set up the iteration limits and run the iteration
#   ifndef QTLAMBDA_MAX_ARGUMENTS
#   define QTLAMBDA_MAX_ARGUMENTS 5
#   endif

#   define BOOST_PP_ITERATION_LIMITS (0, QTLAMBDA_MAX_ARGUMENTS)
#   define BOOST_PP_FILENAME_1 "qtboostintegration.h"
#   include BOOST_PP_ITERATE()

#   endif // QTLAMBDA_H
#else // BOOST_PP_IS_ITERATING

#define n BOOST_PP_ITERATION()

#if n == 0
#define CONNECT_PARTIAL_SPEC void (void)
#else
#define CONNECT_PARTIAL_SPEC void (BOOST_PP_ENUM_PARAMS(n, T))
#endif

namespace QtBoostIntegrationInternal {
    template<BOOST_PP_ENUM_PARAMS(n, typename T)>
    struct connection_adapter<CONNECT_PARTIAL_SPEC> : public connection_adapter_base {
        explicit connection_adapter(const boost::function<CONNECT_PARTIAL_SPEC> &f)
            : connection_adapter_base(), mFn(f)
        {
        }

        virtual void call(void **args) {
#define PARAM(z, N, arg) *reinterpret_cast<BOOST_PP_CAT(T, N) *>(args[N+1])
            mFn(BOOST_PP_ENUM(n, PARAM, ~));
#undef PARAM
        }

        virtual int types(int maxTypes, int *typeList) {
            if (maxTypes < n)
                return -1;
#define STORE_METATYPE(z, n, arg) (typeList[n] = qMetaTypeId<BOOST_PP_CAT(T, n)>())
            BOOST_PP_ENUM(n, STORE_METATYPE, ~);
#undef STORE_METATYPE
            return n;
        }

        boost::function<CONNECT_PARTIAL_SPEC> mFn;
    };

}; // namespace QtBoostIntegrationInternal

#undef n
#undef CONNECT_PARTIAL_SPEC

#endif // BOOST_PP_IS_ITERATING

