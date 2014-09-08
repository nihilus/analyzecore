//
//
//


void idaapi qt_run(void);
int idaapi qt_init(void);
void idaapi qt_term(void);



class login_info
{
private:
	netnode con;
	std::string hostname;
	std::string username;
	std::string command;
	std::string binary;
	std::string startup;
	std::string password;
	std::string core;

public:
	typedef enum {
		Hostname,
		Username,
		Command,
		Binary,
		Startup,
		Core,
		Password,		
	} index_id_t;

private:
	std::string get_by_id(index_id_t id, const char *default)
	{
		int len = con.supstr(id, NULL, 0);
		std::string retval("");
		if (len>0) {
			char *buf = new char[len];
			con.supstr(id, buf, len);
			retval = buf;
			delete[] buf;
		} else {
			retval = default;
		}
		return retval;
	}

public:
	login_info() : con("$ login info", 0, true), 
		hostname(get_by_id(Hostname, "nlbautd3")), 
		username(get_by_id(Username, "acohens")), 
		command(get_by_id(Command, "/usr/bin/dbx")), 
		binary(get_by_id(Binary, "flextronics/bshell6.2")), 
		startup(get_by_id(Startup, "flextronics/.dbxrc")), 
		password(get_by_id(Password, "")),
		core(get_by_id(Core, "")) {
			

	}

private:
	void set_by_id(index_id_t id, const std::string& val)
	{
		con.supset(id, val.c_str());
	}


public:
	void set_hostname(const std::string& h)
	{
		hostname = h;
		set_by_id(Hostname, hostname);
	}
	void set_username(const std::string& h)
	{
		username = h;
		set_by_id(Username, username);
	}
	void set_command(const std::string& h)
	{
		command = h;
		set_by_id(Command, command);
	}
	void set_binary(const std::string& h)
	{
		binary = h;
		set_by_id(Binary, binary);
	}
	void set_startup(const std::string& h)
	{
		startup = h;
		set_by_id(Startup, startup);
	}
	void set_password(const std::string& h)
	{
		password = h;
		set_by_id(Password, password);
	}
	void set_core(const std::string& h)
	{
		core = h;
		set_by_id(Core, core);
	}

	const std::string get_hostname(void) { return hostname;};
	const std::string get_username(void) { return username;};
	const std::string get_command(void) { return command;};
	const std::string get_binary(void) { return binary;};
	const std::string get_startup(void) { return startup;};
	const std::string get_password(void) { return password;};	
	const std::string get_core(void) { return core;};

};
