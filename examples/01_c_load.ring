/*
 * Example 01: Load Library (C API)
 * API: cffi_load, cffi_isnull
 */
load "cffi.ring"

pLib = cffi_load("libc.so.6")
? "Library loaded: " + (not cffi_isnull(pLib))
