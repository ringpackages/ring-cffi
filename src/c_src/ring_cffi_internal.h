/*
 * RingCFFI - Internal shared declarations
 * Author: Youssef Saeed <youssefelkholey@gmail.com>
 * Copyright (c) 2026
 */

#ifndef RING_CFFI_INTERNAL_H
#define RING_CFFI_INTERNAL_H

#include "ring.h"
#include <errno.h>
#include <ffi.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

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

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Type System
 * ============================================================ */

/* Forward declarations for circular references */
typedef struct FFI_Type FFI_Type;
typedef struct FFI_Context FFI_Context;

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
	FFI_KIND_STRING,
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

typedef struct FFI_StructField {
	char *name;
	FFI_Type *type;
	size_t offset;
	size_t size;
	size_t bit_width;
	size_t bit_offset;
	struct FFI_StructField *next;
} FFI_StructField;

typedef struct FFI_StructType {
	char *name;
	FFI_StructField *fields;
	int field_count;
	size_t size;
	size_t alignment;
	ffi_type ffi_type_def;
	ffi_type **ffi_elements;
} FFI_StructType;

typedef struct FFI_UnionType {
	char *name;
	FFI_StructField *fields;
	int field_count;
	size_t size;
	size_t alignment;
} FFI_UnionType;

typedef struct FFI_EnumConst {
	char *name;
	int64_t value;
	struct FFI_EnumConst *next;
} FFI_EnumConst;

typedef struct FFI_EnumType {
	char *name;
	FFI_EnumConst *constants;
	int const_count;
} FFI_EnumType;

typedef struct FFI_FuncType {
	FFI_Type *return_type;
	FFI_Type **param_types;
	int param_count;
	bool is_variadic;
	ffi_cif cif;
} FFI_FuncType;

typedef struct FFI_Type {
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
} FFI_Type;

typedef struct FFI_Function {
	void *func_ptr;
	FFI_FuncType *type;
	ffi_cif cif;
	ffi_type **ffi_arg_types;
	bool cif_prepared;
} FFI_Function;

typedef struct FFI_Callback {
	ffi_closure *closure;
	void *code_ptr;
	FFI_FuncType *type;
	char *ring_func_name;
	VM *vm;
	FFI_Context *ctx;
	ffi_cif cif;
	ffi_type **ffi_arg_types;
	char *call_buf;
} FFI_Callback;

typedef struct FFI_BoundFunc {
	FFI_Context *ctx;
	FFI_Function *func;
	ffi_closure *closure;
	void *code_ptr;
	ffi_cif cif;
	ffi_type **arg_types;
} FFI_BoundFunc;

typedef struct FFI_Library {
	char *path;
	FFI_LibHandle handle;
	RingState *ring_state;
} FFI_Library;

typedef struct FFI_Context {
	RingState *ring_state;
	VM *vm;
	HashTable *structs;
	HashTable *unions;
	HashTable *enums;
	HashTable *type_cache;
	HashTable *cdef_funcs;
	List *gc_list;
	char error_msg[1024];
	int error_code;
} FFI_Context;

typedef struct CParser {
	FFI_Context *ctx;
	FFI_Library *lib;
	char *src;
	char *pos;
	char error[256];
	List *result_list;
	int decl_count;
} CParser;

/* Thread-local context storage */
#ifdef _WIN32
#define FFI_TLS __declspec(thread)
#else
#define FFI_TLS __thread
#endif

extern FFI_TLS FFI_Context *g_ffi_ctx;

/* ============================================================
 * Utility Macros
 * ============================================================ */

#define FFI_ALIGN(size, alignment) (((size) + (alignment) - 1) & ~((alignment) - 1))
#define FFI_BITFIELD_TYPE_TAG "BF"
#define FFI_IS_POINTER_TYPE(t)                                                                     \
	((t)->kind == FFI_KIND_POINTER || (t)->kind == FFI_KIND_STRING || (t)->pointer_depth > 0)

