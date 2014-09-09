/*
 *  This is a sample plugin module. It demonstrates how to create your
 *  own window and populate it with Qt widgets.
 *
 */

#include <WinSock2.h>
#include <QtGui>


#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include <expr.hpp>
#include <nalt.hpp>
#include <name.hpp>

#include <algorithm>

#include "qwindow.h"
#include "myactions.h"
#include "cli_telnet.hpp"
#include "ui_unix_core.h"


//--------------------------------------------------------------------------


void CoreCallbacks::connect_rexec()
{
	if (rexec_curr==0) {
		std::string hostname(core_ui->hostname->text().toLatin1().data());
		std::string username(core_ui->username->text().toLatin1().data());
		std::string password(core_ui->password->text().toLatin1().data());
		std::string command(core_ui->command->text().toLatin1().data());
		std::string startup(core_ui->startup->text().toLatin1().data());
		std::string binary(core_ui->binary->text().toLatin1().data());
		std::string core(core_ui->core->text().toLatin1().data());
		
		char idb_path_c[QMAXPATH];
		
		::qstrncpy(idb_path_c, database_idb, sizeof(idb_path_c));
		std::string idb_path(idb_path_c);
		login_info info;

		if (info.get_core() == "") {
			info.set_core(idb_path.c_str());
			core_ui->core->setText(QString(idb_path.c_str()));
		} else {
			core_ui->core->setText(QString(info.get_core().c_str()));
		}
		
		
		if (rexec_curr) {
			rexec_curr->Close();
			rexec_curr->Terminate();
		}
		try{
			rexec_curr = new RExec(hostname);
		} catch(...) {
			msg("Rexec failed\n");
			return ;
		}
		
		core_ui->debug_control->setEnabled(true);
		core_ui->tabs->setCurrentIndex(1); //make tab 1 active

		std::string cmd(command);
		cmd += " ";
		if (command.find("gdb") != command.npos) {
			if (startup!="") cmd += "-command=" + startup + " ";
			set_debugger("gdb");
		}
		if (command.find("dbx") != command.npos) {
			if (startup!="") cmd += "-s " + startup + " ";
			set_debugger("dbx");
		}
		cmd += binary;
		cmd += " ";
		cmd += core;
		rexec_curr->RExecExec(username, password, cmd);
		rexec_curr->CreateThread(RExec__WorkingThread);
		SetRExec(rexec_curr);
		core_ui->connect->setText(QString("disconnect"));
	} else {
		rexec_curr->Terminate();
		rexec_curr->Close();
		delete rexec_curr;
		rexec_curr = 0;
		core_ui->tabs->setCurrentIndex(0); // make tab 0 active
		core_ui->debug_control->setEnabled(false);		
		core_ui->connect->setText(QString("connect"));
		SetRExec(rexec_curr);
	}
}
	
void CoreCallbacks::dbg_stack()
{
	send_dbg_cmd(DBG_CMD_STACK_TRACE);
}


void CoreCallbacks::dbg_registers()
{
	send_dbg_cmd(DBG_CMD_REGISTERS);
}

void CoreCallbacks::dbg_shared_libs()
{
	send_dbg_cmd(DBG_CMD_SHARED_LIBS);
}

void CoreCallbacks::dbg_frame_select()
{
	std::string framenr(core_ui->framenr->cleanText().toLatin1().data());
	send_dbg_cmd(DBG_CMD_FRAME_CURR, framenr);
}

void set_hex_number(QLineEdit *w, const char *name)
{
	ea_t value = get_name_ea(-1, name);
	QString tmp;
	tmp.setNum(ulong(value), 16);
	w->setText(tmp);
	if (value==BADADDR) {

	}
}


void CoreCallbacks::make_unix_pathname(std::string& path)
{
	std::for_each(path.begin(), path.end(), replace_char('\\', '/'));
}


void CoreCallbacks::dbg_curr_frame()
{
	send_dbg_cmd(DBG_CMD_FRAME_CURR);
}

void CoreCallbacks::send_dbg_cmd(const std::string& cmd)
{
	if (rexec) {
		rexec_curr->SendLine(cmd);
	}
}

void CoreCallbacks::send_dbg_cmd(const char *cmd)
{
	send_dbg_cmd(std::string(cmd));
}

void CoreCallbacks::send_dbg_cmd(dbg_cmd_t cmd_id)
{
	send_dbg_cmd(cmd_id, std::string(""));
}

void CoreCallbacks::send_dbg_cmd(dbg_cmd_t cmd_id, const std::string& args)
{
	dbg_to_cmd_t_i di = cmd_dbg.find(cmd_id);
	if (di == cmd_dbg.end()) {
			msg(" unknown cmd: %d\n", cmd_id);
	} else {
		send_dbg_cmd( (*di).second + " " + args);
	}
}

void CoreCallbacks::set_debugger(const std::string &dbg)
{
	if (dbg == "gdb") {
		setup_gdb_cmd();
		return;
	}
	if (dbg == "dbx") {
		setup_dbx_cmd();
		return;
	}
	msg(" cannot find debugger '%s'\n", dbg.c_str());
}

