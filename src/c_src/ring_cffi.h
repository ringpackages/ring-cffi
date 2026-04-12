/*
 * RingCFFI - Foreign Function Interface for Ring Language
 * Author: Youssef Saeed <youssefelkholey@gmail.com>
 * Copyright (c) 2026
 */

#ifndef RING_CFFI_H
#define RING_CFFI_H

#include "ring.h"
#include <ffi.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
typedef HMODULE FFI_LibHandle;
FFI_LibHandle FFI_LoadLib_UTF8(const char *path);
#define FFI_LoadLib(path) FFI_LoadLib_UTF8(path)
#define FFI_GetSym(h, name) GetProcAddress(h, name)
#define FFI_CloseLib(h) FreeLibrary(h)
#define FFI_LibError() "LoadLibrary failed"
#else
#include <dlfcn.h>
typedef void *FFI_LibHandle;
#define FFI_LoadLib(path) dlopen(path, RTLD_NOW | RTLD_GLOBAL)
#define FFI_GetSym(h, name) dlsym(h, name)
#define FFI_CloseLib(h) dlclose(h)
#define FFI_LibError() dlerror()
#endif

/* ============================================================
 * Forward Declarations
 * ============================================================ */
typedef struct FFI_Context FFI_Context;
typedef struct FFI_Library FFI_Library;
typedef struct FFI_Type FFI_Type;
typedef struct FFI_StructType FFI_StructType;
typedef struct FFI_UnionType FFI_UnionType;
typedef struct FFI_EnumType FFI_EnumType;
typedef struct FFI_FuncType FFI_FuncType;
typedef struct FFI_Function FFI_Function;
typedef struct FFI_Callback FFI_Callback;

/* ============================================================
 * Type System
 * ============================================================ */

/* Type kinds */
typedef enum {
	FFI_KIND_VOID,
	FFI_KIND_INT8,
	FFI_KIND_UINT8,
	FFI_KIND_INT16,
	FFI_KIND_UINT16,
	FFI_KIND_INT32,
	FFI_KIND_UINT32,
	FFI_KIND_INT64,
	FFI_KIND_UINT64,
	FFI_KIND_FLOAT,
	FFI_KIND_DOUBLE,
	FFI_KIND_LONGDOUBLE,
	FFI_KIND_POINTER,
	FFI_KIND_STRUCT,
	FFI_KIND_UNION,
	FFI_KIND_FUNCTION,
	FFI_KIND_ENUM,
	FFI_KIND_BOOL,
	FFI_KIND_CHAR,
	FFI_KIND_SCHAR,
	FFI_KIND_UCHAR,
	FFI_KIND_SHORT,
	FFI_KIND_USHORT,
	FFI_KIND_INT,
	FFI_KIND_UINT,
	FFI_KIND_LONG,
	FFI_KIND_ULONG,
	FFI_KIND_LONGLONG,
	FFI_KIND_ULONGLONG,
	FFI_KIND_SIZE_T,
	FFI_KIND_SSIZE_T,
	FFI_KIND_PTRDIFF_T,
	FFI_KIND_INTPTR_T,
	FFI_KIND_UINTPTR_T,
	FFI_KIND_WCHAR_T,
	FFI_KIND_UNKNOWN
} FFI_TypeKind;

/* Struct field */
typedef struct FFI_StructField {
	char *name;
	FFI_Type *type;
	size_t offset;
	size_t size;
	struct FFI_StructField *next;
} FFI_StructField;

/* Struct type definition */
struct FFI_StructType {
	char *name;
	FFI_StructField *fields;
	int field_count;
	size_t size;
	size_t alignment;
	ffi_type ffi_type_def;
	ffi_type **ffi_elements;
};

/* Union type definition */
struct FFI_UnionType {
	char *name;
	FFI_StructField *fields;
	int field_count;
	size_t size;
	size_t alignment;
};

/* Enum constant */
typedef struct FFI_EnumConst {
	char *name;
	int64_t value;
	struct FFI_EnumConst *next;
} FFI_EnumConst;

/* Enum type definition */
struct FFI_EnumType {
	char *name;
	FFI_EnumConst *constants;
	int const_count;
};

/* Function type definition */
struct FFI_FuncType {
	FFI_Type *return_type;
	FFI_Type **param_types;
	int param_count;
	bool is_variadic;
	ffi_cif cif;
};

