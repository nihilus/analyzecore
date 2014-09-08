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
#include "includes.hpp"
#include <string>
#include <map>
#include <stack>
#include <sstream>

//
//  lnincludes()
//
typedef struct {
	bool view;
	const char *prefix;
	const char **entry;
	size_t size;
} ps_includes_t;

typedef struct {
	const char *name;
	compiler_info_t *cm;
	ps_includes_t *ps_includes;
	size_t ps_includes_size;
	char **ps_macros;
	size_t ps_macros_size;
	char **os_macros;
	size_t os_macros_size;
} os_chooser_t;

static const char 
*ps_includes_general[] = {
	"headers", 
	"include",
	"lib/al_1",
	"lib/nw_1",
	"lib/bcrypt",
	"lib/yy_1",
};

static const char 
*ps_includes_thirdparty[] = {
	"slm/SunOS9/include"
};

static const char 
*ps_includes_logic[] = {
	"mir/mir",  
	"mir/ds_link",  
	"lib/licence",
	"lib/bclmerp",
	"lib/mb",
	"lib/xml"
};

static const char 
*ps_includes_ui[] = {
	"lib/ds_1"
};

static const char 
*ps_includes_common[] = {
	"include"
};

static const char 
*ps_includes_db[] = {
	"lib/dbc",
	"bdbint",
	"lib/qp",
	"lib/sql/include",
	"lib/qpd",
	"lib/blat",
	"lib/vers"
};

static const char
*ps_sys_includes[] = {
	"",
};

#if 0
__builtin_va_alist;unix;__unix;__sparc;
SUN_SPARC;SOLARIS10;
REL6_2;SYSV_PT;fork=fork1;SOLARIS;QPC_LIB;BCLM_ENABLED;HIGH_LOW;
ICU_AVAILABLE;PAM_AVAILABLE;
__BUILTIN_VA_ARG_INCR;__STDC__;DATAMODEL32;

__builtin_va_alist;unix;__unix;__sparc;
SUN_SPARC;SOLARIS10;
REL6_2;SYSV_PT;fork=fork1;SOLARIS;QPC_LIB;BCLM_ENABLED;HIGH_LOW;
ICU_AVAILABLE;PAM_AVAILABLE;
__BUILTIN_VA_ARG_INCR;__STDC__;DATAMODEL32;
#endif

static const char 
*sparc_macros[] = {
	"__builtin_va_alist", 
	"unix", 
	"__unix",
	"__sparc",
	"__BUILTIN_VA_ARG_INCR", 
	"__STDC__",
	"SUN_SPARC",	"SOLARIS10",	"REL6_2",	"SYSV_PT",	"fork=fork1",
	"SOLARIS",	
};

static const char
*hpia_macros[] = {
	"HP_IA64", "HPUX11_23", "HPUX", "_HPUX_SOURCE", "SYSV_PT", "_HPUX_API_LEVEL=20040821", "__va_list__=char*"
};

static const char 
*ps_macros[] = {
	"QPC_LIB",	"BCLM_ENABLED", "HIGH_LOW",	"ICU_AVAILABLE", 
	"PAM_AVAILABLE",
	"DATAMODEL32",
	"flat=flat_mlv",	// IDA makes fun of flat...
};



ps_includes_t sparc_ps_includes[] = {
	{ true, "logic",	(const char **)ps_includes_logic,	qnumber(ps_includes_logic) },
	{ true, "ui",		(const char **)ps_includes_ui,		qnumber(ps_includes_ui) },
	{ true, "db",		(const char **)ps_includes_db,		qnumber(ps_includes_db) },
	{ true, "general",	(const char **)ps_includes_general,	qnumber(ps_includes_general) },
	{ true, "thirdparty",	(const char **)ps_includes_thirdparty,	qnumber(ps_includes_thirdparty) },
	{ true, "common",	(const char **)ps_includes_common,	qnumber(ps_includes_common) },
	{ false,"solaris10",	(const char **)ps_sys_includes,		qnumber(ps_sys_includes) }
};

compiler_info_t sparc32_cm = {
	COMP_GNU,
	C_PC_FLAT,
	4, // sizeof int
	1, // sizeof bool
	4, // sizeof enum
	8, // default alignment for structures
	2, // size of short
	4,
	8
};

compiler_info_t hpia32_cm = {
	COMP_GNU,
	C_PC_FLAT,
	4, // sizeof int
	1, // sizeof bool
	4, // sizeof enum
	8, // default alignment for structures
	2, // size of short
	4,
	8
};