void CoreCallbacks::setup_dbx_cmd()
{
	cmd_dbg[DBG_CMD_SHARED_LIBS] = "proc -map";
	cmd_dbg[DBG_CMD_STACK_TRACE] = "where";
	cmd_dbg[DBG_CMD_FRAME_UP] = "frame up";
	cmd_dbg[DBG_CMD_FRAME_DOWN] = "frame down";
	cmd_dbg[DBG_CMD_FRAME_CURR] = "frame";
	cmd_dbg[DBG_CMD_REGISTERS] = "regs";
}

void CoreCallbacks::setup_gdb_cmd()
{
	cmd_dbg[DBG_CMD_SHARED_LIBS] = "info shared";
	cmd_dbg[DBG_CMD_STACK_TRACE] = "where";
	cmd_dbg[DBG_CMD_FRAME_UP] = "up";
	cmd_dbg[DBG_CMD_FRAME_DOWN] = "down";
	cmd_dbg[DBG_CMD_FRAME_CURR] = "frame";
	cmd_dbg[DBG_CMD_REGISTERS] = "info reg";
}

bool find_address(const char *name)
{
	ea_t addr = get_name_ea(-1, name);
	if (isEnabled(addr) ) {
		jumpto(addr);
		return true;
	} else {
		msg("Cannot find '%s'\n", name);
			return false;
	}
}


Ui_core_analysis *core_ui;

//--------------------------------------------------------------------------
static int idaapi ui_callback(void *user_data, int notification_code, va_list va)
{
  if ( notification_code == ui_tform_visible )
  {
    TForm *form = va_arg(va, TForm *);
    if ( form == user_data )
    {
      QWidget *w = (QWidget *)form;

	  core_ui = new Ui_core_analysis();
	  
	  QVBoxLayout *layout = new QVBoxLayout();
	  core_ui->setupUi(w);	  
	  layout->addWidget(core_ui->tabs);	  
	  w->setLayout(layout);
	  // position and display it
	  CoreCallbacks *actions = new CoreCallbacks(w, core_ui);

	  QObject::connect(core_ui->connect, SIGNAL(clicked()), 
		  actions, SLOT(connect_rexec()));
	  QObject::connect(core_ui->stack, SIGNAL(clicked()),
		  actions, SLOT(dbg_stack()));
	  QObject::connect(core_ui->registers, SIGNAL(clicked()),
		  actions, SLOT(dbg_registers()));
	  QObject::connect(core_ui->sharedlibs, SIGNAL(clicked()),
		  actions, SLOT(dbg_shared_libs()));
	  QObject::connect(core_ui->framenr, SIGNAL(valueChanged(int)),
		  actions, SLOT(dbg_frame_select()));
	  QObject::connect(core_ui->curr_frame, SIGNAL(clicked()),
		  actions, SLOT(dbg_curr_frame()));


	  login_info info; // find my stored login info in the idb

	  core_ui->hostname->setText(QString(info.get_hostname().c_str()));
	  core_ui->username->setText(QString(info.get_username().c_str()));
	  core_ui->command->setText(QString(info.get_command().c_str()));
	  core_ui->binary->setText(QString(info.get_binary().c_str()));
	  core_ui->startup->setText(QString(info.get_startup().c_str()));
	  core_ui->password->setText(QString(info.get_password().c_str()));


	  if (info.get_password().empty()==false) {
		  core_ui->save_password->setChecked(true);
	  }

	  w->show();
      msg("Qt form is displayed\n");

    }
  }
  else if ( notification_code == ui_tform_invisible )
  {
    TForm *form = va_arg(va, TForm *);
    if ( form == user_data )
    {
		if (core_ui) {
			login_info info; // find my stored login info in the idb
			
			std::string hostname(core_ui->hostname->text().toLatin1().data());
			std::string username(core_ui->username->text().toLatin1().data());
			std::string password("");
			std::string command(core_ui->command->text().toLatin1().data());
			std::string startup(core_ui->startup->text().toLatin1().data());
			std::string binary(core_ui->binary->text().toLatin1().data());
			std::string core(core_ui->core->text().toLatin1().data());

			if (core_ui->save_password->isChecked()) {
				password = core_ui->password->text().toLatin1().data();
			}
			
			info.set_hostname(hostname);
			info.set_username(username);
			info.set_binary(binary);
			info.set_command(command);
			info.set_startup(startup);
			info.set_binary(binary);
			info.set_password(password);			
			info.set_core(core);

		}
		msg("Qt form is closed\n");
	}
  }
  return 0;
}

static const char qt_core_3gl_args[] = { 0 };
static error_t idaapi qt_core_3gl_idc(idc_value_t *argv, idc_value_t *res)
{
	qt_run();
	return 0;
}

//--------------------------------------------------------------------------
int idaapi qt_init(void)
{
  // the plugin works only with idaq
	return is_idaq() ? PLUGIN_OK : PLUGIN_SKIP;
}

//--------------------------------------------------------------------------
void idaapi qt_term(void)
{
	unhook_from_notification_point(HT_UI, ui_callback);
}

//--------------------------------------------------------------------------
void idaapi qt_run(void)
{
	HWND hwnd = NULL;
	TForm *form = create_tform("Core analysis", &hwnd);
	if ( hwnd != NULL )
	{
		hook_to_notification_point(HT_UI, ui_callback, form);
		open_tform(form, FORM_TAB|FORM_MENU|FORM_RESTORE|FORM_QWIDGET);
	} else {
		close_tform(form, FORM_SAVE);
	}
}
