
#ifndef WISPRENDERER_EXPORT_H
#define WISPRENDERER_EXPORT_H

#ifdef WISPRENDERER_STATIC_DEFINE
#  define WISPRENDERER_EXPORT
#  define WISPRENDERER_NO_EXPORT
#else
#  ifndef WISPRENDERER_EXPORT
#    ifdef WispRenderer_EXPORTS
        /* We are building this library */
#      define WISPRENDERER_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define WISPRENDERER_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef WISPRENDERER_NO_EXPORT
#    define WISPRENDERER_NO_EXPORT 
#  endif
#endif

#ifndef WISPRENDERER_DEPRECATED
#  define WISPRENDERER_DEPRECATED __declspec(deprecated)
#endif

#ifndef WISPRENDERER_DEPRECATED_EXPORT
#  define WISPRENDERER_DEPRECATED_EXPORT WISPRENDERER_EXPORT WISPRENDERER_DEPRECATED
#endif

#ifndef WISPRENDERER_DEPRECATED_NO_EXPORT
#  define WISPRENDERER_DEPRECATED_NO_EXPORT WISPRENDERER_NO_EXPORT WISPRENDERER_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef WISPRENDERER_NO_DEPRECATED
#    define WISPRENDERER_NO_DEPRECATED
#  endif
#endif

#endif /* WISPRENDERER_EXPORT_H */
