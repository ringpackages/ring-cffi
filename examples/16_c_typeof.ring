/*
 * Example 16: typeof (C API)
 * API: cffi_typeof
 */
load "cffi.ring"

pLib = cffi_load("libc.so.6")

oPoint = cffi_struct("Point", [
    ["x", "int"],
    ["y", "int"]
])

oType = cffi_typeof("Point")
? "Point type: " + (not cffi_isnull(oType))
? "Point size: " + cffi_struct_size(oType)
