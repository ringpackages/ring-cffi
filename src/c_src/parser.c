/*
 * RingCFFI - C definition parser for cdef() and bind() declarations
 * Author: Youssef Saeed <youssefelkholey@gmail.com>
 * Copyright (c) 2026
 */

#include "ring_cffi_internal.h"

static void cparser_init(CParser *p, FFI_Context *ctx, FFI_Library *lib, const char *src)
{
	p->ctx = ctx;
	p->lib = lib;
	p->src = ring_state_malloc(ctx->ring_state, strlen(src) + 1);
	if (p->src)
		strcpy(p->src, src);
	p->pos = p->src;
	p->error[0] = '\0';
	p->result_list = ring_list_new_gc(ctx->ring_state, 0);
	p->decl_count = 0;
}

static void cparser_free(CParser *p)
{
	if (p->src)
		ring_state_free(p->ctx->ring_state, p->src);
	if (p->result_list)
		ring_list_delete_gc(p->ctx->ring_state, p->result_list);
}

static void cparser_skip_ws(CParser *p)
{
	while (*p->pos) {
		if (isspace(*p->pos)) {
			p->pos++;
		} else if (p->pos[0] == '/' && p->pos[1] == '/') {
			while (*p->pos && *p->pos != '\n')
				p->pos++;
		} else if (p->pos[0] == '/' && p->pos[1] == '*') {
			p->pos += 2;
			while (*p->pos && !(p->pos[0] == '*' && p->pos[1] == '/'))
				p->pos++;
			if (*p->pos)
				p->pos += 2;
		} else {
			break;
		}
	}
}

static bool cparser_match(CParser *p, const char *s)
{
	cparser_skip_ws(p);
	size_t len = strlen(s);
	if (strncmp(p->pos, s, len) == 0 && !isalnum(p->pos[len]) && p->pos[len] != '_') {
		p->pos += len;
		return true;
	}
	return false;
}

static bool cparser_match_exact(CParser *p, const char *s)
{
	cparser_skip_ws(p);
	size_t len = strlen(s);
	if (strncmp(p->pos, s, len) == 0) {
		p->pos += len;
		return true;
	}
	return false;
}

static bool cparser_peek(CParser *p, const char *s)
{
	cparser_skip_ws(p);
	size_t len = strlen(s);
	return strncmp(p->pos, s, len) == 0;
}

static bool cparser_ident(CParser *p, char *out, size_t sz)
{
	cparser_skip_ws(p);
	if (!isalpha(*p->pos) && *p->pos != '_')
		return false;
	size_t i = 0;
	while ((isalnum(*p->pos) || *p->pos == '_') && i < sz - 1) {
		out[i++] = *p->pos++;
	}
	out[i] = '\0';
	if (isalnum(*p->pos) || *p->pos == '_') {
		ffi_set_error(p->ctx, "Identifier too long (max %zu)", sz - 1);
		return false;
	}
	return true;
}

static bool cparser_number(CParser *p, int64_t *val)
{
	cparser_skip_ws(p);
	if (!isdigit(*p->pos) && *p->pos != '-' && *p->pos != '+')
		return false;
	char *end;
	*val = strtoll(p->pos, &end, 0);
	if (end == p->pos)
		return false;
	p->pos = end;
	return true;
}

static void cparser_skip_attributes(CParser *p)
{
	while (true) {
		cparser_skip_ws(p);

		if (cparser_match(p, "const") || cparser_match(p, "volatile") ||
			cparser_match(p, "restrict") || cparser_match(p, "__restrict") ||
			cparser_match(p, "extern") || cparser_match(p, "static") ||
			cparser_match(p, "inline") || cparser_match(p, "__inline") ||
			cparser_match(p, "__inline__") || cparser_match(p, "__forceinline") ||
			cparser_match(p, "__stdcall") || cparser_match(p, "_stdcall") ||
			cparser_match(p, "__cdecl") || cparser_match(p, "_cdecl") ||
			cparser_match(p, "__fastcall") || cparser_match(p, "_fastcall") ||
			cparser_match(p, "__thiscall") || cparser_match(p, "_thiscall") ||
			cparser_match(p, "__ptr32") || cparser_match(p, "__ptr64") ||
			cparser_match(p, "WINAPI") || cparser_match(p, "APIENTRY") ||
			cparser_match(p, "CALLBACK") || cparser_match(p, "WINAPIV")) {
			continue;
		}

		if (cparser_match(p, "__declspec") || cparser_match(p, "__attribute__")) {
			cparser_skip_ws(p);
			if (*p->pos == '(') {
				int depth = 0;
				do {
					if (*p->pos == '(')
						depth++;
					else if (*p->pos == ')')
						depth--;
					else if (*p->pos == '\0')
						break;
					p->pos++;
				} while (depth > 0);
			}
			continue;
		}
		break;
	}
}

