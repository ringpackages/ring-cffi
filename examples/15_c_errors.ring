/*
 * Example 15: Errors (C API)
 * API: cffi_errno, cffi_strerror
 */
load "cffi.ring"

pLib = cffi_load("libc.so.6")

? "errno: " + cffi_errno()
? "error: " + cffi_strerror()
? "errno 2: " + cffi_strerror(2)
