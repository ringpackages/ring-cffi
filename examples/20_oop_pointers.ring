/*
 * Example 20: OOP - Pointer Operations
 * API: FFI (offset, deref, ptrGet, ptrSet)
 */
load "cffi.ring"

oFFI = new FFI(getLibcPath())

# Array via offset
pArr = oFFI.allocArray("int", 3)
for i = 0 to 2
    pElem = oFFI.offset(pArr, i * oFFI.sizeof("int"))
    oFFI.ptrSet(pElem, "int", (i + 1) * 100)
next
for i = 0 to 2
    pElem = oFFI.offset(pArr, i * oFFI.sizeof("int"))
    ? "arr[" + i + "] = " + oFFI.ptrGet(pElem, "int")
next

# Deref — typed (returns value)
pPtr = oFFI.alloc("ptr")
oFFI.ptrSet(pPtr, "ptr", pArr)
pDeref = oFFI.derefTyped(pPtr, "ptr")
? "deref'd first element: " + oFFI.ptrGet(pDeref, "int")

# Deref — raw (returns pointer)
pDeref2 = oFFI.deref(pPtr)
? "raw deref: " + (not isNull(pDeref2))

# 64-bit Integers (handled as strings to avoid precision loss)
? nl + "--- 64-bit Integers ---"
pI64 = oFFI.alloc("int64")
cVal = "1234567890123456789"
oFFI.i64Set(pI64, cVal, NULL)
? "Set i64: " + cVal
? "Get i64: " + oFFI.i64Get(pI64, NULL)

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