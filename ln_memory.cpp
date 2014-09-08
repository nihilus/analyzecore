#include <ida.hpp>
#include <idp.hpp>
#include <bytes.hpp>
#include <struct.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include <name.hpp>
#include <expr.hpp>
#include <diskio.hpp>
#include <offset.hpp>
#include <typeinf.hpp>
#include <lex.hpp>
#include <string>
#include <map>

#define GROUP_HASH_TABLE_MASK ( 0xff )
#define GROUP_HASH_TABLE_SIZE (GROUP_HASH_TABLE_MASK+1)

#include "helpers.hpp"
#include "ln_memory.hpp"

static int m_entry_id = 0;		// id of the "M_ENTRY" data type
static int m_entry_fid = 0;		// id of the "M_ENTRY" data format

int m_entry_used_helper_cb(ea_t, void*);
int m_entry_free_helper_cb(ea_t, void*);
int validate_m_entry_used_cb(ea_t ea, void *obj);
int validate_m_entry_free_cb(ea_t ea, void *obj);

struct m_entry_t {
	int id;
	int color;
	int marker;
	int size;
	ea_t ea;
	ea_t next;
	ea_t prev;
	ea_t start;
	ea_t end;
	bool free;
	m_entry_t(ea_t ea, bool _free=false);
};

m_entry_t::m_entry_t(ea_t _ea, bool _free) : 
id(0),
	color(0),
	size(0),
	marker(0),
	ea(_ea),
	next(0), prev(0), start(0), end(0), free(_free)
{
	tid_t m_entry_id = get_struc_id("M_ENTRY");
	if (m_entry_id == BADADDR)  return;

	ssize_t m_entry_size = get_struc_size(m_entry_id);

	unsigned int ic =   (unsigned int) struct_member_ea_ref(ea, m_entry_id, "_ic");
	size = struct_member_ea_ref(ea, m_entry_id, "size");	
	size -= m_entry_size;
	next = struct_member_ea_ref(ea, m_entry_id, "next");
	prev = struct_member_ea_ref(ea, m_entry_id, "prev");

	start  = ea + m_entry_size;
	end = start + size -1 ; // the size is total M_ENTRY + size requested + rounding

	marker = 0;
	id = 0;
	color = 0;

	tid_t mem_alloc_ic = get_struc_id("mem_alloc::ic");

	if (mem_alloc_ic == BADADDR) {
		color = -1; // no colors
		marker = -1; // no marker
		id = ic;
	} else {
		member_t *marker_m = get_member_by_name(get_struc(mem_alloc_ic), "marker");
		if (marker_m) {
			marker = (ic & 0x3);
			color = (ic & 0xFC) >> 2;
			id = (ic & 0xffffff00) >> 8;
		} else {
			marker = -1;
			color = (ic & 0xf);
			id = (ic & 0xfffffff0) >> 4;
		}
	}
}

typedef qvector<m_entry_t *> m_entry_list_t;
typedef std::map<ea_t, m_entry_t *> m_entry_map_t;
typedef std::map<ea_t, m_entry_t *>::iterator MEMI;

typedef enum _m_e_type {
	m_entry_invalid = 1,
	m_entry_used = 2,
	m_entry_free = 3
} m_e_type_t;

class m_group_t {
public:
	int id;
	std::string name;
	std::string loc;
	int blocksize;
	int current;
	int total;
	int freed;
	ea_t addr;
	ea_t used_mem;
	ea_t free_mem;
	ea_t block;
	m_entry_list_t ul;
	m_entry_map_t  um;
	m_entry_list_t fl;
	m_entry_map_t  fm;
	bool used_visible;
	bool free_visible;
	int my_entry;
	int bad_used;
	int bad_free;
	tid_t m_group_id;
	int free_mem_size;
	qvector<ea_t> free_mem_array;

private:
	int get_free_mem_size(void) {
		member_t *member = get_member_by_name(get_struc(m_group_id), "free_mem");
		asize_t tot_size = get_member_size(member);
		asize_t size = get_full_data_elsize(BADADDR, member->flag);
		return tot_size/size;
	}

