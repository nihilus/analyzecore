#include "helpers.hpp"

//
// loc: ea  of type loc_t
// loc_t: type of loc
// struct_name: name of structure element containing a ea of the structure name
// member_name: name of the structure element to be named
// prefix: prefix to add to member_name

bgcolor_t 
bgcolor(unsigned char blue, unsigned char green, unsigned char red)
{
	return bgcolor_t( (blue<<16) + (green<<8) + red);
}


bool make_struct_name(ea_t loc, tid_t loc_t, const char *struct_name, const char *member_name, const char *prefix)
{
	if (loc == BADADDR) { return false; }
	ea_t member_loc = struct_member_ea_ref(loc, loc_t, member_name);
	if (member_loc == BADADDR) { return false;};
	ea_t struct_name_loc = struct_member_ea_ref(loc, loc_t, struct_name);
	if (struct_name_loc == BADADDR) { return false;};

	size_t len = get_max_ascii_length(struct_name_loc, ASCSTR_C);
	int pr_len;
	char buf[1024];

	if (len!=0) {
		char *name_type = new char[len+1];

		get_ascii_contents(struct_name_loc, len, ASCSTR_C, name_type, len);
		pr_len = qsnprintf(buf, 1023, "%s_%s_%s", prefix, name_type, member_name);

		delete[] name_type;
	} else {
		pr_len = qsnprintf(buf, 1023, "%s_%08x_%s", prefix, struct_name_loc, member_name);
	}

	len = strlen(buf);
	for(int i=0;i<pr_len;i++) {
		if ( 
			(buf[i]=='<') ||
			(buf[i]=='>') ||
			(buf[i]=='-') 
			){
				buf[i] = '_';
		}
	}
	
	return set_name(member_loc, buf, SN_AUTO);
}

bool	make_struct(ea_t ea, size_t size, const char *strname)
{
	tid_t idx = get_struc_id(strname);
	if (size == -1) {
		size = get_struc_size(get_struc(idx));
	}
	do_unknown_range(ea, size, DOUNK_SIMPLE);
	return doStruct(ea, size, idx);
}

ea_t	struct_member_ea_val(ea_t loc, tid_t tid, const char* name)
{
	return loc + get_member_by_name(get_struc(tid), name)->soff;
}

ea_t	struct_member_ea_ref(ea_t loc, tid_t tid, const char *name)
{
	member_t *member = get_member_by_name(get_struc(tid), name);
	if (member) {
		return ea_t(get_full_long( loc + member->get_soff()));
	} else {
		return BADADDR;
	}
}

ea_t	struct_member_ea_ref_byte(ea_t loc, tid_t tid, const char *name)
{
	member_t *member = get_member_by_name(get_struc(tid), name);
	if (member) {
		return ea_t(get_full_byte( loc + member->get_soff()));
	} else {
		return BADADDR;
	}
}

ea_t	struct_member_ea_ref_word(ea_t loc, tid_t tid, const char *name)
{
	member_t *member = get_member_by_name(get_struc(tid), name);
	if (member) {
		return ea_t(get_full_word( loc + member->get_soff()));
	} else {
		return BADADDR;
	}
}

bool struct_member_makestr(ea_t loc, tid_t tid, const char *name)
{
	ea_t name_loc = struct_member_ea_ref(loc, tid, name);
	return make_ascii_string(name_loc, -1, ASCSTR_C);
}

