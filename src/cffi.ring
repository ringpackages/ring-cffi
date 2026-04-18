/*
 * Ring CFFI Library
 *
 * Provides Foreign Function Interface (FFI) bindings for calling C libraries
 * from the Ring programming language using libffi.
 *
 * Features:
 *   - Load shared libraries (.so, .dll, .dylib)
 *   - Call C functions with automatic type conversion
 *   - Pointer arithmetic, dereferencing, and memory access
 *   - Create and access C structs and unions
 *   - Define and use C enums
 *   - Create callbacks (C calling Ring functions)
 *   - Allocate and manage C memory
 *   - Handle C strings
 *   - Call variadic functions (e.g., printf, sprintf)
 *   - Parse C declarations (structs, unions, enums, functions)
 */

/**
 * Class FFI: Main class for interacting with C libraries.
 *
 * Provides an object-oriented interface for loading shared libraries,
 * calling C functions, managing memory, and working with C types.
 *
 * Example usage:
 *   load "cffi.ring"
 *
 *   new FFI("libc.so.6") {
 *       oStrlen = cFunc("strlen", "int", ["ptr"])
 *       pStr = "Hello!"
 *       ? invoke(oStrlen, pStr)
 *   }
 */
 class FFI

    self.pLib = NULL

    /**
     * Creates a new FFI instance, optionally loading a library.
     * @param cPath Optional path to a shared library to load.
     * @return Self for method chaining.
     * @raises Error if the library cannot be loaded.
     */
    func init cPath
        if not isNull(cPath)
            loadLib(cPath)
        ok
        return self

    /**
     * Loads a shared library from the given path.
     * @param cPath Path to the shared library file.
     * @return Self for method chaining.
     * @raises Error if the library cannot be loaded.
     */
    func loadLib cPath
        self.pLib = cffi_load(cPath)
        return self

    /**
     * Gets the raw library handle.
     * @return The library pointer, or NULL if no library is loaded.
     */
    func library
        return pLib

    /*
     * ========================================
     * Function Creation and Calling
     * ========================================
     */

    /**
     * Creates a callable C function wrapper.
     * @param cName Name of the C function in the library.
     * @param cRetType Return type (e.g., "int", "void", "string", "ptr").
     * @param ... Optional argument type strings.
     * @return FFI_Function object that can be called with :call().
     * @raises Error if the function cannot be found or types are unknown.
     */
    func cFunc cName, cRetType, aArgTypes
        if pLib = NULL
            raise("No library loaded. Call loadLib() first.")
        ok
        if isNull(aArgTypes)
            return cffi_func(pLib, cName, cRetType)
        ok
        if isList(aArgTypes)
            return cffi_func(pLib, cName, cRetType, aArgTypes)
        ok
        return cffi_func(pLib, cName, cRetType, aArgTypes)

    /**
     * Creates a callable function from a raw function pointer.
     * @param pPtr Function pointer.
     * @param cRetType Return type.
     * @param ... Optional argument types.
     * @return FFI_Function object.
     */
    func funcPtr pPtr, cRetType, aArgTypes
        if isNull(aArgTypes)
            return cffi_funcptr(pPtr, cRetType)
        ok
        return cffi_funcptr(pPtr, cRetType, aArgTypes)

    /**
     * Creates a variadic function wrapper (supports variable arguments).
     * @param cName Name of the C function.
     * @param cRetType Return type.
     * @param aArgTypes Fixed argument type strings (fixed count inferred from list length).
     * @return FFI_Function object for variadic calls.
     */
    func varFunc cName, cRetType, aArgTypes
        if pLib = NULL
            raise("No library loaded. Call loadLib() first.")
        ok
        if isNull(aArgTypes)
            return cffi_varfunc(pLib, cName, cRetType)
        ok
        return cffi_varfunc(pLib, cName, cRetType, aArgTypes)

    /*
     * ========================================
     * Memory Allocation
     * ========================================
     */

    /**
     * Allocates memory for a single C type.
     * @param cType Type name (e.g., "int", "double").
     * @return Pointer to allocated memory.
     * @raises Error if the type is unknown or allocation fails.
     */
    func alloc cType
        return cffi_new(cType)

    /**
     * Allocates memory for an array of C types.
     * @param cType Type name (e.g., "int", "double").
     * @param nCount Number of elements.
     * @return Pointer to allocated memory.
     * @raises Error if the type is unknown or allocation fails.
     */
    func allocArray cType, nCount
        return cffi_new(cType, nCount)

    /**
     * Gets the size in bytes of a C type.
     * @param cType Type name.
     * @return Size in bytes.
     */
    func sizeof cType
        return cffi_sizeof(cType)

    /**
     * Creates a NULL pointer.
     * @return NULL pointer object.
     */
    func nullptr
        return cffi_nullptr()

    /**
     * Checks if a pointer is NULL.
     * @param pPtr Pointer to check.
     * @return 1 if NULL, 0 otherwise.
     */
    func isNullPtr pPtr
        return cffi_isnull(pPtr)

    /*
     * ========================================
     * Pointer Operations
     * ========================================
     */

    /**
     * Reads a value from a pointer.
     * @param pPtr Pointer to read from.
     * @param cType Type to read as.
     * @return The value at the pointer.
     */
    func ptrGet pPtr, cType
        return cffi_get(pPtr, cType)

    /**
     * Writes a value to a pointer.
     * @param pPtr Pointer to write to.
     * @param cType Type to write as.
     * @param value Value to write.
     */
    func ptrSet pPtr, cType, value
        cffi_set(pPtr, cType, value)

    /**
     * Reads a 64-bit integer from a pointer as a string to avoid precision loss.
     * @param pPtr Pointer to read from.
     * @param nIndex Optional array index.
     * @return The 64-bit integer as a Ring string.
     */
    func i64Get pPtr, nIndex
        if isNull(nIndex)
            return cffi_get_i64(pPtr)
        ok
        return cffi_get_i64(pPtr, nIndex)

    /**
     * Writes a 64-bit integer to a pointer from a string to avoid precision loss.
     * @param pPtr Pointer to write to.
     * @param cValue 64-bit integer as a Ring string.
     * @param nIndex Optional array index.
     */
    func i64Set pPtr, cValue, nIndex
        if isNull(nIndex)
            cffi_set_i64(pPtr, cValue)
        else
            cffi_set_i64(pPtr, cValue, nIndex)
        ok

    /**
     * Dereferences a pointer and returns the pointed-to pointer.
     * @param pPtr Pointer to dereference.
     * @return The dereferenced pointer.
     */
    func deref pPtr
        return cffi_deref(pPtr)

    /**
     * Dereferences a pointer with an explicit type and returns the typed value.
     * @param pPtr Pointer to dereference.
     * @param cType Type of the pointed-to value.
     * @return The dereferenced value or pointer.
     */
    func derefTyped pPtr, cType
        return cffi_deref(pPtr, cType)

    /**
     * Offsets a pointer by a given number of bytes.
     * @param pPtr Base pointer.
     * @param nOffset Byte offset.
     * @return New pointer at the offset.
     */
    func offset pPtr, nOffset
        return cffi_offset(pPtr, nOffset)

    /**
     * Invokes a C function wrapper with the given arguments.
     * @param oFunc Function wrapper from cFunc() or funcPtr().
     * @param aArgs List of arguments to pass to the C function.
     * @return The result of the C function call.
     */
    func invoke oFunc, aArgs
        if isNull(aArgs)
            return cffi_invoke(oFunc)
        ok
        return cffi_invoke(oFunc, aArgs)

    /**
     * Calls a variadic function wrapper.
     * @param oFunc Variadic function wrapper from varFunc().
     * @param aArgs List of arguments (fixed + variadic).
     * @return The result of the C function call.
     */
    func varcall oFunc, aArgs
        if isNull(aArgs)
            return cffi_varcall(oFunc)
        ok
        return cffi_varcall(oFunc, aArgs)

    /**
     * Gets a field pointer from a struct instance.
     * @param pStruct Struct instance pointer.
     * @param oStruct Struct definition from defineStruct().
     * @param cField Field name.
     * @return Pointer to the field.
     */
    func fieldPtr pStruct, oStruct, cField
        return cffi_field(pStruct, oStruct, cField)

    /*
     * ========================================
     * String Operations
     * ========================================
     */

    /**
     * Reads a null-terminated C string from a pointer.
     * @param pPtr Pointer to the string.
     * @return Ring string.
     */
    func toString pPtr
        return cffi_tostring(pPtr)

    /**
     * Creates a C string from a Ring string.
     * @param cString Ring string.
     * @return Pointer to the C string.
     */
    func string cString
        return cffi_string(cString)

    /*
     * ========================================
     * Symbol Resolution
     * ========================================
     */

    /**
     * Looks up a symbol (function or variable) in the loaded library.
     * @param cName Symbol name.
     * @return Pointer to the symbol.
     */
    func sym cName
        if pLib = NULL
            raise("No library loaded. Call loadLib() first.")
        ok
        return cffi_sym(pLib, cName)

    /*
     * ========================================
     * Struct Operations
     * ========================================
     */

    /**
     * Defines a C struct type.
     * @param cName Struct name (e.g., "Point").
     * @param aFields List of [name, type] pairs.
     *                Example: [["x", "int"], ["y", "double"], ["name", "string"]]
     * @return FFI_Struct definition object.
     */
    func defineStruct cName, aFields
        return cffi_struct(cName, aFields)

    /**
     * Allocates memory for a struct definition.
     * @param oStruct Struct definition from struct().
     * @return Pointer to the allocated struct.
     */
    func structNew oStruct
        return cffi_struct_new(oStruct)

    /**
     * Gets a field value from a struct.
     * @param pStruct Struct instance pointer.
     * @param oStruct Struct definition.
     * @param cField Field name.
     * @return Pointer to the field.
     */
    func field pStruct, oStruct, cField
        return cffi_field(pStruct, oStruct, cField)

    /**
     * Gets the byte offset of a field within a struct.
     * @param oStruct Struct definition.
     * @param cField Field name.
     * @return Byte offset.
     */
    func fieldOffset oStruct, cField
        return cffi_field_offset(oStruct, cField)

    /**
     * Gets the total size of a struct in bytes.
     * @param oStruct Struct definition.
     * @return Size in bytes.
     */
    func structSize oStruct
        return cffi_struct_size(oStruct)

    /*
     * ========================================
     * Union Operations
     * ========================================
     */

    /**
     * Defines a C union type.
     * @param cName Union name (e.g., "Data").
     * @param aFields List of [name, type] pairs.
     * @return FFI_Union definition object.
     */
    func defineUnion cName, aFields
        return cffi_union(cName, aFields)

    /**
     * Allocates memory for a union definition.
     * @param oUnion Union definition from defineUnion().
     * @return Pointer to the allocated union.
     */
    func unionNew oUnion
        return cffi_union_new(oUnion)

    /**
     * Gets the total size of a union in bytes.
     * @param oUnion Union definition.
     * @return Size in bytes.
     */
    func unionSize oUnion
        return cffi_union_size(oUnion)

    /*
     * ========================================
     * Enum Operations
     * ========================================
     */

    /**
     * Defines a C enum type.
     * @param cName Enum name.
     * @param aVariants List of variant names (auto-numbered from 0).
     * @return FFI_Enum definition object.
     */
    func enum cName, aVariants
        return cffi_enum(cName, aVariants)

    /**
     * Gets the numeric value of an enum variant.
     * @param oEnum Enum definition.
     * @param cVariant Variant name.
     * @return The enum value.
     */
    func enumValue oEnum, cVariant
        return cffi_enum_value(oEnum, cVariant)

    /*
     * ========================================
     * Callback Operations
     * ========================================
     */

    /**
     * Creates a C callback that can be passed to C functions.
     * @param cRingFunc Name of the Ring function to call.
     * @param cRetType Return type of the callback.
     * @param aArgTypes List of argument types.
     * @return FFI_Callback object.
     */
    func callback cRingFunc, cRetType, aArgTypes
        if isNull(aArgTypes)
            return cffi_callback(cRingFunc, cRetType)
        ok
        return cffi_callback(cRingFunc, cRetType, aArgTypes)

    /*
     * ========================================
     * Dynamic Binding
     * ========================================
     */

    /**
     * Binds a C function as a method on the FFI object using addMethod().
     * After calling bind(), the function can be called as oFFI.name(args).
     *
     * Example:
     *   oFFI = new FFI("libc.so.6")
     *   oFFI.bind("abs", "int", ["int"])
     *   ? oFFI.abs(-42)
     *
     * @param cName Name of the C function in the library.
     * @param cRetType Return type (e.g., "int", "void", "ptr").
     * @param aArgTypes Optional list of argument type strings.
     * @return FFI_Function object that was bound.
     * @raises Error if the library is not loaded or binding fails.
     */
    func bind cName, cRetType, aArgTypes
        if pLib = NULL
            raise("No library loaded. Call loadLib() first.")
        ok
        oFunc = cFunc(cName, cRetType, aArgTypes)
        cFuncAttr = "__cffi_bm_" + cName
        addattribute(self, cFuncAttr)
        setattribute(self, cFuncAttr, oFunc)
        if isNull(aArgTypes)
            nArgs = 0
        else
            nArgs = len(aArgTypes)
        ok
        cParams = bindParamList(nArgs)
        if nArgs = 0
            cInvoke = "cffi_invoke(getattribute(self, '" + cFuncAttr + "'))"
        else
            cInvoke = "cffi_invoke(getattribute(self, '" + cFuncAttr + "'), [" + bindArgRefs(nArgs) + "])"
        ok
        cFuncName = "__cffi_m_" + cName
        eval(print2str(`func #{cFuncName}(#{cParams}) { return #{cInvoke} }`))
        addMethod(self, cName, cFuncName)
        return oFunc

    /**
     * Binds a single C function as a native Ring function using
     * ring_vm_funcregister2(). No eval/runcode overhead per call.
     *
     *   oFFI.bindNative("abs", "int", ["int"])
     *   ? abs(-42)
     *
     * @param cName Name of the C function in the library.
     * @param cRetType Return type (e.g., "int", "void", "ptr").
     * @param aArgTypes Optional list of argument type strings.
     * @return 1 on success.
     * @raises Error if the library is not loaded.
     */
    func bindNative cName, cRetType, aArgTypes
        if pLib = NULL
            raise("No library loaded. Call loadLib() first.")
        ok
        if isNull(aArgTypes)
            return cffi_bind(pLib, cName, cRetType)
        ok
        return cffi_bind(pLib, cName, cRetType, aArgTypes)

    /**
     * Binds ALL functions declared via cdef() as native Ring functions.
     * Uses ring_vm_funcregister2() for each — no eval overhead per call.
     *
     *   oFFI.cdef(pLib, "int abs(int); long labs(long);")
     *   oFFI.bindAll()
     *   ? abs(-42)   # native call
     *
     * @return Number of functions registered.
     */
    func bindAll
        return cffi_bind()

    /*
     * ========================================
     * C Definition Parser
     * ========================================
     */

    /**
     * Parses a C definition string and creates corresponding FFI objects.
     * Supports struct, union, enum, and function declarations.
     * @param cDef C definition string.
     * @return Number of parsed definitions.
     */
    func cdef cDef
        return cffi_cdef(pLib, cDef)

    /*
     * ========================================
     * Error Handling
     * ========================================
     */

    /**
     * Gets the last C errno value.
     * @return Error number.
     */
    func errno
        return cffi_errno()

    /**
     * Gets a human-readable error string for the current errno.
     * @return Error message string.
     */
    func strError
        return cffi_strerror()


    private

    /*
     * ========================================
     * Private Helpers
     * ========================================
     */

    /**
     * Generates a comma-separated parameter list for a generated function.
     * E.g. bindParamList(3) returns "a, b, c"
     * @param nCount Number of parameters.
     * @return Parameter list string.
     */
    func bindParamList nCount
        if nCount = 0
            return ""
        ok
        cParams = ""
        for nI = 1 to nCount
            if nI > 1
                cParams += ", "
            ok
            cParams += char(96 + nI)
        next
        return cParams

    /**
     * Generates a comma-separated argument reference list for cffi_invoke().
     * E.g. bindArgRefs(3) returns "a, b, c"
     * @param nCount Number of arguments.
     * @return Argument reference string.
     */
    func bindArgRefs nCount
        if nCount = 0
            return ""
        ok
        cRefs = ""
        for nI = 1 to nCount
            if nI > 1
                cRefs += ", "
            ok
            cRefs += char(96 + nI)
        next
        return cRefs