/*
 * Example 10: Enums (C API)
 * API: cffi_enum, cffi_enum_value
 */
load "cffi.ring"

pLib = cffi_load(getLibcPath())

oColor = cffi_enum("Color", [
    ["RED", 0],
    ["GREEN", 1],
    ["BLUE", 2],
    ["YELLOW", 10]
])

? "RED    = " + cffi_enum_value(oColor, "RED")
? "GREEN  = " + cffi_enum_value(oColor, "GREEN")
? "BLUE   = " + cffi_enum_value(oColor, "BLUE")
? "YELLOW = " + cffi_enum_value(oColor, "YELLOW")

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