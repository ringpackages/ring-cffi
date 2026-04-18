/*
 * Example 24: Dynamic Binding - Three Approaches
 * API: cffi_cdef, cffi_bind, FFI (bind, bindNative, bindAll)
 *
 *   cffi_bind()  - Low-level: bind single or all cdef'd functions as native
 *   bindNative() - High-level: single native C trampoline
 *   bindAll()    - High-level: auto-register all cdef'd functions as native
 *   bind()       - High-level OOP: method on FFI object via addMethod()
 */
load "cffi.ring"

# ============================================================
# Part 1: Low-level C API (pure functions)
# ============================================================

pLib = cffi_load(getLibcPath())

# ---- cffi_bind() - single function ----
? "=== cffi_bind() - single ==="

cffi_bind(pLib, "abs", "int", ["int"])
? "abs(-42) = " + abs(-42)

# ---- cffi_cdef() + cffi_bind() - batch ----
? "=== cffi_cdef() + cffi_bind() - batch ==="

cffi_cdef(pLib, "long labs(long); int atoi(const char*);")
nCount = cffi_bind()
? "Registered " + nCount + " functions"
? "labs(-123456) = " + labs(-123456)
pStr = cffi_string("777")
? "atoi('777') = " + atoi(pStr)

# ============================================================
# Part 2: High-level OOP API
# ============================================================

oFFI = new FFI(getLibcPath())

# ---- bindNative() - single native function ----
? "=== bindNative() ==="

oFFI.bindNative("abs", "int", ["int"])
? "abs(-42) = " + abs(-42)

# ---- bindAll() - auto-register all cdef'd functions ----
? "=== bindAll() ==="

oFFI.cdef("long labs(long); int atoi(const char*);")
nCount = oFFI.bindAll()
? "Registered " + nCount + " functions"
? "labs(-123456) = " + labs(-123456)
? "atoi('777') = " + atoi(oFFI.string("777"))

# ---- bind() - method on FFI object (OOP style) ----
? "=== bind() ==="

oFFI.bind("strlen", "int", ["ptr"])
? "oFFI.strlen('hello') = " + oFFI.strlen("hello")

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
