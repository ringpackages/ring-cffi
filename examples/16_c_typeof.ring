/*
 * Example 16: typeof (C API)
 * API: cffi_typeof
 */
load "cffi.ring"

pLib = cffi_load(getLibcPath())

oPoint = cffi_struct("Point", [
    ["x", "int"],
    ["y", "int"]
])

oType = cffi_typeof("Point")
? "Point type: " + (not cffi_isnull(oType))
? "Point size: " + cffi_struct_size(oType)

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