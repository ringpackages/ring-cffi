/*
 * Example 07: Function Pointers (C API)
 * API: cffi_funcptr, cffi_invoke
 */
load "cffi.ring"

pLib = cffi_load(getLibcPath())

pSym = cffi_sym(pLib, "atoi")
oFunc = cffi_funcptr(pSym, "int", ["ptr"])
? "atoi: " + cffi_invoke(oFunc, cffi_string("12345"))

pPuts = cffi_sym(pLib, "puts")
oPuts = cffi_funcptr(pPuts, "int", ["ptr"])
cffi_invoke(oPuts, cffi_string("Hello via funcptr!"))

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