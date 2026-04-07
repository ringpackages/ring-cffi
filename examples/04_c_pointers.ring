/*
 * Example 04: Pointer Operations (C API)
 * API: cffi_nullptr, cffi_isnull, cffi_deref, cffi_offset, cffi_get, cffi_set
 */
load "cffi.ring"

pLib = cffi_load("libc.so.6")

pNull = cffi_nullptr()
? "nullptr is null: " + cffi_isnull(pNull)

pInt = cffi_new("int")
cffi_set(pInt, "int", 777)
? "value: " + cffi_get(pInt, "int")

pPtr = cffi_new("ptr")
cffi_set(pPtr, "ptr", pInt)
pDeref = cffi_deref(pPtr, "ptr")
? "deref'd (with type): " + cffi_get(pDeref, "int")

# deref without explicit type (defaults to pointer)
pDeref2 = cffi_deref(pPtr)
? "deref'd (no type): " + cffi_get(pDeref2, "int")
