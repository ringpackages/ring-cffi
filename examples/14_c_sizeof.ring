/*
 * Example 14: sizeof (C API)
 * API: cffi_sizeof
 */
load "cffi.ring"

pLib = cffi_load(getLibcPath())

? "sizeof(char)       = " + cffi_sizeof("char")
? "sizeof(short)      = " + cffi_sizeof("short")
? "sizeof(int)        = " + cffi_sizeof("int")
? "sizeof(long)       = " + cffi_sizeof("long")
? "sizeof(long long)  = " + cffi_sizeof("long long")
? "sizeof(float)      = " + cffi_sizeof("float")
? "sizeof(double)     = " + cffi_sizeof("double")
? "sizeof(ptr)        = " + cffi_sizeof("ptr")
? "sizeof(size_t)     = " + cffi_sizeof("size_t")
? "sizeof(int32_t)    = " + cffi_sizeof("int32_t")
? "sizeof(int64_t)    = " + cffi_sizeof("int64_t")

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