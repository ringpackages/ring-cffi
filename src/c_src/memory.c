/*
 * RingCFFI - Memory allocation, pointer casts, and C string / wide-string helpers
 * Author: Youssef Saeed <youssefelkholey@gmail.com>
 * Copyright (c) 2026
 */

#include "ring_cffi_internal.h"

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
	char *result = ffi_cstring_unescape(ctx->ring_state, str);
	return result ? result : NULL;
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
	if (RING_API_PARACOUNT != 1) {
		RING_API_ERROR("ffi_tostring(ptr) expects a pointer");
		return;
	}
	if (RING_API_ISSTRING(1)) {
		RING_API_RETSTRING(RING_API_GETSTRING(1));
		return;
	}
	if (!RING_API_ISCPOINTER(1)) {
		RING_API_ERROR("ffi_tostring(ptr) expects a pointer");
		return;
	}

	List *pList = RING_API_GETLIST(1);
	char *ptr = (char *)ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	if (!ptr)
		return;

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

RING_FUNC(ring_cffi_cast)
{
	if (RING_API_PARACOUNT < 2) {
		RING_API_ERROR("ffi_cast(ptr, type) requires 2 parameters");
		return;
	}

	if (!RING_API_ISCPOINTER(1) || !RING_API_ISSTRING(2)) {
		RING_API_ERROR("ffi_cast: invalid parameters");
		return;
	}

	List *pList = RING_API_GETLIST(1);
	void *ptr = ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	const char *new_type = RING_API_GETSTRING(2);

	RING_API_RETCPOINTER(ptr, new_type);
}

RING_FUNC(ring_cffi_string_array)
{
	if (RING_API_PARACOUNT < 1 || !RING_API_ISLIST(1)) {
		RING_API_ERROR("ffi_string_array(list) requires a list of strings");
		return;
	}

	FFI_Context *ctx = get_or_create_context(pPointer);
	List *str_list = RING_API_GETLIST(1);
	int count = ring_list_getsize(str_list);

	char **arr = (char **)ring_state_calloc(ctx->ring_state, 1, sizeof(char *) * (count + 1));
	if (!arr) {
		RING_API_ERROR("ffi_string_array: out of memory");
		return;
	}

	for (int i = 1; i <= count; i++) {
		if (ring_list_isstring(str_list, i)) {
			const char *raw = ring_list_getstring(str_list, i);
			arr[i - 1] = ffi_string_new(ctx, raw);
			if (!arr[i - 1]) {
				ring_state_free(ctx->ring_state, arr);
				RING_API_ERROR("ffi_string_array: out of memory");
				return;
			}
			ring_list_addcustomringpointer_gc(ctx->ring_state, ctx->gc_list, arr[i - 1],
											  ffi_gc_free_ptr);
		} else {
			arr[i - 1] = NULL;
		}
	}
	arr[count] = NULL;

	ring_list_addcustomringpointer_gc(ctx->ring_state, ctx->gc_list, arr, ffi_gc_free_ptr);
	RING_API_RETCPOINTER(arr, "FFI_Ptr");
}

RING_FUNC(ring_cffi_wstring)
{
	if (RING_API_PARACOUNT < 1 || !RING_API_ISSTRING(1)) {
		RING_API_ERROR("ffi_wstring(str) expects a string");
		return;
	}

	FFI_Context *ctx = get_or_create_context(pPointer);
	const char *str = RING_API_GETSTRING(1);
	size_t len = strlen(str);

#ifdef _WIN32
	int wlen = MultiByteToWideChar(CP_UTF8, 0, str, (int)len, NULL, 0);
	if (wlen == 0) {
		RING_API_ERROR("ffi_wstring: conversion failed");
		return;
	}
	wchar_t *wbuf = (wchar_t *)ring_state_calloc(ctx->ring_state, 1, sizeof(wchar_t) * (wlen + 1));
	if (!wbuf) {
		RING_API_ERROR("ffi_wstring: out of memory");
		return;
	}
	MultiByteToWideChar(CP_UTF8, 0, str, (int)len, wbuf, wlen);
	wbuf[wlen] = L'\0';
#else
	size_t wlen = mbstowcs(NULL, str, 0);
	if (wlen == (size_t)-1) {
		wlen = len;
	}
	wchar_t *wbuf = (wchar_t *)ring_state_calloc(ctx->ring_state, 1, sizeof(wchar_t) * (wlen + 1));
	if (!wbuf) {
		RING_API_ERROR("ffi_wstring: out of memory");
		return;
	}
	mbstowcs(wbuf, str, wlen + 1);
#endif

	RING_API_RETMANAGEDCPOINTER(wbuf, "FFI_Ptr", ffi_gc_free_ptr);
}