static void cparser_type(CParser *p, char *out, size_t sz)
{
	cparser_skip_attributes(p);
	size_t i = 0;
	out[0] = '\0';

	bool has_signed = false, has_unsigned = false;
	int long_count = 0;
	bool has_short = false;
	bool has_struct = false, has_union = false, has_enum = false;
	char base_type[512] = "";

	while (true) {
		cparser_skip_ws(p);
		if (cparser_match(p, "signed")) {
			has_signed = true;
			continue;
		}
		if (cparser_match(p, "unsigned")) {
			has_unsigned = true;
			continue;
		}
		if (cparser_match(p, "long")) {
			long_count++;
			continue;
		}
		if (cparser_match(p, "short")) {
			has_short = true;
			continue;
		}
		if (cparser_match(p, "struct")) {
			has_struct = true;
			continue;
		}
		if (cparser_match(p, "union")) {
			has_union = true;
			continue;
		}
		if (cparser_match(p, "enum")) {
			has_enum = true;
			continue;
		}

		char *save = p->pos;
		cparser_skip_attributes(p);
		if (save != p->pos)
			continue;

		break;
	}

	cparser_skip_attributes(p);

	bool has_modifier = has_signed || has_unsigned || long_count > 0 || has_short;
	bool has_tag = has_struct || has_union || has_enum;

	char peek_ident[512] = "";
	char *save_pos = p->pos;
	if (isalpha(*p->pos) || *p->pos == '_') {
		cparser_ident(p, peek_ident, sizeof(peek_ident));
	}

	bool consume_ident = false;
	if (has_tag) {
		consume_ident = true;
	} else if (has_modifier) {
		if (strcmp(peek_ident, "int") == 0 || strcmp(peek_ident, "char") == 0 ||
			strcmp(peek_ident, "double") == 0) {
			consume_ident = true;
		}
	} else {
		consume_ident = true;
	}

	if (consume_ident) {
		strcpy(base_type, peek_ident);
	} else {
		p->pos = save_pos;
	}

	if (has_enum) {
		snprintf(out, sz, "int");
	} else if (long_count >= 2) {
		snprintf(out, sz, has_unsigned ? "unsigned long long" : "long long");
	} else if (long_count == 1 && strcmp(base_type, "double") == 0) {
		snprintf(out, sz, "long double");
	} else if (long_count == 1) {
		snprintf(out, sz, has_unsigned ? "unsigned long" : "long");
	} else if (has_short) {
		snprintf(out, sz, has_unsigned ? "unsigned short" : "short");
	} else if (base_type[0]) {
		if (has_unsigned && (strcmp(base_type, "char") == 0 || strcmp(base_type, "int") == 0)) {
			snprintf(out, sz, "unsigned %s", base_type);
		} else if (has_signed && strcmp(base_type, "char") == 0) {
			snprintf(out, sz, "signed char");
		} else {
			strncpy(out, base_type, sz - 1);
			out[sz - 1] = '\0';
		}
	} else if (has_unsigned) {
		snprintf(out, sz, "unsigned int");
	} else if (has_signed) {
		snprintf(out, sz, "int");
	}

	cparser_skip_attributes(p);
	i = strlen(out);
	while (*p->pos == '*') {
		if (i < sz - 1) {
			out[i++] = '*';
		} else {
			snprintf(p->error, sizeof(p->error), "Type too long (pointer depth too high)");
			break;
		}
		p->pos++;
		cparser_skip_attributes(p);
	}
	out[i] = '\0';

	if ((has_struct || has_union) && strchr(out, '*')) {
		snprintf(out, sz, "ptr");
	}
}

