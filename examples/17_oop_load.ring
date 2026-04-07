/*
 * Example 17: OOP - Library Loading
 * API: FFI (init, loadLib, library)
 */
load "cffi.ring"

# Constructor loading
oFFI = new FFI(getLibcPath())
? "Library loaded: " + (not isNull(oFFI.library()))

# Deferred / dynamic loading
oFFI2 = new FFI
oFFI2.loadLib(getLibmPath())
? "Deferred load: " + (not isNull(oFFI2.library()))

func getLibmPath
    if isWindows() or isFreeBSD() or isMacOSX()
        return getLibcPath()
    ok
    return "libm.so.6"

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