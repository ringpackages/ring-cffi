/*
 * RingCFFI - Foreign Function Interface for Ring Language
 * Author: Youssef Saeed <youssefelkholey@gmail.com>
 * Copyright (c) 2026
 */

#ifndef _WIN32
#define _GNU_SOURCE
#endif

#include "ring_cffi.h"
#include <errno.h>
#include <stdarg.h>

static void ffi_gc_free_struct_type(void *state, void *ptr);
static void ffi_gc_free_union_type(void *state, void *ptr);
static void ffi_gc_free_enum(void *state, void *ptr);
static void ffi_gc_free_func(void *state, void *ptr);
static void ffi_gc_free_type(void *state, void *ptr);

#ifdef _WIN32
FFI_LibHandle FFI_LoadLib_UTF8(const char *path)
{
	if (!path)
		return NULL;
	int wlen = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
	if (wlen == 0)
		return NULL;
	wchar_t *wpath = (wchar_t *)malloc((size_t)wlen * sizeof(wchar_t));
	if (!wpath)
		return NULL;
	MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, wlen);
	HMODULE handle = LoadLibraryW(wpath);
	free(wpath);
	return handle;
}
#endif

FFI_TLS FFI_Context *g_ffi_ctx = NULL;

static ffi_type *ffi_get_primitive_type(FFI_TypeKind kind)
{
	switch (kind) {
	case FFI_KIND_VOID:
		return &ffi_type_void;
	case FFI_KIND_INT8:
		return &ffi_type_sint8;
	case FFI_KIND_UINT8:
		return &ffi_type_uint8;
	case FFI_KIND_INT16:
		return &ffi_type_sint16;
	case FFI_KIND_UINT16:
		return &ffi_type_uint16;
	case FFI_KIND_INT32:
		return &ffi_type_sint32;
	case FFI_KIND_UINT32:
		return &ffi_type_uint32;
	case FFI_KIND_INT64:
		return &ffi_type_sint64;
	case FFI_KIND_UINT64:
		return &ffi_type_uint64;
	case FFI_KIND_FLOAT:
		return &ffi_type_float;
	case FFI_KIND_DOUBLE:
		return &ffi_type_double;
	case FFI_KIND_LONGDOUBLE:
		return &ffi_type_longdouble;
	case FFI_KIND_POINTER:
		return &ffi_type_pointer;
	case FFI_KIND_BOOL:
		return &ffi_type_uint8;
	case FFI_KIND_CHAR:
		return &ffi_type_schar;
	case FFI_KIND_SCHAR:
		return &ffi_type_schar;
	case FFI_KIND_UCHAR:
		return &ffi_type_uchar;
	case FFI_KIND_SHORT:
		return &ffi_type_sshort;
	case FFI_KIND_USHORT:
		return &ffi_type_ushort;
	case FFI_KIND_INT:
		return &ffi_type_sint;
	case FFI_KIND_UINT:
		return &ffi_type_uint;
	case FFI_KIND_LONG:
		return &ffi_type_slong;
	case FFI_KIND_ULONG:
		return &ffi_type_ulong;
	case FFI_KIND_LONGLONG:
		return &ffi_type_sint64;
	case FFI_KIND_ULONGLONG:
		return &ffi_type_uint64;
	case FFI_KIND_SIZE_T:
		return (sizeof(size_t) == 8) ? &ffi_type_uint64 : &ffi_type_uint32;
	case FFI_KIND_SSIZE_T:
		return (sizeof(size_t) == 8) ? &ffi_type_sint64 : &ffi_type_sint32;
	case FFI_KIND_PTRDIFF_T:
		return (sizeof(ptrdiff_t) == 8) ? &ffi_type_sint64 : &ffi_type_sint32;
	case FFI_KIND_INTPTR_T:
		return (sizeof(intptr_t) == 8) ? &ffi_type_sint64 : &ffi_type_sint32;
	case FFI_KIND_UINTPTR_T:
		return (sizeof(uintptr_t) == 8) ? &ffi_type_uint64 : &ffi_type_uint32;
	case FFI_KIND_WCHAR_T:
		if (sizeof(wchar_t) == 2)
			return &ffi_type_uint16;
		else
			return &ffi_type_uint32;
	default:
		return &ffi_type_void;
	}
}

static size_t ffi_get_primitive_size(FFI_TypeKind kind)
{
	switch (kind) {
	case FFI_KIND_VOID:
		return 0;
	case FFI_KIND_INT8:
	case FFI_KIND_UINT8:
	case FFI_KIND_BOOL:
	case FFI_KIND_CHAR:
	case FFI_KIND_SCHAR:
	case FFI_KIND_UCHAR:
		return 1;
	case FFI_KIND_INT16:
	case FFI_KIND_UINT16:
	case FFI_KIND_SHORT:
	case FFI_KIND_USHORT:
		return 2;
	case FFI_KIND_INT32:
	case FFI_KIND_UINT32:
	case FFI_KIND_INT:
	case FFI_KIND_UINT:
		return 4;
	case FFI_KIND_INT64:
	case FFI_KIND_UINT64:
	case FFI_KIND_LONGLONG:
	case FFI_KIND_ULONGLONG:
		return 8;
	case FFI_KIND_SIZE_T:
		return sizeof(size_t);
	case FFI_KIND_SSIZE_T:
	case FFI_KIND_PTRDIFF_T:
		return sizeof(ptrdiff_t);
	case FFI_KIND_INTPTR_T:
	case FFI_KIND_UINTPTR_T:
		return sizeof(intptr_t);
	case FFI_KIND_FLOAT:
		return sizeof(float);
	case FFI_KIND_DOUBLE:
		return sizeof(double);
	case FFI_KIND_LONGDOUBLE:
		return sizeof(long double);
	case FFI_KIND_LONG:
		return sizeof(long);
	case FFI_KIND_ULONG:
		return sizeof(unsigned long);
	case FFI_KIND_POINTER:
		return sizeof(void *);
	case FFI_KIND_WCHAR_T:
		return sizeof(wchar_t);
	default:
		return 0;
	}
}

static bool ffi_is_64bit_int(FFI_TypeKind kind)
{
	switch (kind) {
	case FFI_KIND_INT64:
	case FFI_KIND_UINT64:
	case FFI_KIND_LONGLONG:
	case FFI_KIND_ULONGLONG:
		return true;
	case FFI_KIND_SIZE_T:
	case FFI_KIND_SSIZE_T:
	case FFI_KIND_PTRDIFF_T:
	case FFI_KIND_INTPTR_T:
	case FFI_KIND_UINTPTR_T:
		return sizeof(void *) == 8;
	case FFI_KIND_LONG:
	case FFI_KIND_ULONG:
		return sizeof(long) == 8;
	default:
		return false;
	}
}

FFI_Context *ffi_context_new(RingState *state, VM *vm)
{
	FFI_Context *ctx = (FFI_Context *)ring_state_malloc(state, sizeof(FFI_Context));
	if (!ctx)
		return NULL;

	memset(ctx, 0, sizeof(FFI_Context));
	ctx->ring_state = state;
	ctx->vm = vm;

	ctx->structs = ring_hashtable_new_gc(state);
	ctx->unions = ring_hashtable_new_gc(state);
	ctx->enums = ring_hashtable_new_gc(state);
	ctx->type_cache = ring_hashtable_new_gc(state);
	ctx->cdef_funcs = ring_hashtable_new_gc(state);
	ctx->gc_list = ring_list_new_gc(state, 0);

	g_ffi_ctx = ctx;
	return ctx;
}

void ffi_set_error(FFI_Context *ctx, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vsnprintf(ctx->error_msg, sizeof(ctx->error_msg), fmt, args);
	va_end(args);
	ctx->error_code = -1;
}

const char *ffi_get_error(FFI_Context *ctx) { return ctx->error_msg; }

FFI_Library *ffi_library_open(FFI_Context *ctx, const char *path)
{
	FFI_LibHandle handle = FFI_LoadLib(path);
	if (!handle) {
		ffi_set_error(ctx, "Failed to load library '%s': %s", path, FFI_LibError());
		return NULL;
	}

	FFI_Library *lib = (FFI_Library *)ring_state_malloc(ctx->ring_state, sizeof(FFI_Library));
	if (!lib) {
		FFI_CloseLib(handle);
		ffi_set_error(ctx, "Out of memory allocating library struct");
		return NULL;
	}

	memset(lib, 0, sizeof(FFI_Library));
	lib->handle = handle;
	lib->path = ring_state_malloc(ctx->ring_state, strlen(path) + 1);
	if (lib->path)
		strcpy(lib->path, path);
	lib->ring_state = ctx->ring_state;

	return lib;
}

void *ffi_library_symbol(FFI_Library *lib, const char *name)
{
	if (!lib || !lib->handle)
		return NULL;
	return FFI_GetSym(lib->handle, name);
}

FFI_Type *ffi_type_primitive(FFI_Context *ctx, FFI_TypeKind kind)
{
	FFI_Type *type = (FFI_Type *)ring_state_malloc(ctx->ring_state, sizeof(FFI_Type));
	if (!type)
		return NULL;

	memset(type, 0, sizeof(FFI_Type));
	type->kind = kind;
	type->ffi_type_ptr = ffi_get_primitive_type(kind);
	type->size = type->ffi_type_ptr->size;
	type->alignment = type->ffi_type_ptr->alignment;
	type->pointer_depth = 0;

	return type;
}

FFI_Type *ffi_type_ptr(FFI_Context *ctx, FFI_Type *base)
{
	FFI_Type *type = (FFI_Type *)ring_state_malloc(ctx->ring_state, sizeof(FFI_Type));
	if (!type)
		return NULL;

	memset(type, 0, sizeof(FFI_Type));
	type->kind = FFI_KIND_POINTER;
	type->ffi_type_ptr = ffi_get_primitive_type(FFI_KIND_POINTER);
	type->size = sizeof(void *);
	type->alignment = sizeof(void *);
	type->pointer_depth = base ? base->pointer_depth + 1 : 1;
	type->info.pointed_type = base;

	return type;
}

FFI_StructType *ffi_struct_define(FFI_Context *ctx, const char *name)
{
	FFI_StructType *st =
		(FFI_StructType *)ring_state_malloc(ctx->ring_state, sizeof(FFI_StructType));
	if (!st)
		return NULL;

	memset(st, 0, sizeof(FFI_StructType));
	if (name) {
		st->name = (char *)ring_state_malloc(ctx->ring_state, strlen(name) + 1);
		if (!st->name) {
			ring_state_free(ctx->ring_state, st);
			return NULL;
		}
		strcpy(st->name, name);
	}

	ring_list_addcustomringpointer_gc(ctx->ring_state, ctx->gc_list, st, ffi_gc_free_struct_type);

	return st;
}

int ffi_struct_add_field(FFI_Context *ctx, FFI_StructType *st, const char *name, FFI_Type *type)
{
	if (!st || !name || !type)
		return -1;

	FFI_StructField *field =
		(FFI_StructField *)ring_state_malloc(ctx->ring_state, sizeof(FFI_StructField));
	if (!field)
		return -1;

	memset(field, 0, sizeof(FFI_StructField));
	field->name = ring_state_malloc(ctx->ring_state, strlen(name) + 1);
	if (field->name)
		strcpy(field->name, name);
	field->type = type;

	if (!st->fields) {
		st->fields = field;
	} else {
		FFI_StructField *last = st->fields;
		while (last->next)
			last = last->next;
		last->next = field;
	}
	st->field_count++;

	return 0;
}

int ffi_struct_finalize(FFI_Context *ctx, FFI_StructType *st)
{
	if (!st)
		return -1;

	int count = st->field_count;
	if (count == 0) {
		st->size = 0;
		st->alignment = 1;
		return 0;
	}

	st->ffi_elements =
		(ffi_type **)ring_state_malloc(ctx->ring_state, sizeof(ffi_type *) * (count + 1));
	if (!st->ffi_elements)
		return -1;

	FFI_StructField *field = st->fields;
	size_t offset = 0;
	size_t max_align = 1;
	int i = 0;

	while (field && i < count) {
		size_t align = field->type->alignment;
		if (align > max_align)
			max_align = align;

		offset = FFI_ALIGN(offset, align);
		field->offset = offset;
		field->size = field->type->size;
		offset += field->size;

		st->ffi_elements[i] = field->type->ffi_type_ptr;
		field = field->next;
		i++;
	}
	st->ffi_elements[count] = NULL;

	st->alignment = max_align;
	st->size = FFI_ALIGN(offset, max_align);

	st->ffi_type_def.size = 0;
	st->ffi_type_def.alignment = 0;
	st->ffi_type_def.type = FFI_TYPE_STRUCT;
	st->ffi_type_def.elements = st->ffi_elements;

	if (st->name) {
		ring_hashtable_newpointer_gc(ctx->ring_state, ctx->structs, st->name, st);
	}

	return 0;
}

int ffi_union_add_field(FFI_Context *ctx, FFI_UnionType *ut, const char *name, FFI_Type *type)
{
	if (!ut || !name || !type)
		return -1;

	FFI_StructField *field =
		(FFI_StructField *)ring_state_malloc(ctx->ring_state, sizeof(FFI_StructField));
	if (!field)
		return -1;

	memset(field, 0, sizeof(FFI_StructField));
	field->name = ring_state_malloc(ctx->ring_state, strlen(name) + 1);
	if (field->name)
		strcpy(field->name, name);
	field->type = type;
	field->offset = 0;
	field->size = type->size;

	if (!ut->fields) {
		ut->fields = field;
	} else {
		FFI_StructField *last = ut->fields;
		while (last->next)
			last = last->next;
		last->next = field;
	}
	ut->field_count++;

	if (type->size > ut->size)
		ut->size = type->size;
	if (type->alignment > ut->alignment)
		ut->alignment = type->alignment;

	return 0;
}

void *ffi_alloc(FFI_Context *ctx, FFI_Type *type)
{
	if (!ctx || !type)
		return NULL;

	size_t size = type->size;
	if (size == 0)
		size = sizeof(void *);

	void *ptr = ring_state_calloc(ctx->ring_state, 1, size);
	if (!ptr) {
		ffi_set_error(ctx, "Out of memory allocating %zu bytes", size);
		return NULL;
	}

	return ptr;
}

void *ffi_alloc_array(FFI_Context *ctx, FFI_Type *type, size_t count)
{
	if (!ctx || !type || count == 0)
		return NULL;

	size_t elem_size = type->size;
	if (elem_size == 0)
		elem_size = sizeof(void *);

	size_t total = elem_size * count;
	if (count != 0 && total / count != elem_size) {
		ffi_set_error(ctx, "Integer overflow in array allocation");
		return NULL;
	}
	void *ptr = ring_state_calloc(ctx->ring_state, count, elem_size);
	if (!ptr) {
		ffi_set_error(ctx, "Out of memory allocating array of %zu elements", count);
		return NULL;
	}

	return ptr;
}

void *ffi_offset(void *ptr, ptrdiff_t offset)
{
	if (!ptr)
		return NULL;
	return (char *)ptr + offset;
}

char *ffi_string_new(FFI_Context *ctx, const char *str)
{
	if (!str)
		return NULL;
	size_t len = strlen(str);
	char *copy = (char *)ring_state_malloc(ctx->ring_state, len + 1);
	if (copy) {
		memcpy(copy, str, len);
		copy[len] = '\0';
	}
	return copy;
}

static double ffi_read_typed_value(void *src, FFI_Type *type)
{
	if (type->kind == FFI_KIND_POINTER || type->pointer_depth > 0) {
		return (double)(uintptr_t)*(void **)src;
	}
	switch (type->kind) {
	case FFI_KIND_INT8:
	case FFI_KIND_SCHAR:
	case FFI_KIND_CHAR:
		return (double)*(int8_t *)src;
	case FFI_KIND_UINT8:
	case FFI_KIND_UCHAR:
	case FFI_KIND_BOOL:
		return (double)*(uint8_t *)src;
	case FFI_KIND_INT16:
	case FFI_KIND_SHORT:
		return (double)*(int16_t *)src;
	case FFI_KIND_UINT16:
	case FFI_KIND_USHORT:
		return (double)*(uint16_t *)src;
	case FFI_KIND_INT32:
	case FFI_KIND_INT:
		return (double)*(int32_t *)src;
	case FFI_KIND_UINT32:
	case FFI_KIND_UINT:
		return (double)*(uint32_t *)src;
	case FFI_KIND_INT64:
	case FFI_KIND_LONGLONG:
	case FFI_KIND_SSIZE_T:
	case FFI_KIND_INTPTR_T:
	case FFI_KIND_PTRDIFF_T:
		return (double)*(int64_t *)src;
	case FFI_KIND_UINT64:
	case FFI_KIND_ULONGLONG:
	case FFI_KIND_SIZE_T:
	case FFI_KIND_UINTPTR_T:
		return (double)*(uint64_t *)src;
	case FFI_KIND_LONG:
		return (double)*(long *)src;
	case FFI_KIND_ULONG:
		return (double)*(unsigned long *)src;
	case FFI_KIND_FLOAT:
		return (double)*(float *)src;
	case FFI_KIND_DOUBLE:
		return *(double *)src;
	case FFI_KIND_LONGDOUBLE:
		return (double)*(long double *)src;
	default:
		return (double)*(int *)src;
	}
}

