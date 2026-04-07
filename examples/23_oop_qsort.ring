/*
 * Example 23: OOP - Complete Example: qsort from libc
 * API: FFI (alloc, sizeof, offset, ptrGet, ptrSet, cFunc, callback, invoke)
 */
load "cffi.ring"

oFFI = new FFI("libc.so.6")

# Allocate array
pArr = oFFI.allocArray("int", 10)
aValues = [50, 20, 80, 10, 90, 30, 70, 40, 60, 100]
for i = 0 to 9
    pElem = oFFI.offset(pArr, i * oFFI.sizeof("int"))
    oFFI.ptrSet(pElem, "int", aValues[i + 1])
next

? "Before sort:"
for i = 0 to 9
    pElem = oFFI.offset(pArr, i * oFFI.sizeof("int"))
    see " " + oFFI.ptrGet(pElem, "int")
next
? ""

# Create comparison callback
oCb = oFFI.callback("compare_ints", "int", ["ptr", "ptr"])

# Call qsort: qsort(base, nmemb, size, compar)
oQsort = oFFI.cFunc("qsort", "void", ["ptr", "int", "int", "ptr"])
oFFI.invoke(oQsort, [pArr, 10, oFFI.sizeof("int"), oCb])

? "After sort:"
for i = 0 to 9
    pElem = oFFI.offset(pArr, i * oFFI.sizeof("int"))
    see " " + oFFI.ptrGet(pElem, "int")
next
? ""

func compare_ints pA, pB
    nA = cffi_get(pA, "int")
    nB = cffi_get(pB, "int")
    return nA - nB