#ifdef _WIN64
#define FFI_VARIADIC_INT_TYPE &ffi_type_sint64
#define FFI_VARIADIC_INT_SIZE 8
#elif defined(_WIN32)
#define FFI_VARIADIC_INT_TYPE &ffi_type_sint
#define FFI_VARIADIC_INT_SIZE 4
#elif defined(__LP64__) || defined(__x86_64__) || defined(__aarch64__)
#define FFI_VARIADIC_INT_TYPE &ffi_type_sint64
#define FFI_VARIADIC_INT_SIZE 8
#else
#define FFI_VARIADIC_INT_TYPE &ffi_type_sint
#define FFI_VARIADIC_INT_SIZE 4
#endif

/* ============================================================
 * Context Management
 * ============================================================ */

FFI_Context *ffi_context_new(RingState *state, VM *vm);
void ffi_context_free(void *state, void *ptr);
void ffi_set_error(FFI_Context *ctx, const char *fmt, ...);
const char *ffi_get_error(FFI_Context *ctx);
FFI_Context *get_or_create_context(void *pPointer);
void cdef_funcs_set(FFI_Context *ctx, const char *key, void *value);

/* ============================================================
 * GC free routines
 * ============================================================ */

void ffi_gc_free_ptr(void *state, void *ptr);
void ffi_gc_free_lib(void *state, void *ptr);
void ffi_gc_free_func(void *state, void *ptr);
void ffi_gc_free_callback(void *state, void *ptr);
void ffi_gc_free_type(void *state, void *ptr);
void ffi_gc_free_enum(void *state, void *ptr);
void ffi_gc_free_struct_type(void *state, void *ptr);
void ffi_gc_free_union_type(void *state, void *ptr);
void ffi_gc_free_bound_func(void *state, void *ptr);

/* ============================================================
 * Utility
 * ============================================================ */

char *ffi_lowerdup(FFI_Context *ctx, const char *str);
char *ffi_cstring_unescape(RingState *state, const char *src);

/* ============================================================
 * Type system helpers (used across modules)
 * ============================================================ */

FFI_Type *ffi_type_primitive(FFI_Context *ctx, FFI_TypeKind kind);
FFI_Type *ffi_type_ptr(FFI_Context *ctx, FFI_Type *base);
FFI_Type *ffi_type_parse(FFI_Context *ctx, const char *type_str);
size_t ffi_sizeof(FFI_Type *type);
bool ffi_is_64bit_int(FFI_TypeKind kind);

/* ============================================================
 * Library helpers
 * ============================================================ */

FFI_Library *ffi_library_open(FFI_Context *ctx, const char *path);
void *ffi_library_symbol(FFI_Library *lib, const char *name);

/* ============================================================
 * Struct / Union / Enum helpers
 * ============================================================ */

FFI_StructType *ffi_struct_define(FFI_Context *ctx, const char *name);
int ffi_struct_add_field(FFI_Context *ctx, FFI_StructType *st, const char *name, FFI_Type *type,
						 size_t bit_width);
int ffi_struct_add_field_full(FFI_Context *ctx, FFI_StructType *st, const char *name,
							  FFI_Type *type, size_t bit_width);
int ffi_struct_finalize(FFI_Context *ctx, FFI_StructType *st);
int ffi_union_add_field(FFI_Context *ctx, FFI_UnionType *ut, const char *name, FFI_Type *type);

/* ============================================================
 * Memory helpers
 * ============================================================ */

void *ffi_alloc(FFI_Context *ctx, FFI_Type *type);
void *ffi_alloc_array(FFI_Context *ctx, FFI_Type *type, size_t count);
void *ffi_offset(void *ptr, ptrdiff_t offset);
char *ffi_string_new(FFI_Context *ctx, const char *str);

/* ============================================================
 * Value conversion helpers
 * ============================================================ */

