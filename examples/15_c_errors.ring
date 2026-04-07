/*
 * Example 15: Errors (C API)
 * API: cffi_errno, cffi_strerror
 */
load "cffi.ring"

pLib = cffi_load(getLibcPath())

? "errno: " + cffi_errno()
? "error: " + cffi_strerror()
? "errno 2: " + cffi_strerror(2)

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