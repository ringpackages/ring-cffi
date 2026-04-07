/*
 * CFFI Extension Test Suite
 * Tests all functionality of the Ring CFFI library
 */

load "stdlibcore.ring"

arch = getarch()
osDir = ""
archDir = ""
libName = ""
libVariant = ""

if isWindows()
	osDir = "windows"
	libName = "ring_cffi.dll"
	if arch = "x64"
		archDir = "amd64"
	but arch = "arm64"
		archDir = "arm64"
	but arch = "x86"
		archDir = "i386"
	else
		raise("Unsupported Windows architecture: " + arch)
	ok
but isLinux()
	osDir = "linux"
	libName = "libring_cffi.so"
	if arch = "x64"
		archDir = "amd64"
	but arch = "arm64"
		archDir = "arm64"
	else
		raise("Unsupported Linux architecture: " + arch)
	ok
	if isMusl()
		libVariant = "musl/"
	ok
but isFreeBSD()
	osDir = "freebsd"
	libName = "libring_cffi.so"
	if arch = "x64"
		archDir = "amd64"
	but arch = "arm64"
		archDir = "arm64"
	else
		raise("Unsupported FreeBSD architecture: " + arch)
	ok
but isMacOSX()
	osDir = "macos"
	libName = "libring_cffi.dylib"
	if arch = "x64"
		archDir = "amd64"
	but arch = "arm64"
		archDir = "arm64"
	else
		raise("Unsupported macOS architecture: " + arch)
	ok
else
	raise("Unsupported OS! You need to build the library for your OS.")
ok

loadlib("../lib/" + osDir + "/" + libVariant + archDir + "/" + libName)

load "../src/cffi.ring"

func main
	new CFFITest()

func isMusl
	cOutput = systemCmd("sh -c 'ldd 2>&1'")
	return substr(cOutput, "musl") > 0

