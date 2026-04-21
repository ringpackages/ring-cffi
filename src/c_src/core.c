/*
 * RingCFFI - Context lifecycle, error handling, GC free routines, and Ring API registration
 * Author: Youssef Saeed <youssefelkholey@gmail.com>
 * Copyright (c) 2026
 */

#ifndef _WIN32
#define _GNU_SOURCE
#endif

#include "ring_cffi_internal.h"

FFI_TLS FFI_Context *g_ffi_ctx = NULL;

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

void ffi_gc_free_ptr(void *state, void *ptr)
{
	if (!ptr)
		return;
	ring_state_free((RingState *)state, ptr);
}

void ffi_gc_free_lib(void *state, void *ptr)
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

void ffi_gc_free_func(void *state, void *ptr)
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

void ffi_gc_free_callback(void *state, void *ptr)
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

void ffi_gc_free_type(void *state, void *ptr)
{
	FFI_Type *type = (FFI_Type *)ptr;
	if (!type)
		return;
	ring_state_free((RingState *)state, type);
}

void ffi_gc_free_enum(void *state, void *ptr)
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

void ffi_gc_free_struct_type(void *state, void *ptr)
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

void ffi_gc_free_union_type(void *state, void *ptr)
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

void ffi_gc_free_bound_func(void *state, void *ptr)
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

void ffi_context_free(void *state, void *ptr)
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

void cdef_funcs_set(FFI_Context *ctx, const char *key, void *value)
{
	HashItem *item = ring_hashtable_finditem_gc(ctx->ring_state, ctx->cdef_funcs, key);
	if (item && item->nItemType == RING_HASHITEMTYPE_POINTER) {
		item->HashValue.pValue = value;
	} else {
		ring_hashtable_newpointer_gc(ctx->ring_state, ctx->cdef_funcs, key, value);
	}
}

FFI_Context *get_or_create_context(void *pPointer)
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

char *ffi_lowerdup(FFI_Context *ctx, const char *str)
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

char *ffi_cstring_unescape(RingState *state, const char *src)
{
	size_t len = strlen(src);
	char *dst = (char *)ring_state_malloc(state, len + 1);
	if (!dst)
		return NULL;

	char *out = dst;
	const char *in = src;
	while (*in) {
		if (*in == '\\' && *(in + 1)) {
			in++;
			switch (*in) {
			case 'n':
				*out++ = '\n';
				break;
			case 't':
				*out++ = '\t';
				break;
			case 'r':
				*out++ = '\r';
				break;
			case '0':
				*out++ = '\0';
				break;
			case 'a':
				*out++ = '\a';
				break;
			case 'b':
				*out++ = '\b';
				break;
			case 'f':
				*out++ = '\f';
				break;
			case 'v':
				*out++ = '\v';
				break;
			case '\\':
				*out++ = '\\';
				break;
			case '\'':
				*out++ = '\'';
				break;
			case '\"':
				*out++ = '\"';
				break;
			case 'x': {
				in++;
				unsigned int val = 0;
				for (int i = 0; i < 2 && isxdigit((unsigned char)*in); i++, in++)
					val = (val << 4) |
						  (isdigit((unsigned char)*in) ? (*in - '0')
													   : (tolower((unsigned char)*in) - 'a' + 10));
				in--;
				*out++ = (char)val;
				break;
			}
			default:
				if (isdigit((unsigned char)*in) && *in >= '0' && *in <= '7') {
					unsigned int val = *in - '0';
					in++;
					if (isdigit((unsigned char)*in) && *in >= '0' && *in <= '7') {
						val = (val << 3) | (*in - '0');
						in++;
						if (isdigit((unsigned char)*in) && *in >= '0' && *in <= '7')
							val = (val << 3) | (*in - '0');
						else
							in--;
					} else {
						in--;
					}
					*out++ = (char)val;
				} else {
					*out++ = '\\';
					*out++ = *in;
				}
				break;
			}
		} else {
			*out++ = *in;
		}
		in++;
	}
	*out = '\0';
	return dst;
}

#ifndef _WIN32
__attribute__((visibility("default")))
#endif
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
	RING_API_REGISTER("cffi_cast", ring_cffi_cast);
	RING_API_REGISTER("cffi_string_array", ring_cffi_string_array);
	RING_API_REGISTER("cffi_wstring", ring_cffi_wstring);
	RING_API_REGISTER("cffi_wtostring", ring_cffi_wtostring);
}
