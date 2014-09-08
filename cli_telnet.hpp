/* 
   Socket.h

   Copyright (C) 2002-2004 

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

*/

#ifndef SOCKET_H
#define SOCKET_H

#include <WinSock2.h>

#include <string>
#include "pro.h"
#include "kernwin.hpp"

enum TypeSocket {BlockingSocket, NonBlockingSocket};

class Socket {
public:

  virtual ~Socket();
  Socket(const Socket&);
  Socket& operator=(Socket&);

  std::string ReceiveLine();
  std::string ReceiveBytes();
  int ReceiveBytes(unsigned char*, size_t);
  int ReceiveByte(unsigned char *ch);

  void   Close();

  // The parameter of SendLine is not a const reference
  // because SendLine modifes the std::string passed.
  void   SendLine (std::string);

  // The parameter of SendBytes is a const reference
  // because SendBytes does not modify the std::string passed 
  // (in contrast to SendLine).
  void   SendBytes(const std::string&);
  void   SendBytes(const char*, size_t);
  void   SendByte(const char);

protected:
  friend class SocketServer;
  friend class SocketSelect;

  Socket(SOCKET s);
  Socket();


  SOCKET s_;

  int* refCounter_;

private:
  static void Start();
  static void End();
  static int  nofSockets_;
};

class SocketClient : public Socket {
public:
  SocketClient(const std::string& host, int port);
};

class SocketServer : public Socket {
public:
  SocketServer(int port, int connections, TypeSocket type=BlockingSocket);

  Socket* Accept();
};

// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/winsock/wsapiref_2tiq.asp
class SocketSelect {
  public:
    SocketSelect(Socket const * const s1, Socket const * const s2=NULL, TypeSocket type=BlockingSocket);

    bool Readable(Socket const * const s);

  private:
    fd_set fds_;
}; 

class SocketClientT : public SocketClient 
{
protected:
	bool running;
	char sname[100];
	char lname[100];
	char hint[100];
	qthread_t my_t;
	cli_t *my_cli;
public:	

	SocketClientT(const std::string &host, int port) :
	SocketClient(host, port), running(false), my_t(0), my_cli(0) 
	{
		qstrcpy(sname, host.c_str());
		qstrcpy(lname, host.c_str());
		qstrcpy(hint, host.c_str());
		my_cli = new cli_t;
		my_cli->size = sizeof(cli_t);
		my_cli->sname = sname;
		my_cli->lname = lname;
		my_cli->hint = hint;
		my_cli->execute_line = 0;
		my_cli->complete_line = 0;
		my_cli->keydown = 0;
	};

	void Terminate(void) {
		qthread_kill(my_t);
		my_t = 0;
		remove_command_interpreter(my_cli);
		delete my_cli;
		running = false;
	};
	void StartCli( 
		bool(idaapi*execute_line)(const char*),
		bool(idaapi*keydown)(qstring *, int *, int *, int *, int) = 0,
		bool(idaapi*complete_line)(qstring *, const char*, int, const char*, int) = 0
		)
	{
		my_cli->execute_line = execute_line;
		my_cli->keydown = keydown;
		my_cli->complete_line = complete_line;
		install_command_interpreter(my_cli);
	};
		
	void Running(void) { running = true; };
	bool Terminated(void) { return running == false; };
	void CreateThread(qthread_cb_t thr_cb) 
	{
		my_t = qthread_create(thr_cb, (void*)this);
	};
};

//telnet command code 
#define IAC 255
#define DODO 253
#define DONT 254
#define WILL 251
#define WONT 252
#define SB 250
#define SE 240
#define IS '0'
#define SEND '1'
#define INFO '2'
#define VAR '0'
#define VALUE '1'
#define ESC '2'
#define USERVAR '3'

//option name and id
#define ECHO_ON 1
#define GO_AHEAD 3
#define STATUS 5
#define TIMER 6
#define TERMINAL 24 
#define NAWS 31
#define TSPEED 32
#define FLOW 33
#define LINEMODE 34
#define XDISPLOC 35 // X Display Location 
#define OLD_ENVIRON 36 // Old - Environment variables 
#define AUTH 37
#define NEW_ENVIRON 39

class TelnetClient : public SocketClientT {
private:
	bool debug;
public:
	TelnetClient(const std::string &host, int dbg=0);
	void TelnetCommand(const char *fmt, ...);
	int ParseAndRespondServer();	
};


class Rlogin : public SocketClientT {
private:
public:
	bool debug;
	Rlogin(const std::string &host, int dbg=0);
	void RloginLogin(const std::string &username, 
		const std::string &password, 
		const std::string &term,
		const std::string &baud);
	int ParseAndRespondServer(void);
};

class RExec : public SocketClientT {
private:
public:
	bool debug;
	RExec(const std::string &host, int dbg=0);
	void RExecExec(const std::string &username, 
		const std::string &password, 
		const std::string &cmd);
	int ParseAndRespondServer(void);
};


class telnet_helpers {
public:
	telnet_helpers() ;
	~telnet_helpers();
private:
	void setup_telnet(void);
	void remove_telnet(void);
};

error_t telnet_connect(void);
error_t rlogin_connect(void);
error_t rexec_connect(void);

extern TelnetClient *telnet_curr;
extern Rlogin *rlogin_curr;
extern RExec *rexec_curr;

int idaapi RExec__WorkingThread(void *ud);


#endif
