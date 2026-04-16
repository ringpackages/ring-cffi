/*
 * Example 04: Pointer Operations (C API)
 * API: cffi_nullptr, cffi_isnull, cffi_deref, cffi_offset, cffi_get, cffi_set
 */
load "cffi.ring"

pLib = cffi_load(getLibcPath())

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

# 64-bit Integers (handled as strings to avoid precision loss)
? nl + "--- 64-bit Integers ---"
pI64 = cffi_new("int64")
cVal = "9223372036854775807" # Max int64
cffi_set_i64(pI64, cVal)
? "Set i64: " + cVal
? "Get i64: " + cffi_get_i64(pI64)

func getLibcPath
    if isWindows()
        return "msvcrt.dll"
    but isFreeBSD()
        return "libc.so.7"
    but isMacOSX()
        return "libSystem.B.dylib"
    else
        return "libc.so.6"
    ok