#ifndef APP_EXPORT_H
#define APP_EXPORT_H

#include <QtCore/qglobal.h>

#if defined(GLOG_LIB)
#define APP_EXPORT Q_DECL_EXPORT
#elif defined(GLOG_STATIC_LIB)
#define APP_EXPORT
#else
#define APP_EXPORT Q_DECL_IMPORT
#endif

#endif // APP_EXPORT_H