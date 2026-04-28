#ifndef APP_EXPORT_H
#define APP_EXPORT_H

#if (defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) ||                  \
     defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
#define DECL_EXPORT __declspec(dllexport)
#define DECL_IMPORT __declspec(dllimport)
#else
#define DECL_EXPORT __attribute__((visibility("default")))
#define DECL_IMPORT __attribute__((visibility("default")))
#endif

#if (defined(GLOG_LIB) || defined(GLOGKIT_LIB))
#define GLOGKIT_API DECL_EXPORT
#else
#define GLOGKIT_API DECL_IMPORT
#endif

#endif // APP_EXPORT_H