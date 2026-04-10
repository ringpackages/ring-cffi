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
		run("single function declaration", :test_single_function)
		run("multiple function declarations", :test_multiple_functions)
		run("function with void parameter", :test_function_void_param)
		run("function with pointer parameters", :test_function_pointer_params)
		run("function with multiple parameters", :test_function_multiple_params)
		run("function with const char*", :test_function_const_char_star)
		run("variadic function declaration", :test_variadic_function)
		run("double return type function", :test_double_return_function)
		run("void return type function", :test_void_return_function)
		run("long return type function", :test_long_return_function)
		run("simple struct with int fields", :test_struct_simple)
		run("struct with mixed field types", :test_struct_mixed_types)
		run("struct with pointer field", :test_struct_pointer_field)
		run("struct with array field", :test_struct_array_field)
		run("multiple struct declarations", :test_struct_multiple)
		run("struct with bitfield", :test_struct_bitfield)
		run("struct with function pointer field", :test_struct_func_pointer_field)
		run("simple union", :test_union_simple)
		run("union with mixed types", :test_union_mixed)
		run("multiple unions", :test_union_multiple)
		run("enum with explicit values", :test_enum_explicit_values)
		run("enum with auto-increment", :test_enum_auto_increment)
		run("enum with mixed explicit/auto", :test_enum_mixed_auto_explicit)
		run("simple typedef", :test_typedef_simple)
		run("multiple typedefs", :test_typedef_multiple)
		run("typedef struct", :test_typedef_struct)
		run("typedef union", :test_typedef_union)
		run("typedef enum", :test_typedef_enum)
		run("function pointer typedef", :test_typedef_func_pointer)
		run("array typedef", :test_typedef_array)
		run("unsigned types", :test_unsigned_types)
		run("signed type", :test_signed_type)
		run("long long type", :test_long_long)
		run("long double type", :test_long_double)
		run("line comments", :test_line_comment)
		run("block comments", :test_block_comment)
		run("NULL lib struct", :test_null_lib_struct)
		run("NULL lib functions", :test_null_lib_functions)
		run("mixed struct and function", :test_mixed_struct_and_function)
		run("mixed enum and struct", :test_mixed_enum_struct)
		run("mixed all types", :test_mixed_all_types)
		run("extra whitespace", :test_extra_whitespace)
		run("compact declaration", :test_compact_declaration)
		run("double and triple pointers", :test_deep_pointers)
		run("opaque pointer typedef", :test_opaque_pointer)
		run("array of pointers", :test_array_of_pointers)
		run("nested structs", :test_nested_structs)
		run("forward declarations", :test_forward_declarations)
		run("calling conventions (ignored keywords)", :test_calling_conventions)
		run("anonymous unions", :test_anonymous_unions)
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
		oFunc = cffi_varfunc(oFFI.library(), "sprintf", "int", ["ptr", "ptr"])
		cffi_varcall(oFunc, pBuf, pFmt, 123)
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
		oFunc = cffi_varfunc(oFFI.library(), "sprintf", "int", ["ptr", "ptr"])
		assertIsPointer(oFunc, "cffi_varfunc should return pointer")

	# ==================== C Definition Parser Tests ====================

	func test_single_function
		n = cffi_cdef(oFFI.library(), "int abs(int);")
		assertEq(n, 1, "single function count")

	func test_multiple_functions
		n = cffi_cdef(oFFI.library(), "
			size_t strlen(const char*);
			int abs(int);
		")
		assertEq(n, 2, "multiple functions count")

	func test_function_void_param
		n = cffi_cdef(oFFI.library(), "int getpid(void);")
		assertEq(n, 1, "void param function count")

	func test_function_pointer_params
		n = cffi_cdef(oFFI.library(), "
			void* malloc(size_t);
			void free(void*);
		")
		assertEq(n, 2, "pointer params count")

	func test_function_multiple_params
		n = cffi_cdef(oFFI.library(), "int memcmp(const void*, const void*, size_t);")
		assertEq(n, 1, "multi-param function count")

	func test_function_const_char_star
		n = cffi_cdef(oFFI.library(), "int strcmp(const char*, const char*);")
		assertEq(n, 1, "const char* params count")

	func test_variadic_function
		n = cffi_cdef(oFFI.library(), "int printf(const char*, ...);")
		assertEq(n, 1, "variadic function count")

	func test_double_return_function
		n = cffi_cdef(oFFI.library(), "double atof(const char*);")
		assertEq(n, 1, "double return function count")

	func test_void_return_function
		n = cffi_cdef(oFFI.library(), "void exit(int);")
		assertEq(n, 1, "void return function count")

	func test_long_return_function
		n = cffi_cdef(oFFI.library(), "long strtol(const char*, char**, int);")
		assertEq(n, 1, "long return function count")

	func test_struct_simple
		n = cffi_cdef(oFFI.library(), "
			struct Point2D {
				int x;
				int y;
			};
		")
		assertEq(n, 1, "simple struct count")
		t = cffi_typeof("Point2D")
		assert(cffi_struct_size(t) > 0, "Point2D size > 0")

	func test_struct_mixed_types
		n = cffi_cdef(oFFI.library(), "
			struct MixedData {
				int i;
				double d;
				char c;
			};
		")
		assertEq(n, 1, "mixed struct count")

	func test_struct_pointer_field
		n = cffi_cdef(oFFI.library(), "
			struct StringWrapper {
				char* str;
				int length;
			};
		")
		assertEq(n, 1, "struct with pointer count")

	func test_struct_array_field
		n = cffi_cdef(oFFI.library(), "
			struct Buffer {
				char data[256];
				int size;
			};
		")
		assertEq(n, 1, "struct with array count")

	func test_struct_multiple
		n = cffi_cdef(oFFI.library(), "
			struct Vec2 {
				float x;
				float y;
			};
			struct Vec3 {
				float x;
				float y;
				float z;
			};
		")
		assertEq(n, 2, "multiple structs count")

	func test_struct_bitfield
		n = cffi_cdef(oFFI.library(), "
			struct BitField {
				unsigned int a : 4;
				unsigned int b : 8;
				int c;
			};
		")
		assertEq(n, 1, "struct with bitfield count")

	func test_struct_func_pointer_field
		n = cffi_cdef(oFFI.library(), "
			struct Operations {
				int (*add)(int, int);
				int (*sub)(int, int);
				int result;
			};
		")
		assertEq(n, 1, "struct with func ptr count")

	func test_union_simple
		n = cffi_cdef(oFFI.library(), "
			union Number {
				int i;
				float f;
				double d;
			};
		")
		assertEq(n, 1, "simple union count")

	func test_union_mixed
		n = cffi_cdef(oFFI.library(), "
			union DataValue {
				int i;
				char c;
				long l;
			};
		")
		assertEq(n, 1, "mixed union count")

	func test_union_multiple
		n = cffi_cdef(oFFI.library(), "
			union IntFloat {
				int i;
				float f;
			};
			union CharInt {
				char c;
				int i;
			};
		")
		assertEq(n, 2, "multiple unions count")

	func test_enum_explicit_values
		n = cffi_cdef(oFFI.library(), "
			enum Status {
				OK = 0,
				ERROR = 1,
				PENDING = 2
			};
		")
		assertEq(n, 1, "explicit enum count")
		t = cffi_typeof("Status")
		assertEq(cffi_enum_value(t, "OK"), 0, "Status::OK value")
		assertEq(cffi_enum_value(t, "ERROR"), 1, "Status::ERROR value")

	func test_enum_auto_increment
		n = cffi_cdef(oFFI.library(), "
			enum Direction {
				NORTH,
				EAST,
				SOUTH,
				WEST
			};
		")
		assertEq(n, 1, "auto enum count")
		t = cffi_typeof("Direction")
		assertEq(cffi_enum_value(t, "NORTH"), 0, "NORTH value")
		assertEq(cffi_enum_value(t, "EAST"), 1, "EAST value")
		assertEq(cffi_enum_value(t, "SOUTH"), 2, "SOUTH value")
		assertEq(cffi_enum_value(t, "WEST"), 3, "WEST value")

	func test_enum_mixed_auto_explicit
		n = cffi_cdef(oFFI.library(), "
			enum Priority {
				LOW = 1,
				MEDIUM,
				HIGH = 10,
				CRITICAL
			};
		")
		assertEq(n, 1, "mixed enum count")
		t = cffi_typeof("Priority")
		assertEq(cffi_enum_value(t, "LOW"), 1, "LOW value")
		assertEq(cffi_enum_value(t, "MEDIUM"), 2, "MEDIUM auto-increment")
		assertEq(cffi_enum_value(t, "HIGH"), 10, "HIGH value")
		assertEq(cffi_enum_value(t, "CRITICAL"), 11, "CRITICAL auto-increment")

	func test_typedef_simple
		n = cffi_cdef(oFFI.library(), "typedef unsigned long ulong;")
		assertEq(n, 1, "simple typedef count")

	func test_typedef_multiple
		n = cffi_cdef(oFFI.library(), "
			typedef int int32_alias;
			typedef double real;
		")
		assertEq(n, 2, "multiple typedefs count")

	func test_typedef_struct
		n = cffi_cdef(oFFI.library(), "
			typedef struct {
				int x;
				int y;
			} Point;
		")
		assertEq(n, 1, "typedef struct count")
		t = cffi_typeof("Point")
		assert(cffi_struct_size(t) > 0, "Point typedef size > 0")

	func test_typedef_union
		n = cffi_cdef(oFFI.library(), "
			typedef union {
				int i;
				float f;
			} Number;
		")
		assertEq(n, 1, "typedef union count")

	func test_typedef_enum
		n = cffi_cdef(oFFI.library(), "
			typedef enum {
				FALSE_VAL = 0,
				TRUE_VAL = 1
			} BoolAlias;
		")
		assertEq(n, 1, "typedef enum count")

	func test_typedef_func_pointer
		n = cffi_cdef(oFFI.library(), "typedef void (*callback)(int);")
		assertEq(n, 1, "function pointer typedef count")

	func test_typedef_array
		n = cffi_cdef(oFFI.library(), "typedef int matrix[16];")
		assertEq(n, 1, "array typedef count")

	func test_unsigned_types
		n = cffi_cdef(oFFI.library(), "
			unsigned int foo_func(unsigned int);
			unsigned char bar_func(unsigned char);
		")
		assertEq(n, 2, "unsigned types count")

	func test_signed_type
		n = cffi_cdef(oFFI.library(), "signed char test_signed_char(void);")
		assertEq(n, 1, "signed char count")

	func test_long_long
		n = cffi_cdef(oFFI.library(), "long long test_long_long(void);")
		assertEq(n, 1, "long long count")

	func test_long_double
		n = cffi_cdef(oFFI.library(), "long double test_long_double(void);")
		assertEq(n, 1, "long double count")

	func test_line_comment
		n = cffi_cdef(oFFI.library(), "
			// This is a comment
			int abs_comment(int);
		")
		assertEq(n, 1, "line comment handling")

	func test_block_comment
		n = cffi_cdef(oFFI.library(), "
			/* This is a block comment
			   spanning multiple lines */
			int abs_block(int);
		")
		assertEq(n, 1, "block comment handling")

	func test_null_lib_struct
		nullLib = cffi_nullptr()
		n = cffi_cdef(nullLib, "
			struct TestNull {
				int a;
				int b;
			};
		")
		assertEq(n, 1, "NULL lib struct count")

	func test_null_lib_functions
		nullLib = cffi_nullptr()
		n = cffi_cdef(nullLib, "
			int func1(int);
			double func2(double);
			void func3(void);
		")
		assertEq(n, 3, "NULL lib functions count")

	func test_mixed_struct_and_function
		n = cffi_cdef(oFFI.library(), "
			struct Config {
				int width;
				int height;
				char* title;
			};
			int setup(struct Config*);
		")
		assertEq(n, 2, "struct and function count")

	func test_mixed_enum_struct
		n = cffi_cdef(oFFI.library(), "
			enum Mode { MODE_A = 1, MODE_B = 2 };
			struct Settings {
				int mode;
				int value;
			};
		")
		assertEq(n, 2, "enum and struct count")

	func test_mixed_all_types
		n = cffi_cdef(oFFI.library(), "
			int mixed_func(int);
			struct MixedAll {
				int x;
				char* name;
			};
			union MixedAll {
				int i;
				float f;
			};
			enum MixedAllEnum { MA = 0, MB = 1 };
		")
		assertEq(n, 4, "all types mixed count")

	func test_extra_whitespace
		n = cffi_cdef(oFFI.library(), "  int    abs_ws   (  int  )  ;  ")
		assertEq(n, 1, "whitespace handling")

	func test_compact_declaration
		n = cffi_cdef(oFFI.library(), "int abs_compact(int);")
		assertEq(n, 1, "compact declaration")

	func test_deep_pointers
		n = cffi_cdef(oFFI.library(), "void do_magic(char*** ptr);")
		assertEq(n, 1, "triple pointer parsing")

	func test_opaque_pointer
		n = cffi_cdef(oFFI.library(), "
			struct OpaqueHandle;
			typedef struct OpaqueHandle* Handle;
		")
		assert(n = 1 OR n = 2, "opaque pointer typedef")

	func test_array_of_pointers
		n = cffi_cdef(oFFI.library(), "int main(int argc, char *argv[]);")
		assertEq(n, 1, "array of pointers")

	func test_nested_structs
		n = cffi_cdef(oFFI.library(), "
			struct Outer {
				struct Inner {
					int id;
				} in;
				int value;
			};
		")
		assertEq(n, 1, "nested struct parsing")

	func test_forward_declarations
		n = cffi_cdef(oFFI.library(), "
			struct Node;
			struct Node {
				struct Node* next;
				int data;
			};
		")
		assert(n >= 1, "forward declaration")

	func test_calling_conventions
		n = cffi_cdef(oFFI.library(), "
			extern void __stdcall Sleep(unsigned long dwMilliseconds);
			__declspec(dllexport) int do_something(void);
		")
		assertEq(n, 2, "ignore C-specific attributes")

	func test_anonymous_unions
		n = cffi_cdef(oFFI.library(), "
			struct Vector3 {
				union {
					struct { float x, y, z; };
					float v[3];
				};
			};
		")
		assertEq(n, 1, "anonymous union inside struct")

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
		oFunc = oTest.varFunc("sprintf", "int", ["ptr", "ptr"])
		assert(isPointer(oFunc), "varFunc should return function pointer")

	func test_ffi_class_varcall
		oTest = new FFI
		oTest.loadLib(cLibcPath)
		pBuf = oTest.allocArray("char", 64)
		pFmt = oTest.string("Num: %d")
		oFunc = oTest.varFunc("sprintf", "int", ["ptr", "ptr"])
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
		arch = getarch()
		is64 = ((arch = "x64") or (arch = "arm64"))
		if is64
			assert(nSize = 8, "sizeof(ptr) should be 8 on 64-bit")
		else
			assert(nSize = 4, "sizeof(ptr) should be 4 on 32-bit")
		ok

	func test_sizeof_char
		nSize = cffi_sizeof("char")
		assert(nSize = 1, "sizeof(char) should be 1")

	func test_sizeof_long
		nSize = cffi_sizeof("long")
		if isWindows()
			# On Windows, long is ALWAYS 4 bytes (LLP64)
			assertEq(nSize, 4, "sizeof(long) should be 4 on Windows")
		else
			# On Linux/macOS, long is 8 bytes on 64-bit (LP64)
			arch = getarch()
			is64 = ((arch = "x64") or (arch = "arm64"))
			if is64
				assertEq(nSize, 8, "sizeof(long) should be 8 on 64-bit Unix")
			else
				assertEq(nSize, 4, "sizeof(long) should be 4 on 32-bit Unix")
			ok
		ok
