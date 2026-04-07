/*
 * Example 22: OOP - Enums, Callbacks, Strings, Errors, CDef
 * API: FFI (enum, enumValue, callback, string, toString, sym, isNullPtr, errno, strError, cdef)
 */
load "cffi.ring"

oFFI = new FFI("libc.so.6")

# Enums
oColor = oFFI.enum("Color", [
    ["RED", 0],
    ["GREEN", 1],
    ["BLUE", 2]
])
? "RED   = " + oFFI.enumValue(oColor, "RED")
? "GREEN = " + oFFI.enumValue(oColor, "GREEN")
? "BLUE  = " + oFFI.enumValue(oColor, "BLUE")

# Callbacks
oCb = oFFI.callback("my_handler", "void", "int")
? "callback created: " + (not isNull(oCb))

# Strings
pStr = oFFI.string("Hello OOP String!")
? "C string: " + oFFI.toString(pStr)

# Symbol resolution
pSym = oFFI.sym("strlen")
? "strlen symbol: " + (not oFFI.isNullPtr(pSym))

# Error handling
? "errno: " + oFFI.errno()
? "error: " + oFFI.strError()

# C Definition Parser
cDef = "
    struct Vec2 {
        float x;
        float y;
    };
"
nParsed = oFFI.cdef(cDef)
? "Parsed " + nParsed + " definitions"

func my_handler nValue
    ? "callback got: " + nValue
