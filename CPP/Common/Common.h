// Common.h

#ifndef __COMMON_COMMON_H
#define __COMMON_COMMON_H

/*
This file is included to all cpp files in 7-Zip.
Each folder contains StdAfx.h file that includes "Common.h".
So 7-Zip includes "Common.h" in both modes:
  with precompiled StdAfx.h
and
  without precompiled StdAfx.h

If you use 7-Zip code, you must include "Common.h" before other h files of 7-zip.
If you don't need some things that are used in 7-Zip,
you can change this h file or h files included in this file.
*/

// compiler pragmas to disable some warnings
#include "../../C/Compiler.h"

// it's <windows.h> or code that defines windows things, if it's not _WIN32
#include "MyWindows.h"

// NewHandler.h and NewHandler.cpp redefine operator new() to throw exceptions, if compiled with old MSVC compilers
#include "NewHandler.h"



#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))


/* There is BUG in MSVC 6.0 compiler for operator new[]:
   It doesn't check overflow, when it calculates size in bytes for allocated array.
   So we can use MY_ARRAY_NEW macro instead of new[] operator. */

#if defined(_MSC_VER) && (_MSC_VER == 1200) && !defined(_WIN64)
  #define MY_ARRAY_NEW(p, T, size) p = new T[(size > (unsigned)0xFFFFFFFF / sizeof(T)) ? (unsigned)0xFFFFFFFF / sizeof(T) : size];
#else
  #define MY_ARRAY_NEW(p, T, size) p = new T[size];
#endif

#if defined(__clang__) || defined(__GNUC__)
#define Z7_CAST_FUNC(t, e) reinterpret_cast<t>(reinterpret_cast<Z7_void_Function>(e))
#else
#define Z7_CAST_FUNC(t, e) reinterpret_cast<t>(reinterpret_cast<void*>(e))
// #define Z7_CAST_FUNC(t, e) reinterpret_cast<t>(e)
#endif

#define Z7_GET_PROC_ADDRESS(func_type, hmodule, func_name)  \
    Z7_CAST_FUNC(func_type, GetProcAddress(hmodule, func_name))

#if defined (__cplusplus) && __cplusplus >= 201103L \
    || (defined(_MSC_VER) && _MSC_VER >= 1800)
  #define Z7_CPP_IS_SUPPORTED_default
  #define Z7_eq_delete  = delete
  // #define Z7_DECL_DEFAULT_COPY_CONSTRUCTOR_IF_SUPPORTED(c) c(const c& k) = default;
#else
  #define Z7_eq_delete
  // #define Z7_DECL_DEFAULT_COPY_CONSTRUCTOR_IF_SUPPORTED(c)
#endif

#define Z7_CLASS_NO_COPY(cls) \
  private: \
  cls(const cls &) Z7_eq_delete; \
  cls &operator=(const cls &) Z7_eq_delete;

#if defined(_MSC_VER) && (_MSC_VER == 1200) && !defined(_WIN64)
  #define Z7_ARRAY_NEW(p, T, size)  p = new T[((size) > 0xFFFFFFFFu / sizeof(T)) ? 0xFFFFFFFFu / sizeof(T) : (size)];
#else
  #define Z7_ARRAY_NEW(p, T, size)  p = new T[size];
#endif

#endif