	void fill_free_mem_array(void) {
		if (free_mem_size==1) { // free_mem contains the list
			free_mem_array.push_back(free_mem);
		} else {
			// free_mem is a set of pointers
			for(int i=0;i<free_mem_size;i++) {
				ea_t fa = struct_member_ea_val(addr, m_group_id, "free_mem");
				free_mem_array.push_back(ea_t(get_full_long(fa + i*4) ) );
			}
		}
	}

private:
	m_group_t() :
	   id(0), name(""), loc(""), blocksize(0), 
		   current(0), total(0), freed(0),
		addr(0), used_mem(0), free_mem(0), block(0), 
		ul(), um(), 
		fl(), fm(),
		used_visible(false), free_visible(false), 
		my_entry(0), bad_used(0), bad_free(0), 
		m_group_id(get_struc_id("M_GROUP")),
		free_mem_size(0), free_mem_array()
	{
		free_mem_size = get_free_mem_size();
	};
public:
	m_group_t(int _id, std::string _name, std::string _loc, int _blocksize, 
		int _current, int _total, int _freed, ea_t _addr, ea_t _used_mem, 
		ea_t _free_mem, ea_t _block) :
	id(_id), name(_name), loc(_loc), blocksize(_blocksize),
		current(_current), total(_total), freed(_freed), addr(_addr),
		used_mem(_used_mem), free_mem(_free_mem), block(_block),
		ul(), um(),		
		fl(), fm(),
		used_visible(false), free_visible(false), 
		my_entry(0), bad_used(0), bad_free(0), 
		m_group_id(get_struc_id("M_GROUP")),
		free_mem_size(0), free_mem_array()
	{
		free_mem_size = get_free_mem_size();
		fill_free_mem_array();
	}
private:
	m_group_t(ea_t ea) {

	}
public:
	~m_group_t()
	{

	};
	bool walk_used_list(void *obj, int (*list_cb)(ea_t, void *), bool looponly=false ) {
		follow_linked_list(used_mem, "M_ENTRY", "next",
			m_entry_id, m_entry_fid,
			obj, list_cb, looponly);
		return true;
	}
	bool walk_free_list(void *obj, int (*list_cb)(ea_t, void *), bool looponly=false ) {
		for(int i = 0; i<free_mem_size; i++) {
			follow_linked_list(free_mem_array[i], "M_ENTRY", "next",
				m_entry_id, m_entry_fid,
				obj, list_cb, looponly);
		}
		return true;
	}
};



typedef qvector<m_group_t *> m_group_list_t;

typedef std::map<ea_t, m_entry_t*> m_entry_map_t;
typedef std::map<ea_t, m_entry_t*>::iterator MEMT;

static int chooser_m_group_item_cb(m_group_t *, void *my_dat);
static int chooser_m_entry_used_item_cb(m_group_t *, void *my_dat);
static int chooser_m_entry_free_item_cb(m_group_t *, void *my_dat);


typedef enum {
	chooser_m_group = 1,
	chooser_m_entry_used = 2,
	chooser_m_entry_free = 3
} chooser_m_type;

class m_groups_t;

typedef struct {
	chooser_m_type type;
	int (*item_cb)(m_groups_t *, void *);
	int entry;
	int n;
	char name[256];
	chooser_item_attrs_t *attrs;
} chooser_m_data;

typedef std::map<void *, chooser_m_data> chooser_m_t;
typedef std::map<void *, chooser_m_data>::iterator CMI;

class m_groups_t {
public:
	m_group_list_t m_groups;
	m_entry_map_t m_entries;
	chooser_m_t my_choosers;

	tid_t m_group_id;
	tid_t m_group_color_id;
	tid_t m_color_detail_id;

	ea_t group_tbl;
	ea_t group_hash_tbl;

private:
	bool validate(bool show_error) {
		if (m_group_id == BADADDR)  { 
			if (show_error) msg("please define M_GROUP\n"); 
			return false;
		}
		if (m_group_color_id == BADADDR)  { 
			if (show_error) msg("please define M_GROUP\n"); 
			return false;
		}
		if (m_color_detail_id == BADADDR)  { 
			if (show_error) msg("please define M_GROUP\n"); 
			return false;
		}
		if (group_tbl == BADADDR) { 
			if (show_error) msg("please find group_tbl\n");
			return false;
		}
		return true;
	};
public:
	m_groups_t() : m_groups(), m_entries(), my_choosers() 
	{
		m_group_id = get_struc_id("M_GROUP");
		m_group_color_id = get_struc_id("M_GROUP_COLOR");
		m_color_detail_id = get_struc_id("M_COLOR_DETAIL");

		group_tbl = get_name_ea(-1, "group_tbl");
		group_hash_tbl = get_name_ea(-1, "group_hash_tbl");

		validate(true);
	};

