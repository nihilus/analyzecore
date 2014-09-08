/* Custom data type sample plugin.
  *
 */

#include "cli_telnet.hpp"

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
#include <stack>
#include <sstream>



#include "helpers.hpp"

#include "qwindow.h"


telnet_helpers *telnethelpers = 0;


//---------------------------------------------------------------------------
void idaapi run(int)
{
	qt_run();
}



//--------------------------------------------------------------------------
int idaapi init(void)
{
	telnethelpers = new telnet_helpers();

	qt_init();

	return PLUGIN_KEEP;
}

//--------------------------------------------------------------------------
void idaapi term(void)
{
	delete telnethelpers;
	telnethelpers = 0;

	qt_term();
}

//--------------------------------------------------------------------------
//
//      PLUGIN DESCRIPTION BLOCK
//
//--------------------------------------------------------------------------
plugin_t PLUGIN =
{
	IDP_INTERFACE_VERSION,
	PLUGIN_PROC,           // plugin flags
	// we want the plugin to load as soon as possible
	// immediately after the processor module
	// be a menu item in the Edit submenu
	init,                 // initialize
	term,                 // terminate. this pointer may be NULL.
	run,                  // invoke plugin
	"Core file analysis tool",
	// it could appear in the status line
	// or as a hint
	"",                   // multiline help about the plugin

	"CoreAnalysis",    // the preferred short name of the plugin
	"Ctrl-Alt-C"       // the preferred hotkey to run the plugin
};

