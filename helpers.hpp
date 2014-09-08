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

bgcolor_t bgcolor(unsigned char blue, unsigned char green = 0xff, unsigned char red = 0xff);
bool	make_struct(ea_t ea, size_t size, const char *strname);
bool make_struct_name(ea_t loc, tid_t loc_t, const char *struct_name, const char *member_name, const char *prefix);
ea_t	struct_member_ea_val(ea_t loc, tid_t tid, const char* name);
ea_t	struct_member_ea_ref(ea_t loc, tid_t tid, const char *name);
ea_t	struct_member_ea_ref_word(ea_t loc, tid_t tid, const char *name);
ea_t	struct_member_ea_ref_byte(ea_t loc, tid_t tid, const char *name);
bool	struct_member_makestr(ea_t loc, tid_t tid, const char *name);
bool	make_name(ea_t loc, tid_t tid, const char *str_name, const char *prefix, bool addloc = false);
void set_name_pre_post(ea_t ins, const char* pre, const char* post=0);
void set_name_pre_post_dec(ea_t ins, int nr, const char* pre, const char* post=0);
void set_name_instr(ea_t ea);
void set_name_func(ea_t ea);
void set_name_sym(ea_t ea);
void set_name_sym_dims(ea_t ea);
void set_name_sym_val(ea_t ea);
void set_name_id(ea_t ea, int nr);
std::string get_string_from_loc(ea_t loc);
void follow_linked_list(ea_t start, 
			const char *type_name, 
			const char *next_name, 
			int cust_id, 
			int cust_fid,
			void *obj, 
			int (*list_cb)(ea_t, void *),
			bool looponly = false);
int idb_ptr_size_shift(void);
ptrdiff_t idb_ptr_size(void);
void make_ptr(ea_t addr);

