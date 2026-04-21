/*
 * RingCFFI - Struct, union, and enum definitions with field access
 * Author: Youssef Saeed <youssefelkholey@gmail.com>
 * Copyright (c) 2026
 */

#include "ring_cffi_internal.h"

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

int ffi_struct_add_field(FFI_Context *ctx, FFI_StructType *st, const char *name, FFI_Type *type,
						 size_t bit_width)
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
	field->bit_width = bit_width;

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

int ffi_struct_add_field_full(FFI_Context *ctx, FFI_StructType *st, const char *name,
							  FFI_Type *type, size_t bit_width)
{
	return ffi_struct_add_field(ctx, st, name, type, bit_width);
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
	size_t byte_offset = 0;
	size_t bit_pos = 0;
	size_t max_align = 1;
	size_t current_bf_unit_size = 0;
	int ffi_idx = 0;

	while (field && ffi_idx < count) {
		if (field->bit_width > 0) {
			size_t unit_bits = field->type->size * 8;
			size_t field_align = field->type->alignment;

			if (field_align > max_align)
				max_align = field_align;

			if (bit_pos > 0 && (bit_pos + field->bit_width) > unit_bits) {
				byte_offset += current_bf_unit_size;
				bit_pos = 0;
			}

			if (bit_pos == 0) {
				byte_offset = FFI_ALIGN(byte_offset, field_align);
				current_bf_unit_size = field->type->size;
			}

			field->offset = byte_offset;
			field->size = field->type->size;
			field->bit_offset = bit_pos;
			bit_pos += field->bit_width;

			while (bit_pos >= unit_bits) {
				byte_offset += current_bf_unit_size;
				bit_pos -= unit_bits;
			}

			st->ffi_elements[ffi_idx] = field->type->ffi_type_ptr;
		} else {
			if (bit_pos > 0) {
				byte_offset += current_bf_unit_size;
				bit_pos = 0;
			}

			size_t align = field->type->alignment;
			if (align > max_align)
				max_align = align;

			byte_offset = FFI_ALIGN(byte_offset, align);
			field->offset = byte_offset;
			field->size = field->type->size;
			byte_offset += field->size;

			st->ffi_elements[ffi_idx] = field->type->ffi_type_ptr;
		}
		field = field->next;
		ffi_idx++;
	}
	st->ffi_elements[count] = NULL;

	if (bit_pos > 0) {
		byte_offset += current_bf_unit_size;
		bit_pos = 0;
	}

	st->alignment = max_align;
	st->size = FFI_ALIGN(byte_offset, max_align);

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
	field->bit_width = 0;
	field->bit_offset = 0;

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
				ffi_struct_add_field(ctx, st, field_name, type, 0);
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

	const char *field_path = RING_API_GETSTRING(3);

	char path_buf[256];
	strncpy(path_buf, field_path, sizeof(path_buf) - 1);
	path_buf[sizeof(path_buf) - 1] = '\0';

	void *cur_ptr = ptr;
	FFI_Type *cur_type = type;
	char *segment = path_buf;
	char *dot;

	while (segment && *segment) {
		dot = strchr(segment, '.');
		if (dot)
			*dot = '\0';

		if (cur_type->kind == FFI_KIND_STRUCT) {
			FFI_StructType *st = cur_type->info.struct_type;
			FFI_StructField *field = ffi_struct_find_field(st, segment);
			if (!field) {
				RING_API_ERROR("ffi_field: field not found in struct");
				return;
			}
			if (field->bit_width > 0) {
				void *field_ptr = (char *)cur_ptr + field->offset;
				char tag[64];
				snprintf(tag, sizeof(tag), "%s_%d_%d_%d", FFI_BITFIELD_TYPE_TAG,
						 (int)field->type->kind, (int)field->bit_offset, (int)field->bit_width);
				RING_API_RETCPOINTER(field_ptr, tag);
				return;
			}
			cur_ptr = (char *)cur_ptr + field->offset;
			cur_type = field->type;
		} else if (cur_type->kind == FFI_KIND_UNION) {
			FFI_UnionType *ut = cur_type->info.union_type;
			FFI_StructField *ufield = ffi_union_find_field(ut, segment);
			if (!ufield) {
				RING_API_ERROR("ffi_field: field not found in union");
				return;
			}
			if (ufield->bit_width > 0) {
				char tag[64];
				snprintf(tag, sizeof(tag), "%s_%d_%d_%d", FFI_BITFIELD_TYPE_TAG,
						 (int)ufield->type->kind, (int)ufield->bit_offset, (int)ufield->bit_width);
				RING_API_RETCPOINTER(cur_ptr, tag);
				return;
			}
			cur_type = ufield->type;
		} else {
			if (!dot) {
				RING_API_RETCPOINTER(cur_ptr, "FFI_Ptr");
				return;
			}
			RING_API_ERROR("ffi_field: intermediate field is not a struct or union");
			return;
		}

		segment = dot ? dot + 1 : NULL;
	}

	RING_API_RETCPOINTER(cur_ptr, "FFI_Ptr");
}

RING_FUNC(ring_cffi_field_offset)
{
	if (RING_API_PARACOUNT < 2) {
		RING_API_ERROR("ffi_field_offset(type, fieldname) requires 2 parameters");
		return;
	}

	if (!RING_API_ISCPOINTER(1) || !RING_API_ISSTRING(2)) {
		RING_API_ERROR("ffi_field_offset: invalid parameters");
		return;
	}

	List *pList = RING_API_GETLIST(1);
	FFI_Type *type = (FFI_Type *)ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	if (!type) {
		RING_API_ERROR("ffi_field_offset: first parameter must be a struct or union type");
		return;
	}

	const char *field_path = RING_API_GETSTRING(2);

	char path_buf[256];
	strncpy(path_buf, field_path, sizeof(path_buf) - 1);
	path_buf[sizeof(path_buf) - 1] = '\0';

	FFI_Type *cur_type = type;
	size_t cumulative_offset = 0;
	char *segment = path_buf;
	char *dot;

	while (segment && *segment) {
		dot = strchr(segment, '.');
		if (dot)
			*dot = '\0';

		if (cur_type->kind == FFI_KIND_STRUCT) {
			FFI_StructType *st = cur_type->info.struct_type;
			FFI_StructField *field = ffi_struct_find_field(st, segment);
			if (!field) {
				RING_API_RETNUMBER(-1);
				return;
			}
			cumulative_offset += field->offset;
			cur_type = field->type;
		} else if (cur_type->kind == FFI_KIND_UNION) {
			FFI_UnionType *ut = cur_type->info.union_type;
			FFI_StructField *ufield = ffi_union_find_field(ut, segment);
			if (!ufield) {
				RING_API_RETNUMBER(-1);
				return;
			}
			cur_type = ufield->type;
		} else {
			RING_API_RETNUMBER(-1);
			return;
		}

		segment = dot ? dot + 1 : NULL;
	}

	RING_API_RETNUMBER((double)cumulative_offset);
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
