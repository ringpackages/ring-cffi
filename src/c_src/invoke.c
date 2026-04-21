/*
 * RingCFFI - Call engine: argument marshalling, ffi_call, and variadic support
 * Author: Youssef Saeed <youssefelkholey@gmail.com>
 * Copyright (c) 2026
 */

#include "ring_cffi_internal.h"

int ffi_store_arg(FFI_Context *ctx, VM *pVM, List *aArgs, int i, int param_idx, FFI_Type *ptype,
				  char *storage_ptr, ffi_type **out_ffi_type, size_t *out_size)
{
	int is_num =
		aArgs ? ring_list_isdouble(aArgs, i + 1) : ring_vm_api_isnumber((void *)pVM, param_idx);
	int is_str =
		aArgs ? ring_list_isstring(aArgs, i + 1) : ring_vm_api_isstring((void *)pVM, param_idx);
	int is_ptr =
		aArgs ? ring_list_islist(aArgs, i + 1) : ring_vm_api_iscpointer((void *)pVM, param_idx);

	if (ptype && FFI_IS_POINTER_TYPE(ptype)) {
		void *ptr_val = NULL;
		const char *ptr_type = NULL;
		if (aArgs) {
			if (ring_list_islist(aArgs, i + 1)) {
				List *argList = ring_list_getlist(aArgs, i + 1);
				ptr_val = ring_list_getpointer(argList, RING_CPOINTER_POINTER);
				ptr_type = ring_list_getstring(argList, RING_CPOINTER_TYPE);
			} else if (ring_list_isstring(aArgs, i + 1)) {
				char *unescaped =
					ffi_cstring_unescape(ctx->ring_state, ring_list_getstring(aArgs, i + 1));
				if (unescaped)
					ring_list_addcustomringpointer_gc(ctx->ring_state, ctx->gc_list, unescaped,
													  ffi_gc_free_ptr);
				ptr_val = unescaped ? unescaped : (void *)ring_list_getstring(aArgs, i + 1);
			} else if (ring_list_isdouble(aArgs, i + 1) && ring_list_getdouble(aArgs, i + 1) == 0) {
				ptr_val = NULL;
			} else {
				ring_vm_error(pVM, "expected pointer argument");
				return -1;
			}
		} else {
			if (ring_vm_api_iscpointer((void *)pVM, param_idx)) {
				List *argList = ring_vm_api_getlist((void *)pVM, param_idx);
				ptr_val = ring_list_getpointer(argList, RING_CPOINTER_POINTER);
				ptr_type = ring_list_getstring(argList, RING_CPOINTER_TYPE);
			} else if (ring_vm_api_isstring((void *)pVM, param_idx)) {
				char *unescaped = ffi_cstring_unescape(
					ctx->ring_state, ring_vm_api_getstring((void *)pVM, param_idx));
				if (unescaped)
					ring_list_addcustomringpointer_gc(ctx->ring_state, ctx->gc_list, unescaped,
													  ffi_gc_free_ptr);
				ptr_val =
					unescaped ? unescaped : (void *)ring_vm_api_getstring((void *)pVM, param_idx);
			} else if (ring_vm_api_isnumber((void *)pVM, param_idx) &&
					   ring_vm_api_getnumber((void *)pVM, param_idx) == 0) {
				ptr_val = NULL;
			} else {
				ring_vm_error(pVM, "expected pointer argument");
				return -1;
			}
		}

		if (ptr_type && strcmp(ptr_type, "FFI_Callback") == 0 && ptr_val)
			ptr_val = ((FFI_Callback *)ptr_val)->code_ptr;

		*(void **)storage_ptr = ptr_val;
		*out_size = sizeof(void *);
		return 0;
	}

	if (is_num) {
		double val = aArgs ? ring_list_getdouble(aArgs, i + 1)
						   : ring_vm_api_getnumber((void *)pVM, param_idx);
		if (ptype) {
			ffi_write_typed_value(storage_ptr, ptype, val);
			*out_size = ptype->size > 0 ? ptype->size : sizeof(int);
		} else {
			if (val == (double)(int)val && val >= -2147483648.0 && val <= 2147483647.0) {
				*out_ffi_type = FFI_VARIADIC_INT_TYPE;
				*(ffi_sarg *)storage_ptr = (ffi_sarg)(int)val;
				*out_size = FFI_VARIADIC_INT_SIZE;
			} else {
				*out_ffi_type = &ffi_type_double;
				*(double *)storage_ptr = val;
				*out_size = sizeof(double);
			}
		}
		return 0;
	}

	if (is_str) {
		const char *str = aArgs ? ring_list_getstring(aArgs, i + 1)
								: ring_vm_api_getstring((void *)pVM, param_idx);
		if (ptype && ffi_is_64bit_int(ptype->kind)) {
			if (ptype->kind == FFI_KIND_UINT64 || ptype->kind == FFI_KIND_ULONGLONG ||
				(ptype->kind == FFI_KIND_SIZE_T && sizeof(size_t) == 8) ||
				(ptype->kind == FFI_KIND_UINTPTR_T && sizeof(uintptr_t) == 8) ||
				(ptype->kind == FFI_KIND_ULONG && sizeof(unsigned long) == 8)) {
				*(uint64_t *)storage_ptr = (uint64_t)strtoull(str, NULL, 10);
			} else {
				*(int64_t *)storage_ptr = (int64_t)strtoll(str, NULL, 10);
			}
			*out_size = ptype->size;
		} else if (ptype && FFI_IS_POINTER_TYPE(ptype)) {
			char *unescaped = ffi_cstring_unescape(ctx->ring_state, str);
			if (unescaped)
				ring_list_addcustomringpointer_gc(ctx->ring_state, ctx->gc_list, unescaped,
												  ffi_gc_free_ptr);
			*(const char **)storage_ptr = unescaped ? unescaped : str;
			*out_size = sizeof(void *);
		} else if (!ptype) {
			*out_ffi_type = &ffi_type_pointer;
			char *unescaped = ffi_cstring_unescape(ctx->ring_state, str);
			if (unescaped)
				ring_list_addcustomringpointer_gc(ctx->ring_state, ctx->gc_list, unescaped,
												  ffi_gc_free_ptr);
			*(const char **)storage_ptr = unescaped ? unescaped : str;
			*out_size = sizeof(void *);
		} else {
			ring_vm_error(pVM, "type mismatch, string passed to non-pointer/non-64bit parameter");
			return -1;
		}
		return 0;
	}

	if (is_ptr) {
		void *ptr_val = NULL;
		const char *ptr_type = NULL;
		if (aArgs) {
			List *argList = ring_list_getlist(aArgs, i + 1);
			ptr_val = ring_list_getpointer(argList, RING_CPOINTER_POINTER);
			ptr_type = ring_list_getstring(argList, RING_CPOINTER_TYPE);
		} else {
			List *argList = ring_vm_api_getlist((void *)pVM, param_idx);
			ptr_val = ring_list_getpointer(argList, RING_CPOINTER_POINTER);
			ptr_type = ring_list_getstring(argList, RING_CPOINTER_TYPE);
		}

		if (ptr_type && strcmp(ptr_type, "FFI_Callback") == 0 && ptr_val)
			ptr_val = ((FFI_Callback *)ptr_val)->code_ptr;

		if (!ptype)
			*out_ffi_type = &ffi_type_pointer;
		*(void **)storage_ptr = ptr_val;
		*out_size = sizeof(void *);
		return 0;
	}

	if (!ptype) {
		*out_ffi_type = FFI_VARIADIC_INT_TYPE;
		*(ffi_sarg *)storage_ptr = 0;
		*out_size = FFI_VARIADIC_INT_SIZE;
		return 0;
	}

	ring_vm_error(pVM, "unsupported argument type");
	return -1;
}

