/*
 * Example 03: Memory Allocation (C API)
 * API: cffi_new, cffi_get, cffi_set
 */
load "cffi.ring"

pLib = cffi_load(getLibcPath())

pInt = cffi_new("int")
cffi_set(pInt, "int", 999)
? "int: " + cffi_get(pInt, "int")

pArr = cffi_new("int", 5)
for i = 0 to 4
    pElem = cffi_offset(pArr, i * cffi_sizeof("int"))
    cffi_set(pElem, "int", (i + 1) * 10)
next
for i = 0 to 4
    pElem = cffi_offset(pArr, i * cffi_sizeof("int"))
    ? "arr[" + i + "]: " + cffi_get(pElem, "int")
next

pDbl = cffi_new("double")
cffi_set(pDbl, "double", 3.14)
? "double: " + cffi_get(pDbl, "double")

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