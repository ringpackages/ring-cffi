/*
 * Example 20: OOP - Pointer Operations
 * API: FFI (offset, deref, ptrGet, ptrSet)
 */
load "cffi.ring"

oFFI = new FFI("libc.so.6")

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