	~m_groups_t() 
	{
		if (validate(false)==false) return;
		MEMT memt = m_entries.begin();
		while(memt != m_entries.end()) {
			m_entry_t *mt = (*memt).second;
			delete mt;
			memt++;
		}
		for(int i=0;i<m_groups.size();i++) {
			m_group_t *mg = m_groups[i];
			delete mg;
		}
	}

	m_entry_t *m_entry(ea_t ea, bool _free) {
		if (validate(false)==false) return 0;
		MEMT memt = m_entries.find(ea);
		if (memt == m_entries.end() ) {
			m_entry_t *mt = new m_entry_t(ea, _free);
			m_entries[ea] = mt;
			return mt;
		} else {
			return(*memt).second;
		}
	}

	m_entry_t *get_m_entry(ea_t ea) {
		MEMT memt = m_entries.find(ea);
		return (*memt).second;
	}


	m_group_t *m_group(ea_t curr_m_group) {
		if ( (curr_m_group == 0) || (curr_m_group == BADADDR) ) return 0;
		make_struct(curr_m_group, -1, "M_GROUP");
		ea_t mc = struct_member_ea_ref(curr_m_group, m_group_id, "mc");
		make_struct(mc, -1, "M_GROUP_COLOR");
		size_t m_color_detail_size = get_struc_size(get_struc(m_color_detail_id));
		int nr_colors = struct_member_ea_ref(curr_m_group, m_group_id, "nr_of_colors");

		ea_t cd = struct_member_ea_ref(mc, m_group_color_id, "cd");
		for(int j=0;j<nr_colors;j++) {
			make_struct(cd, -1, "M_COLOR_DETAIL");
			cd += m_color_detail_size;
		}

		int id =	    struct_member_ea_ref(curr_m_group, m_group_id, "id");
		int blocksize = struct_member_ea_ref(curr_m_group, m_group_id, "def_size");
		int current =	struct_member_ea_ref(curr_m_group, m_group_id, "curr_in_use_list");
		int total =	    struct_member_ea_ref(curr_m_group, m_group_id, "tot_block_size");
		int freed =	    struct_member_ea_ref(curr_m_group, m_group_id, "tot_free_size");
		ea_t used_mem = struct_member_ea_ref(curr_m_group, m_group_id, "used_mem");
		ea_t free_mem = struct_member_ea_ref(curr_m_group, m_group_id, "free_mem");
		ea_t block    = struct_member_ea_ref(curr_m_group, m_group_id, "block");
		std::string name = get_string_from_loc(struct_member_ea_ref(curr_m_group, m_group_id, "id_name"));
		std::string loc = get_string_from_loc(struct_member_ea_ref(curr_m_group, m_group_id, "source_loc")); 
		if (curr_m_group != group_tbl ) { // don't rename my initial group_tbl...
			set_name_pre_post_dec(curr_m_group, id, "m_group");
		}
		m_group_t *mg = new m_group_t(id, name, loc, blocksize, current, total, 
			freed, curr_m_group, used_mem, free_mem, block);
		m_groups.push_back(mg);
		mg->walk_used_list((void*)mg, m_entry_used_helper_cb);
		mg->walk_free_list((void*)mg, m_entry_free_helper_cb);

		mg->walk_used_list((void*)mg, validate_m_entry_used_cb);
		mg->walk_free_list((void*)mg, validate_m_entry_free_cb);
		return mg;
	}
	bool fill_m_groups(void) {
		if (validate(true)==false) return false;
		ea_t curr_m_group = group_tbl;
		while( (curr_m_group != 0) && (curr_m_group != BADADDR) && isEnabled(curr_m_group) ){
			m_group_t *mg = m_group(curr_m_group);
			curr_m_group = struct_member_ea_ref(curr_m_group, m_group_id, "next_group");
		}
		// new version of mal_get_group()
		if (group_hash_tbl != BADADDR) {
			for(int i=0; i < GROUP_HASH_TABLE_SIZE; i++) {
				ea_t entry = group_hash_tbl + i * 4;
				ea_t mg_ea = ea_t(get_full_long(entry));
				while( ( mg_ea !=0) && (mg_ea != BADADDR) && isEnabled(mg_ea) ) {
					m_group_t *mg = m_group(mg_ea);
					mg_ea = struct_member_ea_ref(mg_ea, m_group_id, "next_group");
				}
			}	
		}
		return true;
	}
	m_e_type_t get_m_entry_type(ea_t ea) {
		MEMT memt = m_entries.find(ea);
		if (memt == m_entries.end() )  return m_entry_invalid;
		return memt->second->free ? m_entry_free : m_entry_used;
	}
};


