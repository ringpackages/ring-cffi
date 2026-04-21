/*
 * RingCFFI - Callback handler, bound functions, and closure trampolines
 * Author: Youssef Saeed <youssefelkholey@gmail.com>
 * Copyright (c) 2026
 */

#include "ring_cffi_internal.h"

void ffi_callback_handler(ffi_cif *cif, void *ret, void **args, void *user_data)
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

	List *cb_res_stack = ring_state_findvar(state, "__cffi_cb_res");
	if (!cb_res_stack) {
		cb_res_stack = ring_state_newvar(state, "__cffi_cb_res");
		ring_list_setint_gc(state, cb_res_stack, RING_VAR_TYPE, RING_VM_LIST);
		ring_list_setlist_gc(state, cb_res_stack, RING_VAR_VALUE);
	}

	for (int i = 0; i < cif->nargs; i++) {
		FFI_Type *ptype = ftype->param_types[i];

		if (ptype->kind == FFI_KIND_STRING && ptype->pointer_depth == 0) {
			char *str_val = *(char **)args[i];
			if (str_val)
				ring_list_addstring_gc(state, current_args, str_val);
			else
				ring_list_addcpointer_gc(state, current_args, NULL, "FFI_Ptr");
		} else if (FFI_IS_POINTER_TYPE(ptype)) {
			void *val = *(void **)args[i];
			ring_list_addcpointer_gc(state, current_args, val, "FFI_Ptr");
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

	tn = snprintf(tp, trem, ")");
	tp += tn;
	trem -= tn;

	size_t call_len = (size_t)(tp - tmp_buf) + 1;
	cb->call_buf = ring_state_malloc(ctx->ring_state, call_len);
	if (cb->call_buf) {
		memcpy(cb->call_buf, tmp_buf, call_len);
	}

	RING_API_RETMANAGEDCPOINTER(cb, "FFI_Callback", ffi_gc_free_callback);
}

void ffi_bound_handler(ffi_cif *cif, void *ret, void **args, void *user_data)
{
	FFI_BoundFunc *bf = (FFI_BoundFunc *)user_data;
	if (!bf || !bf->ctx || !bf->func)
		return;

	VM *pVM = (VM *)(*(void **)args[0]);

	FFI_Context *old_ctx = g_ffi_ctx;
	g_ffi_ctx = bf->ctx;

	if (bf->func->type && bf->func->type->is_variadic)
		ffi_call_variadic(bf->ctx, pVM, bf->func, NULL, 1);
	else
		ffi_call_function(bf->ctx, pVM, bf->func, NULL, 1);

	g_ffi_ctx = old_ctx;
}

void *ffi_create_trampoline(FFI_Context *ctx, FFI_Function *func)
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