static bool cparser_parse_struct(CParser *p, bool is_union)
{
	char name[512] = "";
	cparser_skip_ws(p);
	if (isalpha(*p->pos) || *p->pos == '_') {
		cparser_ident(p, name, sizeof(name));
	}

	cparser_skip_ws(p);
	if (!cparser_match_exact(p, "{")) {
		return false;
	}

	List *fields = ring_list_new_gc(p->ctx->ring_state, 0);

	while (!cparser_peek(p, "}")) {
		cparser_skip_ws(p);
		if (*p->pos == '\0')
			break;

		char field_type[512] = "";
		cparser_type(p, field_type, sizeof(field_type));
		if (!field_type[0])
			break;

		char field_name[512] = "";
		cparser_skip_ws(p);

		if (*p->pos == '(') {
			p->pos++;
			cparser_skip_attributes(p);
			cparser_skip_ws(p);
			while (*p->pos == '*')
				p->pos++;
			cparser_ident(p, field_name, sizeof(field_name));
			while (*p->pos && *p->pos != ')')
				p->pos++;
			if (*p->pos == ')')
				p->pos++;
			while (*p->pos && *p->pos != ';')
				p->pos++;
			if (*p->pos == ';')
				p->pos++;
			if (strlen(field_type) < sizeof(field_type) - 2)
				strcat(field_type, "*");
		} else {
			cparser_ident(p, field_name, sizeof(field_name));

			cparser_skip_ws(p);
			while (*p->pos == '[') {
				while (*p->pos && *p->pos != ']')
					p->pos++;
				if (*p->pos == ']')
					p->pos++;
				if (strlen(field_type) < sizeof(field_type) - 2)
					strcat(field_type, "*");
			}

			cparser_skip_ws(p);
			if (*p->pos == ':') {
				p->pos++;
				int64_t bits;
				cparser_number(p, &bits);
				if (bits > 0) {
					if (strlen(field_type) + 16 < sizeof(field_type)) {
						char bw[32];
						snprintf(bw, sizeof(bw), ":%lld", (long long)bits);
						strcat(field_type, bw);
					}
				}
			}

			cparser_skip_ws(p);
			if (*p->pos == ';')
				p->pos++;
		}

		if (field_name[0] && field_type[0]) {
			List *field = ring_list_newlist_gc(p->ctx->ring_state, fields);
			ring_list_addstring_gc(p->ctx->ring_state, field, field_name);
			ring_list_addstring_gc(p->ctx->ring_state, field, field_type);
		}
	}

	cparser_match_exact(p, "}");
	cparser_skip_ws(p);

	if (!name[0] && (isalpha(*p->pos) || *p->pos == '_')) {
		cparser_ident(p, name, sizeof(name));
	}

	cparser_match_exact(p, ";");

	if (name[0]) {
		if (is_union) {
			FFI_UnionType *ut =
				(FFI_UnionType *)ring_state_malloc(p->ctx->ring_state, sizeof(FFI_UnionType));
			memset(ut, 0, sizeof(FFI_UnionType));
			ring_list_addcustomringpointer_gc(p->ctx->ring_state, p->ctx->gc_list, ut,
											  ffi_gc_free_union_type);
			ut->name = ring_state_malloc(p->ctx->ring_state, strlen(name) + 1);
			if (ut->name)
				strcpy(ut->name, name);

			int fcount = ring_list_getsize(fields);
			for (int i = 1; i <= fcount; i++) {
				List *fdef = ring_list_getlist(fields, i);
				const char *fname = ring_list_getstring(fdef, 1);
				const char *ftype = ring_list_getstring(fdef, 2);
				FFI_Type *type = ffi_type_parse(p->ctx, ftype);
				if (type) {
					ffi_union_add_field(p->ctx, ut, fname, type);
				}
			}
			ut->size = FFI_ALIGN(ut->size, ut->alignment);
			ring_hashtable_newpointer_gc(p->ctx->ring_state, p->ctx->unions, name, ut);
			ring_list_addpointer_gc(p->ctx->ring_state, p->result_list, ut);
			p->decl_count++;
		} else {
			FFI_StructType *st = ffi_struct_define(p->ctx, name);
			int fcount = ring_list_getsize(fields);
			for (int i = 1; i <= fcount; i++) {
				List *fdef = ring_list_getlist(fields, i);
				const char *fname = ring_list_getstring(fdef, 1);
				const char *ftype_raw = ring_list_getstring(fdef, 2);
				size_t bw = 0;
				char ftype_buf[512];
				strncpy(ftype_buf, ftype_raw, sizeof(ftype_buf) - 1);
				ftype_buf[sizeof(ftype_buf) - 1] = '\0';
				char *colon = strchr(ftype_buf, ':');
				if (colon) {
					bw = (size_t)strtoull(colon + 1, NULL, 10);
					*colon = '\0';
				}
				FFI_Type *type = ffi_type_parse(p->ctx, ftype_buf);
				if (type) {
					ffi_struct_add_field(p->ctx, st, fname, type, bw);
				}
			}
			ffi_struct_finalize(p->ctx, st);
			ring_list_addpointer_gc(p->ctx->ring_state, p->result_list, st);
			p->decl_count++;
		}
	}

	return true;
}