static void ffi_write_typed_value(void *dst, FFI_Type *type, double val)
{
	if (type->kind == FFI_KIND_POINTER || type->pointer_depth > 0) {
		*(void **)dst = (void *)(uintptr_t)val;
		return;
	}
	switch (type->kind) {
	case FFI_KIND_INT8:
	case FFI_KIND_SCHAR:
	case FFI_KIND_CHAR:
		*(int8_t *)dst = (int8_t)val;
		break;
	case FFI_KIND_UINT8:
	case FFI_KIND_UCHAR:
	case FFI_KIND_BOOL:
		*(uint8_t *)dst = (uint8_t)val;
		break;
	case FFI_KIND_INT16:
	case FFI_KIND_SHORT:
		*(int16_t *)dst = (int16_t)val;
		break;
	case FFI_KIND_UINT16:
	case FFI_KIND_USHORT:
		*(uint16_t *)dst = (uint16_t)val;
		break;
	case FFI_KIND_INT32:
	case FFI_KIND_INT:
		*(int32_t *)dst = (int32_t)val;
		break;
	case FFI_KIND_UINT32:
	case FFI_KIND_UINT:
		*(uint32_t *)dst = (uint32_t)val;
		break;
	case FFI_KIND_INT64:
	case FFI_KIND_LONGLONG:
	case FFI_KIND_SSIZE_T:
	case FFI_KIND_INTPTR_T:
	case FFI_KIND_PTRDIFF_T:
		*(int64_t *)dst = (int64_t)val;
		break;
	case FFI_KIND_UINT64:
	case FFI_KIND_ULONGLONG:
	case FFI_KIND_SIZE_T:
	case FFI_KIND_UINTPTR_T:
		*(uint64_t *)dst = (uint64_t)val;
		break;
	case FFI_KIND_LONG:
		*(long *)dst = (long)val;
		break;
	case FFI_KIND_ULONG:
		*(unsigned long *)dst = (unsigned long)val;
		break;
	case FFI_KIND_FLOAT:
		*(float *)dst = (float)val;
		break;
	case FFI_KIND_DOUBLE:
		*(double *)dst = val;
		break;
	case FFI_KIND_LONGDOUBLE:
		*(long double *)dst = (long double)val;
		break;
	default:
		*(int *)dst = (int)val;
		break;
	}
}

static void ffi_push_return_value(VM *vm, void *result_ptr, FFI_Type *rtype)
{
	if (rtype->kind == FFI_KIND_VOID) {
		ring_vm_api_retnumber(vm, 0);
	} else if (rtype->kind == FFI_KIND_POINTER || rtype->pointer_depth > 0) {
		ring_vm_api_retcpointer(vm, *(void **)result_ptr, "FFI_Ptr");
	} else if (rtype->kind == FFI_KIND_FLOAT) {
		ring_vm_api_retnumber(vm, (double)*(float *)result_ptr);
	} else if (rtype->kind == FFI_KIND_DOUBLE) {
		ring_vm_api_retnumber(vm, *(double *)result_ptr);
	} else if (rtype->kind == FFI_KIND_LONGDOUBLE) {
		ring_vm_api_retnumber(vm, (double)*(long double *)result_ptr);
	} else if (ffi_is_64bit_int(rtype->kind)) {
		char buf[32];
		if (rtype->kind == FFI_KIND_UINT64 || rtype->kind == FFI_KIND_ULONGLONG ||
			(rtype->kind == FFI_KIND_SIZE_T && sizeof(size_t) == 8) ||
			(rtype->kind == FFI_KIND_UINTPTR_T && sizeof(uintptr_t) == 8) ||
			(rtype->kind == FFI_KIND_ULONG && sizeof(unsigned long) == 8)) {
			snprintf(buf, sizeof(buf), "%llu", (unsigned long long)*(uint64_t *)result_ptr);
		} else {
			snprintf(buf, sizeof(buf), "%lld", (long long)*(int64_t *)result_ptr);
		}
		ring_vm_api_retstring(vm, buf);
	} else {
		ffi_arg res = *(ffi_arg *)result_ptr;
		switch (rtype->kind) {
		case FFI_KIND_INT8:
		case FFI_KIND_SCHAR:
		case FFI_KIND_CHAR:
			ring_vm_api_retnumber(vm, (double)(int8_t)res);
			break;
		case FFI_KIND_UINT8:
		case FFI_KIND_UCHAR:
		case FFI_KIND_BOOL:
			ring_vm_api_retnumber(vm, (double)(uint8_t)res);
			break;
		case FFI_KIND_INT16:
		case FFI_KIND_SHORT:
			ring_vm_api_retnumber(vm, (double)(int16_t)res);
			break;
		case FFI_KIND_UINT16:
		case FFI_KIND_USHORT:
			ring_vm_api_retnumber(vm, (double)(uint16_t)res);
			break;
		case FFI_KIND_INT32:
		case FFI_KIND_INT:
			ring_vm_api_retnumber(vm, (double)(int32_t)res);
			break;
		case FFI_KIND_UINT32:
		case FFI_KIND_UINT:
			ring_vm_api_retnumber(vm, (double)(uint32_t)res);
			break;
		case FFI_KIND_LONG:
			ring_vm_api_retnumber(vm, (double)(long)res);
			break;
		case FFI_KIND_ULONG:
			ring_vm_api_retnumber(vm, (double)(unsigned long)res);
			break;
		default:
			ring_vm_api_retnumber(vm, (double)(int)res);
			break;
		}
	}
}

static void ffi_ret_value(VM *vm, void *src, FFI_Type *type)
{
	if (type->kind == FFI_KIND_POINTER || type->pointer_depth > 0) {
		ring_vm_api_retcpointer(vm, *(void **)src, "FFI_Ptr");
	} else if (ffi_is_64bit_int(type->kind)) {
		char buf[32];
		if (type->kind == FFI_KIND_UINT64 || type->kind == FFI_KIND_ULONGLONG ||
			(type->kind == FFI_KIND_SIZE_T && sizeof(size_t) == 8) ||
			(type->kind == FFI_KIND_UINTPTR_T && sizeof(uintptr_t) == 8) ||
			(type->kind == FFI_KIND_ULONG && sizeof(unsigned long) == 8)) {
			snprintf(buf, sizeof(buf), "%llu", (unsigned long long)*(uint64_t *)src);
		} else {
			snprintf(buf, sizeof(buf), "%lld", (long long)*(int64_t *)src);
		}
		ring_vm_api_retstring(vm, buf);
	} else {
		ring_vm_api_retnumber(vm, ffi_read_typed_value(src, type));
	}
}

size_t ffi_sizeof(FFI_Type *type) { return type ? type->size : 0; }

static void ffi_gc_free_ptr(void *state, void *ptr)
{
	if (!ptr)
		return;
	ring_state_free((RingState *)state, ptr);
}

static void ffi_gc_free_lib(void *state, void *ptr)
{
	FFI_Library *lib = (FFI_Library *)ptr;
	if (!lib)
		return;
	if (lib->handle)
		FFI_CloseLib(lib->handle);
	if (lib->path)
		ring_state_free((RingState *)state, lib->path);
	ring_state_free((RingState *)state, lib);
}

static void ffi_gc_free_func(void *state, void *ptr)
{
	FFI_Function *func = (FFI_Function *)ptr;
	if (!func)
		return;
	if (func->type) {
		if (func->type->param_types)
			ring_state_free((RingState *)state, func->type->param_types);
		ring_state_free((RingState *)state, func->type);
	}
	if (func->ffi_arg_types)
		ring_state_free((RingState *)state, func->ffi_arg_types);
	ring_state_free((RingState *)state, func);
}

static void ffi_gc_free_callback(void *state, void *ptr)
{
	FFI_Callback *cb = (FFI_Callback *)ptr;
	if (!cb)
		return;

	if (cb->closure)
		ffi_closure_free(cb->closure);

	if (cb->type) {
		if (cb->type->param_types)
			ring_state_free((RingState *)state, cb->type->param_types);
		ring_state_free((RingState *)state, cb->type);
	}
	if (cb->ffi_arg_types)
		ring_state_free((RingState *)state, cb->ffi_arg_types);
	if (cb->ring_func_name)
		ring_state_free((RingState *)state, cb->ring_func_name);
	if (cb->call_buf)
		ring_state_free((RingState *)state, cb->call_buf);
	ring_state_free((RingState *)state, cb);
}

static void ffi_gc_free_type(void *state, void *ptr)
{
	FFI_Type *type = (FFI_Type *)ptr;
	if (!type)
		return;
	/* Don't free sub-structures (struct_type, etc.) as they may be shared */
	ring_state_free((RingState *)state, type);
}

static void ffi_gc_free_enum(void *state, void *ptr)
{
	FFI_EnumType *et = (FFI_EnumType *)ptr;
	if (!et)
		return;
	FFI_EnumConst *ec = et->constants;
	while (ec) {
		FFI_EnumConst *next = ec->next;
		if (ec->name)
			ring_state_free((RingState *)state, ec->name);
		ring_state_free((RingState *)state, ec);
		ec = next;
	}
	if (et->name)
		ring_state_free((RingState *)state, et->name);
	ring_state_free((RingState *)state, et);
}

static void ffi_gc_free_struct_type(void *state, void *ptr)
{
	FFI_StructType *st = (FFI_StructType *)ptr;
	if (!st)
		return;
	if (st->name)
		ring_state_free((RingState *)state, st->name);
	FFI_StructField *field = st->fields;
	while (field) {
		FFI_StructField *next = field->next;
		if (field->name)
			ring_state_free((RingState *)state, field->name);
		ring_state_free((RingState *)state, field);
		field = next;
	}
	if (st->ffi_elements)
		ring_state_free((RingState *)state, st->ffi_elements);
	ring_state_free((RingState *)state, st);
}

static void ffi_gc_free_union_type(void *state, void *ptr)
{
	FFI_UnionType *ut = (FFI_UnionType *)ptr;
	if (!ut)
		return;
	if (ut->name)
		ring_state_free((RingState *)state, ut->name);
	FFI_StructField *field = ut->fields;
	while (field) {
		FFI_StructField *next = field->next;
		if (field->name)
			ring_state_free((RingState *)state, field->name);
		ring_state_free((RingState *)state, field);
		field = next;
	}
	ring_state_free((RingState *)state, ut);
}

static void ffi_context_free(void *state, void *ptr)
{
	FFI_Context *ctx = (FFI_Context *)ptr;
	if (!ctx)
		return;
	RingState *pRingState = (RingState *)state;

	if (ctx->structs)
		ring_hashtable_delete_gc(pRingState, ctx->structs);
	if (ctx->unions)
		ring_hashtable_delete_gc(pRingState, ctx->unions);
	if (ctx->enums)
		ring_hashtable_delete_gc(pRingState, ctx->enums);
	if (ctx->type_cache)
		ring_hashtable_delete_gc(pRingState, ctx->type_cache);
	if (ctx->cdef_funcs)
		ring_hashtable_delete_gc(pRingState, ctx->cdef_funcs);

	if (ctx->gc_list)
		ring_list_delete_gc(pRingState, ctx->gc_list);

	ring_state_free(pRingState, ctx);
	if (g_ffi_ctx == ctx)
		g_ffi_ctx = NULL;
}

static FFI_Context *get_or_create_context(void *pPointer)
{
	VM *vm = (VM *)pPointer;
	if (g_ffi_ctx && g_ffi_ctx->vm == vm) {
		return g_ffi_ctx;
	}

	int root_scope_idx = vm->pRingState->lRunFromSubThread ? 2 : 1;
	List *rootScope = &(vm->aScopes[root_scope_idx]);
	int nSize = ring_list_getsize(rootScope);
	for (int i = 1; i <= nSize; i++) {
		List *pVar = ring_list_getlist(rootScope, i);
		const char *varName = ring_list_getstring(pVar, RING_VAR_NAME);
		if (strcmp(varName, "__cffi_ctx") == 0) {
			g_ffi_ctx = (FFI_Context *)ring_list_getpointer(pVar, RING_VAR_VALUE);
			g_ffi_ctx->vm = vm;
			return g_ffi_ctx;
		}
	}

	g_ffi_ctx = ffi_context_new(vm->pRingState, vm);
	if (!g_ffi_ctx)
		return NULL;

	List *pVar = ring_list_newlist_gc(vm->pRingState, rootScope);
	ring_list_addstring_gc(vm->pRingState, pVar, "__cffi_ctx");
	ring_list_addint_gc(vm->pRingState, pVar, RING_VM_POINTER);
	ring_list_addpointer_gc(vm->pRingState, pVar, g_ffi_ctx);
	ring_list_addint_gc(vm->pRingState, pVar, RING_OBJTYPE_NOTYPE);

	Item *pItem = ring_list_getitem_gc(vm->pRingState, pVar, RING_VAR_VALUE);
	ring_vm_gc_setfreefunc(pItem, ffi_context_free);

	return g_ffi_ctx;
}

static char *ffi_lowerdup(FFI_Context *ctx, const char *str)
{
	char *dup = (char *)ring_state_malloc(ctx->ring_state, strlen(str) + 1);
	if (!dup)
		return NULL;
	for (int i = 0; str[i]; i++)
		dup[i] = tolower((unsigned char)str[i]);
	dup[strlen(str)] = '\0';
	ring_list_addcustomringpointer_gc(ctx->ring_state, ctx->gc_list, dup, ffi_gc_free_ptr);
	return dup;
}

RING_FUNC(ring_cffi_load)
{
	if (RING_API_PARACOUNT != 1 || !RING_API_ISSTRING(1)) {
		RING_API_ERROR("ffi_load(path) expects a string argument");
		return;
	}

	FFI_Context *ctx = get_or_create_context(pPointer);
	const char *path = RING_API_GETSTRING(1);

	FFI_Library *lib = ffi_library_open(ctx, path);
	if (!lib) {
		RING_API_ERROR(ffi_get_error(ctx));
		return;
	}

	RING_API_RETMANAGEDCPOINTER(lib, "FFI_Library", ffi_gc_free_lib);
}

RING_FUNC(ring_cffi_sizeof)
{
	if (RING_API_PARACOUNT != 1 || !RING_API_ISSTRING(1)) {
		RING_API_ERROR("ffi_sizeof(type) expects a type name string");
		return;
	}

	FFI_Context *ctx = get_or_create_context(pPointer);
	const char *type_name = RING_API_GETSTRING(1);

	FFI_Type *type = ffi_type_parse(ctx, type_name);
	if (!type) {
		RING_API_ERROR("Unknown type");
		return;
	}

	RING_API_RETNUMBER((double)ffi_sizeof(type));
}

RING_FUNC(ring_cffi_new)
{
	if (RING_API_PARACOUNT < 1 || !RING_API_ISSTRING(1)) {
		RING_API_ERROR("ffi_new(type [, count]) expects type name");
		return;
	}

	FFI_Context *ctx = get_or_create_context(pPointer);
	const char *type_name = RING_API_GETSTRING(1);
	size_t count = 1;

	if (RING_API_PARACOUNT >= 2 && RING_API_ISNUMBER(2)) {
		count = (size_t)RING_API_GETNUMBER(2);
	}

	FFI_Type *type = ffi_type_parse(ctx, type_name);
	if (!type) {
		RING_API_ERROR("Unknown type");
		return;
	}

	void *ptr;
	if (count > 1) {
		ptr = ffi_alloc_array(ctx, type, count);
	} else {
		ptr = ffi_alloc(ctx, type);
	}

	if (!ptr) {
		RING_API_ERROR(ffi_get_error(ctx));
		return;
	}

	RING_API_RETMANAGEDCPOINTER(ptr, "FFI_Ptr", ffi_gc_free_ptr);
}

RING_FUNC(ring_cffi_nullptr) { RING_API_RETCPOINTER(NULL, "FFI_Ptr"); }

RING_FUNC(ring_cffi_isnull)
{
	if (RING_API_PARACOUNT != 1) {
		RING_API_ERROR("ffi_isnull(ptr) expects one argument");
		return;
	}

	if (RING_API_ISNUMBER(1) && RING_API_GETNUMBER(1) == 0) {
		RING_API_RETNUMBER(1);
		return;
	}

	if (!RING_API_ISCPOINTER(1)) {
		RING_API_RETNUMBER(1);
		return;
	}

	List *pList = RING_API_GETLIST(1);
	void *ptr = ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	RING_API_RETNUMBER(ptr == NULL ? 1 : 0);
}

RING_FUNC(ring_cffi_string)
{
	if (RING_API_PARACOUNT != 1 || !RING_API_ISSTRING(1)) {
		RING_API_ERROR("ffi_string(str) expects a string");
		return;
	}

	FFI_Context *ctx = get_or_create_context(pPointer);
	const char *str = RING_API_GETSTRING(1);

	char *copy = ffi_string_new(ctx, str);
	if (!copy) {
		RING_API_ERROR("Out of memory");
		return;
	}

	RING_API_RETMANAGEDCPOINTER(copy, "FFI_String", ffi_gc_free_ptr);
}

RING_FUNC(ring_cffi_tostring)
{
	if (RING_API_PARACOUNT != 1 || !RING_API_ISCPOINTER(1)) {
		RING_API_ERROR("ffi_tostring(ptr) expects a pointer");
		return;
	}

	List *pList = RING_API_GETLIST(1);
	char *ptr = (char *)ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	if (!ptr) {
		RING_API_RETSTRING("");
		return;
	}

	RING_API_RETSTRING(ptr);
}

RING_FUNC(ring_cffi_errno) { RING_API_RETNUMBER(errno); }

RING_FUNC(ring_cffi_strerror)
{
	int err = errno;
	if (RING_API_PARACOUNT >= 1 && RING_API_ISNUMBER(1)) {
		err = (int)RING_API_GETNUMBER(1);
	}
	RING_API_RETSTRING(strerror(err));
}

