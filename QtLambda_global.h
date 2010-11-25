#ifndef QTLAMBDA_GLOBAL_H
#define QTLAMBDA_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QTLAMBDA_LIBRARY)
#  define QTLAMBDASHARED_EXPORT Q_DECL_EXPORT
#else
#  define QTLAMBDASHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // QTLAMBDA_GLOBAL_H
