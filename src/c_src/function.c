/*
 * RingCFFI - Non-variadic function wrapper creation (ffi_func / ffi_funcptr)
 * Author: Youssef Saeed <youssefelkholey@gmail.com>
 * Copyright (c) 2026
 */

#include "ring_cffi_internal.h"

FFI_Function *ffi_function_create(FFI_Context *ctx, void *func_ptr, FFI_Type *ret_type,
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

FFI_Type **parse_type_list(FFI_Context *ctx, List *type_list, int *out_count)
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
