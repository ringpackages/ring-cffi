/*
 * Example 21: OOP - Structs and Unions
 * API: FFI (defineStruct, structNew, fieldPtr, fieldOffset, structSize, defineUnion, unionNew, unionSize)
 */
load "cffi.ring"

oFFI = new FFI(getLibcPath())

# Struct
oPoint = oFFI.defineStruct("Point", [
    ["x", "int"],
    ["y", "int"],
    ["z", "double"]
])
? "Point size: " + oFFI.structSize(oPoint)
? "x offset: " + oFFI.fieldOffset(oPoint, "x")
? "y offset: " + oFFI.fieldOffset(oPoint, "y")

pPt = oFFI.structNew(oPoint)
oFFI.ptrSet(oFFI.fieldPtr(pPt, oPoint, "x"), "int", 10)
oFFI.ptrSet(oFFI.fieldPtr(pPt, oPoint, "y"), "int", 20)
oFFI.ptrSet(oFFI.fieldPtr(pPt, oPoint, "z"), "double", 30.5)
# Read via fieldPtr
? "Point via fieldPtr(" + oFFI.ptrGet(oFFI.fieldPtr(pPt, oPoint, "x"), "int") + ", " + oFFI.ptrGet(oFFI.fieldPtr(pPt, oPoint, "y"), "int") + ", " + oFFI.ptrGet(oFFI.fieldPtr(pPt, oPoint, "z"), "double") + ")"
# Read via field() — returns pointer, same as fieldPtr()
? "Point via field(" + oFFI.ptrGet(oFFI.field(pPt, oPoint, "x"), "int") + ", " + oFFI.ptrGet(oFFI.field(pPt, oPoint, "y"), "int") + ", " + oFFI.ptrGet(oFFI.field(pPt, oPoint, "z"), "double") + ")"

# Union
oData = oFFI.defineUnion("Data", [
    ["i", "int"],
    ["d", "double"]
])
? "Data union size: " + oFFI.unionSize(oData)
pU = oFFI.unionNew(oData)
oFFI.ptrSet(oFFI.fieldPtr(pU, oData, "i"), "int", 99)
? "union.i = " + oFFI.ptrGet(oFFI.fieldPtr(pU, oData, "i"), "int")

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