static m_groups_t *m_groups = 0;


int m_entry_used_helper_cb(ea_t ea, void *obj)
{
	m_group_t *mg = (m_group_t*) obj;
	m_entry_t *mt = m_groups->m_entry(ea, false);
	MEMI mei = mg->um.find(ea);
	if (mei == mg->um.end() ) {
		mg->um[ea] = mt;
		mg->ul.push_back(mt);
	}
	return 0;
}

int m_entry_free_helper_cb(ea_t ea, void *obj)
{
	m_group_t *mg = (m_group_t*) obj;
	m_entry_t *mt = m_groups->m_entry(ea, true);
	MEMI mei = mg->fm.find(ea);
	if (mei == mg->fm.end() ) {
		mg->fm[ea] = mt;
		mg->fl.push_back(mt);
	}
	return 0;
}


static m_e_type_t get_m_entry_type(ea_t ea)
{
	if (m_groups == 0) return m_entry_invalid;

	return m_groups->get_m_entry_type(ea);	
}

//
// M_ENTRY list
//

int list_cb_m_entry(ea_t ea, void *obj)
{
	m_group_t *mg = (m_group_t *) obj;
	return 0;
}

int list_cb_m_entry_f(ea_t ea, void *obj)
{
	m_group_t *mg = (m_group_t *) obj;
	return 0;
}

//
// M_GROUP
//

uint32 idaapi m_group_show_mem_blocks_cb(void *obj, uint32 n)
{
	m_group_list_t *m_groups = (m_group_list_t *) obj;
	if (IS_SEL(n)) {
		--n;
		m_group_t *mg = (*m_groups)[n];
		follow_linked_list(mg->block, "MEM_BLOCK", "next", 0, 0, 0, 0);
	}
	return true;
}


static const char *const m_group_show_all_m_entry_headers[] =
{
	"STAT",       "address",
	"next",       "prev", 
	"id",          "id (hex)", "color", 
	"size",        "size(hex)", 
	"start",       "end"
};

int static m_group_show_all_m_entry_width[] = 
{
	4,             CHCOL_HEX | 8,
	CHCOL_HEX | 8, CHCOL_HEX | 8, 
	CHCOL_DEC | 5, CHCOL_DEC | 5, CHCOL_DEC | 2,
	CHCOL_DEC | 4, CHCOL_HEX | 4, 
	CHCOL_HEX | 8, CHCOL_HEX | 8
};

uint32 idaapi m_group_show_used_m_entry_sizer(void *obj) 
{
	m_group_t *mg = (m_group_t *) obj;
	return mg->ul.size();
}

void idaapi m_group_show_used_m_entry_desc(void *obj, uint32 n, char *const *arrptr)
{
	if (n==0) {
		for (int i = 0; i < qnumber(m_group_show_all_m_entry_headers); i++) {
			qstrncpy(arrptr[i], m_group_show_all_m_entry_headers[i], MAXSTR);
		}
		return;
	}
	n--;
	m_group_t *mg = (m_group_t *) obj;
	m_entry_list_t &ml = mg->ul;

	qsnprintf(arrptr[0], MAXSTR, "%s", ml[n]->id != mg->id ? "BAD" : "");
	qsnprintf(arrptr[1], MAXSTR, "0x%x", ml[n]->ea);
	qsnprintf(arrptr[2], MAXSTR, "0x%x", ml[n]->next);
	qsnprintf(arrptr[3], MAXSTR, "0x%x", ml[n]->prev);
	qsnprintf(arrptr[4], MAXSTR, "%d",   ml[n]->id);
	qsnprintf(arrptr[5], MAXSTR, "0x%x", ml[n]->id);
	qsnprintf(arrptr[6], MAXSTR, "%d",   ml[n]->color);
	qsnprintf(arrptr[7], MAXSTR, "%d",   ml[n]->size);
	qsnprintf(arrptr[8], MAXSTR, "0x%x", ml[n]->size);
	qsnprintf(arrptr[9], MAXSTR, "0x%x", ml[n]->start);
	qsnprintf(arrptr[10], MAXSTR, "0x%x", ml[n]->end);
}




void idaapi m_group_show_used_m_entry_enter_cb(void *obj, uint32 n)
{
	m_group_t *mg = (m_group_t *) obj;
	m_entry_list_t &ml = mg->ul;
	m_entry_t *me = ml[n-1];
	jumpto(me->ea);
}

