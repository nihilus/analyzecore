
#include <QObject>
#include "ui_unix_core.h"
#include "cli_telnet.hpp"


class replace_char {
private:
	const char c1, c2;
	replace_char();
public:
	replace_char(const char _c1, const char _c2) : c1(_c1), c2(_c2) {};
	void operator()(char& c) {
		if (c == c1) c = c2;
	};
};


class  CoreCallbacks : public QObject
{
	Q_OBJECT
	  
	Ui_core_analysis *core_ui;
	RExec *rexec;

	typedef enum {
		DBG_CMD_SHARED_LIBS,
		DBG_CMD_STACK_TRACE,
		DBG_CMD_FRAME_UP,
		DBG_CMD_FRAME_DOWN,
		DBG_CMD_FRAME_CURR,
		DBG_CMD_REGISTERS
	} dbg_cmd_t;

	typedef std::map<dbg_cmd_t, std::string> dbg_to_cmd_t;
	typedef dbg_to_cmd_t::iterator dbg_to_cmd_t_i;
	typedef dbg_to_cmd_t::const_iterator dbg_to_cmd_t_ci;

	dbg_to_cmd_t cmd_dbg;

public:
	CoreCallbacks(QObject *parent, Ui_core_analysis* _core_ui) :
	  QObject(parent) , core_ui(_core_ui), rexec(0),
		  cmd_dbg() {};

	  void SetRExec(RExec *r) { rexec = r; };

private slots:
	void connect_rexec();
	void dbg_stack();
	void dbg_registers();
	void dbg_shared_libs();
	void dbg_frame_select();
	void dbg_curr_frame();

private:
	void send_dbg_cmd(const std::string& cmd);
	void send_dbg_cmd(const char *cmd);
	void send_dbg_cmd(dbg_cmd_t);
	void send_dbg_cmd(dbg_cmd_t, const std::string& args);
	void setup_gdb_cmd(void);
	void setup_dbx_cmd(void);
	void set_debugger(const std::string&);

private:
	//void remove_prefix(std::string& path)
	//{
	//	std::string local_prefix(core_ui->local_prefix->text().toAscii().data());
	//	path.erase(0, local_prefix.length());
	//}

	void make_unix_pathname(std::string& path);

};

