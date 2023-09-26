/* -----------------------------------------------------------------*-C-*-
   ffitarget.h - Copyright (c) 2023  Neale Ferguson
                 Copyright (c) 2012  Anthony Green
                 Copyright (c) 2010  CodeSourcery
                 Copyright (c) 1996-2003  Red Hat, Inc.

   Target configuration macros for ZOS/ZVM.

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   ``Software''), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED ``AS IS'', WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.

   ----------------------------------------------------------------------- */

#ifndef LIBFFI_TARGET_H
#define LIBFFI_TARGET_H

#ifndef LIBFFI_H
#error "Please do not include ffitarget.h directly into your source.  Use ffi.h instead."
#endif

#ifndef LIBFFI_ASM
typedef unsigned long long     ffi_arg;
typedef signed long long       ffi_sarg;

typedef enum ffi_abi {
  FFI_FIRST_ABI = 0,
  FFI_STANDARD,
  FFI_FASTLINK,
  FFI_XPLINK,
  FFI_XPLINK_64,
  FFI_LAST_ABI,
#if !defined(__XPLINK__)
  FFI_DEFAULT_ABI = FFI_STANDARD,
#elif !defined( _LP64)
  FFI_DEFAULT_ABI = FFI_XPLINK,
#else
  FFI_DEFAULT_ABI = FFI_XPLINK_64,
#endif
} ffi_abi;

#define FFI_EXTRA_CIF_FIELDS    \
  unsigned is_varargs;          \
  unsigned nfixedargs;          \
  void *call_fn;                \
  void *closure_fn

#define FFI_TARGET_SPECIFIC_VARIADIC

/* ---- Definitions for closures ----------------------------------------- */

#define FFI_CLOSURES 1
#define FFI_NATIVE_RAW_API 0
#define FFI_DEFAULT_ABI 0

#define FFI_TRAMPOLINE_SIZE 24

#endif
#endif