void idaapi m_group_show_used_m_entry_destroy_cb(void *obj)
{
	m_group_t *mg = (m_group_t *)obj;
	mg->used_visible = false;
}

uint32 idaapi m_group_show_free_m_entry_sizer(void *obj) 
{
	m_group_t *mg = (m_group_t *) obj;
	return mg->fl.size();
}

void idaapi m_group_show_free_m_entry_desc(void *obj, uint32 n, char *const *arrptr)
{
	if (n==0) {
		for (int i = 0; i < qnumber(m_group_show_all_m_entry_headers); i++) {
			qstrncpy(arrptr[i], m_group_show_all_m_entry_headers[i], MAXSTR);
		}
		return;
	}
	n--;
	m_group_t *mg = (m_group_t *) obj;
	m_entry_list_t &ml = mg->fl;

	qsnprintf(arrptr[0], MAXSTR, "%s", ml[n]->id != mg->id ? "BAD" : "");
	qsnprintf(arrptr[1], MAXSTR, "0x%x", ml[n]->ea);
	qsnprintf(arrptr[2], MAXSTR, "0x%x", ml[n]->next);
	qsnprintf(arrptr[3], MAXSTR, "0x%x", ml[n]->prev);
	qsnprintf(arrptr[4], MAXSTR, "%d",   ml[n]->id);
	qsnprintf(arrptr[5], MAXSTR, "0x%x", ml[n]->id);
	qsnprintf(arrptr[6], MAXSTR, "%d",   ml[n]->color);
	qsnprintf(arrptr[7], MAXSTR, "%d",   ml[n]->size);
	qsnprintf(arrptr[8], MAXSTR, "0x%x", ml[n]->size);
	qsnprintf(arrptr[9], MAXSTR, "0x%x", ml[n]->start);
	qsnprintf(arrptr[10], MAXSTR, "0x%x", ml[n]->end);
}

void idaapi m_group_show_free_m_entry_enter_cb(void *obj, uint32 n)
{
	m_group_t *mg = (m_group_t *) obj;
	m_entry_list_t &ml = mg->fl;
	m_entry_t *me = ml[n-1];
	jumpto(me->ea);
}

void idaapi m_group_show_free_m_entry_destroy_cb(void *obj)
{
	m_group_t *mg = (m_group_t *)obj;
	mg->free_visible = false;
}


int idaapi m_group_item_ui_cb(void *obj, int notification_code, va_list va)
{
	if (notification_code != ui_get_chooser_item_attrs) {
		return 0;
	}
	void *mychooser = va_arg(va, void *);
	m_groups_t *my_m = *(m_groups_t**) obj;
	if (my_m==0) return 0;

	CMI cmi = my_m->my_choosers.find(mychooser);
	if (cmi == my_m->my_choosers.end() ) return 0;
	chooser_m_data my_dat = (*cmi).second;
	int n = int(va_arg(va, uint32));

	if (n==0) return 0;
	--n;

	my_dat.n = n;
	//if (n>my_m->m_groups.size()) return 0;
	//my_m->m_groups[n]->my_entry = n;	
	my_dat.attrs = va_arg(va, chooser_item_attrs_t *);
	return my_dat.item_cb(my_m, &my_dat);
}

static int chooser_m_group_item_cb(m_groups_t *my_m, void *obj)
{
	chooser_m_data *my_dat = (chooser_m_data *)obj;
	m_group_t *mg = my_m->m_groups[my_dat->n]; // My M_GROUP

	if ( (mg->bad_used>0) || (mg->bad_free>0) ) {
		my_dat->attrs->color = bgcolor(75,75,218);
		my_dat->attrs->flags += CHITEM_BOLD;	
	}

	return 0;
}

static int chooser_m_entry_used_item_cb(m_groups_t *my_m, void *obj)
{
	chooser_m_data *my_dat = (chooser_m_data *)obj;
	m_group_t *mg = my_m->m_groups[my_dat->entry]; // My M_GROUP

	m_entry_t *mt = mg->ul[my_dat->n]; // my M_ENTRY

	if (mg->id != mt->id) {
		my_dat->attrs->color = bgcolor(0,0,255);
	}

	return 1;
}

static int chooser_m_entry_free_item_cb(m_groups_t *, void *my_dat)
{
	return 0;
}

