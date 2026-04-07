/*
 * Example 17: OOP - Library Loading
 * API: FFI (init, loadLib, library)
 */
load "cffi.ring"

# Constructor loading
oFFI = new FFI("libc.so.6")
? "Library loaded: " + (not isNull(oFFI.library()))

# Deferred / dynamic loading
oFFI2 = new FFI
oFFI2.loadLib("libm.so.6")
? "Deferred load: " + (not isNull(oFFI2.library()))