double ffi_read_typed_value(void *src, FFI_Type *type);
void ffi_write_typed_value(void *dst, FFI_Type *type, double val);
void ffi_push_to_ring(VM *vm, void *src, FFI_Type *type, bool is_ffi_arg);
void ffi_push_return_value(VM *vm, void *result_ptr, FFI_Type *rtype);
void ffi_ret_value(VM *vm, void *src, FFI_Type *type);
bool ffi_parse_bitfield_tag(const char *tag, FFI_TypeKind *kind, int *bit_off, int *bit_w);
void ffi_read_bitfield(VM *vm, FFI_Context *ctx, void *ptr, FFI_TypeKind bf_kind, int bit_off,
					   int bit_w);
void ffi_write_bitfield(VM *vm, FFI_Context *ctx, void *ptr, FFI_TypeKind bf_kind, int bit_off,
						int bit_w, uint64_t new_val);

/* ============================================================
 * Function / Invoke helpers
 * ============================================================ */

FFI_Function *ffi_function_create(FFI_Context *ctx, void *func_ptr, FFI_Type *ret_type,
								  FFI_Type **param_types, int param_count);
FFI_Type **parse_type_list(FFI_Context *ctx, List *type_list, int *out_count);
int ffi_store_arg(FFI_Context *ctx, VM *pVM, List *aArgs, int i, int param_idx, FFI_Type *ptype,
				  char *storage_ptr, ffi_type **out_ffi_type, size_t *out_size);
int ffi_call_function(FFI_Context *ctx, VM *pVM, FFI_Function *func, List *aArgs, int api_offset);
int ffi_call_variadic(FFI_Context *ctx, VM *pVM, FFI_Function *func, List *aArgs, int api_offset);

/* ============================================================
 * Callback helpers
 * ============================================================ */

void ffi_callback_handler(ffi_cif *cif, void *ret, void **args, void *user_data);
void ffi_bound_handler(ffi_cif *cif, void *ret, void **args, void *user_data);
void *ffi_create_trampoline(FFI_Context *ctx, FFI_Function *func);
void ffi_gc_free_bound_func(void *state, void *ptr);

/* ============================================================
 * Ring API entry points
 * ============================================================ */

void ring_cffi_load(void *pPointer);
void ring_cffi_new(void *pPointer);
void ring_cffi_sizeof(void *pPointer);
void ring_cffi_nullptr(void *pPointer);
void ring_cffi_isnull(void *pPointer);
void ring_cffi_string(void *pPointer);
void ring_cffi_tostring(void *pPointer);
void ring_cffi_errno(void *pPointer);
void ring_cffi_strerror(void *pPointer);
void ring_cffi_func(void *pPointer);
void ring_cffi_funcptr(void *pPointer);
void ring_cffi_invoke(void *pPointer);
void ring_cffi_sym(void *pPointer);
void ring_cffi_get(void *pPointer);
void ring_cffi_set(void *pPointer);
void ring_cffi_get_i64(void *pPointer);
void ring_cffi_set_i64(void *pPointer);
void ring_cffi_deref(void *pPointer);
void ring_cffi_offset(void *pPointer);
void ring_cffi_struct(void *pPointer);
void ring_cffi_typeof(void *pPointer);
void ring_cffi_struct_new(void *pPointer);
void ring_cffi_field(void *pPointer);
void ring_cffi_field_offset(void *pPointer);
void ring_cffi_struct_size(void *pPointer);
void ring_cffi_callback(void *pPointer);
void ring_cffi_enum(void *pPointer);
void ring_cffi_enum_value(void *pPointer);
void ring_cffi_union(void *pPointer);
void ring_cffi_union_new(void *pPointer);
void ring_cffi_union_size(void *pPointer);
void ring_cffi_varfunc(void *pPointer);
void ring_cffi_varcall(void *pPointer);
void ring_cffi_cdef(void *pPointer);
void ring_cffi_bind(void *pPointer);
void ring_cffi_cast(void *pPointer);
void ring_cffi_string_array(void *pPointer);
void ring_cffi_wstring(void *pPointer);
void ring_cffi_wtostring(void *pPointer);

#ifdef __cplusplus
}
#endif

#endif /* RING_CFFI_INTERNAL_H */