uint32 idaapi m_group_show_used_m_entry_cb(void *obj, uint32 n)
{
	m_groups_t *m_groups = (m_groups_t *) obj;
	m_group_t *mg;
	if (IS_SEL(n)) {
		--n;
		mg = m_groups->m_groups[n];
		if (mg->used_visible==false) {
			char title[80];
			qsnprintf(title, 79, "Used M_ENTRY's 0x%x %d %20.20s", mg->id, mg->id, mg->name.c_str());
			choose2(CH_ATTRS+CH_MULTI, -1, -1, -1, -1,
				(void*)mg, qnumber(m_group_show_all_m_entry_headers),
				m_group_show_all_m_entry_width,
				m_group_show_used_m_entry_sizer,
				m_group_show_used_m_entry_desc,
				title,
				-1,
				0, 0, 0, 0, 0,
				m_group_show_used_m_entry_enter_cb,
				m_group_show_used_m_entry_destroy_cb,
				0, 0);
			mg->used_visible = true;
			chooser_m_data my_dat;
			my_dat.type = chooser_m_entry_used;
			my_dat.item_cb = chooser_m_entry_used_item_cb;
			my_dat.entry = mg->my_entry;
			strcpy(my_dat.name, title);
			m_groups->my_choosers[get_chooser_obj(title)] = my_dat;
		}
		return true;
	}
	return true;
}


uint32 idaapi m_group_show_free_m_entry_cb(void *obj, uint32 n)
{
	m_groups_t *m_groups = (m_groups_t *) obj;
	m_group_t *mg;
	if (IS_SEL(n)) {
		--n;
		mg = m_groups->m_groups[n];
		if (mg->free_visible == false) {
			char title[80];
			qsnprintf(title, 79, "Free M_ENTRY's 0x%x %d %20.20s", mg->id, mg->id, mg->name.c_str());
			choose2(CH_ATTRS+CH_MULTI, -1, -1, -1, -1,
				(void*)mg, qnumber(m_group_show_all_m_entry_headers),
				m_group_show_all_m_entry_width,
				m_group_show_free_m_entry_sizer,
				m_group_show_free_m_entry_desc,
				title,
				-1,
				0, 0, 0, 0, 0,
				m_group_show_free_m_entry_enter_cb,
				m_group_show_free_m_entry_destroy_cb,
				0, 0);
			chooser_m_data my_dat;
			my_dat.type = chooser_m_entry_free;
			my_dat.item_cb=	chooser_m_entry_free_item_cb;
			my_dat.entry = 	mg->my_entry;
			strcpy(my_dat.name, title);
			m_groups->my_choosers[get_chooser_obj(title)] = my_dat;
		} 
		return true;
	}
	return true;
}


static const char *const m_group_headers[] = {
	"id",	     "id(hex)",   "addr",      "name",  
	"block size",
	"current",   "total", "   free",
	"bad_used",  "bad_free", 
	"loc"
};
int m_group_width[] = { 
	CHCOL_DEC|5, CHCOL_HEX|5, CHCOL_HEX|5, 40,       
	CHCOL_DEC|6,
	CHCOL_DEC|6, CHCOL_DEC|6, CHCOL_DEC|6, 
	CHCOL_DEC|3, CHCOL_DEC|3,
	40
};

uint32 idaapi m_group_sizer(void *obj)
{
	m_groups_t *ml = (m_groups_t*)obj;
	return (uint32)ml->m_groups.size();
}

void idaapi m_group_desc(void *obj, uint32 n, char *const *arrptr)
{
	if (n==0) { 
		for(int i=0;i<qnumber(m_group_headers);i++) {
			qstrncpy(arrptr[i], m_group_headers[i], MAXSTR);
		}
		return;
	}
	n--;
	m_groups_t *ml = (m_groups_t *)obj;
	m_group_t *mt = ml->m_groups[n];
	mt->my_entry = n;
	qsnprintf(arrptr[0], MAXSTR, "%d",	mt->id);
	qsnprintf(arrptr[1], MAXSTR, "0x%08x",	mt->id);
	qsnprintf(arrptr[2], MAXSTR, "%x",	mt->addr);
	qsnprintf(arrptr[3], MAXSTR, "%s",	mt->name.c_str());
	qsnprintf(arrptr[4], MAXSTR, "%d",	mt->blocksize);
	qsnprintf(arrptr[5], MAXSTR, "%d",	mt->current);
	qsnprintf(arrptr[6], MAXSTR, "%d",	mt->total);
	qsnprintf(arrptr[7], MAXSTR, "%d",	mt->freed);
	qsnprintf(arrptr[8], MAXSTR, "%d",	mt->bad_used);
	qsnprintf(arrptr[9], MAXSTR, "%d",	mt->bad_free);
	qsnprintf(arrptr[10], MAXSTR, "%s",	mt->loc.c_str());
}