/* Generic type wrapper */
struct FFI_Type {
	FFI_TypeKind kind;
	int pointer_depth;

	union {
		FFI_StructType *struct_type;
		FFI_UnionType *union_type;
		FFI_EnumType *enum_type;
		FFI_FuncType *func_type;
		FFI_Type *pointed_type;
	} info;

	ffi_type *ffi_type_ptr;
	size_t size;
	size_t alignment;
};

/* ============================================================
 * Function and Callback
 * ============================================================ */

struct FFI_Function {
	void *func_ptr;
	FFI_FuncType *type;
	ffi_cif cif;
	ffi_type **ffi_arg_types;
	bool cif_prepared;
};

struct FFI_Callback {
	ffi_closure *closure;
	void *code_ptr;
	FFI_FuncType *type;
	char *ring_func_name;
	VM *vm;
	ffi_cif cif;
	ffi_type **ffi_arg_types;
	char *call_buf;
	char **arg_var_names;
};

/* ============================================================
 * Library Handle
 * ============================================================ */

struct FFI_Library {
	char *path;
	FFI_LibHandle handle;
	FFI_Context *ctx;
};

/* ============================================================
 * Main Context - Holds all FFI state
 * ============================================================ */

struct FFI_Context {
	RingState *ring_state;
	VM *vm;

	/* Type registries */
	HashTable *structs;
	HashTable *unions;
	HashTable *enums;

	/* Parsed type cache (interning) */
	HashTable *type_cache;

	/* Libraries */
	List *libraries;

	/* Callbacks */
	HashTable *callbacks;

	/* Error handling */
	char error_msg[1024];
	int error_code;
};

/* Thread-local context storage */
#ifdef _WIN32
#define FFI_TLS __declspec(thread)
#else
#define FFI_TLS __thread
#endif

/* Per-VM context storage using Ring's VM API */
extern FFI_TLS FFI_Context *g_ffi_ctx;

/* ============================================================
 * C Parser
 * ============================================================ */

typedef struct CParser {
	FFI_Context *ctx;
	FFI_Library *lib;
	char *src;
	char *pos;
	char error[256];
	List *result_list;
	int decl_count;
} CParser;

/* ============================================================
 * API Functions - Context Management
 * ============================================================ */

FFI_Context *ffi_context_new(RingState *state, VM *vm);
void ffi_set_error(FFI_Context *ctx, const char *fmt, ...);
const char *ffi_get_error(FFI_Context *ctx);

/* ============================================================
 * API Functions - Library Loading
 * ============================================================ */

FFI_Library *ffi_library_open(FFI_Context *ctx, const char *path);
void ffi_library_close(FFI_Library *lib);
void *ffi_library_symbol(FFI_Library *lib, const char *name);

/* ============================================================
 * API Functions - Type System
 * ============================================================ */

FFI_Type *ffi_type_primitive(FFI_Context *ctx, FFI_TypeKind kind);
FFI_Type *ffi_type_ptr(FFI_Context *ctx, FFI_Type *base);
FFI_Type *ffi_type_parse(FFI_Context *ctx, const char *type_str);

size_t ffi_sizeof(FFI_Type *type);

/* ============================================================
 * API Functions - Struct/Union Management
 * ============================================================ */

FFI_StructType *ffi_struct_define(FFI_Context *ctx, const char *name);
int ffi_struct_add_field(FFI_Context *ctx, FFI_StructType *st, const char *name, FFI_Type *type);
int ffi_struct_finalize(FFI_Context *ctx, FFI_StructType *st);

int ffi_union_add_field(FFI_Context *ctx, FFI_UnionType *ut, const char *name, FFI_Type *type);

/* ============================================================
 * API Functions - Memory Management
 * ============================================================ */

void *ffi_alloc(FFI_Context *ctx, FFI_Type *type);
void *ffi_alloc_array(FFI_Context *ctx, FFI_Type *type, size_t count);

/* ============================================================
 * API Functions - Pointer Operations
 * ============================================================ */

void *ffi_offset(void *ptr, ptrdiff_t offset);

/* ============================================================
 * API Functions - String Handling
 * ============================================================ */

char *ffi_string_new(FFI_Context *ctx, const char *str);

/* ============================================================
 * Utility Macros
 * ============================================================ */

#define FFI_ALIGN(size, alignment) (((size) + (alignment) - 1) & ~((alignment) - 1))

#endif /* RING_CFFI_H */
