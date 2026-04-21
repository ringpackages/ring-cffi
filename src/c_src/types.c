/*
 * RingCFFI - Primitive types, type parsing, sizeof, and type cache
 * Author: Youssef Saeed <youssefelkholey@gmail.com>
 * Copyright (c) 2026
 */

#include "ring_cffi_internal.h"

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
	case FFI_KIND_STRING:
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
	case FFI_KIND_STRING:
	case FFI_KIND_POINTER:
		return sizeof(void *);
	case FFI_KIND_WCHAR_T:
		return sizeof(wchar_t);
	default:
		return 0;
	}
}

bool ffi_is_64bit_int(FFI_TypeKind kind)
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

	ring_list_addcustomringpointer_gc(ctx->ring_state, ctx->gc_list, type, ffi_gc_free_type);
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

	ring_list_addcustomringpointer_gc(ctx->ring_state, ctx->gc_list, type, ffi_gc_free_type);
	return type;
}

size_t ffi_sizeof(FFI_Type *type) { return type ? type->size : 0; }

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
		return FFI_KIND_STRING;

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
		if (cached != NULL)
			return cached;
	}

	char *buf = (char *)ring_state_malloc(ctx->ring_state, strlen(type_str) + 1);
	if (!buf)
		return NULL;
	strcpy(buf, type_str);

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
		FFI_StructType *st_lookup = (FFI_StructType *)ring_hashtable_findpointer(ctx->structs, p);
		if (st_lookup) {
			kind = FFI_KIND_STRUCT;
		} else {
			FFI_UnionType *ut_lookup = (FFI_UnionType *)ring_hashtable_findpointer(ctx->unions, p);
			if (ut_lookup) {
				kind = FFI_KIND_UNION;
			}
		}
	}
	if (kind == FFI_KIND_UNKNOWN) {
		ffi_set_error(ctx, "Unknown type: '%s'", type_str);
		ring_state_free(ctx->ring_state, buf);
		return NULL;
	}
	FFI_Type *base;
	if (kind == FFI_KIND_STRUCT) {
		FFI_StructType *st_found = (FFI_StructType *)ring_hashtable_findpointer(ctx->structs, p);
		base = (FFI_Type *)ring_state_malloc(ctx->ring_state, sizeof(FFI_Type));
		if (!base) {
			ring_state_free(ctx->ring_state, buf);
			return NULL;
		}
		memset(base, 0, sizeof(FFI_Type));
		base->kind = FFI_KIND_STRUCT;
		base->info.struct_type = st_found;
		base->ffi_type_ptr = &st_found->ffi_type_def;
		base->size = st_found->size;
		base->alignment = st_found->alignment;
		ring_list_addcustomringpointer_gc(ctx->ring_state, ctx->gc_list, base, ffi_gc_free_type);
	} else if (kind == FFI_KIND_UNION) {
		FFI_UnionType *ut_found = (FFI_UnionType *)ring_hashtable_findpointer(ctx->unions, p);
		base = (FFI_Type *)ring_state_malloc(ctx->ring_state, sizeof(FFI_Type));
		if (!base) {
			ring_state_free(ctx->ring_state, buf);
			return NULL;
		}
		memset(base, 0, sizeof(FFI_Type));
		base->kind = FFI_KIND_UNION;
		base->info.union_type = ut_found;
		base->ffi_type_ptr = &ffi_type_void;
		base->size = ut_found->size;
		base->alignment = ut_found->alignment;
		ring_list_addcustomringpointer_gc(ctx->ring_state, ctx->gc_list, base, ffi_gc_free_type);
	} else {
		base = ffi_type_primitive(ctx, kind);
	}

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
	}

	ring_state_free(ctx->ring_state, buf);
	return result;
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