ps_includes_t hpia_ps_includes[] = {
	{ true, "logic",	(const char **)ps_includes_logic,	qnumber(ps_includes_logic) },
	{ true, "ui",		(const char **)ps_includes_ui,		qnumber(ps_includes_ui) },
	{ true, "db",		(const char **)ps_includes_db,		qnumber(ps_includes_db) },
	{ true, "general",	(const char **)ps_includes_general,	qnumber(ps_includes_general) },
	{ true, "thirdparty",	(const char **)ps_includes_thirdparty,	qnumber(ps_includes_thirdparty) },
	{ true, "common",	(const char **)ps_includes_common,	qnumber(ps_includes_common) },
	{ false,"hpux/hpia/include",	(const char **)ps_sys_includes,		qnumber(ps_sys_includes) }
};


os_chooser_t os_chooser[] =
{
	{ 
		"Sparc 32 bit",		&sparc32_cm,		
			(ps_includes_t*)&sparc_ps_includes,	qnumber(sparc_ps_includes), 
			(char**)ps_macros,			qnumber(ps_macros),	
			(char**)sparc_macros,			qnumber(sparc_macros)	
	},
	{ 
		"HPIA 32 bits",		&hpia32_cm,		
			(ps_includes_t*)&hpia_ps_includes,	qnumber(hpia_ps_includes),
			(char**)ps_macros,			qnumber(ps_macros),	
			(char**)hpia_macros,			qnumber(hpia_macros)	
	},
};

static void set_predefined_macros(lexer_t *lx, const char *macros) {
   lxgetstrm(lx, true);      // delete macros if any
   if ( macros != NULL && macros[0] != '\0' )
   {
     char *tofree = qstrdup(macros);
     for ( char *ptr = tofree; ptr != NULL; )
     {
       char *end = strchr(ptr, ';');
       if ( end != NULL )
         *end++ = '\0';
       const char *body = "1";
       char *eq = strchr(ptr, '=');
       if ( eq != NULL )
       {
         *eq = '\0';
         body = eq + 1;
       }
       lex_define(lx, ptr, body);
       ptr = end;
     }
     qfree(tofree);
   }
}
uint32 idaapi os_type_sizer(void *obj)
{
	return qnumber(os_chooser);
}

char * idaapi os_type_getline(void *obj, uint32 n, char *buf)
{
	if (n==0) { // header case
		qstrncpy(buf, "OS Type", strlen("OS Type")+1);
	} else {
		os_chooser_t *p = (os_chooser_t *)obj;
		const char *name = p[n-1].name;
		qsnprintf(buf, 32, "%s", name);
	}
	return buf;
}


static const char lnincludes_proc_tbl_args[] = { 0 };

static error_t idaapi lnincludes_proc_tbl_idc(idc_value_t *argv, idc_value_t *res)
{
	return lnincludes_proc_tbl();
}

error_t lnincludes_proc_tbl(void) {
	ps_includes_t *my_incl;
	std::ostringstream paths("", std::ios_base::ate);
	std::ostringstream macros("", std::ios_base::ate);

	const char *base = "x:";
	const char *sys_base = "j:/IDA";


	int choice = choose(os_chooser, 16, os_type_sizer, os_type_getline, "OS include");
	if (choice <=0 ) return 0;

	os_chooser_t *c = &os_chooser[choice-1];

	bool app_path = false;
	for (int i=0;i<c->ps_includes_size;i++) {
		my_incl = &(c->ps_includes[i]);
		for (int j=0;j<my_incl->size;j++) {
//			std::ostringstream paths("", std::ios_base::ate);
			msg("element: %s\n", my_incl->entry[j]);
			if (my_incl->view==true) {
				paths	<< base << "/"
					<< my_incl->prefix << "/"
					<< my_incl->entry[j] << ";";
			} else {
				paths	<< sys_base << "/"
					<< my_incl->prefix << "/"
					<< my_incl->entry[j] << ";";
			}
//			set_header_path(paths.str().c_str(), app_path);
			if (app_path==false) app_path=true;
		}
	}

	for (int i=0;i<c->os_macros_size;i++) {
		macros <<c->os_macros[i] << ";";
	}
	for (int i=0;i<c->ps_macros_size;i++) {
		macros << c->ps_macros[i] << ";";
	}

	msg("set macros to %s\n", macros.str().c_str());

	set_c_header_path(paths.str().c_str());

	char incl_buf[1025];
	ssize_t incl_buf_size = get_c_header_path(incl_buf, 1024);

	msg("incl path: %d %s\n", incl_buf_size, incl_buf);



	set_compiler(*(c->cm), SETCOMP_OVERRIDE);
	set_c_macros(macros.str().c_str());

	return 0;
}

void setup_lnincludes(void)
{
	set_idc_func_ex("lnincludes", lnincludes_proc_tbl_idc, lnincludes_proc_tbl_args, 0);
}
