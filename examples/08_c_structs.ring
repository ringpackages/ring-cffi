/*
 * Example 08: Structs (C API)
 * API: cffi_struct, cffi_struct_new, cffi_field, cffi_field_offset, cffi_struct_size
 */
load "cffi.ring"

pLib = cffi_load("libc.so.6")

oPoint = cffi_struct("Point", [
    ["x", "int"],
    ["y", "int"]
])

? "struct size: " + cffi_struct_size(oPoint)
? "x offset: " + cffi_field_offset(oPoint, "x")
? "y offset: " + cffi_field_offset(oPoint, "y")

pPt = cffi_struct_new(oPoint)
cffi_set(cffi_field(pPt, oPoint, "x"), "int", 100)
cffi_set(cffi_field(pPt, oPoint, "y"), "int", 200)
? "Point(" + cffi_get(cffi_field(pPt, oPoint, "x"), "int") + ", " + cffi_get(cffi_field(pPt, oPoint, "y"), "int") + ")"