class CFFITest

	nTestsRun = 0
	nTestsFailed = 0
	oFFI = NULL
	cLibcPath = ""
	cTestDir = "cffi_test_data"

	func init
		? "Setting up test environment..."
		setupTestData()
		detectLibc()
		? "Test environment ready." + nl
		runAllTests()

	func setupTestData
		if isWindows()
			system("rmdir /s /q " + cTestDir + " 2>nul")
			system("mkdir " + cTestDir)
		else
			system("rm -rf " + cTestDir + " 2>/dev/null")
			system("mkdir -p " + cTestDir)
		ok
		write(cTestDir + "/hello.txt", "Hello from CFFI test!")
		write(cTestDir + "/binary.bin", char(0) + char(1) + char(255) + char(128))

	func cleanup
		if isWindows()
			system("rmdir /s /q " + cTestDir + " 2>nul")
		else
			system("rm -rf " + cTestDir + " 2>/dev/null")
		ok

	func assert(condition, message)
		if !condition
			raise("Assertion Failed: " + message)
		ok

	func assertEq(actual, expected, message)
		if actual != expected
			raise("Assertion Failed: " + message + " - expected: " + expected + " got: " + actual)
		ok

	func assertIsPointer(val, message)
		if !isPointer(val)
			raise("Assertion Failed: " + message + " - expected pointer")
		ok

	func run(testName, methodName)
		nTestsRun++
		see "  " + testName + "..."
		try
			call methodName()
			see " [PASS]" + nl
		catch
			nTestsFailed++
			see " [FAIL]" + nl
			see "    -> " + cCatchError + nl
		done

	func detectLibc
		if isWindows()
			cLibcPath = "msvcrt.dll"
		but isLinux()
			cLibcPath = "libc.so.6"
		but isFreeBSD()
			cLibcPath = "libc.so.7"
		but isMacOSX()
			cLibcPath = "libSystem.B.dylib"
		else
			raise("Unknown OS for libc detection")
		ok

	func runAllTests
		? "========================================"
		? "  Running CFFI Extension Test Suite"
		? "========================================" + nl

		? "Testing Library Loading..."
		run("test_load_libc", :test_load_libc)
		run("test_load_nonexistent", :test_load_nonexistent)
		? ""

		? "Testing Function Creation & Invocation..."
		run("test_strlen", :test_strlen)
		run("test_strcpy", :test_strcpy)
		run("test_strcmp", :test_strcmp)
		run("test_memset", :test_memset)
		run("test_memcpy", :test_memcpy)
		run("test_atoi", :test_atoi)
		run("test_atof", :test_atof)
		run("test_sprintf", :test_sprintf)
		? ""

		? "Testing Memory Allocation..."
		run("test_alloc_int", :test_alloc_int)
		run("test_alloc_double", :test_alloc_double)
		run("test_alloc_array", :test_alloc_array)
		run("test_alloc_string", :test_alloc_string)
		? ""

		? "Testing Pointer Operations..."
		run("test_nullptr", :test_nullptr)
		run("test_isnull", :test_isnull)
		run("test_ptr_get_set_int", :test_ptr_get_set_int)
		run("test_ptr_get_set_double", :test_ptr_get_set_double)
		run("test_ptr_offset", :test_ptr_offset)
		run("test_deref", :test_deref)
		? ""

		? "Testing String Operations..."
		run("test_cffi_string", :test_cffi_string)
		run("test_cffi_tostring", :test_cffi_tostring)
		? ""

		? "Testing Symbol Resolution..."
		run("test_sym", :test_sym)
		run("test_sym_nonexistent", :test_sym_nonexistent)
		? ""

		? "Testing Function Pointers..."
		run("test_funcptr", :test_funcptr)
		? ""

		? "Testing Struct Operations..."
		run("test_struct_define", :test_struct_define)
		run("test_struct_new", :test_struct_new)
		run("test_struct_field_get_set", :test_struct_field_get_set)
		run("test_struct_field_offset", :test_struct_field_offset)
		run("test_struct_size", :test_struct_size)
		run("test_nested_struct", :test_nested_struct)
		? ""

		? "Testing Union Operations..."
		run("test_union_define", :test_union_define)
		run("test_union_new", :test_union_new)
		run("test_union_size", :test_union_size)
		? ""

		? "Testing Enum Operations..."
		run("test_enum_define", :test_enum_define)
		run("test_enum_value", :test_enum_value)
		? ""

		? "Testing Callbacks..."
		run("test_callback_simple", :test_callback_simple)
		run("test_callback_with_args", :test_callback_with_args)
		? ""

		? "Testing Variadic Functions..."
		run("test_varfunc_sprintf", :test_varfunc_sprintf)
		? ""

		? "Testing C Definition Parser..."
		run("test_cdef_struct", :test_cdef_struct)
		? ""

		? "Testing Error Handling..."
		run("test_errno", :test_errno)
		run("test_strerror", :test_strerror)
		? ""

		? "Testing OOP FFI Class..."
		run("test_ffi_class_load", :test_ffi_class_load)
		run("test_ffi_class_library", :test_ffi_class_library)
		run("test_ffi_class_cFunc", :test_ffi_class_cFunc)
		run("test_ffi_class_funcPtr", :test_ffi_class_funcPtr)
		run("test_ffi_class_invoke", :test_ffi_class_invoke)
		run("test_ffi_class_varFunc", :test_ffi_class_varFunc)
		run("test_ffi_class_varcall", :test_ffi_class_varcall)
		run("test_ffi_class_alloc", :test_ffi_class_alloc)
		run("test_ffi_class_allocArray", :test_ffi_class_allocArray)
		run("test_ffi_class_sizeof", :test_ffi_class_sizeof)
		run("test_ffi_class_nullptr", :test_ffi_class_nullptr)
		run("test_ffi_class_isNullPtr", :test_ffi_class_isNullPtr)
		run("test_ffi_class_ptrGet", :test_ffi_class_ptrGet)
		run("test_ffi_class_ptrSet", :test_ffi_class_ptrSet)
		run("test_ffi_class_deref", :test_ffi_class_deref)
		run("test_ffi_class_derefTyped", :test_ffi_class_derefTyped)
		run("test_ffi_class_offset", :test_ffi_class_offset)
		run("test_ffi_class_string", :test_ffi_class_string)
		run("test_ffi_class_toString", :test_ffi_class_toString)
		run("test_ffi_class_sym", :test_ffi_class_sym)
		run("test_ffi_class_struct", :test_ffi_class_struct)
		run("test_ffi_class_structNew", :test_ffi_class_structNew)
		run("test_ffi_class_fieldPtr", :test_ffi_class_fieldPtr)
		run("test_ffi_class_field", :test_ffi_class_field)
		run("test_ffi_class_fieldOffset", :test_ffi_class_fieldOffset)
		run("test_ffi_class_structSize", :test_ffi_class_structSize)
		run("test_ffi_class_union", :test_ffi_class_union)
		run("test_ffi_class_unionNew", :test_ffi_class_unionNew)
		run("test_ffi_class_unionSize", :test_ffi_class_unionSize)
		run("test_ffi_class_enum", :test_ffi_class_enum)
		run("test_ffi_class_enumValue", :test_ffi_class_enumValue)
		run("test_ffi_class_callback", :test_ffi_class_callback)
		run("test_ffi_class_cdef", :test_ffi_class_cdef)
		run("test_ffi_class_errno", :test_ffi_class_errno)
		run("test_ffi_class_strerror", :test_ffi_class_strerror)
		? ""

		? "Testing sizeof..."
		run("test_sizeof_int", :test_sizeof_int)
		run("test_sizeof_double", :test_sizeof_double)
		run("test_sizeof_pointer", :test_sizeof_pointer)
		run("test_sizeof_char", :test_sizeof_char)
		run("test_sizeof_long", :test_sizeof_long)
		? ""

		# Cleanup
		cleanup()

		? "========================================"
		? "Test Summary:"
		? "  Total Tests: " + nTestsRun
		? "  Passed: " + (nTestsRun - nTestsFailed)
		? "  Failed: " + nTestsFailed
		? "========================================"
		if nTestsFailed = 0
			? "SUCCESS: All tests passed!"
		else
			? "FAILURE: Some tests did not pass."
		ok

		shutdown(nTestsFailed)

	# ==================== Library Loading Tests ====================

	func test_load_libc
		oFFI = new FFI
		oFFI.loadLib(cLibcPath)
		assert(oFFI.library() != NULL, "Library should be loaded")

	func test_load_nonexistent
		try
			oBad = new FFI
			oBad.loadLib("nonexistent_lib_12345.so")
		catch
			# Expected
		done
		assert(true, "Should handle nonexistent library gracefully")

	# ==================== Function Creation & Invocation Tests ====================

	func test_strlen
		pStr = cffi_string("Hello, World!")
		oFunc = cffi_func(oFFI.library(), "strlen", "int", ["ptr"])
		nLen = cffi_invoke(oFunc, pStr)
		assertEq(nLen, 13, "strlen('Hello, World!') should be 13")

	func test_strcpy
		pDest = cffi_new("char", 64)
		pSrc = cffi_string("CFFI test string")
		oFunc = cffi_func(oFFI.library(), "strcpy", "ptr", ["ptr", "ptr"])
		cffi_invoke(oFunc, pDest, pSrc)
		cResult = cffi_tostring(pDest)
		assertEq(cResult, "CFFI test string", "strcpy should copy string correctly")

	func test_strcmp
		pStr1 = cffi_string("abc")
		pStr2 = cffi_string("abc")
		pStr3 = cffi_string("def")
		oFunc = cffi_func(oFFI.library(), "strcmp", "int", ["ptr", "ptr"])
		nResult1 = cffi_invoke(oFunc, pStr1, pStr2)
		nResult2 = cffi_invoke(oFunc, pStr1, pStr3)
		assertEq(nResult1, 0, "strcmp('abc', 'abc') should be 0")
		assert(nResult2 < 0, "strcmp('abc', 'def') should be negative")

	func test_memset
		pBuf = cffi_new("char", 16)
		oFunc = cffi_func(oFFI.library(), "memset", "ptr", ["ptr", "int", "int"])
		cffi_invoke(oFunc, pBuf, 65, 10)
		cResult = cffi_tostring(pBuf)
		assertEq(left(cResult, 10), "AAAAAAAAAA", "memset should fill buffer with 'A'")

	func test_memcpy
		pSrc = cffi_string("Hello memcpy!")
		pDest = cffi_new("char", 32)
		oFunc = cffi_func(oFFI.library(), "memcpy", "ptr", ["ptr", "ptr", "int"])
		cffi_invoke(oFunc, pDest, pSrc, 14)
		cResult = cffi_tostring(pDest)
		assertEq(cResult, "Hello memcpy!", "memcpy should copy bytes correctly")

	func test_atoi
		pStr = cffi_string("42")
		oFunc = cffi_func(oFFI.library(), "atoi", "int", ["ptr"])
		nResult = cffi_invoke(oFunc, pStr)
		assertEq(nResult, 42, "atoi('42') should be 42")

	func test_atof
		pStr = cffi_string("3.14159")
		oFunc = cffi_func(oFFI.library(), "atof", "double", ["ptr"])
		nResult = cffi_invoke(oFunc, pStr)
		assert(nResult > 3.14 and nResult < 3.15, "atof('3.14159') should be ~3.14159")

	func test_sprintf
		pBuf = cffi_new("char", 64)
		pFmt = cffi_string("Value: %d")
		oFunc = cffi_func(oFFI.library(), "sprintf", "int", ["ptr", "ptr", "int"])
		cffi_invoke(oFunc, pBuf, pFmt, 123)
		cResult = cffi_tostring(pBuf)
		assertEq(cResult, "Value: 123", "sprintf should format string")

	# ==================== Memory Allocation Tests ====================

	func test_alloc_int
		pInt = cffi_new("int")
		assertIsPointer(pInt, "cffi_new('int') should return pointer")
		cffi_set(pInt, "int", 999)
		nVal = cffi_get(pInt, "int")
		assertEq(nVal, 999, "Allocated int should be set and read correctly")

	func test_alloc_double
		pDbl = cffi_new("double")
		assertIsPointer(pDbl, "cffi_new('double') should return pointer")
		cffi_set(pDbl, "double", 3.14159)
		nVal = cffi_get(pDbl, "double")
		assert(nVal > 3.14 and nVal < 3.15, "Allocated double should be set correctly")

	func test_alloc_array
		pArr = cffi_new("int", 5)
		assertIsPointer(pArr, "cffi_new('int', 5) should return pointer")
		for i = 0 to 4
			pElem = cffi_offset(pArr, i * cffi_sizeof("int"))
			cffi_set(pElem, "int", (i + 1) * 10)
		next
		pFirst = cffi_offset(pArr, 0)
		assertEq(cffi_get(pFirst, "int"), 10, "First element should be 10")
		pLast = cffi_offset(pArr, 4 * cffi_sizeof("int"))
		assertEq(cffi_get(pLast, "int"), 50, "Last element should be 50")

	func test_alloc_string
		pStr = cffi_new("char", 32)
		assertIsPointer(pStr, "cffi_new('char', 32) should return pointer")

	# ==================== Pointer Operations Tests ====================

	func test_nullptr
		pNull = cffi_nullptr()
		assert(cffi_isnull(pNull), "nullptr should be null")

	func test_isnull
		pNull = cffi_nullptr()
		assert(cffi_isnull(pNull), "nullptr should be null")
		pInt = cffi_new("int")
		assert(!cffi_isnull(pInt), "Allocated pointer should not be null")

	func test_ptr_get_set_int
		pInt = cffi_new("int")
		cffi_set(pInt, "int", 12345)
		nVal = cffi_get(pInt, "int")
		assertEq(nVal, 12345, "get/set int should work")

	func test_ptr_get_set_double
		pDbl = cffi_new("double")
		cffi_set(pDbl, "double", 2.71828)
		nVal = cffi_get(pDbl, "double")
		assert(nVal > 2.71 and nVal < 2.72, "get/set double should work")

	func test_ptr_offset
		pArr = cffi_new("int", 3)
		nSize = cffi_sizeof("int")
		pElem0 = cffi_offset(pArr, 0)
		pElem1 = cffi_offset(pArr, nSize)
		pElem2 = cffi_offset(pArr, nSize * 2)
		cffi_set(pElem0, "int", 100)
		cffi_set(pElem1, "int", 200)
		cffi_set(pElem2, "int", 300)
		assertEq(cffi_get(pElem0, "int"), 100, "offset element 0")
		assertEq(cffi_get(pElem1, "int"), 200, "offset element 1")
		assertEq(cffi_get(pElem2, "int"), 300, "offset element 2")

	func test_deref
		pInt = cffi_new("int")
		cffi_set(pInt, "int", 777)
		pPtr = cffi_new("ptr")
		cffi_set(pPtr, "ptr", pInt)
		pDeref = cffi_deref(pPtr, "ptr")
		assert(isPointer(pDeref), "cffi_deref with type should return pointer")
		assertEq(cffi_get(pDeref, "int"), 777, "deref'd pointer should resolve to original value")

	func test_derefTyped
		pInt = cffi_new("int")
		cffi_set(pInt, "int", 555)
		pPtr = cffi_new("ptr")
		cffi_set(pPtr, "ptr", pInt)
		pDeref = oFFI.derefTyped(pPtr, "ptr")
		assert(isPointer(pDeref), "derefTyped should return pointer")
		assertEq(cffi_get(pDeref, "int"), 555, "derefTyped should resolve correctly")

	# ==================== String Operations Tests ====================

	func test_cffi_string
		pStr = cffi_string("test string")
		assertIsPointer(pStr, "cffi_string should return pointer")
		cResult = cffi_tostring(pStr)
		assertEq(cResult, "test string", "cffi_string + cffi_tostring roundtrip")

	func test_cffi_tostring
		pStr = cffi_string("Hello CFFI!")
		cResult = cffi_tostring(pStr)
		assertEq(cResult, "Hello CFFI!", "cffi_tostring should read C string")

	# ==================== Symbol Resolution Tests ====================

	func test_sym
		pSym = cffi_sym(oFFI.library(), "strlen")
		assertIsPointer(pSym, "cffi_sym('strlen') should return pointer")

	func test_sym_nonexistent
		pSym = cffi_sym(oFFI.library(), "nonexistent_symbol_xyz")
		assert(cffi_isnull(pSym), "cffi_sym of nonexistent symbol should be null")

	# ==================== Function Pointer Tests ====================

	func test_funcptr
		pSym = cffi_sym(oFFI.library(), "atoi")
		oFunc = cffi_funcptr(pSym, "int", ["ptr"])
		pStr = cffi_string("999")
		nResult = cffi_invoke(oFunc, pStr)
		assertEq(nResult, 999, "funcptr + invoke should work")

	# ==================== Struct Operation Tests ====================

	func test_struct_define
		oStruct = cffi_struct("Point", [
			["x", "int"],
			["y", "int"],
			["name", "string"]
		])
		assertIsPointer(oStruct, "cffi_struct should return pointer")

	func test_struct_new
		oStruct = cffi_struct("Vec2", [
			["x", "int"],
			["y", "double"]
		])
		pStruct = cffi_struct_new(oStruct)
		assertIsPointer(pStruct, "cffi_struct_new should return pointer")

	func test_struct_field_get_set
		oStruct = cffi_struct("Data", [
			["x", "int"],
			["y", "double"]
		])
		pStruct = cffi_struct_new(oStruct)
		pX = cffi_field(pStruct, oStruct, "x")
		cffi_set(pX, "int", 42)
		pY = cffi_field(pStruct, oStruct, "y")
		cffi_set(pY, "double", 3.14)
		nX = cffi_get(pX, "int")
		assertEq(nX, 42, "Struct field x should be 42")
		nY = cffi_get(pY, "double")
		assert(nY > 3.13 and nY < 3.15, "Struct field y should be ~3.14")

	func test_struct_field_offset
		oStruct = cffi_struct("AB", [
			["a", "char"],
			["b", "int"]
		])
		nOffsetA = cffi_field_offset(oStruct, "a")
		nOffsetB = cffi_field_offset(oStruct, "b")
		assertEq(nOffsetA, 0, "First field offset should be 0")
		assert(nOffsetB > 0, "Second field offset should be > 0")

	func test_struct_size
		oStruct = cffi_struct("Triple", [
			["a", "int"],
			["b", "int"],
			["c", "int"]
		])
		nSize = cffi_struct_size(oStruct)
		assert(nSize >= 12, "Struct of 3 ints should be at least 12 bytes")

	func test_nested_struct
		oInner = cffi_struct("Inner", [
			["x", "int"],
			["y", "int"]
		])
		oOuter = cffi_struct("Outer", [
			["id", "int"],
			["pos", "ptr"]
		])
		assert(true, "Nested struct definition should not crash")

	# ==================== Union Operation Tests ====================

	func test_union_define
		oUnion = cffi_union("Number", [
			["i", "int"],
			["d", "double"],
			["c", "char"]
		])
		assertIsPointer(oUnion, "cffi_union should return pointer")

	func test_union_new
		oUnion = cffi_union("Data", [
			["i", "int"],
			["d", "double"]
		])
		pUnion = cffi_union_new(oUnion)
		assertIsPointer(pUnion, "cffi_union_new should return pointer")

	func test_union_size
		oUnion = cffi_union("Mixed", [
			["i", "int"],
			["d", "double"],
			["c", "char"]
		])
		nSize = cffi_union_size(oUnion)
		assert(nSize >= 8, "Union with double should be at least 8 bytes")

	# ==================== Enum Operation Tests ====================

	func test_enum_define
		oEnum = cffi_enum("Color", [
			["RED", 0],
			["GREEN", 1],
			["BLUE", 2]
		])
		assertIsPointer(oEnum, "cffi_enum should return pointer")

	func test_enum_value
		oEnum = cffi_enum("Color", [
			["RED", 0],
			["GREEN", 1],
			["BLUE", 2]
		])
		nRed = cffi_enum_value(oEnum, "RED")
		nGreen = cffi_enum_value(oEnum, "GREEN")
		nBlue = cffi_enum_value(oEnum, "BLUE")
		assertEq(nRed, 0, "First enum value should be 0")
		assertEq(nGreen, 1, "Second enum value should be 1")
		assertEq(nBlue, 2, "Third enum value should be 2")

	# ==================== Callback Tests ====================

	func test_callback_simple
		oCb = cffi_callback("test_callback_handler", "void", ["int"])
		assertIsPointer(oCb, "cffi_callback should return pointer")

	func test_callback_with_args
		oCb = cffi_callback("test_callback_handler2", "int", ["int", "int"])
		assertIsPointer(oCb, "Callback with multiple args should work")

	func test_callback_handler nValue
		see ""

	func test_callback_handler2 a, b
		return a + b

	# ==================== Variadic Function Tests ====================

	func test_varfunc_sprintf
		pBuf = cffi_new("char", 128)
		pFmt = cffi_string("Int: %d, Double: %.2f")
		oFunc = cffi_varfunc(oFFI.library(), "sprintf", "int", 2, ["ptr", "ptr"])
		assertIsPointer(oFunc, "cffi_varfunc should return pointer")

	# ==================== C Definition Parser Tests ====================

	func test_cdef_struct
		cDef = "struct Point { int x; int y; };"
		oDef = cffi_cdef(oFFI.library(), cDef)
		assert(true, "cffi_cdef should parse struct definition")

	# ==================== Error Handling Tests ====================

	func test_errno
		nErr = cffi_errno()
		assert(isNumber(nErr), "cffi_errno should return number")

	func test_strerror
		cMsg = cffi_strerror()
		assert(isString(cMsg), "cffi_strerror should return string")

	# ==================== OOP FFI Class Tests ====================

	func test_ffi_class_load
		oTest = new FFI
		oTest.loadLib(cLibcPath)
		assert(oTest.library() != NULL, "FFI class should load library")

	func test_ffi_class_library
		oTest = new FFI
		oTest.loadLib(cLibcPath)
		pLib = oTest.library()
		assert(isPointer(pLib), "library() should return pointer")

	func test_ffi_class_cFunc
		oTest = new FFI
		oTest.loadLib(cLibcPath)
		oFunc = oTest.cFunc("strlen", "int", ["ptr"])
		assert(isPointer(oFunc), "cFunc should return function pointer")

	func test_ffi_class_funcPtr
		oTest = new FFI
		oTest.loadLib(cLibcPath)
		pSym = oTest.sym("atoi")
		oFunc = oTest.funcPtr(pSym, "int", ["ptr"])
		assert(isPointer(oFunc), "funcPtr should return function pointer")

	func test_ffi_class_invoke
		oTest = new FFI
		oTest.loadLib(cLibcPath)
		oFunc = oTest.cFunc("strlen", "int", ["ptr"])
		pStr = oTest.string("Hello invoke!")
		nLen = oTest.invoke(oFunc, [pStr])
		assertEq(nLen, 13, "invoke(strlen) should return correct length")

	func test_ffi_class_varFunc
		oTest = new FFI
		oTest.loadLib(cLibcPath)
		oFunc = oTest.varFunc("sprintf", "int", 2, ["ptr", "ptr"])
		assert(isPointer(oFunc), "varFunc should return function pointer")

	func test_ffi_class_varcall
		oTest = new FFI
		oTest.loadLib(cLibcPath)
		pBuf = oTest.allocArray("char", 64)
		pFmt = oTest.string("Num: %d")
		oFunc = oTest.varFunc("sprintf", "int", 2, ["ptr", "ptr"])
		assert(isPointer(oFunc), "varFunc should return variadic function pointer")
		oTest.varcall(oFunc, [pBuf, pFmt, 42])
		cResult = oTest.toString(pBuf)
		assertEq(cResult, "Num: 42", "varcall should format string")

	func test_ffi_class_alloc
		oTest = new FFI
		pInt = oTest.alloc("int")
		assert(isPointer(pInt), "alloc should return pointer")
		oTest.ptrSet(pInt, "int", 42)
		assertEq(oTest.ptrGet(pInt, "int"), 42, "alloc should work for single value")

	func test_ffi_class_allocArray
		oTest = new FFI
		pArr = oTest.allocArray("int", 3)
		assert(isPointer(pArr), "allocArray should return pointer")

	func test_ffi_class_sizeof
		oTest = new FFI
		assertEq(oTest.sizeof("int"), 4, "sizeof(int) should be 4")
		assertEq(oTest.sizeof("double"), 8, "sizeof(double) should be 8")

	func test_ffi_class_nullptr
		oTest = new FFI
		pNull = oTest.nullptr()
		assert(isNull(pNull), "nullptr should return NULL")

	func test_ffi_class_isNullPtr
		oTest = new FFI
		assert(oTest.isNullPtr(oTest.nullptr()), "isNullPtr(nullptr) should be true")
		pInt = oTest.alloc("int")
		assert(!oTest.isNullPtr(pInt), "isNullPtr(valid) should be false")

	func test_ffi_class_ptrGet
		oTest = new FFI
		pInt = oTest.alloc("int")
		oTest.ptrSet(pInt, "int", 12345)
		assertEq(oTest.ptrGet(pInt, "int"), 12345, "ptrGet should read value")

	func test_ffi_class_ptrSet
		oTest = new FFI
		pInt = oTest.alloc("int")
		oTest.ptrSet(pInt, "int", 99999)
		assertEq(oTest.ptrGet(pInt, "int"), 99999, "ptrSet should write value")

	func test_ffi_class_deref
		oTest = new FFI
		pInt = oTest.alloc("int")
		oTest.ptrSet(pInt, "int", 777)
		pPtr = oTest.alloc("ptr")
		oTest.ptrSet(pPtr, "ptr", pInt)
		pDeref = oTest.deref(pPtr)
		assert(isPointer(pDeref), "deref without type should return pointer")

	func test_ffi_class_derefTyped
		oTest = new FFI
		pInt = oTest.alloc("int")
		oTest.ptrSet(pInt, "int", 555)
		pPtr = oTest.alloc("ptr")
		oTest.ptrSet(pPtr, "ptr", pInt)
		pDeref = oTest.derefTyped(pPtr, "ptr")
		assert(isPointer(pDeref), "derefTyped should return pointer")
		assertEq(oTest.ptrGet(pDeref, "int"), 555, "derefTyped should resolve correctly")

	func test_ffi_class_offset
		oTest = new FFI
		pArr = oTest.allocArray("int", 3)
		pElem1 = oTest.offset(pArr, 0)
		pElem2 = oTest.offset(pArr, oTest.sizeof("int"))
		oTest.ptrSet(pElem1, "int", 100)
		oTest.ptrSet(pElem2, "int", 200)
		assertEq(oTest.ptrGet(pElem1, "int"), 100, "offset element 0")
		assertEq(oTest.ptrGet(pElem2, "int"), 200, "offset element 1")

	func test_ffi_class_string
		oTest = new FFI
		pStr = oTest.string("test string")
		assert(isPointer(pStr), "string() should return pointer")

	func test_ffi_class_toString
		oTest = new FFI
		pStr = oTest.string("Hello toString!")
		cResult = oTest.toString(pStr)
		assertEq(cResult, "Hello toString!", "toString should read C string")

	func test_ffi_class_sym
		oTest = new FFI
		oTest.loadLib(cLibcPath)
		pSym = oTest.sym("strlen")
		assert(isPointer(pSym), "sym should return pointer")

	func test_ffi_class_struct
		oTest = new FFI
		oStruct = oTest.defineStruct("TestStruct", [["x", "int"], ["y", "int"]])
		assert(isPointer(oStruct), "defineStruct should return pointer")

	func test_ffi_class_structNew
		oTest = new FFI
		oStruct = oTest.defineStruct("Vec2", [["x", "int"], ["y", "double"]])
		pStruct = oTest.structNew(oStruct)
		assert(isPointer(pStruct), "structNew should return pointer")

	func test_ffi_class_fieldPtr
		oTest = new FFI
		oStruct = oTest.defineStruct("Point", [["x", "int"]])
		pStruct = oTest.structNew(oStruct)
		pX = oTest.fieldPtr(pStruct, oStruct, "x")
		assert(isPointer(pX), "fieldPtr should return pointer")
		oTest.ptrSet(pX, "int", 42)
		assertEq(oTest.ptrGet(pX, "int"), 42, "fieldPtr should allow read/write")

	func test_ffi_class_field
		oTest = new FFI
		oStruct = oTest.defineStruct("Point", [["x", "int"]])
		pStruct = oTest.structNew(oStruct)
		pX = oTest.field(pStruct, oStruct, "x")
		assert(isPointer(pX), "field should return pointer")

	func test_ffi_class_fieldOffset
		oTest = new FFI
		oStruct = oTest.defineStruct("AB", [["a", "char"], ["b", "int"]])
		nOffsetA = oTest.fieldOffset(oStruct, "a")
		nOffsetB = oTest.fieldOffset(oStruct, "b")
		assertEq(nOffsetA, 0, "First field offset should be 0")
		assert(nOffsetB > 0, "Second field offset should be > 0")

	func test_ffi_class_structSize
		oTest = new FFI
		oStruct = oTest.defineStruct("Triple", [["a", "int"], ["b", "int"], ["c", "int"]])
		nSize = oTest.structSize(oStruct)
		assert(nSize >= 12, "Struct of 3 ints should be at least 12 bytes")

	func test_ffi_class_union
		oTest = new FFI
		oUnion = oTest.defineUnion("Number", [["i", "int"], ["d", "double"]])
		assert(isPointer(oUnion), "defineUnion should return pointer")

	func test_ffi_class_unionNew
		oTest = new FFI
		oUnion = oTest.defineUnion("Data", [["i", "int"], ["d", "double"]])
		pUnion = oTest.unionNew(oUnion)
		assert(isPointer(pUnion), "unionNew should return pointer")

	func test_ffi_class_unionSize
		oTest = new FFI
		oUnion = oTest.defineUnion("Mixed", [["i", "int"], ["d", "double"], ["c", "char"]])
		nSize = oTest.unionSize(oUnion)
		assert(nSize >= 8, "Union with double should be at least 8 bytes")

	func test_ffi_class_enum
		oTest = new FFI
		oEnum = oTest.enum("Status", [
			["OK", 0],
			["ERROR", 1],
			["WARN", 2]
		])
		assert(isPointer(oEnum), "enum should return pointer")

	func test_ffi_class_enumValue
		oTest = new FFI
		oEnum = oTest.enum("Color", [
			["RED", 0],
			["GREEN", 1],
			["BLUE", 2]
		])
		assertEq(oTest.enumValue(oEnum, "RED"), 0, "First enum value should be 0")
		assertEq(oTest.enumValue(oEnum, "GREEN"), 1, "Second enum value should be 1")
		assertEq(oTest.enumValue(oEnum, "BLUE"), 2, "Third enum value should be 2")

	func test_ffi_class_callback
		oTest = new FFI
		oCb = oTest.callback("test_callback_handler2", "int", ["int", "int"])
		assert(isPointer(oCb), "callback should return pointer")

	func test_ffi_class_cdef
		oTest = new FFI
		oTest.loadLib(cLibcPath)
		nParsed = oTest.cdef("struct TestCDef { int x; int y; };")
		assert(nParsed >= 0, "cdef should parse without error")

	func test_ffi_class_errno
		oTest = new FFI
		nErr = oTest.errno()
		assert(isNumber(nErr), "errno should return number")

	func test_ffi_class_strerror
		oTest = new FFI
		cMsg = oTest.strError()
		assert(isString(cMsg), "strError should return string")

	# ==================== sizeof Tests ====================

	func test_sizeof_int
		nSize = cffi_sizeof("int")
		assert(nSize = 4, "sizeof(int) should be 4")

	func test_sizeof_double
		nSize = cffi_sizeof("double")
		assert(nSize = 8, "sizeof(double) should be 8")

	func test_sizeof_pointer
		nSize = cffi_sizeof("ptr")
		if isWindows64() or (isLinux() and getarch() = "x64") or (isMacOSX() and getarch() = "x64")
			assert(nSize = 8, "sizeof(ptr) should be 8 on 64-bit")
		else
			assert(nSize = 4, "sizeof(ptr) should be 4 on 32-bit")
		ok

	func test_sizeof_char
		nSize = cffi_sizeof("char")
		assert(nSize = 1, "sizeof(char) should be 1")

	func test_sizeof_long
		nSize = cffi_sizeof("long")
		assert(nSize >= 4, "sizeof(long) should be at least 4")