void idaapi m_group_enter_cb(void *obj, uint32 n)
{
	m_groups_t *ml = (m_groups_t *) obj;
	jumpto ( ml->m_groups[n-1]->addr);
}

void idaapi m_group_destroy_cb(void *obj)
{
	//delete m_groups;
	//m_groups = 0;
}
 
int validate_m_entry_used_cb(ea_t ea, void *obj)
{
	m_group_t *mg = (m_group_t *) obj;
	MEMI memi = mg->um.find(ea);
	if ( memi == mg->um.end()) return 0;
	m_entry_t *mt = (*memi).second;
	if (mt->id != mg->id) {
		mg->bad_used++;
	}
	return 0;
}

int validate_m_entry_free_cb(ea_t ea, void *obj)
{
	m_group_t *mg = (m_group_t *) obj;
	MEMI memi = mg->fm.find(ea);
	if ( memi == mg->fm.end()) return 0;
	m_entry_t *mt = (*memi).second;

	return 0;
}


error_t analyze_m_groups()
{

	tid_t m_group_id = get_struc_id("M_GROUP");
	if (m_group_id == BADADDR)  { msg("please define M_GROUP\n"); return 0; }
	tid_t m_group_color_id = get_struc_id("M_GROUP_COLOR");
	if (m_group_color_id == BADADDR)  { msg("please define M_GROUP\n"); return 0; }
	tid_t m_color_detail_id = get_struc_id("M_COLOR_DETAIL");
	if (m_color_detail_id == BADADDR)  { msg("please define M_GROUP\n"); return 0; }

	ea_t group_tbl = get_name_ea(-1, "group_tbl");
	if (group_tbl == BADADDR) { msg("please find group_tbl\n"); return 0; }

	ea_t curr_m_group = group_tbl;

	if (m_groups==0) {
		m_groups = new m_groups_t();
	}

	m_groups->fill_m_groups();	

	//while( (curr_m_group != 0) && (curr_m_group!=0xffffffff) ) {
	//	m_group_t *mg = m_groups->m_group(curr_m_group);
	//	curr_m_group = struct_member_ea_ref(curr_m_group, m_group_id, "next_group");
	//}

	const char m_group_title[] = "M_GROUP's";
	void *m_group_ch = get_chooser_obj(m_group_title);

	choose2(CH_ATTRS+CH_MULTI, -1, -1, -1, -1, 
		(void*)m_groups, 
		qnumber(m_group_headers), 
		m_group_width, 
		m_group_sizer,
		m_group_desc, 
		m_group_title,  
		-1,
		0, 0,0, 0, 0, m_group_enter_cb, m_group_destroy_cb,0, NULL);

	int flags =
		CHOOSER_NO_SELECTION|CHOOSER_POPUP_MENU;
	
	chooser_m_data my_dat;
	my_dat.type = chooser_m_group;
	my_dat.item_cb = chooser_m_group_item_cb;
	my_dat.entry = -1;
	strcpy(my_dat.name, m_group_title);
	m_groups->my_choosers[get_chooser_obj(m_group_title)] = my_dat;
	
	if (m_group_ch==0) {
		add_chooser_command(m_group_title, "Show MEM_BLOCKs", m_group_show_mem_blocks_cb, CHOOSER_MENU_EDIT, 4, flags);
		add_chooser_command(m_group_title, "Show used M_ENTRY", m_group_show_used_m_entry_cb, CHOOSER_MENU_EDIT, 5, flags);
		add_chooser_command(m_group_title, "Show free M_ENTRY", m_group_show_free_m_entry_cb, CHOOSER_MENU_EDIT, 5, flags);
	}
	return 1;
}

static const char link_m_groups_tbl_args[] = { 0 };

static error_t idaapi link_m_groups_tbl_idc(idc_value_t *argv, idc_value_t *res)
{
	return analyze_m_groups();
}


//
// M_ENTRY
//

static asize_t idaapi calc_m_entry_length(void *, ea_t ea, asize_t maxsize)
{
	tid_t m_entry_id = get_struc_id("M_ENTRY");
	if ( is_member_id(ea) ) return 1;

	if (m_entry_id == BADADDR) {
		return 1;
	} else {
		return get_struc_size(get_struc(m_entry_id));
	}
}