static FFI_TypeKind parse_type_kind(const char *name)
{
	if (strcmp(name, "void") == 0)
		return FFI_KIND_VOID;

	if (strcmp(name, "char") == 0)
		return FFI_KIND_CHAR;
	if (strcmp(name, "signed char") == 0 || strcmp(name, "schar") == 0)
		return FFI_KIND_SCHAR;
	if (strcmp(name, "unsigned char") == 0 || strcmp(name, "uchar") == 0)
		return FFI_KIND_UCHAR;

	if (strcmp(name, "short") == 0 || strcmp(name, "short int") == 0 ||
		strcmp(name, "signed short") == 0)
		return FFI_KIND_SHORT;
	if (strcmp(name, "unsigned short") == 0 || strcmp(name, "ushort") == 0 ||
		strcmp(name, "unsigned short int") == 0)
		return FFI_KIND_USHORT;

	if (strcmp(name, "int") == 0 || strcmp(name, "signed") == 0 || strcmp(name, "signed int") == 0)
		return FFI_KIND_INT;
	if (strcmp(name, "unsigned int") == 0 || strcmp(name, "unsigned") == 0 ||
		strcmp(name, "uint") == 0)
		return FFI_KIND_UINT;

	if (strcmp(name, "long") == 0 || strcmp(name, "long int") == 0 ||
		strcmp(name, "signed long") == 0)
		return FFI_KIND_LONG;
	if (strcmp(name, "unsigned long") == 0 || strcmp(name, "ulong") == 0 ||
		strcmp(name, "unsigned long int") == 0)
		return FFI_KIND_ULONG;

	if (strcmp(name, "long long") == 0 || strcmp(name, "long long int") == 0 ||
		strcmp(name, "signed long long") == 0)
		return FFI_KIND_LONGLONG;
	if (strcmp(name, "unsigned long long") == 0 || strcmp(name, "ulonglong") == 0 ||
		strcmp(name, "unsigned long long int") == 0)
		return FFI_KIND_ULONGLONG;

	if (strcmp(name, "float") == 0)
		return FFI_KIND_FLOAT;
	if (strcmp(name, "double") == 0)
		return FFI_KIND_DOUBLE;
	if (strcmp(name, "long double") == 0)
		return FFI_KIND_LONGDOUBLE;

	if (strcmp(name, "int8") == 0 || strcmp(name, "int8_t") == 0 || strcmp(name, "Sint8") == 0)
		return FFI_KIND_INT8;
	if (strcmp(name, "uint8") == 0 || strcmp(name, "uint8_t") == 0 || strcmp(name, "Uint8") == 0 ||
		strcmp(name, "byte") == 0)
		return FFI_KIND_UINT8;
	if (strcmp(name, "int16") == 0 || strcmp(name, "int16_t") == 0 || strcmp(name, "Sint16") == 0)
		return FFI_KIND_INT16;
	if (strcmp(name, "uint16") == 0 || strcmp(name, "uint16_t") == 0 || strcmp(name, "Uint16") == 0)
		return FFI_KIND_UINT16;
	if (strcmp(name, "int32") == 0 || strcmp(name, "int32_t") == 0 || strcmp(name, "Sint32") == 0)
		return FFI_KIND_INT32;
	if (strcmp(name, "uint32") == 0 || strcmp(name, "uint32_t") == 0 || strcmp(name, "Uint32") == 0)
		return FFI_KIND_UINT32;
	if (strcmp(name, "int64") == 0 || strcmp(name, "int64_t") == 0 || strcmp(name, "Sint64") == 0)
		return FFI_KIND_INT64;
	if (strcmp(name, "uint64") == 0 || strcmp(name, "uint64_t") == 0 || strcmp(name, "Uint64") == 0)
		return FFI_KIND_UINT64;

	if (strcmp(name, "size_t") == 0)
		return FFI_KIND_SIZE_T;
	if (strcmp(name, "ssize_t") == 0)
		return FFI_KIND_SSIZE_T;
	if (strcmp(name, "ptrdiff_t") == 0)
		return FFI_KIND_PTRDIFF_T;
	if (strcmp(name, "intptr_t") == 0)
		return FFI_KIND_INTPTR_T;
	if (strcmp(name, "uintptr_t") == 0)
		return FFI_KIND_UINTPTR_T;
	if (strcmp(name, "wchar_t") == 0)
		return FFI_KIND_WCHAR_T;
	if (strcmp(name, "bool") == 0 || strcmp(name, "_Bool") == 0)
		return FFI_KIND_BOOL;

	if (strcmp(name, "pointer") == 0 || strcmp(name, "void*") == 0 || strcmp(name, "ptr") == 0)
		return FFI_KIND_POINTER;
	if (strcmp(name, "char*") == 0 || strcmp(name, "string") == 0 || strcmp(name, "cstring") == 0)
		return FFI_KIND_POINTER;

	return FFI_KIND_UNKNOWN;
}

