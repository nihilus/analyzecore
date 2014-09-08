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

#include "helpers.hpp"
#include "callstack.hpp"

static int solaris_callframe_id = 0;		// id of the "M_ENTRY" data type
static int solaris_callframe_fid = 0;		// id of the "M_ENTRY" data format

//
// SOLARIS CALL FRAME
//

static asize_t idaapi calc_solaris_callframe_length(void *, ea_t ea, asize_t maxsize)
{
	return 16*4;
}

static data_type_t solaris_callframe_type = 
{
	sizeof(data_type_t),	// size of this structure
	NULL,			// user defined data
	0,
	"callframe",
	"Sparc Stack Frame",
	"Ctrl-Alt-C",
	"SparcFrame",
	1,
	NULL,
	calc_solaris_callframe_length 
};

static bool print_solaris_callframe_type(qstring *name, qstring *decl, qstring *value, ea_t ea)
{
	return true;
}

static bool idaapi 
print_solaris_callframe(
	       void *,
	       qstring *out,
	       const void *value,
	       asize_t size,
	       ea_t ea,
	       int,
	       int)
{
	char funoff[256]; 
	ssize_t fo;

	fo = get_nice_colored_name(get_long(ea + 15*4), 
		funoff, 255, GNCN_NOCOLOR | GNCN_REQFUNC | GNCN_NOLABEL);

	ea_t fp = get_long(ea + 14*4);
	char fp_s[80] = { '0', 0 };
	get_name_expr(ea, 0, fp, fp, fp_s, 79, GETN_NOFIXUP);
	out->cat_sprnt("fp 0x%x %s ret 0x%x %s ", 
		fp, fp_s, 
		get_long(ea + 15*4), 
		fo ? funoff : ""); 
	out->cat_sprnt("args(");
	for (int i=0;i<5;i++) {
		out->cat_sprnt("0x%x,", get_long(ea + (i+8)*4));
	}
	out->cat_sprnt("0x%x%) ", get_long(ea + (13*4)));

	for (int i=0;i<8;i++) { 
		out->cat_sprnt(" 0x%x", get_long(ea + 4*i));
	}
	set_name_pre_post(ea, "StackFrame");
	return true;
}


static data_format_t solaris_callframe_format = 
{
	sizeof(data_format_t),		// size of this structure
	NULL,
	0,
	"SparcFrame",	// internal name of the data format
	"Create Sparc frame",			// Menu name of the format. NULL means 'do not create menu item'
	0,			// HotKey
	0,
	0,
	print_solaris_callframe	// callback to render colored text for the data
};



void callstack_helpers::setup_callstack_type(void)
{
	solaris_callframe_id = register_custom_data_type(&solaris_callframe_type);
	// Register custom data format for it
	solaris_callframe_fid = register_custom_data_format(solaris_callframe_id, &solaris_callframe_format);
}

void callstack_helpers::remove_callstack_type(void)
{
	if (solaris_callframe_id != 0 ) {
		unregister_custom_data_format(solaris_callframe_id, solaris_callframe_fid);
		unregister_custom_data_type(solaris_callframe_id);
	}
}

callstack_helpers::callstack_helpers()
{
	setup_callstack_type();
}

callstack_helpers::~callstack_helpers()
{
	remove_callstack_type();
}