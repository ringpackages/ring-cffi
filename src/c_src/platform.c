/*
 * RingCFFI - Platform-specific library loader (Windows UTF-16)
 * Author: Youssef Saeed <youssefelkholey@gmail.com>
 * Copyright (c) 2026
 */

#include "ring_cffi_internal.h"

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
