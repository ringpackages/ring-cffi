/*
 * Example 09: Unions (C API)
 * API: cffi_union, cffi_union_new, cffi_union_size, cffi_field
 */
load "cffi.ring"

pLib = cffi_load(getLibcPath())

oData = cffi_union("Data", [
    ["i", "int"],
    ["d", "double"],
    ["c", "char"]
])

? "union size: " + cffi_union_size(oData)

pU = cffi_union_new(oData)
cffi_set(cffi_field(pU, oData, "i"), "int", 42)
? "as int: " + cffi_get(cffi_field(pU, oData, "i"), "int")

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