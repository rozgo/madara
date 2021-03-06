
// -*- C++ -*-
// $Id$
// Definition for Win32 Export directives.
// This file is generated automatically by generate_export_file.pl -d KATS
// ------------------------------
#ifndef KATS_EXPORT_H
#define KATS_EXPORT_H

#include "ace/config-all.h"

#if defined (ACE_AS_STATIC_LIBS) && !defined (KATS_HAS_DLL)
#  define KATS_HAS_DLL 0
#endif /* ACE_AS_STATIC_LIBS && KATS_HAS_DLL */

#if !defined (KATS_HAS_DLL)
#  define KATS_HAS_DLL 1
#endif /* ! KATS_HAS_DLL */

#if defined (KATS_HAS_DLL) && (KATS_HAS_DLL == 1)
#  if defined (KATS_BUILD_DLL)
#    define KATSExport ACE_Proper_Export_Flag
#    define KATS_SINGLETON_DECLARATION(T) ACE_EXPORT_SINGLETON_DECLARATION (T)
#    define KATS_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_EXPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  else /* KATS_BUILD_DLL */
#    define KATSExport ACE_Proper_Import_Flag
#    define KATS_SINGLETON_DECLARATION(T) ACE_IMPORT_SINGLETON_DECLARATION (T)
#    define KATS_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK) ACE_IMPORT_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#  endif /* KATS_BUILD_DLL */
#else /* KATS_HAS_DLL == 1 */
#  define KATSExport
#  define KATS_SINGLETON_DECLARATION(T)
#  define KATS_SINGLETON_DECLARE(SINGLETON_TYPE, CLASS, LOCK)
#endif /* KATS_HAS_DLL == 1 */

// Set KATS_NTRACE = 0 to turn on library specific tracing even if
// tracing is turned off for ACE.
#if !defined (KATS_NTRACE)
#  if (ACE_NTRACE == 1)
#    define KATS_NTRACE 1
#  else /* (ACE_NTRACE == 1) */
#    define KATS_NTRACE 0
#  endif /* (ACE_NTRACE == 1) */
#endif /* !KATS_NTRACE */

#if (KATS_NTRACE == 1)
#  define KATS_TRACE(X)
#else /* (KATS_NTRACE == 1) */
#  if !defined (ACE_HAS_TRACE)
#    define ACE_HAS_TRACE
#  endif /* ACE_HAS_TRACE */
#  define KATS_TRACE(X) ACE_TRACE_IMPL(X)
#  include "ace/Trace.h"
#endif /* (KATS_NTRACE == 1) */

#endif /* KATS_EXPORT_H */

// End of auto generated file.