static bool cparser_parse_enum(CParser *p)
{
	char name[512] = "";
	cparser_skip_ws(p);
	if (isalpha(*p->pos) || *p->pos == '_') {
		cparser_ident(p, name, sizeof(name));
	}

	cparser_skip_ws(p);
	if (!cparser_match_exact(p, "{"))
		return false;

	List *consts = ring_list_new_gc(p->ctx->ring_state, 0);
	int64_t next_val = 0;

	while (!cparser_peek(p, "}")) {
		cparser_skip_ws(p);
		if (*p->pos == '\0')
			break;

		char const_name[512];
		if (!cparser_ident(p, const_name, sizeof(const_name)))
			break;

		int64_t val = next_val;
		cparser_skip_ws(p);
		if (cparser_match_exact(p, "=")) {
			cparser_number(p, &val);
		}

		List *c = ring_list_newlist_gc(p->ctx->ring_state, consts);
		ring_list_addstring_gc(p->ctx->ring_state, c, const_name);
		ring_list_adddouble_gc(p->ctx->ring_state, c, (double)val);

		next_val = val + 1;
		cparser_skip_ws(p);
		cparser_match_exact(p, ",");
	}

	cparser_match_exact(p, "}");
	cparser_skip_ws(p);

	if (!name[0] && (isalpha(*p->pos) || *p->pos == '_')) {
		cparser_ident(p, name, sizeof(name));
	}

	cparser_match_exact(p, ";");

	if (name[0]) {
		FFI_EnumType *et =
			(FFI_EnumType *)ring_state_malloc(p->ctx->ring_state, sizeof(FFI_EnumType));
		memset(et, 0, sizeof(FFI_EnumType));
		ring_list_addcustomringpointer_gc(p->ctx->ring_state, p->ctx->gc_list, et,
										  ffi_gc_free_enum);
		et->name = ring_state_malloc(p->ctx->ring_state, strlen(name) + 1);
		if (et->name)
			strcpy(et->name, name);

		int ccount = ring_list_getsize(consts);
		for (int i = 1; i <= ccount; i++) {
			List *cdef = ring_list_getlist(consts, i);
			const char *cname = ring_list_getstring(cdef, 1);
			int64_t cval = (int64_t)ring_list_getdouble(cdef, 2);

			FFI_EnumConst *ec =
				(FFI_EnumConst *)ring_state_malloc(p->ctx->ring_state, sizeof(FFI_EnumConst));
			memset(ec, 0, sizeof(FFI_EnumConst));
			ec->name = ring_state_malloc(p->ctx->ring_state, strlen(cname) + 1);
			if (ec->name)
				strcpy(ec->name, cname);
			ec->value = cval;

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

		ring_hashtable_newpointer_gc(p->ctx->ring_state, p->ctx->enums, name, et);
		ring_list_addpointer_gc(p->ctx->ring_state, p->result_list, et);
		p->decl_count++;
	}

	return true;
}

static bool cparser_parse_typedef(CParser *p)
{
	char *save = p->pos;

	if (cparser_match(p, "struct")) {
		if (cparser_peek(p, "{") || (isalpha(*p->pos) || *p->pos == '_')) {
			char sname[512] = "";
			if (!cparser_peek(p, "{")) {
				cparser_ident(p, sname, sizeof(sname));
			}
			if (cparser_peek(p, "{")) {
				p->pos = save;
				cparser_match(p, "struct");
				cparser_parse_struct(p, false);
				return true;
			}
		}
	}

	if (cparser_match(p, "union")) {
		if (cparser_peek(p, "{") || (isalpha(*p->pos) || *p->pos == '_')) {
			char uname[512] = "";
			if (!cparser_peek(p, "{")) {
				cparser_ident(p, uname, sizeof(uname));
			}
			if (cparser_peek(p, "{")) {
				p->pos = save;
				cparser_match(p, "union");
				cparser_parse_struct(p, true);
				return true;
			}
		}
	}

	if (cparser_match(p, "enum")) {
		if (cparser_peek(p, "{") || (isalpha(*p->pos) || *p->pos == '_')) {
			char ename[512] = "";
			if (!cparser_peek(p, "{")) {
				cparser_ident(p, ename, sizeof(ename));
			}
			if (cparser_peek(p, "{")) {
				p->pos = save;
				cparser_match(p, "enum");
				cparser_parse_enum(p);
				return true;
			}
		}
	}

	p->pos = save;

	char base_type[512];
	cparser_type(p, base_type, sizeof(base_type));
	if (!base_type[0])
		return false;

	cparser_skip_ws(p);

	if (*p->pos == '(') {
		p->pos++;
		cparser_skip_attributes(p);
		while (*p->pos == '*') {
			if (strlen(base_type) < sizeof(base_type) - 2)
				strcat(base_type, "*");
			p->pos++;
			cparser_skip_attributes(p);
		}
		char alias[512];
		cparser_ident(p, alias, sizeof(alias));
		while (*p->pos && *p->pos != ')')
			p->pos++;
		if (*p->pos == ')')
			p->pos++;
		while (*p->pos && *p->pos != ';')
			p->pos++;
		if (*p->pos == ';')
			p->pos++;

		FFI_Type *t = ffi_type_parse(p->ctx, base_type);
		if (t) {
			ring_hashtable_newpointer_gc(p->ctx->ring_state, p->ctx->type_cache, alias, t);
		}
		p->decl_count++;
		return true;
	}

	char alias[512];
	if (!cparser_ident(p, alias, sizeof(alias)))
		return false;

	cparser_skip_ws(p);
	while (*p->pos == '[') {
		while (*p->pos && *p->pos != ']')
			p->pos++;
		if (*p->pos == ']')
			p->pos++;
		if (strlen(base_type) < sizeof(base_type) - 2)
			strcat(base_type, "*");
	}

	cparser_match_exact(p, ";");

	FFI_Type *t = ffi_type_parse(p->ctx, base_type);
	if (t) {
		ring_hashtable_newpointer_gc(p->ctx->ring_state, p->ctx->type_cache, alias, t);
	}
	p->decl_count++;

	return true;
}

static bool cparser_parse_function(CParser *p)
{
	cparser_skip_attributes(p);

	char ret_type[512];
	cparser_type(p, ret_type, sizeof(ret_type));
	if (!ret_type[0])
		return false;

	cparser_skip_attributes(p);

	if (*p->pos == '(') {
		p->pos++;
		cparser_skip_attributes(p);
		while (*p->pos == '*') {
			if (strlen(ret_type) < sizeof(ret_type) - 2)
				strcat(ret_type, "*");
			p->pos++;
			cparser_skip_attributes(p);
		}
		char func_name[512];
		cparser_ident(p, func_name, sizeof(func_name));
		while (*p->pos && *p->pos != ')')
			p->pos++;
		if (*p->pos == ')')
			p->pos++;
		cparser_skip_ws(p);
	}

	char func_name[512];
	if (!cparser_ident(p, func_name, sizeof(func_name))) {
		return false;
	}

	cparser_skip_ws(p);
	if (*p->pos != '(')
		return false;
	p->pos++;

	FFI_Type *ret_ffi = ffi_type_parse(p->ctx, ret_type);
	if (!ret_ffi)
		return false;

	FFI_Type *params[64];
	int param_count = 0;
	bool is_variadic = false;

	cparser_skip_ws(p);
	if (cparser_match(p, "void")) {
		cparser_skip_ws(p);
		if (*p->pos == ')') {
			p->pos++;
			goto finish_func;
		}
		p->pos -= 4;
	}

	while (*p->pos && *p->pos != ')') {
		cparser_skip_ws(p);

		if (cparser_match_exact(p, "...")) {
			is_variadic = true;
			break;
		}

		char ptype[512];
		cparser_type(p, ptype, sizeof(ptype));

		cparser_skip_ws(p);
		if (*p->pos == '(') {
			p->pos++;
			cparser_skip_attributes(p);
			while (*p->pos == '*')
				p->pos++;
			char pname[512];
			cparser_ident(p, pname, sizeof(pname));
			while (*p->pos && *p->pos != ')')
				p->pos++;
			if (*p->pos == ')')
				p->pos++;
			cparser_skip_ws(p);
			if (*p->pos == '(') {
				while (*p->pos && *p->pos != ')')
					p->pos++;
				if (*p->pos == ')')
					p->pos++;
			}
			strcpy(ptype, "void*");
		} else if (isalpha(*p->pos) || *p->pos == '_') {
			char pname[512];
			cparser_ident(p, pname, sizeof(pname));
		}

		cparser_skip_ws(p);
		while (*p->pos == '[') {
			while (*p->pos && *p->pos != ']')
				p->pos++;
			if (*p->pos == ']')
				p->pos++;
			if (strlen(ptype) < sizeof(ptype) - 2)
				strcat(ptype, "*");
		}

		if (ptype[0] && param_count < 64) {
			params[param_count] = ffi_type_parse(p->ctx, ptype);
			if (params[param_count])
				param_count++;
		}

		cparser_skip_ws(p);
		cparser_match_exact(p, ",");
	}

	cparser_match_exact(p, ")");

finish_func:
	cparser_skip_ws(p);
	cparser_match_exact(p, ";");

	if (!p->lib) {
		p->decl_count++;
		return true;
	}

	void *fptr = ffi_library_symbol(p->lib, func_name);
	if (!fptr) {
		p->decl_count++;
		return true;
	}

	if (is_variadic) {
		FFI_Function *func =
			(FFI_Function *)ring_state_malloc(p->ctx->ring_state, sizeof(FFI_Function));
		memset(func, 0, sizeof(FFI_Function));
		func->func_ptr = fptr;

		FFI_FuncType *ft =
			(FFI_FuncType *)ring_state_malloc(p->ctx->ring_state, sizeof(FFI_FuncType));
		memset(ft, 0, sizeof(FFI_FuncType));
		ft->return_type = ret_ffi;
		ft->param_count = param_count;
		ft->is_variadic = true;
		if (param_count > 0) {
			ft->param_types = (FFI_Type **)ring_state_malloc(p->ctx->ring_state,
															 sizeof(FFI_Type *) * param_count);
			for (int i = 0; i < param_count; i++)
				ft->param_types[i] = params[i];
		}
		func->type = ft;

		for (int i = 0; func_name[i]; i++)
			func_name[i] = tolower((unsigned char)func_name[i]);
		cdef_funcs_set(p->ctx, func_name, func);

		ring_list_addpointer_gc(p->ctx->ring_state, p->result_list, func);
		ring_list_addcustomringpointer_gc(p->ctx->ring_state, p->ctx->gc_list, func,
										  ffi_gc_free_func);
		p->decl_count++;
	} else {
		FFI_Type **pcopy = NULL;
		if (param_count > 0) {
			pcopy = (FFI_Type **)ring_state_malloc(p->ctx->ring_state,
												   sizeof(FFI_Type *) * param_count);
			for (int i = 0; i < param_count; i++)
				pcopy[i] = params[i];
		}
		FFI_Function *func = ffi_function_create(p->ctx, fptr, ret_ffi, pcopy, param_count);
		if (func) {
			ring_list_addpointer_gc(p->ctx->ring_state, p->result_list, func);
			ring_list_addcustomringpointer_gc(p->ctx->ring_state, p->ctx->gc_list, func,
											  ffi_gc_free_func);
			for (int i = 0; func_name[i]; i++)
				func_name[i] = tolower((unsigned char)func_name[i]);
			cdef_funcs_set(p->ctx, func_name, func);
			p->decl_count++;
		} else {
			p->decl_count++;
			if (pcopy)
				ring_state_free(p->ctx->ring_state, pcopy);
		}
	}

	return true;
}

static void cparser_parse(CParser *p)
{
	p->ctx->error_msg[0] = '\0';
	while (*p->pos) {
		cparser_skip_attributes(p);
		if (!*p->pos)
			break;

		char *save = p->pos;

		if (cparser_match(p, "typedef")) {
			if (!cparser_parse_typedef(p)) {
				if (p->ctx->error_msg[0] != '\0')
					break;
				while (*p->pos && *p->pos != ';')
					p->pos++;
				if (*p->pos == ';')
					p->pos++;
			}
			continue;
		}

		if (cparser_match(p, "struct")) {
			if (cparser_peek(p, "{") || (isalpha(*p->pos) || *p->pos == '_')) {
				char sname[512] = "";
				char *before_name = p->pos;
				if (!cparser_peek(p, "{")) {
					cparser_ident(p, sname, sizeof(sname));
				}
				if (p->ctx->error_msg[0] != '\0')
					break;
				if (cparser_peek(p, "{")) {
					p->pos = before_name;
					cparser_parse_struct(p, false);
					continue;
				}
			}
			p->pos = save;
		}

		if (cparser_match(p, "union")) {
			if (cparser_peek(p, "{") || (isalpha(*p->pos) || *p->pos == '_')) {
				char uname[512] = "";
				char *before_name = p->pos;
				if (!cparser_peek(p, "{")) {
					cparser_ident(p, uname, sizeof(uname));
				}
				if (p->ctx->error_msg[0] != '\0')
					break;
				if (cparser_peek(p, "{")) {
					p->pos = before_name;
					cparser_parse_struct(p, true);
					continue;
				}
			}
			p->pos = save;
		}

		if (cparser_match(p, "enum")) {
			if (cparser_peek(p, "{") || (isalpha(*p->pos) || *p->pos == '_')) {
				char ename[512] = "";
				char *before_name = p->pos;
				if (!cparser_peek(p, "{")) {
					cparser_ident(p, ename, sizeof(ename));
				}
				if (p->ctx->error_msg[0] != '\0')
					break;
				if (cparser_peek(p, "{")) {
					p->pos = before_name;
					cparser_parse_enum(p);
					continue;
				}
			}
			p->pos = save;
		}

		p->pos = save;
		if (!cparser_parse_function(p)) {
			if (p->ctx->error_msg[0] != '\0')
				break;
			while (*p->pos && *p->pos != ';' && *p->pos != '}')
				p->pos++;
			if (*p->pos == ';')
				p->pos++;
			if (*p->pos == '}')
				p->pos++;
		}
	}
}

RING_FUNC(ring_cffi_cdef)
{
	if (RING_API_PARACOUNT < 2) {
		RING_API_ERROR("ffi_cdef(lib, declarations) requires 2 parameters");
		return;
	}

	if (!RING_API_ISSTRING(2)) {
		RING_API_ERROR("ffi_cdef: declarations must be a string");
		return;
	}

	FFI_Context *ctx = get_or_create_context(pPointer);
	FFI_Library *lib = NULL;

	if (RING_API_ISCPOINTER(1)) {
		List *pList = RING_API_GETLIST(1);
		lib = (FFI_Library *)ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	}

	const char *decl = RING_API_GETSTRING(2);

	CParser parser;
	cparser_init(&parser, ctx, lib, decl);
	cparser_parse(&parser);

	int count = parser.decl_count;

	cparser_free(&parser);

	RING_API_RETNUMBER(count);
}

RING_FUNC(ring_cffi_bind)
{
	FFI_Context *ctx = get_or_create_context(pPointer);
	VM *vm = (VM *)pPointer;

	if (RING_API_PARACOUNT == 0) {
		if (!ctx || !ctx->cdef_funcs) {
			RING_API_ERROR("ffi_bind: no FFI context or no cdef declarations");
			return;
		}
		int count = 0;
		for (unsigned int bucket = 0; bucket < ctx->cdef_funcs->nLinkedLists; bucket++) {
			HashItem *item = ctx->cdef_funcs->pArray[bucket];
			while (item) {
				if (item->nItemType == RING_HASHITEMTYPE_POINTER && item->cKey) {
					FFI_Function *func = (FFI_Function *)item->HashValue.pValue;
					if (func && (func->cif_prepared || (func->type && func->type->is_variadic))) {
						void *trampoline = ffi_create_trampoline(ctx, func);
						if (trampoline) {
							char *lower_name = ffi_lowerdup(ctx, item->cKey);
							ring_vm_funcregister2(vm->pRingState, lower_name,
												  (void (*)(void *))trampoline);
							count++;
						}
					}
				}
				item = item->pNext;
			}
		}
		RING_API_RETNUMBER(count);
		return;
	}

	if (RING_API_PARACOUNT < 3) {
		RING_API_ERROR("ffi_bind(lib, name, rettype [, argtypes_list]) requires "
					   "at least 3 parameters, or 0 to bind all cdef functions");
		return;
	}

	if (!RING_API_ISCPOINTER(1)) {
		RING_API_ERROR("ffi_bind: first parameter must be a library handle");
		return;
	}
	if (!RING_API_ISSTRING(2) || !RING_API_ISSTRING(3)) {
		RING_API_ERROR("ffi_bind: name and return type must be strings");
		return;
	}

	List *pList = RING_API_GETLIST(1);
	FFI_Library *lib = (FFI_Library *)ring_list_getpointer(pList, RING_CPOINTER_POINTER);
	if (!lib) {
		RING_API_ERROR("ffi_bind: invalid library handle");
		return;
	}

	const char *func_name = RING_API_GETSTRING(2);
	const char *ret_type_str = RING_API_GETSTRING(3);

	FFI_Type *ret_type = ffi_type_parse(ctx, ret_type_str);
	if (!ret_type) {
		RING_API_ERROR("ffi_bind: unknown return type");
		return;
	}

	int param_count = 0;
	FFI_Type **param_types = NULL;

	if (RING_API_PARACOUNT >= 4 && RING_API_ISLIST(4)) {
		List *argTypes = RING_API_GETLIST(4);
		param_types = parse_type_list(ctx, argTypes, &param_count);
		if (!param_types && param_count < 0) {
			RING_API_ERROR("ffi_bind: parameter types must be valid strings");
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

	ring_list_addcustomringpointer_gc(ctx->ring_state, ctx->gc_list, func, ffi_gc_free_func);

	char *lower_name = ffi_lowerdup(ctx, func_name);
	if (!lower_name) {
		RING_API_ERROR("ffi_bind: out of memory");
		return;
	}

	cdef_funcs_set(ctx, lower_name, func);

	void *trampoline = ffi_create_trampoline(ctx, func);
	if (!trampoline) {
		RING_API_ERROR("ffi_bind: out of memory creating trampoline");
		return;
	}

	ring_vm_funcregister2(vm->pRingState, lower_name, (void (*)(void *))trampoline);

	RING_API_RETNUMBER(1);
}