FFI_Type *ffi_type_parse(FFI_Context *ctx, const char *type_str)
{
	if (!ctx || !type_str)
		return NULL;

	/* Check the type cache first */
	if (ctx->type_cache) {
		FFI_Type *cached =
			(FFI_Type *)ring_hashtable_findpointer(ctx->type_cache, (char *)type_str);
		if (cached != NULL) {
			return cached;
		}
	}

	char buf[1024];
	strncpy(buf, type_str, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';

	char *p = buf;
	while (*p && isspace(*p))
		p++;

	while (strncmp(p, "const", 5) == 0 || strncmp(p, "volatile", 8) == 0) {
		if (strncmp(p, "const", 5) == 0 && !isalnum(p[5]) && p[5] != '_')
			p += 5;
		else if (strncmp(p, "volatile", 8) == 0 && !isalnum(p[8]) && p[8] != '_')
			p += 8;
		else
			break;
		while (*p && isspace(*p))
			p++;
	}

	int ptr_count = 0;
	char *end = p + strlen(p) - 1;
	while (end > p && (*end == '*' || isspace(*end))) {
		if (*end == '*')
			ptr_count++;
		end--;
	}
	*(end + 1) = '\0';

	end = p + strlen(p) - 1;
	while (end > p && isspace(*end))
		end--;
	*(end + 1) = '\0';

	FFI_TypeKind kind = parse_type_kind(p);
	if (kind == FFI_KIND_UNKNOWN) {
		ffi_set_error(ctx, "Unknown type: '%s'", type_str);
		return NULL;
	}
	FFI_Type *base = ffi_type_primitive(ctx, kind);

	FFI_Type *result = base;
	if (ptr_count > 0) {
		FFI_Type *ptr_type = base;
		for (int i = 0; i < ptr_count; i++) {
			ptr_type = ffi_type_ptr(ctx, ptr_type);
		}
		result = ptr_type;
	}

	/* Store in cache */
	if (ctx->type_cache && result) {
		ring_hashtable_newpointer_gc(ctx->ring_state, ctx->type_cache, type_str, result);
		ring_list_addcustomringpointer_gc(ctx->ring_state, ctx->gc_list, result, ffi_gc_free_type);
	}

	return result;
}

static FFI_Function *ffi_function_create(FFI_Context *ctx, void *func_ptr, FFI_Type *ret_type,
										 FFI_Type **param_types, int param_count)
{
	if (!func_ptr) {
		ffi_set_error(ctx, "Invalid function pointer");
		return NULL;
	}

	FFI_Function *func = (FFI_Function *)ring_state_malloc(ctx->ring_state, sizeof(FFI_Function));
	if (!func)
		return NULL;

	memset(func, 0, sizeof(FFI_Function));
	func->func_ptr = func_ptr;

	FFI_FuncType *ftype = (FFI_FuncType *)ring_state_malloc(ctx->ring_state, sizeof(FFI_FuncType));
	if (!ftype) {
		ring_state_free(ctx->ring_state, func);
		return NULL;
	}
	memset(ftype, 0, sizeof(FFI_FuncType));
	ftype->return_type = ret_type;
	ftype->param_count = param_count;

	if (param_count > 0) {
		ftype->param_types =
			(FFI_Type **)ring_state_malloc(ctx->ring_state, sizeof(FFI_Type *) * param_count);
		if (!ftype->param_types) {
			ring_state_free(ctx->ring_state, ftype);
			ring_state_free(ctx->ring_state, func);
			return NULL;
		}
		for (int i = 0; i < param_count; i++) {
			ftype->param_types[i] = param_types[i];
		}
	}
	func->type = ftype;

	ffi_type **arg_types = NULL;
	if (param_count > 0) {
		arg_types =
			(ffi_type **)ring_state_malloc(ctx->ring_state, sizeof(ffi_type *) * param_count);
		if (!arg_types) {
			if (ftype->param_types)
				ring_state_free(ctx->ring_state, ftype->param_types);
			ring_state_free(ctx->ring_state, ftype);
			ring_state_free(ctx->ring_state, func);
			return NULL;
		}
		for (int i = 0; i < param_count; i++) {
			arg_types[i] = param_types[i]->ffi_type_ptr;
		}
	}

	ffi_status status =
		ffi_prep_cif(&func->cif, FFI_DEFAULT_ABI, param_count, ret_type->ffi_type_ptr, arg_types);
	if (status != FFI_OK) {
		ffi_set_error(ctx, "Failed to prepare FFI call interface");
		if (arg_types)
			ring_state_free(ctx->ring_state, arg_types);
		if (ftype->param_types)
			ring_state_free(ctx->ring_state, ftype->param_types);
		ring_state_free(ctx->ring_state, ftype);
		ring_state_free(ctx->ring_state, func);
		return NULL;
	}
	func->cif_prepared = true;
	func->ffi_arg_types = arg_types;

	return func;
}

static FFI_Type **parse_type_list(FFI_Context *ctx, List *type_list, int *out_count)
{
	int count = ring_list_getsize(type_list);
	if (count == 0) {
		*out_count = 0;
		return NULL;
	}

	FFI_Type **types = (FFI_Type **)ring_state_malloc(ctx->ring_state, sizeof(FFI_Type *) * count);
	if (!types) {
		*out_count = -1;
		return NULL;
	}

	for (int i = 0; i < count; i++) {
		if (!ring_list_isstring(type_list, i + 1)) {
			ring_state_free(ctx->ring_state, types);
			*out_count = 0;
			return NULL;
		}
		types[i] = ffi_type_parse(ctx, ring_list_getstring(type_list, i + 1));
		if (!types[i]) {
			ring_state_free(ctx->ring_state, types);
			*out_count = 0;
			return NULL;
		}
	}

	*out_count = count;
	return types;
}

RING_FUNC(ring_cffi_func)
{
	if (RING_API_PARACOUNT < 3) {
		RING_API_ERROR("ffi_func(lib, name, rettype [, argtypes_list]) requires "
					   "at least 3 parameters");
		return;
	}

	if (!RING_API_ISCPOINTER(1)) {
		RING_API_ERROR("ffi_func: first parameter must be a library handle");
		return;
	}
	if (!RING_API_ISSTRING(2) || !RING_API_ISSTRING(3)) {
		RING_API_ERROR("ffi_func: name and return type must be strings");
		return;
	}

	FFI_Context *ctx = get_or_create_context(pPointer);
	List *pList = RING_API_GETLIST(1);
	FFI_Library *lib = (FFI_Library *)ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	if (!lib) {
		RING_API_ERROR("ffi_func: invalid library handle");
		return;
	}

	const char *func_name = RING_API_GETSTRING(2);
	const char *ret_type_str = RING_API_GETSTRING(3);

	FFI_Type *ret_type = ffi_type_parse(ctx, ret_type_str);
	if (!ret_type) {
		RING_API_ERROR("ffi_func: unknown return type");
		return;
	}

	int param_count = 0;
	FFI_Type **param_types = NULL;

	if (RING_API_PARACOUNT >= 4 && RING_API_ISLIST(4)) {
		List *argTypes = RING_API_GETLIST(4);
		param_types = parse_type_list(ctx, argTypes, &param_count);
		if (!param_types && param_count < 0) {
			RING_API_ERROR("ffi_func: parameter types must be valid strings");
			return;
		}
	}

	void *func_ptr = ffi_library_symbol(lib, func_name);
	if (!func_ptr) {
		ffi_set_error(ctx, "Symbol '%s' not found in library", func_name);
		RING_API_ERROR(ffi_get_error(ctx));
		if (param_types)
			ring_state_free(ctx->ring_state, param_types);
		return;
	}

	FFI_Function *func = ffi_function_create(ctx, func_ptr, ret_type, param_types, param_count);
	if (!func) {
		RING_API_ERROR(ffi_get_error(ctx));
		if (param_types)
			ring_state_free(ctx->ring_state, param_types);
		return;
	}

	if (param_types)
		ring_state_free(ctx->ring_state, param_types);

	RING_API_RETMANAGEDCPOINTER(func, "FFI_Function", ffi_gc_free_func);
}

RING_FUNC(ring_cffi_funcptr)
{
	if (RING_API_PARACOUNT < 2) {
		RING_API_ERROR("ffi_funcptr(ptr, rettype [, argtypes_list]) requires "
					   "at least 2 parameters");
		return;
	}

	if (!RING_API_ISCPOINTER(1)) {
		RING_API_ERROR("ffi_funcptr: first parameter must be a pointer");
		return;
	}

	FFI_Context *ctx = get_or_create_context(pPointer);
	List *pList = RING_API_GETLIST(1);
	void *ptr = ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	if (!ptr) {
		RING_API_ERROR("ffi_funcptr: NULL pointer");
		return;
	}

	const char *ptr_type = ring_list_getstring(pList, RING_CPOINTER_TYPE);
	int is_library = (ptr_type && strcmp(ptr_type, "FFI_Library") == 0);

	void *func_ptr = NULL;
	const char *ret_type_str = NULL;
	const char *func_name = "funcptr";
	int type_start_param;

	if (is_library && RING_API_PARACOUNT >= 3 && RING_API_ISSTRING(2) && RING_API_ISSTRING(3)) {
		FFI_Library *lib = (FFI_Library *)ptr;
		func_name = RING_API_GETSTRING(2);
		ret_type_str = RING_API_GETSTRING(3);
		type_start_param = 4;

		void *func_ptr_var = ffi_library_symbol(lib, func_name);
		if (!func_ptr_var) {
			char err[256];
			snprintf(err, sizeof(err), "ffi_funcptr: symbol '%s' not found", func_name);
			RING_API_ERROR(err);
			return;
		}
		func_ptr = func_ptr_var;
	} else if (RING_API_ISSTRING(2)) {
		func_ptr = ptr;
		ret_type_str = RING_API_GETSTRING(2);
		type_start_param = 3;
	} else {
		RING_API_ERROR("ffi_funcptr: invalid arguments");
		return;
	}

	FFI_Type *ret_type = ffi_type_parse(ctx, ret_type_str);
	if (!ret_type) {
		RING_API_ERROR("ffi_funcptr: unknown return type");
		return;
	}

	int param_count = 0;
	FFI_Type **param_types = NULL;

	if (RING_API_PARACOUNT >= type_start_param && RING_API_ISLIST(type_start_param)) {
		List *argTypes = RING_API_GETLIST(type_start_param);
		param_types = parse_type_list(ctx, argTypes, &param_count);
		if (!param_types && param_count < 0) {
			RING_API_ERROR("ffi_funcptr: parameter types must be valid strings");
			return;
		}
	}

	FFI_Function *func = ffi_function_create(ctx, func_ptr, ret_type, param_types, param_count);
	if (!func) {
		RING_API_ERROR("ffi_funcptr: failed to create function handle");
		if (param_types)
			ring_state_free(ctx->ring_state, param_types);
		return;
	}

	if (param_types)
		ring_state_free(ctx->ring_state, param_types);

	RING_API_RETMANAGEDCPOINTER(func, "FFI_Function", ffi_gc_free_func);
}

static int ffi_call_function(FFI_Context *ctx, VM *pVM, FFI_Function *func, List *aArgs,
							 int api_offset)
{
	int arg_count;
	if (aArgs)
		arg_count = ring_list_getsize(aArgs);
	else
		arg_count = ring_vm_api_paracount((void *)pVM) - api_offset + 1;

	if (arg_count != func->type->param_count) {
		char err[256];
		snprintf(err, sizeof(err), "expected %d arguments, got %d", func->type->param_count,
				 arg_count);
		ring_vm_error(pVM, err);
		return -1;
	}

	void **arg_values = NULL;
	void *arg_storage = NULL;

	if (arg_count > 0) {
		size_t storage_size = 0;
		for (int i = 0; i < arg_count; i++) {
			FFI_Type *ptype = func->type->param_types[i];
			storage_size = FFI_ALIGN(storage_size, ptype->alignment > 0 ? ptype->alignment : 1);
			if (ptype->kind == FFI_KIND_POINTER || ptype->pointer_depth > 0) {
				storage_size += sizeof(void *);
				storage_size = FFI_ALIGN(storage_size, 16);
			} else {
				storage_size += ptype->size > 0 ? ptype->size : sizeof(int);
			}
		}
		storage_size = FFI_ALIGN(storage_size, 16);

		arg_storage = ring_state_calloc(ctx->ring_state, 1, storage_size);
		arg_values = (void **)ring_state_malloc(ctx->ring_state, sizeof(void *) * arg_count);

		if (!arg_storage || !arg_values) {
			ring_vm_error(pVM, "out of memory");
			if (arg_storage)
				ring_state_free(ctx->ring_state, arg_storage);
			if (arg_values)
				ring_state_free(ctx->ring_state, arg_values);
			return -1;
		}

		size_t current_offset = 0;

		for (int i = 0; i < arg_count; i++) {
			FFI_Type *ptype = func->type->param_types[i];
			current_offset = FFI_ALIGN(current_offset, ptype->alignment > 0 ? ptype->alignment : 1);
			arg_values[i] = (char *)arg_storage + current_offset;
			char *storage_ptr = (char *)arg_values[i];

			int param_idx = aArgs ? 0 : (api_offset + i);

			if (ptype->kind == FFI_KIND_POINTER || ptype->pointer_depth > 0) {
				void *ptr_val = NULL;
				const char *ptr_type = NULL;
				if (aArgs) {
					if (ring_list_islist(aArgs, i + 1)) {
						List *argList = ring_list_getlist(aArgs, i + 1);
						ptr_val = ring_list_getpointer(argList, RING_CPOINTER_POINTER);
						ptr_type = ring_list_getstring(argList, RING_CPOINTER_TYPE);
					} else if (ring_list_isstring(aArgs, i + 1)) {
						ptr_val = (void *)ring_list_getstring(aArgs, i + 1);
					} else if (ring_list_isdouble(aArgs, i + 1) &&
							   ring_list_getdouble(aArgs, i + 1) == 0) {
						ptr_val = NULL;
					} else {
						ring_vm_error(pVM, "expected pointer argument");
						goto cleanup;
					}
				} else {
					if (ring_vm_api_iscpointer((void *)pVM, param_idx)) {
						List *argList = ring_vm_api_getlist((void *)pVM, param_idx);
						ptr_val = ring_list_getpointer(argList, RING_CPOINTER_POINTER);
						ptr_type = ring_list_getstring(argList, RING_CPOINTER_TYPE);
					} else if (ring_vm_api_isstring((void *)pVM, param_idx)) {
						ptr_val = (void *)ring_vm_api_getstring((void *)pVM, param_idx);
					} else if (ring_vm_api_isnumber((void *)pVM, param_idx) &&
							   ring_vm_api_getnumber((void *)pVM, param_idx) == 0) {
						ptr_val = NULL;
					} else {
						ring_vm_error(pVM, "expected pointer argument");
						goto cleanup;
					}
				}

				if (ptr_type && strcmp(ptr_type, "FFI_Callback") == 0 && ptr_val)
					ptr_val = ((FFI_Callback *)ptr_val)->code_ptr;

				*(void **)storage_ptr = ptr_val;
				current_offset += sizeof(void *);
				current_offset = FFI_ALIGN(current_offset, 16);
			} else if (aArgs ? ring_list_isdouble(aArgs, i + 1)
							 : ring_vm_api_isnumber((void *)pVM, param_idx)) {
				double val = aArgs ? ring_list_getdouble(aArgs, i + 1)
								   : ring_vm_api_getnumber((void *)pVM, param_idx);
				ffi_write_typed_value(storage_ptr, ptype, val);
				current_offset += ptype->size > 0 ? ptype->size : sizeof(int);
			} else if (aArgs ? ring_list_isstring(aArgs, i + 1)
							 : ring_vm_api_isstring((void *)pVM, param_idx)) {
				const char *str = aArgs ? ring_list_getstring(aArgs, i + 1)
										: ring_vm_api_getstring((void *)pVM, param_idx);
				if (ffi_is_64bit_int(ptype->kind)) {
					if (ptype->kind == FFI_KIND_UINT64 || ptype->kind == FFI_KIND_ULONGLONG ||
						(ptype->kind == FFI_KIND_SIZE_T && sizeof(size_t) == 8) ||
						(ptype->kind == FFI_KIND_UINTPTR_T && sizeof(uintptr_t) == 8) ||
						(ptype->kind == FFI_KIND_ULONG && sizeof(unsigned long) == 8)) {
						*(uint64_t *)storage_ptr = (uint64_t)strtoull(str, NULL, 10);
					} else {
						*(int64_t *)storage_ptr = (int64_t)strtoll(str, NULL, 10);
					}
					current_offset += ptype->size;
				} else {
					ring_vm_error(
						pVM, "type mismatch, string passed to non-pointer/non-64bit parameter");
					goto cleanup;
				}
			} else {
				ring_vm_error(pVM, "unsupported argument type");
				goto cleanup;
			}
		}
	}

	{
		union {
			ffi_arg u;
			int8_t i8;
			uint8_t u8;
			int16_t i16;
			uint16_t u16;
			int32_t i32;
			uint32_t u32;
			int64_t i64;
			uint64_t u64;
			float f;
			double d;
			long double ld;
			void *p;
		} result;

		memset(&result, 0, sizeof(result));
		ffi_call(&func->cif, FFI_FN(func->func_ptr), &result, arg_values);

		if (arg_storage)
			ring_state_free(ctx->ring_state, arg_storage);
		if (arg_values)
			ring_state_free(ctx->ring_state, arg_values);

		ffi_push_return_value(pVM, &result, func->type->return_type);
		return 0;
	}

cleanup:
	if (arg_storage)
		ring_state_free(ctx->ring_state, arg_storage);
	if (arg_values)
		ring_state_free(ctx->ring_state, arg_values);
	return -1;
}

RING_FUNC(ring_cffi_invoke)
{
	if (RING_API_PARACOUNT < 1) {
		RING_API_ERROR("ffi_invoke(func [, args...]) requires at least 1 parameter");
		return;
	}

	if (!RING_API_ISCPOINTER(1)) {
		RING_API_ERROR("ffi_invoke: first parameter must be a function handle");
		return;
	}

	FFI_Context *ctx = get_or_create_context(pPointer);
	List *pList = RING_API_GETLIST(1);
	FFI_Function *func = (FFI_Function *)ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	if (!func || !func->cif_prepared) {
		RING_API_ERROR("ffi_invoke: invalid function handle");
		return;
	}

	List *aArgs = NULL;
	if (RING_API_PARACOUNT >= 2 && RING_API_ISLIST(2) && !RING_API_ISCPOINTER(2)) {
		aArgs = RING_API_GETLIST(2);
	}

	ffi_call_function(ctx, (VM *)pPointer, func, aArgs, 2);
}

RING_FUNC(ring_cffi_sym)
{
	if (RING_API_PARACOUNT != 2) {
		RING_API_ERROR("ffi_sym(lib, name) expects 2 parameters");
		return;
	}

	if (!RING_API_ISCPOINTER(1) || !RING_API_ISSTRING(2)) {
		RING_API_ERROR("ffi_sym(lib, name): lib must be a library handle, name "
					   "must be a string");
		return;
	}

	List *pList = RING_API_GETLIST(1);
	FFI_Library *lib = (FFI_Library *)ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	if (!lib) {
		RING_API_ERROR("ffi_sym: invalid library handle");
		return;
	}

	const char *name = RING_API_GETSTRING(2);
	void *sym = ffi_library_symbol(lib, name);

	if (!sym) {
		RING_API_RETCPOINTER(NULL, "FFI_Ptr");
	} else {
		RING_API_RETCPOINTER(sym, "FFI_Ptr");
	}
}

RING_FUNC(ring_cffi_get)
{
	if (RING_API_PARACOUNT < 2) {
		RING_API_ERROR("ffi_get(ptr, type [, index]) requires at least 2 parameters");
		return;
	}

	if (!RING_API_ISCPOINTER(1) || !RING_API_ISSTRING(2)) {
		RING_API_ERROR("ffi_get: ptr must be a pointer, type must be a string");
		return;
	}

	FFI_Context *ctx = get_or_create_context(pPointer);
	List *pList = RING_API_GETLIST(1);
	void *ptr = ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	if (!ptr) {
		RING_API_ERROR("ffi_get: null pointer");
		return;
	}

	const char *type_str = RING_API_GETSTRING(2);
	FFI_Type *type = ffi_type_parse(ctx, type_str);
	if (!type) {
		RING_API_ERROR("ffi_get: unknown type");
		return;
	}

	size_t index = 0;
	if (RING_API_PARACOUNT >= 3 && RING_API_ISNUMBER(3)) {
		index = (size_t)RING_API_GETNUMBER(3);
	}

	void *elem_ptr = (char *)ptr + (index * type->size);

	if (type->kind == FFI_KIND_POINTER || type->pointer_depth > 0) {
		void *val = *(void **)elem_ptr;
		RING_API_RETCPOINTER(val, "FFI_Ptr");
	} else {
		ffi_ret_value((VM *)pPointer, elem_ptr, type);
	}
}

RING_FUNC(ring_cffi_set)
{
	if (RING_API_PARACOUNT < 3) {
		RING_API_ERROR("ffi_set(ptr, type, value [, index]) requires at least "
					   "3 parameters");
		return;
	}

	if (!RING_API_ISCPOINTER(1) || !RING_API_ISSTRING(2)) {
		RING_API_ERROR("ffi_set: ptr must be a pointer, type must be a string");
		return;
	}

	FFI_Context *ctx = get_or_create_context(pPointer);
	List *pList = RING_API_GETLIST(1);
	void *ptr = ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	if (!ptr) {
		RING_API_ERROR("ffi_set: null pointer");
		return;
	}

	const char *type_str = RING_API_GETSTRING(2);
	FFI_Type *type = ffi_type_parse(ctx, type_str);
	if (!type) {
		RING_API_ERROR("ffi_set: unknown type");
		return;
	}

	size_t index = 0;
	if (RING_API_PARACOUNT >= 4 && RING_API_ISNUMBER(4)) {
		index = (size_t)RING_API_GETNUMBER(4);
	}

	void *elem_ptr = (char *)ptr + (index * type->size);

	if (type->kind == FFI_KIND_POINTER || type->pointer_depth > 0) {
		void *val = NULL;
		if (RING_API_ISCPOINTER(3)) {
			List *valList = RING_API_GETLIST(3);
			val = ring_list_getpointer(valList, RING_CPOINTER_POINTER);
		} else if (RING_API_ISSTRING(3)) {
			const char *raw_str = RING_API_GETSTRING(3);
			val = (void *)ffi_string_new(ctx, raw_str);
			if (!val) {
				RING_API_ERROR("ffi_set: out of memory allocating string");
				return;
			}
			ring_list_addcustomringpointer_gc(ctx->ring_state, ctx->gc_list, val, ffi_gc_free_ptr);
		} else if (RING_API_ISNUMBER(3) && RING_API_GETNUMBER(3) == 0) {
			val = NULL;
		}
		*(void **)elem_ptr = val;
	} else if (RING_API_ISNUMBER(3)) {
		double val = RING_API_GETNUMBER(3);
		ffi_write_typed_value(elem_ptr, type, val);
	} else if (RING_API_ISSTRING(3) && ffi_is_64bit_int(type->kind)) {
		const char *str = RING_API_GETSTRING(3);
		if (type->kind == FFI_KIND_UINT64 || type->kind == FFI_KIND_ULONGLONG ||
			(type->kind == FFI_KIND_SIZE_T && sizeof(size_t) == 8) ||
			(type->kind == FFI_KIND_UINTPTR_T && sizeof(uintptr_t) == 8) ||
			(type->kind == FFI_KIND_ULONG && sizeof(unsigned long) == 8)) {
			*(uint64_t *)elem_ptr = (uint64_t)strtoull(str, NULL, 10);
		} else {
			*(int64_t *)elem_ptr = (int64_t)strtoll(str, NULL, 10);
		}
	} else {
		RING_API_ERROR("ffi_set: value type not supported");
		return;
	}
}

RING_FUNC(ring_cffi_get_i64)
{
	if (RING_API_PARACOUNT < 1 || !RING_API_ISCPOINTER(1)) {
		RING_API_ERROR("ffi_get_i64(ptr [, index]) expects a pointer");
		return;
	}

	List *pList = RING_API_GETLIST(1);
	int64_t *ptr = (int64_t *)ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	if (!ptr) {
		RING_API_ERROR("ffi_get_i64: null pointer");
		return;
	}

	size_t index = 0;
	if (RING_API_PARACOUNT >= 2 && RING_API_ISNUMBER(2)) {
		index = (size_t)RING_API_GETNUMBER(2);
	}

	char buf[64];
	snprintf(buf, sizeof(buf), "%lld", (long long)ptr[index]);
	RING_API_RETSTRING(buf);
}

RING_FUNC(ring_cffi_set_i64)
{
	if (RING_API_PARACOUNT < 2 || !RING_API_ISCPOINTER(1) || !RING_API_ISSTRING(2)) {
		RING_API_ERROR("ffi_set_i64(ptr, value_str [, index]) expects pointer and string");
		return;
	}

	List *pList = RING_API_GETLIST(1);
	int64_t *ptr = (int64_t *)ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	if (!ptr) {
		RING_API_ERROR("ffi_set_i64: null pointer");
		return;
	}

	const char *val_str = RING_API_GETSTRING(2);
	int64_t val = (int64_t)strtoll(val_str, NULL, 10);

	size_t index = 0;
	if (RING_API_PARACOUNT >= 3 && RING_API_ISNUMBER(3)) {
		index = (size_t)RING_API_GETNUMBER(3);
	}

	ptr[index] = val;
}

RING_FUNC(ring_cffi_deref)
{
	if (RING_API_PARACOUNT < 1) {
		RING_API_ERROR("ffi_deref(ptr [, type]) expects at least 1 parameter");
		return;
	}

	if (!RING_API_ISCPOINTER(1)) {
		RING_API_ERROR("ffi_deref: parameter must be a pointer");
		return;
	}

	FFI_Context *ctx = get_or_create_context(pPointer);
	List *pList = RING_API_GETLIST(1);
	void *ptr = ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	if (!ptr) {
		RING_API_RETCPOINTER(NULL, "FFI_Ptr");
		return;
	}

	if (RING_API_PARACOUNT >= 2 && RING_API_ISSTRING(2)) {
		const char *type_str = RING_API_GETSTRING(2);
		FFI_Type *type = ffi_type_parse(ctx, type_str);
		if (!type) {
			RING_API_ERROR("ffi_deref: unknown type");
			return;
		}

		if (type->kind == FFI_KIND_POINTER || type->pointer_depth > 0) {
			void *derefed = *(void **)ptr;
			RING_API_RETCPOINTER(derefed, "FFI_Ptr");
		} else {
			ffi_ret_value((VM *)pPointer, ptr, type);
		}
	} else {
		void *derefed = *(void **)ptr;
		RING_API_RETCPOINTER(derefed, "FFI_Ptr");
	}
}

RING_FUNC(ring_cffi_offset)
{
	if (RING_API_PARACOUNT < 2) {
		RING_API_ERROR("ffi_offset(ptr, bytes) expects 2 parameters");
		return;
	}

	if (!RING_API_ISCPOINTER(1) || !RING_API_ISNUMBER(2)) {
		RING_API_ERROR("ffi_offset: ptr must be a pointer, bytes must be a number");
		return;
	}

	List *pList = RING_API_GETLIST(1);
	void *ptr = ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	if (!ptr) {
		RING_API_RETCPOINTER(NULL, "FFI_Ptr");
		return;
	}

	ptrdiff_t offset = (ptrdiff_t)RING_API_GETNUMBER(2);
	void *new_ptr = ffi_offset(ptr, offset);
	RING_API_RETCPOINTER(new_ptr, "FFI_Ptr");
}

RING_FUNC(ring_cffi_struct)
{
	if (RING_API_PARACOUNT < 1 || !RING_API_ISSTRING(1)) {
		RING_API_ERROR("ffi_struct(name, [[fieldname, type], ...]) requires a name");
		return;
	}

	FFI_Context *ctx = get_or_create_context(pPointer);
	const char *name = RING_API_GETSTRING(1);

	FFI_StructType *st = ffi_struct_define(ctx, name);
	if (!st) {
		RING_API_ERROR("ffi_struct: failed to create struct");
		return;
	}

	if (RING_API_PARACOUNT >= 2 && RING_API_ISLIST(2)) {
		List *fields = RING_API_GETLIST(2);
		int field_count = ring_list_getsize(fields);

		for (int i = 1; i <= field_count; i++) {
			if (!ring_list_islist(fields, i))
				continue;

			List *field_def = ring_list_getlist(fields, i);
			if (ring_list_getsize(field_def) < 2)
				continue;

			const char *field_name = ring_list_getstring(field_def, 1);
			const char *field_type = ring_list_getstring(field_def, 2);

			FFI_Type *type = ffi_type_parse(ctx, field_type);
			if (type) {
				ffi_struct_add_field(ctx, st, field_name, type);
			}
		}
	}

	if (ffi_struct_finalize(ctx, st) != 0) {
		RING_API_ERROR("ffi_struct: failed to finalize struct");
		return;
	}

	FFI_Type *type = (FFI_Type *)ring_state_malloc(ctx->ring_state, sizeof(FFI_Type));
	if (!type) {
		RING_API_ERROR("ffi_struct: out of memory");
		return;
	}
	memset(type, 0, sizeof(FFI_Type));
	type->kind = FFI_KIND_STRUCT;
	type->info.struct_type = st;
	type->ffi_type_ptr = &st->ffi_type_def;
	type->size = st->size;
	type->alignment = st->alignment;

	RING_API_RETMANAGEDCPOINTER(type, "FFI_Type", ffi_gc_free_type);
}

RING_FUNC(ring_cffi_typeof)
{
	if (RING_API_PARACOUNT < 1 || !RING_API_ISSTRING(1)) {
		RING_API_ERROR("ffi_typeof(name) requires a type name string");
		return;
	}

	FFI_Context *ctx = get_or_create_context(pPointer);
	const char *name = RING_API_GETSTRING(1);

	FFI_StructType *st = (FFI_StructType *)ring_hashtable_findpointer(ctx->structs, (char *)name);
	if (st) {
		FFI_Type *type = (FFI_Type *)ring_state_malloc(ctx->ring_state, sizeof(FFI_Type));
		if (!type) {
			RING_API_ERROR("ffi_typeof: out of memory");
			return;
		}
		memset(type, 0, sizeof(FFI_Type));
		type->kind = FFI_KIND_STRUCT;
		type->info.struct_type = st;
		type->ffi_type_ptr = &st->ffi_type_def;
		type->size = st->size;
		type->alignment = st->alignment;
		RING_API_RETMANAGEDCPOINTER(type, "FFI_Type", ffi_gc_free_type);
		return;
	}

	FFI_UnionType *ut = (FFI_UnionType *)ring_hashtable_findpointer(ctx->unions, (char *)name);
	if (ut) {
		FFI_Type *type = (FFI_Type *)ring_state_malloc(ctx->ring_state, sizeof(FFI_Type));
		if (!type) {
			RING_API_ERROR("ffi_typeof: out of memory");
			return;
		}
		memset(type, 0, sizeof(FFI_Type));
		type->kind = FFI_KIND_UNION;
		type->info.union_type = ut;
		type->size = ut->size;
		type->alignment = ut->alignment;
		RING_API_RETMANAGEDCPOINTER(type, "FFI_Type", ffi_gc_free_type);
		return;
	}

	FFI_EnumType *et = (FFI_EnumType *)ring_hashtable_findpointer(ctx->enums, (char *)name);
	if (et) {
		FFI_Type *type = (FFI_Type *)ring_state_malloc(ctx->ring_state, sizeof(FFI_Type));
		if (!type) {
			RING_API_ERROR("ffi_typeof: out of memory");
			return;
		}
		memset(type, 0, sizeof(FFI_Type));
		type->kind = FFI_KIND_ENUM;
		type->info.enum_type = et;
		type->size = sizeof(int);
		type->alignment = sizeof(int);
		RING_API_RETMANAGEDCPOINTER(type, "FFI_Type", ffi_gc_free_type);
		return;
	}

	RING_API_ERROR("ffi_typeof: type not found");
}

static FFI_StructField *ffi_union_find_field(FFI_UnionType *ut, const char *name)
{
	FFI_StructField *field = ut->fields;
	while (field) {
		if (field->name && strcmp(field->name, name) == 0)
			return field;
		field = field->next;
	}
	return NULL;
}

static FFI_StructField *ffi_struct_find_field(FFI_StructType *st, const char *name)
{
	FFI_StructField *field = st->fields;
	while (field) {
		if (field->name && strcmp(field->name, name) == 0)
			return field;
		field = field->next;
	}
	return NULL;
}

RING_FUNC(ring_cffi_struct_new)
{
	if (RING_API_PARACOUNT < 1 || !RING_API_ISCPOINTER(1)) {
		RING_API_ERROR("ffi_struct_new(struct_type) requires a struct type");
		return;
	}

	FFI_Context *ctx = get_or_create_context(pPointer);
	List *pList = RING_API_GETLIST(1);
	FFI_Type *type = (FFI_Type *)ring_list_getpointer(pList, RING_CPOINTER_POINTER);

	if (!type || type->kind != FFI_KIND_STRUCT) {
		RING_API_ERROR("ffi_struct_new: parameter must be a struct type");
		return;
	}

	void *ptr = ffi_alloc(ctx, type);
	if (!ptr) {
		RING_API_ERROR(ffi_get_error(ctx));
		return;
	}

	RING_API_RETMANAGEDCPOINTER(ptr, "FFI_Struct", ffi_gc_free_ptr);
}

RING_FUNC(ring_cffi_field)
{
	if (RING_API_PARACOUNT < 3) {
		RING_API_ERROR("ffi_field(ptr, type, fieldname) requires 3 parameters");
		return;
	}

	if (!RING_API_ISCPOINTER(1) || !RING_API_ISCPOINTER(2) || !RING_API_ISSTRING(3)) {
		RING_API_ERROR("ffi_field: invalid parameters");
		return;
	}

	List *pList1 = RING_API_GETLIST(1);
	void *ptr = ring_list_getpointer(pList1, RING_CPOINTER_POINTER);
	if (!ptr) {
		RING_API_ERROR("ffi_field: null pointer");
		return;
	}

	List *pList2 = RING_API_GETLIST(2);
	FFI_Type *type = (FFI_Type *)ring_list_getpointer(pList2, RING_CPOINTER_POINTER);
	if (!type) {
		RING_API_ERROR("ffi_field: invalid type");
		return;
	}

	const char *field_name = RING_API_GETSTRING(3);

	if (type->kind == FFI_KIND_STRUCT) {
		FFI_StructType *st = type->info.struct_type;
		FFI_StructField *field = ffi_struct_find_field(st, field_name);
		if (field) {
			void *field_ptr = (char *)ptr + field->offset;
			RING_API_RETCPOINTER(field_ptr, "FFI_Ptr");
			return;
		}
		RING_API_ERROR("ffi_field: field not found in struct");
	} else if (type->kind == FFI_KIND_UNION) {
		FFI_UnionType *ut = type->info.union_type;
		FFI_StructField *ufield = ffi_union_find_field(ut, field_name);
		if (ufield) {
			RING_API_RETCPOINTER(ptr, "FFI_Ptr");
			return;
		}
		RING_API_ERROR("ffi_field: field not found in union");
	} else {
		RING_API_ERROR("ffi_field: second parameter must be a struct or union type");
	}
}

RING_FUNC(ring_cffi_field_offset)
{
	if (RING_API_PARACOUNT < 2) {
		RING_API_ERROR("ffi_field_offset(struct_type, fieldname) requires 2 parameters");
		return;
	}

	if (!RING_API_ISCPOINTER(1) || !RING_API_ISSTRING(2)) {
		RING_API_ERROR("ffi_field_offset: invalid parameters");
		return;
	}

	List *pList = RING_API_GETLIST(1);
	FFI_Type *type = (FFI_Type *)ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	if (!type || type->kind != FFI_KIND_STRUCT) {
		RING_API_ERROR("ffi_field_offset: first parameter must be a struct type");
		return;
	}

	const char *field_name = RING_API_GETSTRING(2);
	FFI_StructType *st = type->info.struct_type;

	FFI_StructField *field = ffi_struct_find_field(st, field_name);
	if (field) {
		RING_API_RETNUMBER((double)field->offset);
		return;
	}

	RING_API_RETNUMBER(-1);
}

RING_FUNC(ring_cffi_struct_size)
{
	if (RING_API_PARACOUNT < 1 || !RING_API_ISCPOINTER(1)) {
		RING_API_ERROR("ffi_struct_size(struct_type) requires 1 parameter");
		return;
	}

	List *pList = RING_API_GETLIST(1);
	FFI_Type *type = (FFI_Type *)ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	if (!type || type->kind != FFI_KIND_STRUCT) {
		RING_API_ERROR("ffi_struct_size: parameter must be a struct type");
		return;
	}

	RING_API_RETNUMBER((double)type->size);
}

static void ffi_callback_handler(ffi_cif *cif, void *ret, void **args, void *user_data)
{
	FFI_Callback *cb = (FFI_Callback *)user_data;
	if (!cb || !cb->vm || !cb->ring_func_name)
		return;

	FFI_Context *old_ctx = g_ffi_ctx;
	g_ffi_ctx = cb->ctx;

	VM *vm = cb->vm;
	FFI_FuncType *ftype = cb->type;
	RingState *state = vm->pRingState;

	/* Push arguments to a reentrant stack list */
	List *cb_args_stack = ring_state_findvar(state, "__cffi_cb_args");
	if (!cb_args_stack) {
		cb_args_stack = ring_state_newvar(state, "__cffi_cb_args");
		ring_list_setint_gc(state, cb_args_stack, RING_VAR_TYPE, RING_VM_LIST);
		ring_list_setlist_gc(state, cb_args_stack, RING_VAR_VALUE);
	}
	List *stack_list = ring_list_getlist(cb_args_stack, RING_VAR_VALUE);
	List *current_args = ring_list_newlist_gc(state, stack_list);

	for (int i = 0; i < cif->nargs; i++) {
		FFI_Type *ptype = ftype->param_types[i];

		if (ptype->kind == FFI_KIND_POINTER || ptype->pointer_depth > 0) {
			void *val = *(void **)args[i];
			List *ptr_list = ring_list_newlist_gc(state, current_args);
			ring_list_addpointer_gc(state, ptr_list, val);
			ring_list_addstring_gc(state, ptr_list, "FFI_Ptr");
			ring_list_addint_gc(state, ptr_list, 0);
		} else if (ffi_is_64bit_int(ptype->kind)) {
			char buf[32];
			if (ptype->kind == FFI_KIND_UINT64 || ptype->kind == FFI_KIND_ULONGLONG ||
				(ptype->kind == FFI_KIND_SIZE_T && sizeof(size_t) == 8) ||
				(ptype->kind == FFI_KIND_UINTPTR_T && sizeof(uintptr_t) == 8) ||
				(ptype->kind == FFI_KIND_ULONG && sizeof(unsigned long) == 8)) {
				snprintf(buf, sizeof(buf), "%llu", (unsigned long long)*(uint64_t *)args[i]);
			} else {
				snprintf(buf, sizeof(buf), "%lld", (long long)*(int64_t *)args[i]);
			}
			ring_list_addstring_gc(state, current_args, buf);
		} else {
			double dval = ffi_read_typed_value(args[i], ptype);
			ring_list_adddouble_gc(state, current_args, dval);
		}
	}

	ring_vm_runcode(vm, cb->call_buf);

	if (ftype->return_type->kind != FFI_KIND_VOID) {
		List *cb_res_stack = ring_state_findvar(state, "__cffi_cb_res");
		if (cb_res_stack && ring_list_islist(cb_res_stack, RING_VAR_VALUE)) {
			List *res_list = ring_list_getlist(cb_res_stack, RING_VAR_VALUE);
			int res_idx = ring_list_getsize(stack_list); /* The index we just used */

			if (res_idx > 0 && res_idx <= ring_list_getsize(res_list)) {
				FFI_Type *rtype = ftype->return_type;
				if (rtype->kind == FFI_KIND_POINTER || rtype->pointer_depth > 0) {
					if (ring_list_islist(res_list, res_idx)) {
						List *ptr_list = ring_list_getlist(res_list, res_idx);
						*(void **)ret = ring_list_getpointer(ptr_list, 1);
					} else if (ring_list_ispointer(res_list, res_idx)) {
						*(void **)ret = ring_list_getpointer(res_list, res_idx);
					} else {
						*(void **)ret = NULL;
					}
				} else if (ffi_is_64bit_int(rtype->kind) && ring_list_isstring(res_list, res_idx)) {
					const char *str = ring_list_getstring(res_list, res_idx);
					if (rtype->kind == FFI_KIND_UINT64 || rtype->kind == FFI_KIND_ULONGLONG ||
						(rtype->kind == FFI_KIND_SIZE_T && sizeof(size_t) == 8) ||
						(rtype->kind == FFI_KIND_UINTPTR_T && sizeof(uintptr_t) == 8) ||
						(rtype->kind == FFI_KIND_ULONG && sizeof(unsigned long) == 8)) {
						*(uint64_t *)ret = (uint64_t)strtoull(str, NULL, 10);
					} else {
						*(int64_t *)ret = (int64_t)strtoll(str, NULL, 10);
					}
				} else if (ring_list_isdouble(res_list, res_idx)) {
					double val = ring_list_getdouble(res_list, res_idx);
					ffi_write_typed_value(ret, rtype, val);
				}
			}
		}
	}

	/* Pop the stack */
	ring_list_deleteitem_gc(state, stack_list, ring_list_getsize(stack_list));

	g_ffi_ctx = old_ctx;
}

RING_FUNC(ring_cffi_callback)
{
	if (RING_API_PARACOUNT < 3) {
		RING_API_ERROR(
			"ffi_callback(ring_func, rettype, [argtypes...]) requires at least 3 parameters");
		return;
	}

	if (!RING_API_ISSTRING(1) || !RING_API_ISSTRING(2)) {
		RING_API_ERROR("ffi_callback: function name and return type must be strings");
		return;
	}

	FFI_Context *ctx = get_or_create_context(pPointer);
	const char *func_name = RING_API_GETSTRING(1);
	const char *ret_type_str = RING_API_GETSTRING(2);

	FFI_Type *ret_type = ffi_type_parse(ctx, ret_type_str);
	if (!ret_type) {
		RING_API_ERROR("ffi_callback: unknown return type");
		return;
	}

	int param_count = 0;
	FFI_Type **param_types = NULL;
	ffi_type **arg_types = NULL;

	if (RING_API_PARACOUNT >= 3 && RING_API_ISLIST(3)) {
		List *argTypes = RING_API_GETLIST(3);
		param_types = parse_type_list(ctx, argTypes, &param_count);
		if (!param_types && param_count < 0) {
			RING_API_ERROR("ffi_callback: parameter types must be valid strings");
			return;
		}
		if (param_count > 0) {
			arg_types =
				(ffi_type **)ring_state_malloc(ctx->ring_state, sizeof(ffi_type *) * param_count);
			for (int i = 0; i < param_count; i++) {
				arg_types[i] = param_types[i]->ffi_type_ptr;
			}
		}
	}

	FFI_Callback *cb = (FFI_Callback *)ring_state_malloc(ctx->ring_state, sizeof(FFI_Callback));
	if (!cb) {
		if (arg_types)
			ring_state_free(ctx->ring_state, arg_types);
		if (param_types)
			ring_state_free(ctx->ring_state, param_types);
		RING_API_ERROR("ffi_callback: out of memory");
		return;
	}
	memset(cb, 0, sizeof(FFI_Callback));
	cb->ctx = ctx;
	cb->vm = (VM *)pPointer;
	cb->ring_func_name = ring_state_malloc(ctx->ring_state, strlen(func_name) + 1);
	if (cb->ring_func_name) {
		strcpy(cb->ring_func_name, func_name);
	}

	FFI_FuncType *ftype = (FFI_FuncType *)ring_state_malloc(ctx->ring_state, sizeof(FFI_FuncType));
	if (!ftype) {
		if (cb->ring_func_name)
			ring_state_free(ctx->ring_state, cb->ring_func_name);
		ring_state_free(ctx->ring_state, cb);
		if (arg_types)
			ring_state_free(ctx->ring_state, arg_types);
		if (param_types)
			ring_state_free(ctx->ring_state, param_types);
		RING_API_ERROR("ffi_callback: out of memory");
		return;
	}
	memset(ftype, 0, sizeof(FFI_FuncType));
	ftype->return_type = ret_type;
	ftype->param_types = param_types;
	ftype->param_count = param_count;
	cb->type = ftype;

	cb->closure = ffi_closure_alloc(sizeof(ffi_closure), &cb->code_ptr);
	if (!cb->closure) {
		ring_state_free(ctx->ring_state, ftype);
		if (cb->ring_func_name)
			ring_state_free(ctx->ring_state, cb->ring_func_name);
		ring_state_free(ctx->ring_state, cb);
		if (arg_types)
			ring_state_free(ctx->ring_state, arg_types);
		if (param_types)
			ring_state_free(ctx->ring_state, param_types);
		RING_API_ERROR("ffi_callback: failed to allocate closure");
		return;
	}

	ffi_status status =
		ffi_prep_cif(&cb->cif, FFI_DEFAULT_ABI, param_count, ret_type->ffi_type_ptr, arg_types);
	if (status != FFI_OK) {
		ffi_closure_free(cb->closure);
		ring_state_free(ctx->ring_state, ftype);
		if (cb->ring_func_name)
			ring_state_free(ctx->ring_state, cb->ring_func_name);
		ring_state_free(ctx->ring_state, cb);
		if (arg_types)
			ring_state_free(ctx->ring_state, arg_types);
		if (param_types)
			ring_state_free(ctx->ring_state, param_types);
		RING_API_ERROR("ffi_callback: failed to prepare cif");
		return;
	}

	status = ffi_prep_closure_loc(cb->closure, &cb->cif, ffi_callback_handler, cb, cb->code_ptr);
	if (status != FFI_OK) {
		ffi_closure_free(cb->closure);
		ring_state_free(ctx->ring_state, ftype);
		if (cb->ring_func_name)
			ring_state_free(ctx->ring_state, cb->ring_func_name);
		ring_state_free(ctx->ring_state, cb);
		if (arg_types)
			ring_state_free(ctx->ring_state, arg_types);
		if (param_types)
			ring_state_free(ctx->ring_state, param_types);
		RING_API_ERROR("ffi_callback: failed to prepare closure");
		return;
	}

	cb->ffi_arg_types = arg_types;

	/* Build cached reentrant call string */
	char tmp_buf[4096];
	char *tp = tmp_buf;
	size_t trem = sizeof(tmp_buf);
	int tn;

	tn = snprintf(tp, trem,
				  "if isNULL('__cffi_cb_res') __cffi_cb_res = [] ok\n"
				  "__cb_idx = len(__cffi_cb_args)\n"
				  "if len(__cffi_cb_res) < __cb_idx add(__cffi_cb_res, 0) ok\n");
	tp += tn;
	trem -= tn;

	if (ret_type->kind != FFI_KIND_VOID) {
		tn = snprintf(tp, trem, "__cffi_cb_res[__cb_idx] = ");
		tp += tn;
		trem -= tn;
	}

	tn = snprintf(tp, trem, "%s(", func_name);
	tp += tn;
	trem -= tn;

	for (int i = 0; i < param_count; i++) {
		if (i > 0) {
			tn = snprintf(tp, trem, ", ");
			tp += tn;
			trem -= tn;
		}
		tn = snprintf(tp, trem, "__cffi_cb_args[__cb_idx][%d]", i + 1);
		tp += tn;
		trem -= tn;
	}

	snprintf(tp, trem, ")");

	size_t call_len = (size_t)(tp - tmp_buf) + 1;
	cb->call_buf = ring_state_malloc(ctx->ring_state, call_len);
	if (cb->call_buf) {
		memcpy(cb->call_buf, tmp_buf, call_len);
	}

	RING_API_RETMANAGEDCPOINTER(cb, "FFI_Callback", ffi_gc_free_callback);
}

RING_FUNC(ring_cffi_enum)
{
	if (RING_API_PARACOUNT < 1 || !RING_API_ISSTRING(1)) {
		RING_API_ERROR("ffi_enum(name, [[constname, value], ...]) requires a name");
		return;
	}

	FFI_Context *ctx = get_or_create_context(pPointer);
	const char *name = RING_API_GETSTRING(1);

	FFI_EnumType *et = (FFI_EnumType *)ring_state_malloc(ctx->ring_state, sizeof(FFI_EnumType));
	if (!et) {
		RING_API_ERROR("ffi_enum: out of memory");
		return;
	}
	memset(et, 0, sizeof(FFI_EnumType));
	ring_list_addcustomringpointer_gc(ctx->ring_state, ctx->gc_list, et, ffi_gc_free_enum);

	et->name = ring_state_malloc(ctx->ring_state, strlen(name) + 1);
	if (et->name)
		strcpy(et->name, name);

	if (RING_API_PARACOUNT >= 2 && RING_API_ISLIST(2)) {
		List *consts = RING_API_GETLIST(2);
		int const_count = ring_list_getsize(consts);
		int64_t auto_value = 0;

		for (int i = 1; i <= const_count; i++) {
			if (!ring_list_islist(consts, i))
				continue;

			List *const_def = ring_list_getlist(consts, i);
			int def_size = ring_list_getsize(const_def);
			if (def_size < 1)
				continue;

			const char *const_name = ring_list_getstring(const_def, 1);
			int64_t value = auto_value;

			if (def_size >= 2 && ring_list_isdouble(const_def, 2)) {
				value = (int64_t)ring_list_getdouble(const_def, 2);
			}

			FFI_EnumConst *ec =
				(FFI_EnumConst *)ring_state_malloc(ctx->ring_state, sizeof(FFI_EnumConst));
			if (ec) {
				memset(ec, 0, sizeof(FFI_EnumConst));
				ec->name = ring_state_malloc(ctx->ring_state, strlen(const_name) + 1);
				if (ec->name)
					strcpy(ec->name, const_name);
				ec->value = value;

				if (!et->constants) {
					et->constants = ec;
				} else {
					FFI_EnumConst *last = et->constants;
					while (last->next)
						last = last->next;
					last->next = ec;
				}
				et->const_count++;
			}

			auto_value = value + 1;
		}
	}

	ring_hashtable_newpointer_gc(ctx->ring_state, ctx->enums, name, et);

	RING_API_RETCPOINTER(et, "FFI_Enum");
}

RING_FUNC(ring_cffi_enum_value)
{
	if (RING_API_PARACOUNT < 2) {
		RING_API_ERROR("ffi_enum_value(enum, name) requires 2 parameters");
		return;
	}

	if (!RING_API_ISCPOINTER(1) || !RING_API_ISSTRING(2)) {
		RING_API_ERROR("ffi_enum_value: enum must be an enum type, name must be a string");
		return;
	}

	List *pList = RING_API_GETLIST(1);
	void *ptr = ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	if (!ptr) {
		RING_API_ERROR("ffi_enum_value: invalid enum type");
		return;
	}

	FFI_EnumType *et = NULL;
	const char *ctype = ring_list_getstring(pList, RING_CPOINTER_TYPE);
	if (ctype && strcmp(ctype, "FFI_Type") == 0) {
		FFI_Type *type = (FFI_Type *)ptr;
		if (type->kind == FFI_KIND_ENUM) {
			et = type->info.enum_type;
		}
	} else if (ctype && strcmp(ctype, "FFI_Enum") == 0) {
		et = (FFI_EnumType *)ptr;
	}

	if (!et) {
		RING_API_ERROR("ffi_enum_value: parameter is not an enum type");
		return;
	}

	const char *name = RING_API_GETSTRING(2);

	FFI_EnumConst *ec = et->constants;
	while (ec) {
		if (ec->name && strcmp(ec->name, name) == 0) {
			RING_API_RETNUMBER((double)ec->value);
			return;
		}
		ec = ec->next;
	}

	RING_API_RETNUMBER(-1);
}

RING_FUNC(ring_cffi_union)
{
	if (RING_API_PARACOUNT < 1 || !RING_API_ISSTRING(1)) {
		RING_API_ERROR("ffi_union(name, [[fieldname, type], ...]) requires a name");
		return;
	}

	FFI_Context *ctx = get_or_create_context(pPointer);
	const char *name = RING_API_GETSTRING(1);

	FFI_UnionType *ut = (FFI_UnionType *)ring_state_malloc(ctx->ring_state, sizeof(FFI_UnionType));
	if (!ut) {
		RING_API_ERROR("ffi_union: out of memory");
		return;
	}
	memset(ut, 0, sizeof(FFI_UnionType));
	ring_list_addcustomringpointer_gc(ctx->ring_state, ctx->gc_list, ut, ffi_gc_free_union_type);

	ut->name = (char *)ring_state_malloc(ctx->ring_state, strlen(name) + 1);
	if (!ut->name) {
		RING_API_ERROR("ffi_union: out of memory");
		return;
	}
	strcpy(ut->name, name);

	size_t max_size = 0;
	size_t max_align = 1;

	if (RING_API_PARACOUNT >= 2 && RING_API_ISLIST(2)) {
		List *fields = RING_API_GETLIST(2);
		int field_count = ring_list_getsize(fields);

		for (int i = 1; i <= field_count; i++) {
			if (!ring_list_islist(fields, i))
				continue;

			List *field_def = ring_list_getlist(fields, i);
			if (ring_list_getsize(field_def) < 2)
				continue;

			const char *field_name = ring_list_getstring(field_def, 1);
			const char *field_type = ring_list_getstring(field_def, 2);

			FFI_Type *type = ffi_type_parse(ctx, field_type);
			if (type) {
				FFI_StructField *field =
					(FFI_StructField *)ring_state_malloc(ctx->ring_state, sizeof(FFI_StructField));
				if (field) {
					memset(field, 0, sizeof(FFI_StructField));
					field->name = ring_state_malloc(ctx->ring_state, strlen(field_name) + 1);
					if (field->name)
						strcpy(field->name, field_name);
					field->type = type;
					field->offset = 0;
					field->size = type->size;

					if (!ut->fields) {
						ut->fields = field;
					} else {
						FFI_StructField *last = ut->fields;
						while (last->next)
							last = last->next;
						last->next = field;
					}
					ut->field_count++;

					if (type->size > max_size)
						max_size = type->size;
					if (type->alignment > max_align)
						max_align = type->alignment;
				}
			}
		}
	}

	ut->size = FFI_ALIGN(max_size, max_align);
	ut->alignment = max_align;

	ring_hashtable_newpointer_gc(ctx->ring_state, ctx->unions, name, ut);

	FFI_Type *type = (FFI_Type *)ring_state_malloc(ctx->ring_state, sizeof(FFI_Type));
	if (!type) {
		RING_API_ERROR("ffi_union: out of memory");
		return;
	}
	memset(type, 0, sizeof(FFI_Type));
	type->kind = FFI_KIND_UNION;
	type->info.union_type = ut;
	type->size = ut->size;
	type->alignment = ut->alignment;

	RING_API_RETMANAGEDCPOINTER(type, "FFI_Type", ffi_gc_free_type);
}

RING_FUNC(ring_cffi_union_new)
{
	if (RING_API_PARACOUNT < 1 || !RING_API_ISCPOINTER(1)) {
		RING_API_ERROR("ffi_union_new(union_type) requires a union type");
		return;
	}

	FFI_Context *ctx = get_or_create_context(pPointer);
	List *pList = RING_API_GETLIST(1);
	FFI_Type *type = (FFI_Type *)ring_list_getpointer(pList, RING_CPOINTER_POINTER);

	if (!type || type->kind != FFI_KIND_UNION) {
		RING_API_ERROR("ffi_union_new: parameter must be a union type");
		return;
	}

	void *ptr = ffi_alloc(ctx, type);
	if (!ptr) {
		RING_API_ERROR(ffi_get_error(ctx));
		return;
	}

	RING_API_RETMANAGEDCPOINTER(ptr, "FFI_Union", ffi_gc_free_ptr);
}

RING_FUNC(ring_cffi_union_size)
{
	if (RING_API_PARACOUNT < 1 || !RING_API_ISCPOINTER(1)) {
		RING_API_ERROR("ffi_union_size(union_type) requires 1 parameter");
		return;
	}

	List *pList = RING_API_GETLIST(1);
	FFI_Type *type = (FFI_Type *)ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	if (!type || type->kind != FFI_KIND_UNION) {
		RING_API_ERROR("ffi_union_size: parameter must be a union type");
		return;
	}

	RING_API_RETNUMBER((double)type->size);
}

static void cparser_init(CParser *p, FFI_Context *ctx, FFI_Library *lib, const char *src)
{
	p->ctx = ctx;
	p->lib = lib;
	p->src = ring_state_malloc(ctx->ring_state, strlen(src) + 1);
	if (p->src)
		strcpy(p->src, src);
	p->pos = p->src;
	p->error[0] = '\0';
	p->result_list = ring_list_new_gc(ctx->ring_state, 0);
	p->decl_count = 0;
}

static void cparser_free(CParser *p)
{
	if (p->src)
		ring_state_free(p->ctx->ring_state, p->src);
	if (p->result_list)
		ring_list_delete_gc(p->ctx->ring_state, p->result_list);
}

static void cparser_skip_ws(CParser *p)
{
	while (*p->pos) {
		if (isspace(*p->pos)) {
			p->pos++;
		} else if (p->pos[0] == '/' && p->pos[1] == '/') {
			while (*p->pos && *p->pos != '\n')
				p->pos++;
		} else if (p->pos[0] == '/' && p->pos[1] == '*') {
			p->pos += 2;
			while (*p->pos && !(p->pos[0] == '*' && p->pos[1] == '/'))
				p->pos++;
			if (*p->pos)
				p->pos += 2;
		} else {
			break;
		}
	}
}

static bool cparser_match(CParser *p, const char *s)
{
	cparser_skip_ws(p);
	size_t len = strlen(s);
	if (strncmp(p->pos, s, len) == 0 && !isalnum(p->pos[len]) && p->pos[len] != '_') {
		p->pos += len;
		return true;
	}
	return false;
}

static bool cparser_match_exact(CParser *p, const char *s)
{
	cparser_skip_ws(p);
	size_t len = strlen(s);
	if (strncmp(p->pos, s, len) == 0) {
		p->pos += len;
		return true;
	}
	return false;
}

static bool cparser_peek(CParser *p, const char *s)
{
	cparser_skip_ws(p);
	size_t len = strlen(s);
	return strncmp(p->pos, s, len) == 0;
}

static bool cparser_ident(CParser *p, char *out, size_t sz)
{
	cparser_skip_ws(p);
	if (!isalpha(*p->pos) && *p->pos != '_')
		return false;
	size_t i = 0;
	while ((isalnum(*p->pos) || *p->pos == '_') && i < sz - 1) {
		out[i++] = *p->pos++;
	}
	out[i] = '\0';
	if (isalnum(*p->pos) || *p->pos == '_') {
		snprintf(p->error, sizeof(p->error), "Identifier too long (max %zu)", sz - 1);
		return false;
	}
	return true;
}

static bool cparser_number(CParser *p, int64_t *val)
{
	cparser_skip_ws(p);
	if (!isdigit(*p->pos) && *p->pos != '-' && *p->pos != '+')
		return false;
	char *end;
	*val = strtoll(p->pos, &end, 0);
	if (end == p->pos)
		return false;
	p->pos = end;
	return true;
}

static void cparser_skip_attributes(CParser *p)
{
	while (true) {
		cparser_skip_ws(p);

		if (cparser_match(p, "const") || cparser_match(p, "volatile") ||
			cparser_match(p, "restrict") || cparser_match(p, "__restrict") ||
			cparser_match(p, "extern") || cparser_match(p, "static") ||
			cparser_match(p, "inline") || cparser_match(p, "__inline") ||
			cparser_match(p, "__inline__") || cparser_match(p, "__forceinline") ||
			cparser_match(p, "__stdcall") || cparser_match(p, "_stdcall") ||
			cparser_match(p, "__cdecl") || cparser_match(p, "_cdecl") ||
			cparser_match(p, "__fastcall") || cparser_match(p, "_fastcall") ||
			cparser_match(p, "__thiscall") || cparser_match(p, "_thiscall") ||
			cparser_match(p, "__ptr32") || cparser_match(p, "__ptr64") ||
			cparser_match(p, "WINAPI") || cparser_match(p, "APIENTRY") ||
			cparser_match(p, "CALLBACK") || cparser_match(p, "WINAPIV")) {
			continue;
		}

		if (cparser_match(p, "__declspec") || cparser_match(p, "__attribute__")) {
			cparser_skip_ws(p);
			if (*p->pos == '(') {
				int depth = 0;
				do {
					if (*p->pos == '(')
						depth++;
					else if (*p->pos == ')')
						depth--;
					else if (*p->pos == '\0')
						break;
					p->pos++;
				} while (depth > 0);
			}
			continue;
		}
		break;
	}
}

static void cparser_type(CParser *p, char *out, size_t sz)
{
	cparser_skip_attributes(p);
	size_t i = 0;
	out[0] = '\0';

	bool has_signed = false, has_unsigned = false;
	int long_count = 0;
	bool has_short = false;
	bool has_struct = false, has_union = false, has_enum = false;
	char base_type[512] = "";

	while (true) {
		cparser_skip_ws(p);
		if (cparser_match(p, "signed")) {
			has_signed = true;
			continue;
		}
		if (cparser_match(p, "unsigned")) {
			has_unsigned = true;
			continue;
		}
		if (cparser_match(p, "long")) {
			long_count++;
			continue;
		}
		if (cparser_match(p, "short")) {
			has_short = true;
			continue;
		}
		if (cparser_match(p, "struct")) {
			has_struct = true;
			continue;
		}
		if (cparser_match(p, "union")) {
			has_union = true;
			continue;
		}
		if (cparser_match(p, "enum")) {
			has_enum = true;
			continue;
		}

		char *save = p->pos;
		cparser_skip_attributes(p);
		if (save != p->pos)
			continue;

		break;
	}

	cparser_skip_attributes(p);

	bool has_modifier = has_signed || has_unsigned || long_count > 0 || has_short;
	bool has_tag = has_struct || has_union || has_enum;

	char peek_ident[512] = "";
	char *save_pos = p->pos;
	if (isalpha(*p->pos) || *p->pos == '_') {
		cparser_ident(p, peek_ident, sizeof(peek_ident));
	}

	bool consume_ident = false;
	if (has_tag) {
		consume_ident = true;
	} else if (has_modifier) {
		if (strcmp(peek_ident, "int") == 0 || strcmp(peek_ident, "char") == 0 ||
			strcmp(peek_ident, "double") == 0) {
			consume_ident = true;
		}
	} else {
		consume_ident = true;
	}

	if (consume_ident) {
		strcpy(base_type, peek_ident);
	} else {
		p->pos = save_pos;
	}

	if (has_enum) {
		snprintf(out, sz, "int");
	} else if (long_count >= 2) {
		snprintf(out, sz, has_unsigned ? "unsigned long long" : "long long");
	} else if (long_count == 1 && strcmp(base_type, "double") == 0) {
		snprintf(out, sz, "long double");
	} else if (long_count == 1) {
		snprintf(out, sz, has_unsigned ? "unsigned long" : "long");
	} else if (has_short) {
		snprintf(out, sz, has_unsigned ? "unsigned short" : "short");
	} else if (base_type[0]) {
		if (has_unsigned && (strcmp(base_type, "char") == 0 || strcmp(base_type, "int") == 0)) {
			snprintf(out, sz, "unsigned %s", base_type);
		} else if (has_signed && strcmp(base_type, "char") == 0) {
			snprintf(out, sz, "signed char");
		} else {
			strncpy(out, base_type, sz - 1);
			out[sz - 1] = '\0';
		}
	} else if (has_unsigned) {
		snprintf(out, sz, "unsigned int");
	} else if (has_signed) {
		snprintf(out, sz, "int");
	}

	cparser_skip_attributes(p);
	i = strlen(out);
	while (*p->pos == '*') {
		if (i < sz - 1) {
			out[i++] = '*';
		} else {
			snprintf(p->error, sizeof(p->error), "Type too long (pointer depth too high)");
			break;
		}
		p->pos++;
		cparser_skip_attributes(p);
	}
	out[i] = '\0';
}

static bool cparser_parse_struct(CParser *p, bool is_union)
{
	char name[512] = "";
	cparser_skip_ws(p);
	if (isalpha(*p->pos) || *p->pos == '_') {
		cparser_ident(p, name, sizeof(name));
	}

	cparser_skip_ws(p);
	if (!cparser_match_exact(p, "{")) {
		return false;
	}

	List *fields = ring_list_new_gc(p->ctx->ring_state, 0);

	while (!cparser_peek(p, "}")) {
		cparser_skip_ws(p);
		if (*p->pos == '\0')
			break;

		char field_type[512] = "";
		cparser_type(p, field_type, sizeof(field_type));
		if (!field_type[0])
			break;

		char field_name[512] = "";
		cparser_skip_ws(p);

		if (*p->pos == '(') {
			p->pos++;
			cparser_skip_attributes(p);
			cparser_skip_ws(p);
			while (*p->pos == '*')
				p->pos++;
			cparser_ident(p, field_name, sizeof(field_name));
			while (*p->pos && *p->pos != ')')
				p->pos++;
			if (*p->pos == ')')
				p->pos++;
			while (*p->pos && *p->pos != ';')
				p->pos++;
			if (*p->pos == ';')
				p->pos++;
			if (strlen(field_type) < sizeof(field_type) - 2)
				strcat(field_type, "*");
		} else {
			cparser_ident(p, field_name, sizeof(field_name));

			cparser_skip_ws(p);
			while (*p->pos == '[') {
				while (*p->pos && *p->pos != ']')
					p->pos++;
				if (*p->pos == ']')
					p->pos++;
				if (strlen(field_type) < sizeof(field_type) - 2)
					strcat(field_type, "*");
			}

			cparser_skip_ws(p);
			if (*p->pos == ':') {
				p->pos++;
				int64_t bits;
				cparser_number(p, &bits);
			}

			cparser_skip_ws(p);
			if (*p->pos == ';')
				p->pos++;
		}

		if (field_name[0] && field_type[0]) {
			List *field = ring_list_newlist_gc(p->ctx->ring_state, fields);
			ring_list_addstring_gc(p->ctx->ring_state, field, field_name);
			ring_list_addstring_gc(p->ctx->ring_state, field, field_type);
		}
	}

	cparser_match_exact(p, "}");
	cparser_skip_ws(p);

	if (!name[0] && (isalpha(*p->pos) || *p->pos == '_')) {
		cparser_ident(p, name, sizeof(name));
	}

	cparser_match_exact(p, ";");

	if (name[0]) {
		if (is_union) {
			FFI_UnionType *ut =
				(FFI_UnionType *)ring_state_malloc(p->ctx->ring_state, sizeof(FFI_UnionType));
			memset(ut, 0, sizeof(FFI_UnionType));
			ring_list_addcustomringpointer_gc(p->ctx->ring_state, p->ctx->gc_list, ut,
											  ffi_gc_free_union_type);
			ut->name = ring_state_malloc(p->ctx->ring_state, strlen(name) + 1);
			if (ut->name)
				strcpy(ut->name, name);

			int fcount = ring_list_getsize(fields);
			for (int i = 1; i <= fcount; i++) {
				List *fdef = ring_list_getlist(fields, i);
				const char *fname = ring_list_getstring(fdef, 1);
				const char *ftype = ring_list_getstring(fdef, 2);
				FFI_Type *type = ffi_type_parse(p->ctx, ftype);
				if (type) {
					ffi_union_add_field(p->ctx, ut, fname, type);
				}
			}
			ut->size = FFI_ALIGN(ut->size, ut->alignment);
			ring_hashtable_newpointer_gc(p->ctx->ring_state, p->ctx->unions, name, ut);
			ring_list_addpointer_gc(p->ctx->ring_state, p->result_list, ut);
			p->decl_count++;
		} else {
			FFI_StructType *st = ffi_struct_define(p->ctx, name);
			int fcount = ring_list_getsize(fields);
			for (int i = 1; i <= fcount; i++) {
				List *fdef = ring_list_getlist(fields, i);
				const char *fname = ring_list_getstring(fdef, 1);
				const char *ftype = ring_list_getstring(fdef, 2);
				FFI_Type *type = ffi_type_parse(p->ctx, ftype);
				if (type) {
					ffi_struct_add_field(p->ctx, st, fname, type);
				}
			}
			ffi_struct_finalize(p->ctx, st);
			ring_list_addpointer_gc(p->ctx->ring_state, p->result_list, st);
			p->decl_count++;
		}
	}

	return true;
}

static bool cparser_parse_enum(CParser *p)
{
	char name[512] = "";
	cparser_skip_ws(p);
	if (isalpha(*p->pos) || *p->pos == '_') {
		cparser_ident(p, name, sizeof(name));
	}

	cparser_skip_ws(p);
	if (!cparser_match_exact(p, "{"))
		return false;

	List *consts = ring_list_new_gc(p->ctx->ring_state, 0);
	int64_t next_val = 0;

	while (!cparser_peek(p, "}")) {
		cparser_skip_ws(p);
		if (*p->pos == '\0')
			break;

		char const_name[512];
		if (!cparser_ident(p, const_name, sizeof(const_name)))
			break;

		int64_t val = next_val;
		cparser_skip_ws(p);
		if (cparser_match_exact(p, "=")) {
			cparser_number(p, &val);
		}

		List *c = ring_list_newlist_gc(p->ctx->ring_state, consts);
		ring_list_addstring_gc(p->ctx->ring_state, c, const_name);
		ring_list_adddouble_gc(p->ctx->ring_state, c, (double)val);

		next_val = val + 1;
		cparser_skip_ws(p);
		cparser_match_exact(p, ",");
	}

	cparser_match_exact(p, "}");
	cparser_skip_ws(p);

	if (!name[0] && (isalpha(*p->pos) || *p->pos == '_')) {
		cparser_ident(p, name, sizeof(name));
	}

	cparser_match_exact(p, ";");

	if (name[0]) {
		FFI_EnumType *et =
			(FFI_EnumType *)ring_state_malloc(p->ctx->ring_state, sizeof(FFI_EnumType));
		memset(et, 0, sizeof(FFI_EnumType));
		ring_list_addcustomringpointer_gc(p->ctx->ring_state, p->ctx->gc_list, et,
										  ffi_gc_free_enum);
		et->name = ring_state_malloc(p->ctx->ring_state, strlen(name) + 1);
		if (et->name)
			strcpy(et->name, name);

		int ccount = ring_list_getsize(consts);
		for (int i = 1; i <= ccount; i++) {
			List *cdef = ring_list_getlist(consts, i);
			const char *cname = ring_list_getstring(cdef, 1);
			int64_t cval = (int64_t)ring_list_getdouble(cdef, 2);

			FFI_EnumConst *ec =
				(FFI_EnumConst *)ring_state_malloc(p->ctx->ring_state, sizeof(FFI_EnumConst));
			memset(ec, 0, sizeof(FFI_EnumConst));
			ec->name = ring_state_malloc(p->ctx->ring_state, strlen(cname) + 1);
			if (ec->name)
				strcpy(ec->name, cname);
			ec->value = cval;

			if (!et->constants) {
				et->constants = ec;
			} else {
				FFI_EnumConst *last = et->constants;
				while (last->next)
					last = last->next;
				last->next = ec;
			}
			et->const_count++;
		}

		ring_hashtable_newpointer_gc(p->ctx->ring_state, p->ctx->enums, name, et);
		ring_list_addpointer_gc(p->ctx->ring_state, p->result_list, et);
		p->decl_count++;
	}

	return true;
}

static bool cparser_parse_typedef(CParser *p)
{
	char *save = p->pos;

	if (cparser_match(p, "struct")) {
		if (cparser_peek(p, "{") || (isalpha(*p->pos) || *p->pos == '_')) {
			char sname[512] = "";
			if (!cparser_peek(p, "{")) {
				cparser_ident(p, sname, sizeof(sname));
			}
			if (cparser_peek(p, "{")) {
				p->pos = save;
				cparser_match(p, "struct");
				cparser_parse_struct(p, false);
				return true;
			}
		}
	}

	if (cparser_match(p, "union")) {
		if (cparser_peek(p, "{") || (isalpha(*p->pos) || *p->pos == '_')) {
			char uname[512] = "";
			if (!cparser_peek(p, "{")) {
				cparser_ident(p, uname, sizeof(uname));
			}
			if (cparser_peek(p, "{")) {
				p->pos = save;
				cparser_match(p, "union");
				cparser_parse_struct(p, true);
				return true;
			}
		}
	}

	if (cparser_match(p, "enum")) {
		if (cparser_peek(p, "{") || (isalpha(*p->pos) || *p->pos == '_')) {
			char ename[512] = "";
			if (!cparser_peek(p, "{")) {
				cparser_ident(p, ename, sizeof(ename));
			}
			if (cparser_peek(p, "{")) {
				p->pos = save;
				cparser_match(p, "enum");
				cparser_parse_enum(p);
				return true;
			}
		}
	}

	p->pos = save;

	char base_type[512];
	cparser_type(p, base_type, sizeof(base_type));
	if (!base_type[0])
		return false;

	cparser_skip_ws(p);

	if (*p->pos == '(') {
		p->pos++;
		cparser_skip_attributes(p);
		while (*p->pos == '*') {
			if (strlen(base_type) < sizeof(base_type) - 2)
				strcat(base_type, "*");
			p->pos++;
			cparser_skip_attributes(p);
		}
		char alias[512];
		cparser_ident(p, alias, sizeof(alias));
		while (*p->pos && *p->pos != ')')
			p->pos++;
		if (*p->pos == ')')
			p->pos++;
		while (*p->pos && *p->pos != ';')
			p->pos++;
		if (*p->pos == ';')
			p->pos++;

		FFI_Type *t = ffi_type_parse(p->ctx, base_type);
		if (t) {
			ring_hashtable_newpointer_gc(p->ctx->ring_state, p->ctx->type_cache, alias, t);
		}
		p->decl_count++;
		return true;
	}

	char alias[512];
	if (!cparser_ident(p, alias, sizeof(alias)))
		return false;

	cparser_skip_ws(p);
	while (*p->pos == '[') {
		while (*p->pos && *p->pos != ']')
			p->pos++;
		if (*p->pos == ']')
			p->pos++;
		if (strlen(base_type) < sizeof(base_type) - 2)
			strcat(base_type, "*");
	}

	cparser_match_exact(p, ";");

	FFI_Type *t = ffi_type_parse(p->ctx, base_type);
	if (t) {
		ring_hashtable_newpointer_gc(p->ctx->ring_state, p->ctx->type_cache, alias, t);
	}
	p->decl_count++;

	return true;
}

static bool cparser_parse_function(CParser *p)
{
	cparser_skip_attributes(p);

	char ret_type[512];
	cparser_type(p, ret_type, sizeof(ret_type));
	if (!ret_type[0])
		return false;

	cparser_skip_attributes(p);

	if (*p->pos == '(') {
		p->pos++;
		cparser_skip_attributes(p);
		while (*p->pos == '*') {
			if (strlen(ret_type) < sizeof(ret_type) - 2)
				strcat(ret_type, "*");
			p->pos++;
			cparser_skip_attributes(p);
		}
		char func_name[512];
		cparser_ident(p, func_name, sizeof(func_name));
		while (*p->pos && *p->pos != ')')
			p->pos++;
		if (*p->pos == ')')
			p->pos++;
		cparser_skip_ws(p);
	}

	char func_name[512];
	if (!cparser_ident(p, func_name, sizeof(func_name))) {
		return false;
	}

	cparser_skip_ws(p);
	if (*p->pos != '(')
		return false;
	p->pos++;

	FFI_Type *ret_ffi = ffi_type_parse(p->ctx, ret_type);
	if (!ret_ffi)
		return false;

	FFI_Type *params[64];
	int param_count = 0;
	bool is_variadic = false;

	cparser_skip_ws(p);
	if (cparser_match(p, "void")) {
		cparser_skip_ws(p);
		if (*p->pos == ')') {
			p->pos++;
			goto finish_func;
		}
		p->pos -= 4;
	}

	while (*p->pos && *p->pos != ')') {
		cparser_skip_ws(p);

		if (cparser_match_exact(p, "...")) {
			is_variadic = true;
			break;
		}

		char ptype[512];
		cparser_type(p, ptype, sizeof(ptype));

		cparser_skip_ws(p);
		if (*p->pos == '(') {
			p->pos++;
			cparser_skip_attributes(p);
			while (*p->pos == '*')
				p->pos++;
			char pname[512];
			cparser_ident(p, pname, sizeof(pname));
			while (*p->pos && *p->pos != ')')
				p->pos++;
			if (*p->pos == ')')
				p->pos++;
			cparser_skip_ws(p);
			if (*p->pos == '(') {
				while (*p->pos && *p->pos != ')')
					p->pos++;
				if (*p->pos == ')')
					p->pos++;
			}
			strcpy(ptype, "void*");
		} else if (isalpha(*p->pos) || *p->pos == '_') {
			char pname[512];
			cparser_ident(p, pname, sizeof(pname));
		}

		cparser_skip_ws(p);
		while (*p->pos == '[') {
			while (*p->pos && *p->pos != ']')
				p->pos++;
			if (*p->pos == ']')
				p->pos++;
			if (strlen(ptype) < sizeof(ptype) - 2)
				strcat(ptype, "*");
		}

		if (ptype[0] && param_count < 64) {
			params[param_count] = ffi_type_parse(p->ctx, ptype);
			if (params[param_count])
				param_count++;
		}

		cparser_skip_ws(p);
		cparser_match_exact(p, ",");
	}

	cparser_match_exact(p, ")");

finish_func:
	cparser_skip_ws(p);
	cparser_match_exact(p, ";");

	if (!p->lib) {
		p->decl_count++;
		return true;
	}

	void *fptr = ffi_library_symbol(p->lib, func_name);
	if (!fptr) {
		p->decl_count++;
		return true;
	}

	if (is_variadic) {
		FFI_Function *func =
			(FFI_Function *)ring_state_malloc(p->ctx->ring_state, sizeof(FFI_Function));
		memset(func, 0, sizeof(FFI_Function));
		func->func_ptr = fptr;

		FFI_FuncType *ft =
			(FFI_FuncType *)ring_state_malloc(p->ctx->ring_state, sizeof(FFI_FuncType));
		memset(ft, 0, sizeof(FFI_FuncType));
		ft->return_type = ret_ffi;
		ft->param_count = param_count;
		ft->is_variadic = true;
		if (param_count > 0) {
			ft->param_types = (FFI_Type **)ring_state_malloc(p->ctx->ring_state,
															 sizeof(FFI_Type *) * param_count);
			for (int i = 0; i < param_count; i++)
				ft->param_types[i] = params[i];
		}
		func->type = ft;

		ring_list_addpointer_gc(p->ctx->ring_state, p->result_list, func);
		ring_list_addcustomringpointer_gc(p->ctx->ring_state, p->ctx->gc_list, func,
										  ffi_gc_free_func);
		p->decl_count++;
	} else {
		FFI_Type **pcopy = NULL;
		if (param_count > 0) {
			pcopy = (FFI_Type **)ring_state_malloc(p->ctx->ring_state,
												   sizeof(FFI_Type *) * param_count);
			for (int i = 0; i < param_count; i++)
				pcopy[i] = params[i];
		}
		FFI_Function *func = ffi_function_create(p->ctx, fptr, ret_ffi, pcopy, param_count);
		if (func) {
			ring_list_addpointer_gc(p->ctx->ring_state, p->result_list, func);
			ring_list_addcustomringpointer_gc(p->ctx->ring_state, p->ctx->gc_list, func,
											  ffi_gc_free_func);
			for (int i = 0; func_name[i]; i++)
				func_name[i] = tolower((unsigned char)func_name[i]);
			ring_hashtable_newpointer_gc(p->ctx->ring_state, p->ctx->cdef_funcs, func_name, func);
			p->decl_count++;
		} else {
			p->decl_count++;
			if (pcopy)
				ring_state_free(p->ctx->ring_state, pcopy);
		}
	}

	return true;
}

static void cparser_parse(CParser *p)
{
	while (*p->pos) {
		cparser_skip_attributes(p);
		if (!*p->pos)
			break;

		char *save = p->pos;

		if (cparser_match(p, "typedef")) {
			if (!cparser_parse_typedef(p)) {
				while (*p->pos && *p->pos != ';')
					p->pos++;
				if (*p->pos == ';')
					p->pos++;
			}
			continue;
		}

		if (cparser_match(p, "struct")) {
			if (cparser_peek(p, "{") || (isalpha(*p->pos) || *p->pos == '_')) {
				char sname[512] = "";
				char *before_name = p->pos;
				if (!cparser_peek(p, "{")) {
					cparser_ident(p, sname, sizeof(sname));
				}
				if (cparser_peek(p, "{")) {
					p->pos = before_name;
					cparser_parse_struct(p, false);
					continue;
				}
			}
			p->pos = save;
		}

		if (cparser_match(p, "union")) {
			if (cparser_peek(p, "{") || (isalpha(*p->pos) || *p->pos == '_')) {
				char uname[512] = "";
				char *before_name = p->pos;
				if (!cparser_peek(p, "{")) {
					cparser_ident(p, uname, sizeof(uname));
				}
				if (cparser_peek(p, "{")) {
					p->pos = before_name;
					cparser_parse_struct(p, true);
					continue;
				}
			}
			p->pos = save;
		}

		if (cparser_match(p, "enum")) {
			if (cparser_peek(p, "{") || (isalpha(*p->pos) || *p->pos == '_')) {
				char ename[512] = "";
				char *before_name = p->pos;
				if (!cparser_peek(p, "{")) {
					cparser_ident(p, ename, sizeof(ename));
				}
				if (cparser_peek(p, "{")) {
					p->pos = before_name;
					cparser_parse_enum(p);
					continue;
				}
			}
			p->pos = save;
		}

		p->pos = save;
		if (!cparser_parse_function(p)) {
			while (*p->pos && *p->pos != ';' && *p->pos != '}')
				p->pos++;
			if (*p->pos == ';')
				p->pos++;
			if (*p->pos == '}')
				p->pos++;
		}
	}
}

RING_FUNC(ring_cffi_cdef)
{
	if (RING_API_PARACOUNT < 2) {
		RING_API_ERROR("ffi_cdef(lib, declarations) requires 2 parameters");
		return;
	}

	if (!RING_API_ISSTRING(2)) {
		RING_API_ERROR("ffi_cdef: declarations must be a string");
		return;
	}

	FFI_Context *ctx = get_or_create_context(pPointer);
	FFI_Library *lib = NULL;

	if (RING_API_ISCPOINTER(1)) {
		List *pList = RING_API_GETLIST(1);
		lib = (FFI_Library *)ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	}

	const char *decl = RING_API_GETSTRING(2);

	CParser parser;
	cparser_init(&parser, ctx, lib, decl);
	cparser_parse(&parser);

	int count = parser.decl_count;

	cparser_free(&parser);

	RING_API_RETNUMBER(count);
}

RING_FUNC(ring_cffi_varfunc)
{
	if (RING_API_PARACOUNT < 3) {
		RING_API_ERROR("ffi_varfunc(lib, name, rettype [, argtypes_list]) "
					   "requires at least 3 parameters");
		return;
	}

	if (!RING_API_ISCPOINTER(1)) {
		RING_API_ERROR("ffi_varfunc: first parameter must be a library handle");
		return;
	}
	if (!RING_API_ISSTRING(2) || !RING_API_ISSTRING(3)) {
		RING_API_ERROR("ffi_varfunc: name and return type must be strings");
		return;
	}

	FFI_Context *ctx = get_or_create_context(pPointer);
	List *pList = RING_API_GETLIST(1);
	FFI_Library *lib = (FFI_Library *)ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	if (!lib) {
		RING_API_ERROR("ffi_varfunc: invalid library handle");
		return;
	}

	const char *func_name = RING_API_GETSTRING(2);
	const char *ret_type_str = RING_API_GETSTRING(3);

	void *func_ptr = ffi_library_symbol(lib, func_name);
	if (!func_ptr) {
		ffi_set_error(ctx, "Symbol '%s' not found in library", func_name);
		RING_API_ERROR(ffi_get_error(ctx));
		return;
	}

	FFI_Type *ret_type = ffi_type_parse(ctx, ret_type_str);
	if (!ret_type) {
		RING_API_ERROR("ffi_varfunc: unknown return type");
		return;
	}

	int param_count = 0;
	FFI_Type **param_types = NULL;

	if (RING_API_PARACOUNT >= 4 && RING_API_ISLIST(4)) {
		List *argTypes = RING_API_GETLIST(4);
		param_types = parse_type_list(ctx, argTypes, &param_count);
		if (!param_types && param_count < 0) {
			RING_API_ERROR("ffi_varfunc: parameter types must be valid strings");
			return;
		}
	}

	FFI_Function *func = (FFI_Function *)ring_state_malloc(ctx->ring_state, sizeof(FFI_Function));
	if (!func) {
		if (param_types)
			ring_state_free(ctx->ring_state, param_types);
		RING_API_ERROR("ffi_varfunc: out of memory");
		return;
	}
	memset(func, 0, sizeof(FFI_Function));

	func->func_ptr = func_ptr;

	FFI_FuncType *ftype = (FFI_FuncType *)ring_state_malloc(ctx->ring_state, sizeof(FFI_FuncType));
	if (!ftype) {
		ring_state_free(ctx->ring_state, func);
		if (param_types)
			ring_state_free(ctx->ring_state, param_types);
		RING_API_ERROR("ffi_varfunc: out of memory");
		return;
	}
	memset(ftype, 0, sizeof(FFI_FuncType));
	ftype->return_type = ret_type;
	ftype->param_types = param_types;
	ftype->param_count = param_count;
	ftype->is_variadic = true;
	func->type = ftype;
	func->cif_prepared = false;

	RING_API_RETMANAGEDCPOINTER(func, "FFI_VarFunction", ffi_gc_free_func);
}

RING_FUNC(ring_cffi_varcall)
{
	if (RING_API_PARACOUNT < 1) {
		RING_API_ERROR("ffi_varcall(func, [args...]) requires at least 1 parameter");
		return;
	}

	if (!RING_API_ISCPOINTER(1)) {
		RING_API_ERROR("ffi_varcall: first parameter must be a variadic function handle");
		return;
	}

	FFI_Context *ctx = get_or_create_context(pPointer);
	List *pList = RING_API_GETLIST(1);
	FFI_Function *func = (FFI_Function *)ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	if (!func || !func->type || !func->type->is_variadic) {
		RING_API_ERROR("ffi_varcall: invalid variadic function handle");
		return;
	}

	int fixed_count = func->type->param_count;

	List *aArgs = NULL;
	int total_args = 0;

	if (RING_API_PARACOUNT >= 2 && RING_API_ISLIST(2) && !RING_API_ISCPOINTER(2)) {
		aArgs = RING_API_GETLIST(2);
		total_args = ring_list_getsize(aArgs);
	} else {
		total_args = RING_API_PARACOUNT - 1;
	}

	if (total_args < fixed_count) {
		RING_API_ERROR("ffi_varcall: not enough arguments");
		return;
	}

	ffi_type **arg_types = NULL;
	void **arg_values = NULL;
	void *arg_storage = NULL;

	if (total_args > 0) {
		arg_types =
			(ffi_type **)ring_state_malloc(ctx->ring_state, sizeof(ffi_type *) * total_args);
		arg_values = (void **)ring_state_malloc(ctx->ring_state, sizeof(void *) * total_args);
		arg_storage =
			ring_state_calloc(ctx->ring_state, total_args, sizeof(double) + sizeof(void *));

		if (!arg_types || !arg_values || !arg_storage) {
			if (arg_types)
				ring_state_free(ctx->ring_state, arg_types);
			if (arg_values)
				ring_state_free(ctx->ring_state, arg_values);
			if (arg_storage)
				ring_state_free(ctx->ring_state, arg_storage);
			RING_API_ERROR("ffi_varcall: out of memory");
			return;
		}

		size_t current_offset = 0;

		for (int i = 0; i < total_args; i++) {
			int param_idx;
			if (aArgs) {
				param_idx = 0;
			} else {
				param_idx = 2 + i;
			}

			if (i < fixed_count && func->type->param_types) {
				FFI_Type *ptype = func->type->param_types[i];
				current_offset =
					FFI_ALIGN(current_offset, ptype->alignment > 0 ? ptype->alignment : 1);
				arg_values[i] = (char *)arg_storage + current_offset;
				arg_types[i] = ptype->ffi_type_ptr;
			} else {
				current_offset = FFI_ALIGN(current_offset, sizeof(void *));
				arg_values[i] = (char *)arg_storage + current_offset;
			}
			char *storage_ptr = (char *)arg_values[i];

			int is_num = aArgs ? ring_list_isdouble(aArgs, i + 1) : RING_API_ISNUMBER(param_idx);
			int is_str = aArgs ? ring_list_isstring(aArgs, i + 1) : RING_API_ISSTRING(param_idx);
			int is_ptr = aArgs ? ring_list_islist(aArgs, i + 1) : RING_API_ISCPOINTER(param_idx);

			/*
			 * On 64-bit platforms, C variadic promotion always passes integers as 64-bit.
			 * On 32-bit platforms, integers remain 32-bit.
			 */
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

			if (is_num) {
				double val =
					aArgs ? ring_list_getdouble(aArgs, i + 1) : RING_API_GETNUMBER(param_idx);
				if (val == (double)(int)val && val >= -2147483648.0 && val <= 2147483647.0) {
					if (!(i < fixed_count && func->type->param_types)) {
						arg_types[i] = FFI_VARIADIC_INT_TYPE;
					}
					*(ffi_sarg *)storage_ptr = (ffi_sarg)(int)val;
					current_offset += FFI_VARIADIC_INT_SIZE;
				} else {
					if (!(i < fixed_count && func->type->param_types)) {
						arg_types[i] = &ffi_type_double;
					}
					*(double *)storage_ptr = val;
					current_offset += sizeof(double);
				}
			} else if (is_str) {
				const char *str =
					aArgs ? ring_list_getstring(aArgs, i + 1) : RING_API_GETSTRING(param_idx);
				if (i < fixed_count && func->type->param_types &&
					ffi_is_64bit_int(func->type->param_types[i]->kind)) {
					FFI_Type *ptype = func->type->param_types[i];
					if (ptype->kind == FFI_KIND_UINT64 || ptype->kind == FFI_KIND_ULONGLONG ||
						(ptype->kind == FFI_KIND_SIZE_T && sizeof(size_t) == 8) ||
						(ptype->kind == FFI_KIND_UINTPTR_T && sizeof(uintptr_t) == 8) ||
						(ptype->kind == FFI_KIND_ULONG && sizeof(unsigned long) == 8)) {
						*(uint64_t *)storage_ptr = (uint64_t)strtoull(str, NULL, 10);
					} else {
						*(int64_t *)storage_ptr = (int64_t)strtoll(str, NULL, 10);
					}
					current_offset += ptype->size;
				} else if (!(i < fixed_count && func->type->param_types)) {
					arg_types[i] = &ffi_type_pointer;
					*(const char **)storage_ptr = str;
					current_offset += sizeof(void *);
				} else {
					RING_API_ERROR("ffi_varcall: type mismatch, string passed to "
								   "non-pointer/non-64bit fixed parameter");
					ring_state_free(ctx->ring_state, arg_types);
					ring_state_free(ctx->ring_state, arg_values);
					ring_state_free(ctx->ring_state, arg_storage);
					return;
				}
			} else if (is_ptr) {
				if (!(i < fixed_count && func->type->param_types)) {
					arg_types[i] = &ffi_type_pointer;
				}
				void *ptr_val = NULL;
				const char *ptr_type = NULL;
				if (aArgs) {
					List *argList = ring_list_getlist(aArgs, i + 1);
					ptr_val = ring_list_getpointer(argList, RING_CPOINTER_POINTER);
					ptr_type = ring_list_getstring(argList, RING_CPOINTER_TYPE);
				} else {
					List *argList = RING_API_GETLIST(param_idx);
					ptr_val = ring_list_getpointer(argList, RING_CPOINTER_POINTER);
					ptr_type = ring_list_getstring(argList, RING_CPOINTER_TYPE);
				}

				if (ptr_type && strcmp(ptr_type, "FFI_Callback") == 0 && ptr_val) {
					ptr_val = ((FFI_Callback *)ptr_val)->code_ptr;
				}

				*(void **)storage_ptr = ptr_val;
				current_offset += sizeof(void *);
			} else {
				if (!(i < fixed_count && func->type->param_types)) {
					arg_types[i] = FFI_VARIADIC_INT_TYPE;
				}
				*(ffi_sarg *)storage_ptr = 0;
				current_offset += FFI_VARIADIC_INT_SIZE;
			}
		}
	}

	ffi_cif cif;
	ffi_status status = ffi_prep_cif_var(&cif, FFI_DEFAULT_ABI, fixed_count, total_args,
										 func->type->return_type->ffi_type_ptr, arg_types);
	if (status != FFI_OK) {
		if (arg_types)
			ring_state_free(ctx->ring_state, arg_types);
		if (arg_values)
			ring_state_free(ctx->ring_state, arg_values);
		if (arg_storage)
			ring_state_free(ctx->ring_state, arg_storage);
		RING_API_ERROR("ffi_varcall: failed to prepare cif");
		return;
	}

	union {
		ffi_arg u;
		int8_t i8;
		uint8_t u8;
		int16_t i16;
		uint16_t u16;
		int32_t i32;
		uint32_t u32;
		int64_t i64;
		uint64_t u64;
		float f;
		double d;
		long double ld;
		void *p;
	} result;
	memset(&result, 0, sizeof(result));

	ffi_call(&cif, FFI_FN(func->func_ptr), &result, arg_values);

	if (arg_types)
		ring_state_free(ctx->ring_state, arg_types);
	if (arg_values)
		ring_state_free(ctx->ring_state, arg_values);
	if (arg_storage)
		ring_state_free(ctx->ring_state, arg_storage);

	ffi_push_return_value((VM *)pPointer, &result, func->type->return_type);
}

static void ffi_gc_free_bound_func(void *state, void *ptr)
{
	FFI_BoundFunc *bf = (FFI_BoundFunc *)ptr;
	if (!bf)
		return;
	if (bf->closure)
		ffi_closure_free(bf->closure);
	if (bf->arg_types)
		ring_state_free((RingState *)state, bf->arg_types);
	ring_state_free((RingState *)state, bf);
}

static void ffi_bound_handler(ffi_cif *cif, void *ret, void **args, void *user_data)
{
	FFI_BoundFunc *bf = (FFI_BoundFunc *)user_data;
	if (!bf || !bf->ctx || !bf->func)
		return;

	VM *pVM = (VM *)(*(void **)args[0]);

	FFI_Context *old_ctx = g_ffi_ctx;
	g_ffi_ctx = bf->ctx;

	ffi_call_function(bf->ctx, pVM, bf->func, NULL, 1);

	g_ffi_ctx = old_ctx;
}

static void *ffi_create_trampoline(FFI_Context *ctx, FFI_Function *func)
{
	FFI_BoundFunc *bf = (FFI_BoundFunc *)ring_state_malloc(ctx->ring_state, sizeof(FFI_BoundFunc));
	if (!bf)
		return NULL;
	memset(bf, 0, sizeof(FFI_BoundFunc));

	bf->ctx = ctx;
	bf->func = func;

	bf->closure = ffi_closure_alloc(sizeof(ffi_closure), &bf->code_ptr);
	if (!bf->closure) {
		ring_state_free(ctx->ring_state, bf);
		return NULL;
	}

	bf->arg_types = (ffi_type **)ring_state_malloc(ctx->ring_state, sizeof(ffi_type *));
	if (!bf->arg_types) {
		ffi_closure_free(bf->closure);
		ring_state_free(ctx->ring_state, bf);
		return NULL;
	}
	bf->arg_types[0] = &ffi_type_pointer;

	ffi_status status = ffi_prep_cif(&bf->cif, FFI_DEFAULT_ABI, 1, &ffi_type_void, bf->arg_types);
	if (status != FFI_OK) {
		ring_state_free(ctx->ring_state, bf->arg_types);
		ffi_closure_free(bf->closure);
		ring_state_free(ctx->ring_state, bf);
		return NULL;
	}

	status = ffi_prep_closure_loc(bf->closure, &bf->cif, ffi_bound_handler, bf, bf->code_ptr);
	if (status != FFI_OK) {
		ring_state_free(ctx->ring_state, bf->arg_types);
		ffi_closure_free(bf->closure);
		ring_state_free(ctx->ring_state, bf);
		return NULL;
	}

	ring_list_addcustomringpointer_gc(ctx->ring_state, ctx->gc_list, bf, ffi_gc_free_bound_func);

	return bf->code_ptr;
}

RING_FUNC(ring_cffi_bind)
{
	FFI_Context *ctx = get_or_create_context(pPointer);
	VM *vm = (VM *)pPointer;

	if (RING_API_PARACOUNT == 0) {
		if (!ctx || !ctx->cdef_funcs) {
			RING_API_ERROR("ffi_bind: no FFI context or no cdef declarations");
			return;
		}
		int count = 0;
		for (unsigned int bucket = 0; bucket < ctx->cdef_funcs->nLinkedLists; bucket++) {
			HashItem *item = ctx->cdef_funcs->pArray[bucket];
			while (item) {
				if (item->nItemType == RING_HASHITEMTYPE_POINTER && item->cKey) {
					FFI_Function *func = (FFI_Function *)item->HashValue.pValue;
					if (func && func->cif_prepared) {
						void *trampoline = ffi_create_trampoline(ctx, func);
						if (trampoline) {
							char *lower_name = ffi_lowerdup(ctx, item->cKey);
							ring_vm_funcregister2(vm->pRingState, lower_name,
												  (void (*)(void *))trampoline);
							count++;
						}
					}
				}
				item = item->pNext;
			}
		}
		RING_API_RETNUMBER(count);
		return;
	}

	if (RING_API_PARACOUNT < 3) {
		RING_API_ERROR("ffi_bind(lib, name, rettype [, argtypes_list]) requires "
					   "at least 3 parameters, or 0 to bind all cdef functions");
		return;
	}

	if (!RING_API_ISCPOINTER(1)) {
		RING_API_ERROR("ffi_bind: first parameter must be a library handle");
		return;
	}
	if (!RING_API_ISSTRING(2) || !RING_API_ISSTRING(3)) {
		RING_API_ERROR("ffi_bind: name and return type must be strings");
		return;
	}

	List *pList = RING_API_GETLIST(1);
	FFI_Library *lib = (FFI_Library *)ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	if (!lib) {
		RING_API_ERROR("ffi_bind: invalid library handle");
		return;
	}

	const char *func_name = RING_API_GETSTRING(2);
	const char *ret_type_str = RING_API_GETSTRING(3);

	FFI_Type *ret_type = ffi_type_parse(ctx, ret_type_str);
	if (!ret_type) {
		RING_API_ERROR("ffi_bind: unknown return type");
		return;
	}

	int param_count = 0;
	FFI_Type **param_types = NULL;

	if (RING_API_PARACOUNT >= 4 && RING_API_ISLIST(4)) {
		List *argTypes = RING_API_GETLIST(4);
		param_types = parse_type_list(ctx, argTypes, &param_count);
		if (!param_types && param_count < 0) {
			RING_API_ERROR("ffi_bind: parameter types must be valid strings");
			return;
		}
	}

	void *func_ptr = ffi_library_symbol(lib, func_name);
	if (!func_ptr) {
		ffi_set_error(ctx, "Symbol '%s' not found in library", func_name);
		RING_API_ERROR(ffi_get_error(ctx));
		if (param_types)
			ring_state_free(ctx->ring_state, param_types);
		return;
	}

	FFI_Function *func = ffi_function_create(ctx, func_ptr, ret_type, param_types, param_count);
	if (!func) {
		RING_API_ERROR(ffi_get_error(ctx));
		if (param_types)
			ring_state_free(ctx->ring_state, param_types);
		return;
	}

	if (param_types)
		ring_state_free(ctx->ring_state, param_types);

	ring_list_addcustomringpointer_gc(ctx->ring_state, ctx->gc_list, func, ffi_gc_free_func);

	char *lower_name = ffi_lowerdup(ctx, func_name);
	if (!lower_name) {
		RING_API_ERROR("ffi_bind: out of memory");
		return;
	}

	ring_hashtable_newpointer_gc(ctx->ring_state, ctx->cdef_funcs, lower_name, func);

	void *trampoline = ffi_create_trampoline(ctx, func);
	if (!trampoline) {
		RING_API_ERROR("ffi_bind: out of memory creating trampoline");
		return;
	}

	ring_vm_funcregister2(vm->pRingState, lower_name, (void (*)(void *))trampoline);

	RING_API_RETNUMBER(1);
}

RING_LIBINIT
{
#ifdef _WIN32
	HMODULE hModule;
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
					  (LPCTSTR)&ringlib_init, &hModule);
#else
	Dl_info info;
	if (dladdr((void *)&ringlib_init, &info)) {
		dlopen(info.dli_fname, RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE);
	}
#endif

	RING_API_REGISTER("cffi_load", ring_cffi_load);
	RING_API_REGISTER("cffi_new", ring_cffi_new);
	RING_API_REGISTER("cffi_sizeof", ring_cffi_sizeof);
	RING_API_REGISTER("cffi_nullptr", ring_cffi_nullptr);
	RING_API_REGISTER("cffi_isnull", ring_cffi_isnull);
	RING_API_REGISTER("cffi_string", ring_cffi_string);
	RING_API_REGISTER("cffi_tostring", ring_cffi_tostring);
	RING_API_REGISTER("cffi_errno", ring_cffi_errno);
	RING_API_REGISTER("cffi_strerror", ring_cffi_strerror);
	RING_API_REGISTER("cffi_func", ring_cffi_func);
	RING_API_REGISTER("cffi_funcptr", ring_cffi_funcptr);
	RING_API_REGISTER("cffi_invoke", ring_cffi_invoke);
	RING_API_REGISTER("cffi_sym", ring_cffi_sym);
	RING_API_REGISTER("cffi_get", ring_cffi_get);
	RING_API_REGISTER("cffi_set", ring_cffi_set);
	RING_API_REGISTER("cffi_get_i64", ring_cffi_get_i64);
	RING_API_REGISTER("cffi_set_i64", ring_cffi_set_i64);
	RING_API_REGISTER("cffi_deref", ring_cffi_deref);
	RING_API_REGISTER("cffi_offset", ring_cffi_offset);
	RING_API_REGISTER("cffi_struct", ring_cffi_struct);
	RING_API_REGISTER("cffi_typeof", ring_cffi_typeof);
	RING_API_REGISTER("cffi_struct_new", ring_cffi_struct_new);
	RING_API_REGISTER("cffi_field", ring_cffi_field);
	RING_API_REGISTER("cffi_field_offset", ring_cffi_field_offset);
	RING_API_REGISTER("cffi_struct_size", ring_cffi_struct_size);
	RING_API_REGISTER("cffi_callback", ring_cffi_callback);
	RING_API_REGISTER("cffi_enum", ring_cffi_enum);
	RING_API_REGISTER("cffi_enum_value", ring_cffi_enum_value);
	RING_API_REGISTER("cffi_union", ring_cffi_union);
	RING_API_REGISTER("cffi_union_new", ring_cffi_union_new);
	RING_API_REGISTER("cffi_union_size", ring_cffi_union_size);
	RING_API_REGISTER("cffi_varfunc", ring_cffi_varfunc);
	RING_API_REGISTER("cffi_varcall", ring_cffi_varcall);
	RING_API_REGISTER("cffi_cdef", ring_cffi_cdef);
	RING_API_REGISTER("cffi_bind", ring_cffi_bind);
}
