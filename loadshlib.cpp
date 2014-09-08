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
#include "loadshlib.hpp"

static const char load_shared_library_args[] = { 0 };

static error_t idaapi load_shared_library_idc(idc_value_t *argv, idc_value_t *res)
{
	return load_shared_library(NULL);
}

error_t load_shared_library(const char *default)
{
	msg("Additional file loader\n");
	char *file = askfile_c(0, default, "Specify a shared library");

	if(file != 0) {
		linput_t	*shlib_li	= open_linput(file, false);
		load_info_t	*load_info	= build_loaders_list(shlib_li);
		msg("open_linput() returns %x\n", shlib_li);
		if (shlib_li != 0) {
			bool success;
			success = load_nonbinary_file(file, shlib_li, ".",
				NEF_LOPT|NEF_MAN|NEF_NAME|NEF_LOPT, load_info);
			msg("load_nonbinary_file returns %d\n", success);
			close_linput(shlib_li);
		}
	}
	return true;
}

void setup_openshlib()
{
	set_idc_func_ex("OpenSHLIB", load_shared_library_idc, load_shared_library_args, 0);
}