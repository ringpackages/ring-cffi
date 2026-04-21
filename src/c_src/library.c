/*
 * RingCFFI - Dynamic library loading and symbol resolution
 * Author: Youssef Saeed <youssefelkholey@gmail.com>
 * Copyright (c) 2026
 */

#include "ring_cffi_internal.h"

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