bool make_name(ea_t loc, tid_t tid, const char *str_name, const char *prefix, bool addloc)
{
	ea_t name_ea = struct_member_ea_ref(loc, tid, str_name);
	char *name_type;

	if (chunkstart(name_ea) != BADADDR) {
		make_ascii_string(name_ea, -1, ASCSTR_C);
		size_t len = get_max_ascii_length(name_ea, ASCSTR_C);
		name_type = new char[len+1];
		get_ascii_contents(name_ea, len, ASCSTR_C, name_type, len);
// remove
		for(size_t i=0;i<len;i++) {
			if ( 
				(name_type[i]=='<') ||
				(name_type[i]=='>') ||
				(name_type[i]=='-') 
				){
					name_type[i] = '_';
			}
		}
	} else {
		name_type = 0;
		if (tid == get_struc_id("OBJ_IN_CORE") ) {
			loc += get_struc_size(tid);
			make_ascii_string(loc, -1, ASCSTR_C);
			size_t len = get_max_ascii_length(loc, ASCSTR_C);
			if(len>0) {
				name_type = new char[len+1];
				get_ascii_contents(loc, len, ASCSTR_C, name_type, len);
			} else {
				name_type = 0;
			}
		}
	}
	char buf[1024]; // yeah...

	if (name_type) {
		if (addloc==true) {
			qsnprintf(buf, 1023, "%s_%08x_%s", prefix, loc,  name_type);
		} else {
			qsnprintf(buf, 1023, "%s_%s", prefix, name_type);
		}
	} else {
		if (addloc==true) {
			qsnprintf(buf, 1023, "%s_%08x_%x", prefix, loc,  name_ea);
		} else {
			qsnprintf(buf, 1023, "%s_%x", prefix, name_ea);
		}

	}
	if (name_type) delete[] name_type;
	return set_name(loc, buf);
}

void set_name_pre_post(ea_t ins, const char* pre, const char* post)
{
	char name_exp[40];
	if (post) {
		qsnprintf(name_exp, 40, "%s_%x_%s", pre, ins, post);
	} else {
		qsnprintf(name_exp, 40, "%s_%x", pre, ins);
	}
	set_name(ins, name_exp, SN_CHECK);
}

void set_name_pre_post_dec(ea_t loc, int nr, const char* pre, const char* post)
{
	char name_exp[40];
	if (post) {
		qsnprintf(name_exp, 40, "%s_%d_%s", pre, nr, post);
	} else {
		qsnprintf(name_exp, 40, "%s_%d", pre, nr);
	}
	set_name(loc, name_exp, SN_CHECK);
}

void set_name_instr(ea_t ea)
{
	set_name_pre_post(ea, "bv_loc");
}

void set_name_func(ea_t ea)
{
	set_name_pre_post(ea, "bv_fun");
}

void set_name_sym(ea_t ea)
{
	set_name_pre_post(ea, "sym");
}

void set_name_sym_dims(ea_t ea)
{
	set_name_pre_post(ea, "sym", "dims");
}

void set_name_sym_val(ea_t ea)
{
	set_name_pre_post(ea, "sym", "value");
}

std::string get_string_from_loc(ea_t loc)
{
	size_t len = get_max_ascii_length(loc, ASCSTR_C);
	std::string name_type;
	if (len==0) {
		name_type = "";
	} else {
		char *name = new char[len+1];
		get_ascii_contents(loc, len, ASCSTR_C, name, len);
		name_type = name;
		delete[] name;
	}
	return name_type;
}

void follow_linked_list(ea_t start, 
			const char *type_name, 
			const char *next_name, 
			int cust_id, 
			int cust_fid,
			void *obj, 
			int (*list_cb)(ea_t, void *), bool looponly )
{
	ea_t ea = start;
	tid_t type_id = BADADDR;

	if (type_name) {
		type_id = get_struc_id(type_name);
	} else {
		msg("type_name needs to be fill\n");
		return;
	}
	const data_type_t *dt = 0;
	if (cust_id) {
		dt = get_custom_data_type(cust_id);
	}
	while( (ea!=0) && (ea!=BADADDR) && isEnabled(ea) ) {
		if (looponly==false) {
			if (cust_id) {
				if (dt) {
					size_t size = (*dt->calc_item_size)(0, ea, 1024);
					doCustomData(ea, size, cust_id, cust_fid);
				} else {
					msg("cannot determine size\n");
				}
			} else {
				make_struct(ea, -1, type_name);
			}
		}			
		if (list_cb) {
			int ret = (*list_cb)(ea, obj);
		}
		ea = struct_member_ea_ref(ea, type_id, next_name);
	}
//	if (isEnabled(ea)==false) {
//		msg("bad address %x in follow_linked_list\n", ea);
//	}
}

int idb_ptr_size_shift(void)
{
	return 2;
}

ptrdiff_t idb_ptr_size(void) 
{
	return 4;
}

void make_ptr(ea_t addr)
{
	doDwrd(addr, 4);
}

