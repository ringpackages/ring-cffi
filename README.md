<div align="center">

# Ring CFFI

[license]: https://img.shields.io/github/license/ysdragon/ring-cffi?style=for-the-badge&logo=opensourcehardware&label=License&logoColor=C0CAF5&labelColor=414868&color=8c73cc
[language-ring]: https://img.shields.io/badge/language-Ring-2D54CB.svg?style=for-the-badge&labelColor=414868
[platform]: https://img.shields.io/badge/Platform-Windows%20|%20Linux%20|%20macOS%20|%20FreeBSD-8c73cc.svg?style=for-the-badge&labelColor=414868
[version]: https://img.shields.io/badge/dynamic/regex?url=https%3A%2F%2Fraw.githubusercontent.com%2Fysdragon%2Fring-cffi%2Fmaster%2Fpackage.ring&search=%3Aversion\s*%3D\s*"([^"]%2B)"&replace=%241&style=for-the-badge&label=version&labelColor=414868&color=7664C6
[experimental]: https://img.shields.io/badge/Status-Experimental-orange.svg?style=for-the-badge&labelColor=414868&color=f59e0b

[![][license]](LICENSE)
[![][language-ring]](https://ring-lang.github.io/)
[![][platform]](#)
[![][version]](#)
[![][experimental]](#)

**Foreign Function Interface (FFI) for the Ring programming language**

*Built on [libffi](https://sourceware.org/libffi/) - A portable Foreign Function Interface library*

---

</div>

## ✨ Features

-   🔌 **Dynamic Library Loading**: Load shared libraries (.so, .dll, .dylib) at runtime
-   📞 **C Function Calls**: Call C functions directly from Ring with automatic type conversion
-   🔢 **Pointer Operations**: Pointer arithmetic, dereferencing, read/write access
-   🏗️ **Struct Support**: Define and manipulate C structs with named field access
-   🔗 **Union Support**: Define and use C unions
-   🔢 **Enum Support**: Define and access C enumerations
-   🔄 **Callbacks**: Pass Ring functions as C callbacks
-   🧠 **Memory Management**: Allocate and manage C memory directly
-   📝 **C String Handling**: Seamless conversion between Ring and C strings
-   🌍 **Cross-Platform**: Works on Windows, Linux, macOS, and FreeBSD
-   ⚡ **High-Level OOP API**: Clean `FFI` class with intuitive method names

## 📥 Installation

### Using RingPM

```bash
ringpm install ring-cffi from ysdragon
```

## 🚀 Quick Start

```ring
load "cffi.ring"

# Load libc and call printf
ffi = new FFI {
    loadLib("libc.so.6")  # or msvcrt.dll on Windows
    oPrintf = varFunc("printf", "int", 1, ["string"])
    varcall(oPrintf, ["Hello from Ring! Value: %d\n", 42])
}
```

## 📖 Usage

### Loading Libraries and Calling Functions

```ring
load "cffi.ring"

ffi = new FFI

# Load a shared library
ffi.loadLib("libm.so.6")  # Math library on Linux
# ffi.loadLib("msvcrt.dll")  # C runtime on Windows
# ffi.loadLib("libSystem.B.dylib")  # macOS system library

# Call a simple function (sqrt)
oSqrt = ffi.cFunc("sqrt", "double", ["double"])
result = ffi.invoke(oSqrt, [25.0])
? "sqrt(25) = " + result  # 5.0

# Call printf (variadic function)
oPrintf = ffi.varFunc("printf", "int", 1, ["string"])
ffi.varcall(oPrintf, ["Hello, %s! Number: %d\n", "World", 123])
```

### Memory Allocation and Pointers

```ring
load "cffi.ring"

ffi = new FFI

# Allocate memory for an integer
pInt = ffi.alloc("int")

# Write a value to the pointer
ffi.ptrSet(pInt, "int", 42)

# Read the value back
? ffi.ptrGet(pInt, "int")  # 42

# Pointer arithmetic
pNext = ffi.offset(pInt, 4)  # Move 4 bytes forward

# Check for NULL
pNull = ffi.nullptr()
? ffi.isNullPtr(pNull)  # 1 (true)

# Get size of a type
? ffi.sizeof("double")  # 8
```

### Working with C Strings

```ring
load "cffi.ring"

ffi = new FFI("libc.so.6")

# Create a C string
cStr = ffi.string("Hello from Ring!")

# Read it back
? ffi.toString(cStr)  # "Hello from Ring!"
```

### Defining and Using Structs

```ring
load "cffi.ring"

ffi = new FFI

# Define a Point struct
point = ffi.defineStruct("Point", [
    ["x", "int"],
    ["y", "int"],
    ["name", "string"]
])

# Allocate a new struct instance
pPoint = ffi.structNew(point)

# Set field values
ffi.ptrSet(ffi.fieldPtr(pPoint, point, "x"), "int", 10)
ffi.ptrSet(ffi.fieldPtr(pPoint, point, "y"), "int", 20)
ffi.ptrSet(ffi.fieldPtr(pPoint, point, "name"), "string", "MyPoint")

# Read field values
? "x = " + ffi.field(pPoint, point, "x")  # 10
? "y = " + ffi.field(pPoint, point, "y")  # 20
? "name = " + ffi.field(pPoint, point, "name")  # "MyPoint"

# Get struct info
? "Size: " + ffi.structSize(point) + " bytes"
? "Offset of y: " + ffi.fieldOffset(point, "y")
```

### Defining and Using Unions

```ring
load "cffi.ring"

ffi = new FFI

# Define a union
data = ffi.defineUnion("Data", [
    ["i", "int"],
    ["f", "float"],
    ["c", "char"]
])

# Allocate and use
pData = ffi.unionNew(data)
ffi.ptrSet(pData, "int", 42)
? ffi.field(pData, data, "i")  # 42

? "Union size: " + ffi.unionSize(data)  # Size of largest member
```

### Defining and Using Enums

```ring
load "cffi.ring"

ffi = new FFI

# Define an enum
colors = ffi.enum("Colors", [
    ["RED", 0],
    ["GREEN", 1],
    ["BLUE", 2]
])

? ffi.enumValue(colors, "RED")    # 0
? ffi.enumValue(colors, "GREEN")  # 1
? ffi.enumValue(colors, "BLUE")   # 2
```

### Creating Callbacks

```ring
load "cffi.ring"

ffi = new FFI("libc.so.6")

# Create a C callback
cb = ffi.callback("myCompare", "int", ["ptr", "ptr"])

# Pass callback to C function (e.g., qsort)
oQsort = ffi.cFunc("qsort", "void", ["ptr", "size_t", "size_t", "ptr"])
# ... use with array sorting

# Define a Ring function to use as callback
func myCompare(pA, pB)
    nA = cffi_get(pA, "int")
    nB = cffi_get(pB, "int")
    if nA < nB return -1 ok
    if nA > nB return  1 ok
    return 0
```

### Direct Function Pointer Calls

```ring
load "cffi.ring"

ffi = new FFI("libc.so.6")

# Get a function pointer directly
pFunc = ffi.sym("strlen")

# Create callable from pointer
oFunc = ffi.funcPtr(pFunc, "int", ["string"])
result = ffi.invoke(oFunc, ["Hello"])
? "strlen(Hello) = " + result
```

### Error Handling

```ring
load "cffi.ring"

ffi = new FFI

# Get error information (errno is set by previous C calls)
? "Errno: " + ffi.errno()
? "Error: " + ffi.strError()
```

## 📚 API Reference

### FFI Class

| Method | Taken Arguments | Description |
|--------|-----------------|-------------|
| `new FFI[(cPath]]` | `cPath`: string (optional) — library path | Create FFI instance, optionally loading a library |
| `loadLib(cPath)` | `cPath`: string — library path | Load a shared library |
| `library()` | *(none)* | Get the raw library handle |
| `cFunc(cName, cRetType, [aArgTypes])` | `cName`: string — function name, `cRetType`: string — return type, `aArgTypes`: list (optional) — argument type strings | Create a C function wrapper |
| `funcPtr(pPtr, cRetType, [aArgTypes])` | `pPtr`: pointer — function pointer, `cRetType`: string — return type, `aArgTypes`: list (optional) — argument type strings | Create wrapper from function pointer |
| `varFunc(cName, cRetType, nFixedCount, [aArgTypes])` | `cName`: string — function name, `cRetType`: string — return type, `nFixedCount`: number — fixed arg count, `aArgTypes`: list (optional) — argument type strings | Create variadic function wrapper |
| `alloc(cType)` | `cType`: string — type name | Allocate memory for a single C type |
| `allocArray(cType, nCount)` | `cType`: string — type name, `nCount`: number — array element count | Allocate memory for an array of C types |
| `sizeof(cType)` | `cType`: string — type name | Get size of a C type in bytes |
| `nullptr()` | *(none)* | Create a NULL pointer |
| `isNullPtr(pPtr)` | `pPtr`: pointer — pointer to check | Check if pointer is NULL |
| `ptrGet(pPtr, cType)` | `pPtr`: pointer — memory pointer, `cType`: string — type name | Read value from pointer |
| `ptrSet(pPtr, cType, value)` | `pPtr`: pointer — memory pointer, `cType`: string — type name, `value`: any — value to write | Write value to pointer |
| `deref(pPtr)` | `pPtr`: pointer — pointer to dereference | Dereference a pointer, returning the pointed-to pointer |
| `derefTyped(pPtr, cType)` | `pPtr`: pointer — pointer to dereference, `cType`: string — type name | Dereference a pointer with explicit type |
| `offset(pPtr, nOffset)` | `pPtr`: pointer — base pointer, `nOffset`: number — byte offset | Offset a pointer by bytes |
| `invoke(oFunc, [aArgs])` | `oFunc`: pointer — function wrapper, `aArgs`: list (optional) — arguments | Call a C function wrapper |
| `varcall(oFunc, [aArgs])` | `oFunc`: pointer — variadic function wrapper, `aArgs`: list (optional) — arguments | Call a variadic function wrapper |
| `fieldPtr(pStruct, oStruct, cField)` | `pStruct`: pointer — struct instance, `oStruct`: pointer — struct definition, `cField`: string — field name | Get pointer to struct field |
| `toString(pPtr)` | `pPtr`: pointer — C string pointer | Read null-terminated C string |
| `string(cString)` | `cString`: string — Ring string | Create C string from Ring string |
| `sym(cName)` | `cName`: string — symbol name | Look up symbol in loaded library |
| `defineStruct(cName, aFields)` | `cName`: string — struct name, `aFields`: list — field definitions `[["name", "type"], ...]` | Define a C struct |
| `structNew(oStruct)` | `oStruct`: pointer — struct definition | Allocate struct instance |
| `field(pStruct, oStruct, cField)` | `pStruct`: pointer — struct instance, `oStruct`: pointer — struct definition, `cField`: string — field name | Get pointer to struct field |
| `fieldOffset(oStruct, cField)` | `oStruct`: pointer — struct definition, `cField`: string — field name | Get byte offset of field |
| `structSize(oStruct)` | `oStruct`: pointer — struct definition | Get struct size in bytes |
| `defineUnion(cName, aFields)` | `cName`: string — union name, `aFields`: list — field definitions `[["name", "type"], ...]` | Define a C union |
| `unionNew(oUnion)` | `oUnion`: pointer — union definition | Allocate union instance |
| `unionSize(oUnion)` | `oUnion`: pointer — union definition | Get union size in bytes |
| `enum(cName, aVariants)` | `cName`: string — enum name, `aVariants`: list — variant definitions `[["NAME", value], ...]` | Define a C enum |
| `enumValue(oEnum, cVariant)` | `oEnum`: pointer — enum definition, `cVariant`: string — variant name | Get enum variant value |
| `callback(cRingFunc, cRetType, [aArgTypes])` | `cRingFunc`: string — Ring function name, `cRetType`: string — return type, `aArgTypes`: list (optional) — argument type strings | Create C callback |
| `cdef(cDef)` | `cDef`: string — C definition source | Parse C definition string |
| `errno()` | *(none)* | Get last C errno value |
| `strError()` | *(none)* | Get error string for errno |

### Low Level C Functions

These are the underlying native C extension functions exposed to Ring via `RING_API_REGISTER`:

| Ring Function | Taken Arguments | Description |
|---------------|-----------------|-------------|
| `cffi_load(cPath)` | `cPath`: string — library path | Load a shared library |
| `cffi_new(cType, [nCount])` | `cType`: string — type name, `nCount`: number (optional) — array element count | Allocate memory for a C type |
| `cffi_sizeof(cType)` | `cType`: string — type name | Get size of a C type in bytes |
| `cffi_nullptr()` | *(none)* | Create a NULL pointer |
| `cffi_isnull(pPtr)` | `pPtr`: pointer — pointer to check | Check if pointer is NULL |
| `cffi_string(cStr)` | `cStr`: string — Ring string | Create a C string from Ring string |
| `cffi_tostring(pPtr)` | `pPtr`: pointer — C string pointer | Convert C string to Ring string |
| `cffi_errno()` | *(none)* | Get last C errno value |
| `cffi_strerror([nErr])` | `nErr`: number (optional) — error code | Get error string for errno |
| `cffi_func(pLib, cName, cRetType, [aArgTypes])` | `pLib`: pointer — library handle, `cName`: string — function name, `cRetType`: string — return type, `aArgTypes`: list (optional) — argument type strings | Create a C function wrapper |
| `cffi_funcptr(pPtr, cRetType, [aArgTypes])` | `pPtr`: pointer — function pointer, `cRetType`: string — return type, `aArgTypes`: list (optional) — argument type strings | Create wrapper from function pointer |
| `cffi_invoke(pFunc, [aArgs])` | `pFunc`: pointer — function handle, `aArgs`: list (optional) — arguments | Call a C function wrapper |
| `cffi_sym(pLib, cName)` | `pLib`: pointer — library handle, `cName`: string — symbol name | Look up symbol in loaded library |
| `cffi_get(pPtr, cType, [nIndex])` | `pPtr`: pointer — memory pointer, `cType`: string — type name, `nIndex`: number (optional) — array index | Read value from pointer |
| `cffi_set(pPtr, cType, value, [nIndex])` | `pPtr`: pointer — memory pointer, `cType`: string — type name, `value`: any — value to write, `nIndex`: number (optional) — array index | Write value to pointer |
| `cffi_deref(pPtr, [cType])` | `pPtr`: pointer — pointer to dereference, `cType`: string (optional) — type name | Dereference a pointer |
| `cffi_offset(pPtr, nOffset)` | `pPtr`: pointer — base pointer, `nOffset`: number — byte offset | Offset a pointer by bytes |
| `cffi_struct(cName, [aFields])` | `cName`: string — struct name, `aFields`: list (optional) — field definitions `[["name", "type"], ...]` | Define a C struct |
| `cffi_typeof(cName)` | `cName`: string — type/struct/union/enum name | Get type handle by name |
| `cffi_struct_new(pType)` | `pType`: pointer — struct definition | Allocate struct instance |
| `cffi_field(pPtr, pType, cField)` | `pPtr`: pointer — struct/union instance, `pType`: pointer — type definition, `cField`: string — field name | Get pointer to struct field |
| `cffi_field_offset(pType, cField)` | `pType`: pointer — struct type, `cField`: string — field name | Get byte offset of field |
| `cffi_struct_size(pType)` | `pType`: pointer — struct type | Get struct size in bytes |
| `cffi_callback(cFunc, cRetType, [aArgTypes])` | `cFunc`: string — Ring function name, `cRetType`: string — return type, `aArgTypes`: list (optional) — argument type strings | Create C callback |
| `cffi_enum(cName, aConsts)` | `cName`: string — enum name, `aConsts`: list — constant definitions `[["NAME", value], ...]` | Define a C enum |
| `cffi_enum_value(pEnum, cName)` | `pEnum`: pointer — enum handle, `cName`: string — constant name | Get enum variant value |
| `cffi_union(cName, [aFields])` | `cName`: string — union name, `aFields`: list (optional) — field definitions `[["name", "type"], ...]` | Define a C union |
| `cffi_union_new(pType)` | `pType`: pointer — union definition | Allocate union instance |
| `cffi_union_size(pType)` | `pType`: pointer — union type | Get union size in bytes |
| `cffi_varfunc(pLib, cName, cRetType, nFixedCount, [aArgTypes])` | `pLib`: pointer — library handle, `cName`: string — function name, `cRetType`: string — return type, `nFixedCount`: number — fixed arg count, `aArgTypes`: list (optional) — argument type strings | Create variadic function wrapper |
| `cffi_varcall(pFunc, [aArgs])` | `pFunc`: pointer — variadic function handle, `aArgs`: list (optional) — arguments | Call a variadic function wrapper |
| `cffi_cdef(pLib, cDeclarations)` | `pLib`: pointer — library handle (can be NULL), `cDeclarations`: string — C definition source | Parse C definition string |

### Supported C Types

All type aliases recognized by the parser in `parse_type_kind()`:

#### Fixed-Width Integers

| Type(s) | Description | Size |
|---------|-------------|------|
| `int8`, `int8_t`, `Sint8` | 8-bit signed int | 1 byte |
| `uint8`, `uint8_t`, `Uint8`, `byte` | 8-bit unsigned int | 1 byte |
| `int16`, `int16_t`, `Sint16` | 16-bit signed int | 2 bytes |
| `uint16`, `uint16_t`, `Uint16` | 16-bit unsigned int | 2 bytes |
| `int32`, `int32_t`, `Sint32` | 32-bit signed int | 4 bytes |
| `uint32`, `uint32_t`, `Uint32` | 32-bit unsigned int | 4 bytes |
| `int64`, `int64_t`, `Sint64` | 64-bit signed int | 8 bytes |
| `uint64`, `uint64_t`, `Uint64` | 64-bit unsigned int | 8 bytes |

#### Standard C Types

| Type(s) | Description | Size |
|---------|-------------|------|
| `char` | Character | 1 byte |
| `signed char`, `schar` | Signed character | 1 byte |
| `unsigned char`, `uchar` | Unsigned character | 1 byte |
| `short`, `short int`, `signed short` | Short integer | 2 bytes |
| `unsigned short`, `ushort`, `unsigned short int` | Unsigned short integer | 2 bytes |
| `int`, `signed`, `signed int` | Signed integer | 4 bytes |
| `unsigned`, `unsigned int`, `uint` | Unsigned integer | 4 bytes |
| `long`, `long int`, `signed long` | Long integer | platform-dependent |
| `ulong`, `unsigned long`, `unsigned long int` | Unsigned long integer | platform-dependent |
| `long long`, `long long int`, `signed long long` | Long long integer | 8 bytes |
| `unsigned long long`, `ulonglong`, `unsigned long long int` | Unsigned long long integer | 8 bytes |
| `float` | Single precision float | 4 bytes |
| `double` | Double precision float | 8 bytes |
| `long double` | Extended precision float | platform-dependent |

#### Pointers & Strings

| Type(s) | Description | Size |
|---------|-------------|------|
| `ptr`, `pointer`, `void*` | Generic pointer | platform-dependent |
| `string`, `char*`, `cstring` | Null-terminated C string | platform-dependent |

#### Platform Types

| Type(s) | Description | Size |
|---------|-------------|------|
| `size_t` | Unsigned size type | 8 bytes |
| `ssize_t` | Signed size type | 8 bytes |
| `ptrdiff_t` | Pointer difference type | 8 bytes |
| `intptr_t` | Signed integer pointer type | 8 bytes |
| `uintptr_t` | Unsigned integer pointer type | 8 bytes |

#### Other Types

| Type(s) | Description | Size |
|---------|-------------|------|
| `void` | No return value | — |
| `bool`, `_Bool` | Boolean type | 1 byte |
| `wchar_t` | Wide character type | platform-dependent |

> **Note:** The parser also strips `const`, `volatile`, and `restrict` qualifiers. Pointer suffixes (e.g. `int**`) are handled automatically by counting trailing `*` characters in any type string.

## 📂 Examples

Check the [`examples/`](examples/) directory for usage examples covering:

| # | Example | Description |
|---|---------|-------------|
| 01 | 📥 Library Loading | `cffi_load` — loading shared libraries |
| 02 | 📞 Function Calls | `cffi_func` — calling C functions |
| 03 | 💾 Memory Allocation | `cffi_new` / `cffi_alloc` — allocating C memory |
| 04 | 🔢 Pointer Operations | `cffi_get`, `cffi_set`, `cffi_deref`, `cffi_offset` |
| 05 | 📝 C Strings | `cffi_string`, `cffi_tostring` — string handling |
| 06 | 🔍 Symbol Lookup | `cffi_sym` — resolving library symbols |
| 07 | 🎯 Function Pointers | `cffi_funcptr` — wrapping raw function pointers |
| 08 | 🏗️ Structs | `cffi_struct`, `cffi_struct_new`, `cffi_field` — C structs |
| 09 | 🔗 Unions | `cffi_union`, `cffi_union_new` — C unions |
| 10 | 🔢 Enums | `cffi_enum`, `cffi_enum_value` — C enumerations |
| 11 | 🔄 Callbacks | `cffi_callback` — Ring functions as C callbacks |
| 12 | ⚡ Variadic Functions | `cffi_varfunc`, `cffi_varcall` — variadic C calls |
| 13 | 📜 C Definition Parser | `cffi_cdef` — parsing C declarations |
| 14 | 📏 Sizeof | `cffi_sizeof` — getting type sizes |
| 15 | ⚠️ Error Handling | `cffi_errno`, `cffi_strerror` — C errors |
| 16 | 🔎 Typeof | `cffi_typeof` — runtime type queries |
| 17–18 | 🧩 OOP Basics | High-level `FFI` class — loading and calling |
| 19–20 | 💻 OOP Memory | High-level `FFI` class — memory and pointers |
| 21 | 🏛️ OOP Structs/Unions | High-level `FFI` class — structs and unions |
| 22 | 🧹 OOP Misc | High-level `FFI` class — misc operations |
| 23 | 📊 OOP qsort | Complete `qsort` callback example |

## 🛠️ Development

### Prerequisites

-   **CMake**: Version 3.16 or higher
-   **C Compiler**: GCC, Clang, or MSVC
-   **Ring Source Code**: Ring language source code
-   **Git**: For cloning submodules

### Build Steps

1.  **Clone the Repository:**
    ```sh
    git clone --recursive https://github.com/ysdragon/ring-cffi.git
    ```

2.  **Set the `RING` Environment Variable:**
    ```shell
    # Linux/macOS/FreeBSD
    export RING=/path/to/ring

    # Windows
    set RING=X:\path\to\ring
    ```

3.  **Build:**
    ```sh
    mkdir build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build .
    ```

The compiled library will be in `lib/<os>/<arch>/`.

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.