int ffi_call_function(FFI_Context *ctx, VM *pVM, FFI_Function *func, List *aArgs, int api_offset)
{
	int fixed_count = func->type->param_count;

	int arg_count;
	if (aArgs)
		arg_count = ring_list_getsize(aArgs);
	else
		arg_count = ring_vm_api_paracount((void *)pVM) - api_offset + 1;

	if (arg_count != fixed_count) {
		char err[256];
		snprintf(err, sizeof(err), "expected %d arguments, got %d", fixed_count, arg_count);
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
			if (FFI_IS_POINTER_TYPE(ptype)) {
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

			ffi_type *unused_ffi_type = NULL;
			size_t wrote = 0;
			if (ffi_store_arg(ctx, pVM, aArgs, i, param_idx, ptype, storage_ptr, &unused_ffi_type,
							  &wrote) < 0)
				goto cleanup;

			current_offset += wrote;
			if (FFI_IS_POINTER_TYPE(ptype))
				current_offset = FFI_ALIGN(current_offset, 16);
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

int ffi_call_variadic(FFI_Context *ctx, VM *pVM, FFI_Function *func, List *aArgs, int api_offset)
{
	int fixed_count = func->type->param_count;

	int arg_count;
	if (aArgs)
		arg_count = ring_list_getsize(aArgs);
	else
		arg_count = ring_vm_api_paracount((void *)pVM) - api_offset + 1;

	if (arg_count < fixed_count) {
		ring_vm_error(pVM, "not enough arguments for variadic call");
		return -1;
	}

	ffi_type **arg_types = NULL;
	void **arg_values = NULL;
	void *arg_storage = NULL;

	if (arg_count > 0) {
		arg_types = (ffi_type **)ring_state_malloc(ctx->ring_state, sizeof(ffi_type *) * arg_count);
		arg_values = (void **)ring_state_malloc(ctx->ring_state, sizeof(void *) * arg_count);

		size_t storage_size = 0;
		for (int i = 0; i < arg_count; i++) {
			if (i < fixed_count && func->type->param_types) {
				FFI_Type *ptype = func->type->param_types[i];
				storage_size = FFI_ALIGN(storage_size, ptype->alignment > 0 ? ptype->alignment : 1);
				if (FFI_IS_POINTER_TYPE(ptype)) {
					storage_size += sizeof(void *);
					storage_size = FFI_ALIGN(storage_size, 16);
				} else {
					storage_size += ptype->size > 0 ? ptype->size : sizeof(int);
				}
			} else {
				storage_size = FFI_ALIGN(storage_size, 16);
				storage_size += sizeof(double) + sizeof(void *);
				storage_size = FFI_ALIGN(storage_size, 16);
			}
		}
		storage_size = FFI_ALIGN(storage_size, 16);

		arg_storage = ring_state_calloc(ctx->ring_state, 1, storage_size);

		if (!arg_types || !arg_values || !arg_storage) {
			if (arg_types)
				ring_state_free(ctx->ring_state, arg_types);
			if (arg_values)
				ring_state_free(ctx->ring_state, arg_values);
			if (arg_storage)
				ring_state_free(ctx->ring_state, arg_storage);
			ring_vm_error(pVM, "out of memory");
			return -1;
		}
		memset(arg_types, 0, sizeof(ffi_type *) * arg_count);

		size_t current_offset = 0;

		for (int i = 0; i < arg_count; i++) {
			int param_idx = aArgs ? 0 : (api_offset + i);
			FFI_Type *ptype =
				(i < fixed_count && func->type->param_types) ? func->type->param_types[i] : NULL;

			if (ptype) {
				current_offset =
					FFI_ALIGN(current_offset, ptype->alignment > 0 ? ptype->alignment : 1);
				arg_values[i] = (char *)arg_storage + current_offset;
				arg_types[i] = ptype->ffi_type_ptr;
			} else {
				current_offset = FFI_ALIGN(current_offset, 16);
				arg_values[i] = (char *)arg_storage + current_offset;
			}
			char *storage_ptr = (char *)arg_values[i];

			ffi_type *inferred_ffi_type = NULL;
			size_t wrote = 0;
			if (ffi_store_arg(ctx, pVM, aArgs, i, param_idx, ptype, storage_ptr, &inferred_ffi_type,
							  &wrote) < 0) {
				ring_state_free(ctx->ring_state, arg_types);
				ring_state_free(ctx->ring_state, arg_values);
				ring_state_free(ctx->ring_state, arg_storage);
				return -1;
			}

			if (!ptype && inferred_ffi_type)
				arg_types[i] = inferred_ffi_type;

			current_offset += wrote;
			if ((ptype && FFI_IS_POINTER_TYPE(ptype)) || (!ptype && wrote == sizeof(void *)))
				current_offset = FFI_ALIGN(current_offset, 16);
		}
	}

	ffi_cif var_cif;
	ffi_status status = ffi_prep_cif_var(&var_cif, FFI_DEFAULT_ABI, fixed_count, arg_count,
										 func->type->return_type->ffi_type_ptr, arg_types);
	if (status != FFI_OK) {
		if (arg_types)
			ring_state_free(ctx->ring_state, arg_types);
		if (arg_values)
			ring_state_free(ctx->ring_state, arg_values);
		if (arg_storage)
			ring_state_free(ctx->ring_state, arg_storage);
		ring_vm_error(pVM, "failed to prepare variadic cif");
		return -1;
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

	ffi_call(&var_cif, FFI_FN(func->func_ptr), &result, arg_values);

	if (arg_types)
		ring_state_free(ctx->ring_state, arg_types);
	if (arg_values)
		ring_state_free(ctx->ring_state, arg_values);
	if (arg_storage)
		ring_state_free(ctx->ring_state, arg_storage);

	ffi_push_return_value(pVM, &result, func->type->return_type);
	return 0;
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
	if (!func || (!func->cif_prepared && !(func->type && func->type->is_variadic))) {
		RING_API_ERROR("ffi_invoke: invalid function handle");
		return;
	}

	List *aArgs = NULL;
	if (RING_API_PARACOUNT >= 2 && RING_API_ISLIST(2) && !RING_API_ISCPOINTER(2)) {
		aArgs = RING_API_GETLIST(2);
	}

	if (func->type && func->type->is_variadic)
		ffi_call_variadic(ctx, (VM *)pPointer, func, aArgs, 2);
	else
		ffi_call_function(ctx, (VM *)pPointer, func, aArgs, 2);
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

	List *aArgs = NULL;
	if (RING_API_PARACOUNT >= 2 && RING_API_ISLIST(2) && !RING_API_ISCPOINTER(2)) {
		aArgs = RING_API_GETLIST(2);
	}

	ffi_call_variadic(ctx, (VM *)pPointer, func, aArgs, 2);
}
