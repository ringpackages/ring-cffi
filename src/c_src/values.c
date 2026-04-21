/*
 * RingCFFI - Typed value read/write, bitfield access, and Ring value pushing
 * Author: Youssef Saeed <youssefelkholey@gmail.com>
 * Copyright (c) 2026
 */

#include "ring_cffi_internal.h"

double ffi_read_typed_value(void *src, FFI_Type *type)
{
	if (FFI_IS_POINTER_TYPE(type)) {
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

void ffi_write_typed_value(void *dst, FFI_Type *type, double val)
{
	if (FFI_IS_POINTER_TYPE(type)) {
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

void ffi_push_to_ring(VM *vm, void *src, FFI_Type *type, bool is_ffi_arg)
{
	if (type->kind == FFI_KIND_VOID) {
		ring_vm_api_retnumber(vm, 0);
	} else if (type->kind == FFI_KIND_STRING && type->pointer_depth == 0) {
		char *str_val = *(char **)src;
		if (str_val)
			ring_vm_api_retstring(vm, str_val);
		else
			ring_vm_api_retcpointer(vm, NULL, "FFI_Ptr");
	} else if (FFI_IS_POINTER_TYPE(type)) {
		ring_vm_api_retcpointer(vm, *(void **)src, "FFI_Ptr");
	} else if (ffi_is_64bit_int(type->kind)) {
		uint64_t uval;
		int64_t ival;
		int is_unsigned = (type->kind == FFI_KIND_UINT64 || type->kind == FFI_KIND_ULONGLONG ||
						   (type->kind == FFI_KIND_SIZE_T && sizeof(size_t) == 8) ||
						   (type->kind == FFI_KIND_UINTPTR_T && sizeof(uintptr_t) == 8) ||
						   (type->kind == FFI_KIND_ULONG && sizeof(unsigned long) == 8));
		if (is_unsigned) {
			uval = *(uint64_t *)src;
		} else {
			ival = *(int64_t *)src;
			uval = (uint64_t)ival;
		}
		if (uval <= (1ULL << 53)) {
			ring_vm_api_retnumber(vm, is_unsigned ? (double)uval : (double)ival);
		} else {
			char buf[32];
			if (is_unsigned)
				snprintf(buf, sizeof(buf), "%llu", (unsigned long long)uval);
			else
				snprintf(buf, sizeof(buf), "%lld", (long long)ival);
			ring_vm_api_retstring(vm, buf);
		}
	} else {
		if (is_ffi_arg) {
			ffi_arg res = *(ffi_arg *)src;
			switch (type->kind) {
			case FFI_KIND_FLOAT:
				ring_vm_api_retnumber(vm, (double)*(float *)src);
				break;
			case FFI_KIND_DOUBLE:
				ring_vm_api_retnumber(vm, *(double *)src);
				break;
			case FFI_KIND_LONGDOUBLE:
				ring_vm_api_retnumber(vm, (double)*(long double *)src);
				break;
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
		} else {
			ring_vm_api_retnumber(vm, ffi_read_typed_value(src, type));
		}
	}
}

void ffi_push_return_value(VM *vm, void *result_ptr, FFI_Type *rtype)
{
	ffi_push_to_ring(vm, result_ptr, rtype, true);
}

void ffi_ret_value(VM *vm, void *src, FFI_Type *type) { ffi_push_to_ring(vm, src, type, false); }

bool ffi_parse_bitfield_tag(const char *tag, FFI_TypeKind *kind, int *bit_off, int *bit_w)
{
	if (!tag || strncmp(tag, FFI_BITFIELD_TYPE_TAG "_", 3) != 0)
		return false;
	int k = 0;
	if (sscanf(tag + 3, "%d_%d_%d", &k, bit_off, bit_w) == 3 && *bit_w > 0) {
		*kind = (FFI_TypeKind)k;
		return true;
	}
	return false;
}

void ffi_read_bitfield(VM *vm, FFI_Context *ctx, void *ptr, FFI_TypeKind bf_kind, int bit_off,
					   int bit_w)
{
	FFI_Type *bf_type = ffi_type_primitive(ctx, bf_kind);
	if (!bf_type) {
		ring_vm_error(vm, "ffi_get: bitfield type unknown");
		return;
	}

	uint64_t raw = 0;
	if (bf_type->size == 1)
		raw = *(uint8_t *)ptr;
	else if (bf_type->size == 2)
		raw = *(uint16_t *)ptr;
	else if (bf_type->size == 4)
		raw = *(uint32_t *)ptr;
	else if (bf_type->size == 8)
		raw = *(uint64_t *)ptr;

	uint64_t mask = (bit_w >= 64) ? ~(uint64_t)0 : ((uint64_t)1 << bit_w) - 1;
	uint64_t val = (raw >> bit_off) & mask;

	if (ffi_is_64bit_int(bf_kind)) {
		char buf[32];
		int is_unsigned = (bf_kind == FFI_KIND_UINT64 || bf_kind == FFI_KIND_ULONGLONG ||
						   (bf_kind == FFI_KIND_SIZE_T && sizeof(size_t) == 8) ||
						   (bf_kind == FFI_KIND_UINTPTR_T && sizeof(uintptr_t) == 8) ||
						   (bf_kind == FFI_KIND_ULONG && sizeof(unsigned long) == 8));
		if (is_unsigned) {
			snprintf(buf, sizeof(buf), "%llu", (unsigned long long)val);
		} else {
			int64_t sval = (int64_t)val;
			if (bit_w < 64 && (val & ((uint64_t)1 << (bit_w - 1))))
				sval = (int64_t)(val | ~mask);
			snprintf(buf, sizeof(buf), "%lld", (long long)sval);
		}
		ring_vm_api_retstring(vm, buf);
	} else {
		ring_vm_api_retnumber(vm, (double)val);
	}
}

void ffi_write_bitfield(VM *vm, FFI_Context *ctx, void *ptr, FFI_TypeKind bf_kind, int bit_off,
						int bit_w, uint64_t new_val)
{
	FFI_Type *bf_type = ffi_type_primitive(ctx, bf_kind);
	if (!bf_type) {
		ring_vm_error(vm, "ffi_set: bitfield type unknown");
		return;
	}

	uint64_t mask = (bit_w >= 64) ? ~(uint64_t)0 : ((uint64_t)1 << bit_w) - 1;
	new_val &= mask;

	uint64_t raw = 0;
	if (bf_type->size == 1)
		raw = *(uint8_t *)ptr;
	else if (bf_type->size == 2)
		raw = *(uint16_t *)ptr;
	else if (bf_type->size == 4)
		raw = *(uint32_t *)ptr;
	else if (bf_type->size == 8)
		raw = *(uint64_t *)ptr;

	raw &= ~(mask << bit_off);
	raw |= (new_val << bit_off);

	if (bf_type->size == 1)
		*(uint8_t *)ptr = (uint8_t)raw;
	else if (bf_type->size == 2)
		*(uint16_t *)ptr = (uint16_t)raw;
	else if (bf_type->size == 4)
		*(uint32_t *)ptr = (uint32_t)raw;
	else if (bf_type->size == 8)
		*(uint64_t *)ptr = (uint64_t)raw;
}
