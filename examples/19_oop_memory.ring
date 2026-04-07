/*
 * Example 19: OOP - Memory Management
 * API: FFI (alloc, sizeof, nullptr, isNullPtr, ptrGet, ptrSet)
 */
load "cffi.ring"

oFFI = new FFI("libc.so.6")

# Allocate and use
pInt = oFFI.alloc("int")
oFFI.ptrSet(pInt, "int", 42)
? "int value: " + oFFI.ptrGet(pInt, "int")

# sizeof
? "sizeof(int)    = " + oFFI.sizeof("int")
? "sizeof(double) = " + oFFI.sizeof("double")
? "sizeof(ptr)    = " + oFFI.sizeof("ptr")

# nullptr
pNull = oFFI.nullptr()
? "nullptr check: " + oFFI.isNullPtr(pNull)
