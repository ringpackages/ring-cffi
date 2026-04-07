/*
 * Example 11: Callbacks (C API)
 * API: cffi_callback
 */
load "cffi.ring"

pLib = cffi_load(getLibcPath())

oCb = cffi_callback("my_handler", "void", ["int"])
? "callback created: " + (not cffi_isnull(oCb))

oCb2 = cffi_callback("my_adder", "int", ["int", "int"])
? "callback with args: " + (not cffi_isnull(oCb2))

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

func my_handler nValue
    ? "callback received: " + nValue

func my_adder a, b
    return a + b