RING_FUNC(ring_cffi_wtostring)
{
	if (RING_API_PARACOUNT < 1 || !RING_API_ISCPOINTER(1)) {
		RING_API_ERROR("ffi_wtostring(ptr) expects a pointer");
		return;
	}

	FFI_Context *ctx = get_or_create_context(pPointer);
	List *pList = RING_API_GETLIST(1);
	wchar_t *wptr = (wchar_t *)ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	if (!wptr)
		return;

#ifdef _WIN32
	int mblen = WideCharToMultiByte(CP_UTF8, 0, wptr, -1, NULL, 0, NULL, NULL);
	if (mblen == 0)
		return;
	char *buf = (char *)ring_state_malloc(ctx->ring_state, mblen);
	if (!buf)
		return;
	WideCharToMultiByte(CP_UTF8, 0, wptr, -1, buf, mblen, NULL, NULL);
	RING_API_RETSTRING(buf);
	ring_state_free(ctx->ring_state, buf);
#else
	size_t mblen = wcstombs(NULL, wptr, 0);
	if (mblen == (size_t)-1)
		return;
	char *buf = (char *)ring_state_malloc(ctx->ring_state, mblen + 1);
	if (!buf)
		return;
	wcstombs(buf, wptr, mblen + 1);
	buf[mblen] = '\0';
	RING_API_RETSTRING(buf);
	ring_state_free(ctx->ring_state, buf);
#endif
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

	const char *ptr_type = ring_list_getstring(pList, RING_CPOINTER_TYPE);
	FFI_TypeKind bf_kind;
	int bf_off, bf_w;
	if (ffi_parse_bitfield_tag(ptr_type, &bf_kind, &bf_off, &bf_w)) {
		ffi_read_bitfield((VM *)pPointer, ctx, ptr, bf_kind, bf_off, bf_w);
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

	if (type->kind == FFI_KIND_STRING && type->pointer_depth == 0) {
		char *str_val = *(char **)elem_ptr;
		if (str_val)
			ring_vm_api_retstring((VM *)pPointer, str_val);
		else
			ring_vm_api_retcpointer((VM *)pPointer, NULL, "FFI_Ptr");
	} else if (FFI_IS_POINTER_TYPE(type)) {
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

	const char *ptr_type = ring_list_getstring(pList, RING_CPOINTER_TYPE);
	FFI_TypeKind bf_kind;
	int bf_off, bf_w;
	if (ffi_parse_bitfield_tag(ptr_type, &bf_kind, &bf_off, &bf_w)) {
		uint64_t new_val = 0;
		if (RING_API_ISNUMBER(3)) {
			new_val = (uint64_t)(int64_t)RING_API_GETNUMBER(3);
		} else if (RING_API_ISSTRING(3)) {
			new_val = (uint64_t)strtoull(RING_API_GETSTRING(3), NULL, 10);
		}
		ffi_write_bitfield((VM *)pPointer, ctx, ptr, bf_kind, bf_off, bf_w, new_val);
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

	if (FFI_IS_POINTER_TYPE(type)) {
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

		if (type->kind == FFI_KIND_STRING && type->pointer_depth == 0) {
			char *str_val = *(char **)ptr;
			if (str_val)
				RING_API_RETSTRING(str_val);
			else
				RING_API_RETCPOINTER(NULL, "FFI_Ptr");
		} else if (FFI_IS_POINTER_TYPE(type)) {
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