static data_type_t m_entry_type = 
{
	sizeof(data_type_t),	// size of this structure
	NULL,			// user defined data
	0,
	"m_entry",
	"m_entry",
	"Ctrl-Alt-N",
	"M_ENTRY",
	1,
	NULL,
	calc_m_entry_length
};

static bool print_m_entry_type(qstring *name, qstring *decl, qstring *value, ea_t ea)
{
	return true;
}

static bool idaapi 
print_m_entry(
	       void *,
	       qstring *out,
	       const void *value,
	       asize_t size,
	       ea_t ea,
	       int,
	       int)
{
	m_entry_t *mt;
	if (m_groups) {
		mt = m_groups->m_entry(ea, false);
	} else {
		mt = new m_entry_t(ea);
	}
	m_e_type_t mt_type = get_m_entry_type(ea);

	tid_t m_entry_id = get_struc_id("M_ENTRY");
	if (m_entry_id==BADADDR) {
		out->cat_sprnt("M_ENTRY not found!");
		return true;
	}
	char b_next[80] = { '0', 0 };
	get_name_expr(ea, 0, mt->next, mt->next, b_next, 79, GETN_NOFIXUP);
	char b_prev[80] = { '0', 0 };
	get_name_expr(ea, 0, mt->prev, mt->prev, b_prev, 79, GETN_NOFIXUP);

	switch (mt_type) {
	default:
	case m_entry_invalid:
		out->append('?');
		set_name_pre_post(ea, "ukwn");
		break;
	case m_entry_used:
		out->append('U');
		set_name_pre_post(ea, "used");
		break;
	case m_entry_free:
		out->append('F');
		set_name_pre_post(ea, "free");
		break;
	}
	out->append(' ');

	if (mt->color == -1) {
		out->cat_sprnt("id %d (%X) next: %s prev: %s size %d (%X) %X-%X",
			mt->id, mt->id, b_next, b_prev, mt->size, mt->size,
			mt->start, mt->end);
	} else {
		if (mt->marker>=0) {
			if (mt->marker) {
				out->cat_sprnt("id %d (%X) color %d next: %s prev: %s size %d (%X) %X-%X (m:%d)",
					mt->id, mt->id, mt->color, b_next, b_prev, mt->size, mt->size, 
					mt->start, mt->end, mt->marker);
			} else {
				out->cat_sprnt("id %d (%X) color %d next: %s prev: %s size %d (%X) %X-%X ", 
					mt->id, mt->id, mt->color, b_next, b_prev, mt->size, mt->size,
					mt->start, mt->end);
			}
		} else {
			out->cat_sprnt("id %d (%X) color %d next: %s prev: %s size %d (%X) %X-%X", 
				mt->id, mt->id, mt->color, b_next, b_prev, mt->size, mt->size, 
				mt->start, mt->end);
		}
	}
	return true;
}


static data_format_t m_entry_format = 
{
	sizeof(data_format_t),		// size of this structure
	NULL,
	0,
	"m_entry",	// internal name of the data format
	"Create M_ENTRY",			// Menu name of the format. NULL means 'do not create menu item'
	"Ctrl-Alt-M",			// HotKey
	0,
	0,
	print_m_entry	// callback to render colored text for the data
};

void m_group_helpers::setup_m_entry_type(void)
{
	m_entry_id = register_custom_data_type(&m_entry_type);
	// Register custom data format for it
	m_entry_fid = register_custom_data_format(m_entry_id, &m_entry_format);

}

void m_group_helpers::remove_m_entry_type(void)
{
	if (m_entry_id != 0 ) {
		unregister_custom_data_format(m_entry_id, m_entry_fid);
		unregister_custom_data_type(m_entry_id);
	}
}

m_group_helpers::m_group_helpers()
{
	setup_m_entry_type();
	set_idc_func_ex("link_m_groups", link_m_groups_tbl_idc, link_m_groups_tbl_args, 0);
	hook_to_notification_point(HT_UI,m_group_item_ui_cb, (void*)&m_groups);
}

m_group_helpers::~m_group_helpers()
{
	if (m_groups) {
		delete m_groups;
		m_groups = 0;
	}
	remove_m_entry_type();
	set_idc_func_ex("link_m_groups", NULL, NULL, 0);
	unhook_from_notification_point(HT_UI, m_group_item_ui_cb, (void*)&m_groups);
}