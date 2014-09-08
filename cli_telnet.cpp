/* 
   Socket.cpp

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

#include <ida.hpp>
#include <expr.hpp>
#include <kernwin.hpp>
#include "cli_telnet.hpp"
#include <iostream>
#include <map>

#include <fcntl.h>
#include <stdarg.h>
//#include <sys/types.h>


using namespace std;

static int idaapi telnet_handler(void *);

int Socket::nofSockets_= 0;

#define LINELEN 256

void Socket::Start() {
  if (!nofSockets_) {
    WSADATA info;
    if (WSAStartup(MAKEWORD(2,0), &info)) {
      throw "Could not start WSA";
    }
  }
  ++nofSockets_;
}

void Socket::End() {
  WSACleanup();
}

Socket::Socket() : s_(0) {
  Start();
  // UDP: use SOCK_DGRAM instead of SOCK_STREAM
  s_ = socket(AF_INET,SOCK_STREAM,0);

  if (s_ == INVALID_SOCKET) {
    throw "INVALID_SOCKET";
  }

  refCounter_ = new int(1);
}

Socket::Socket(SOCKET s) : s_(s) {
  Start();
  refCounter_ = new int(1);
};

Socket::~Socket() {
  if (! --(*refCounter_)) {
    Close();
    delete refCounter_;
  }

  --nofSockets_;
  if (!nofSockets_) End();
}

Socket::Socket(const Socket& o) {
  refCounter_=o.refCounter_;
  (*refCounter_)++;
  s_         =o.s_;

  nofSockets_++;
}

Socket& Socket::operator=(Socket& o) {
  (*o.refCounter_)++;

  refCounter_=o.refCounter_;
  s_         =o.s_;

  nofSockets_++;

  return *this;
}

void Socket::Close() {
  closesocket(s_);
}

std::string Socket::ReceiveBytes() {
  std::string ret;
  char buf[1024];
 
  while (1) {
    u_long arg = 0;
    if (ioctlsocket(s_, FIONREAD, &arg) != 0)
      break;

    if (arg == 0)
      break;

    if (arg > 1024) arg = 1024;

    int rv = recv (s_, buf, arg, 0);
    if (rv <= 0) break;

    std::string t;

    t.assign (buf, rv);
    ret += t;
  }
 
  return ret;
}

int Socket::ReceiveBytes(unsigned char *buf, size_t size)
{
	int ret = 0;
	int rv = 0;
	while(1) {
		u_long arg = 0;
		if (ioctlsocket(s_, FIONREAD, &arg) != 0)
			break;
		if (arg == 0) 
			break;
		if (arg>size) arg = size;
		rv = recv(s_, (char*)buf, arg, 0);
		if (rv == size) {
			return size;
		}
		if (rv <=0 ) break;
		ret += rv;
	}
	return ret;
}

int Socket::ReceiveByte(unsigned char *ch)
{
	int rv = ReceiveBytes(ch, 1);
	return rv;
}

std::string Socket::ReceiveLine() {
  std::string ret;
  while (1) {
    char r;

    switch(recv(s_, &r, 1, 0)) {
      case 0: // not connected anymore;
              // ... but last line sent
              // might not end in \n,
              // so return ret anyway.
        return ret;
      case -1:
        return "";
//      if (errno == EAGAIN) {
//        return ret;
//      } else {
//      // not connected anymore
//      return "";
//      }
    }

    ret += r;
    if (r == '\n')  return ret;
  }
}

void Socket::SendLine(std::string s) {
  s += '\n';
  send(s_,s.c_str(),s.length(),0);
}

void Socket::SendBytes(const std::string& s) {
  send(s_,s.c_str(),s.length(),0);
}

void Socket::SendBytes(const char* s, size_t size) {
  send(s_, s ,size,0);
}

void Socket::SendByte(const char byte)
{
	SendBytes(&byte, 1);
}

SocketServer::SocketServer(int port, int connections, TypeSocket type) {
  sockaddr_in sa;

  memset(&sa, 0, sizeof(sa));

  sa.sin_family = PF_INET;             
  sa.sin_port = htons(port);          
  s_ = socket(AF_INET, SOCK_STREAM, 0);
  if (s_ == INVALID_SOCKET) {
    throw "INVALID_SOCKET";
  }

  if(type==NonBlockingSocket) {
    u_long arg = 1;
    ioctlsocket(s_, FIONBIO, &arg);
  }

  /* bind the socket to the internet address */
  if (bind(s_, (sockaddr *)&sa, sizeof(sockaddr_in)) == SOCKET_ERROR) {
    closesocket(s_);
    throw "INVALID_SOCKET";
  }
  
  listen(s_, connections);                               
}

Socket* SocketServer::Accept() {
  SOCKET new_sock = accept(s_, 0, 0);
  if (new_sock == INVALID_SOCKET) {
    int rc = WSAGetLastError();
    if(rc==WSAEWOULDBLOCK) {
      return 0; // non-blocking call, no request pending
    }
    else {
      throw "Invalid Socket";
    }
  }

  Socket* r = new Socket(new_sock);
  return r;
}

SocketClient::SocketClient(const std::string& host, int port) : Socket() {
  std::string error;

  hostent *he;
  if ((he = gethostbyname(host.c_str())) == 0) {
    error = strerror(errno);
    throw error;
  }

  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr = *((in_addr *)he->h_addr);
  memset(&(addr.sin_zero), 0, 8); 

  if (::connect(s_, (sockaddr *) &addr, sizeof(sockaddr))) {
    error = strerror(WSAGetLastError());
    throw error;
  }
}

SocketSelect::SocketSelect(Socket const * const s1, Socket const * const s2, TypeSocket type) {
  FD_ZERO(&fds_);
  FD_SET(const_cast<Socket*>(s1)->s_,&fds_);
  if(s2) {
    FD_SET(const_cast<Socket*>(s2)->s_,&fds_);
  }     

  TIMEVAL tval;
  tval.tv_sec  = 0;
  tval.tv_usec = 1;

  TIMEVAL *ptval;
  if(type==NonBlockingSocket) {
    ptval = &tval;
  }
  else { 
    ptval = 0;
  }

  if (select (0, &fds_, (fd_set*) 0, (fd_set*) 0, ptval) == SOCKET_ERROR) 
    throw "Error in select";
}

bool SocketSelect::Readable(Socket const* const s) {
  if (FD_ISSET(s->s_,&fds_)) return true;
  return false;
}


void TelnetClient::TelnetCommand(const char *fmt, ...)
{
	char buf[LINELEN];
	va_list args;
	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	SendBytes(buf, strlen(buf));
}

//tel_cli.c:
// Name: tel_cli.c
// Author: Robert 
// Date: Jan 03 2002--- ? 
// Called: This module is a part of <<Traffic Data Management>> application. 
// Purpose: Used to login into the remote machine and run command on the remote machine. 
// Format: tel_cli $1
// Input: $1 is the hostname of the remote machine, config in the local /etc/hosts file. 
// Example: tel_cli 134.132.5.11 or tel_cli lyts1
// return: none





// the struction include option id, option name and option flag, the flag indicates 
// if the client answer to the option yes or no, flag ==1 means that yes.
struct OptionFlag
{
	int Id;
	char * Name;
	int Flag;
};
struct OptionFlag struOptionFlag[40];


//extern int errno;

int create_server(int port);
int create_ftp_cli(const char *host, const char *service);
int get_reply(int sock);
int get_ts(int sock);
int connectTCP(const char *host, const char *service);
int errexit(const char * format,...);

void sigroutine(int unused);

int ser_sock; //the server socket waiting for the ftp server to connect and accept them a data_sock. 
int data_sock = 0; //the socket for receiving or sending data file.
int ctrl_sock;

int para[7];
char * gs_filename; //file name we need 
char * gs_omc_passwd;
int g_iFileLen; //the length of file
int g_iReceivedLen = 0; //the bytes has been received

//char gsSendMsg[LINELEN];
//char gsReceiveMsg[LINELEN];


fd_set all_fds;


static int idaapi telnet_handler(void *ud);



//// main progress, according to the number of parameters, the application will decide what to do. 
//int main(int argc,char * argv[])
//{
//	char *host = "lyts1"; // host to use if none supplied
//	char *service = "telnet"; // default service name
//	
//	switch (argc)
//	{
//	case 1:
//		
//		printf("Usage: getfile [front-name yymmdd]\n");
//		exit(1);
//		
//	case 3:
//		//service = argv[2];
//		printf("Usage: getfile [front-name yymmdd]\n");
//		
//	case 2:
//		host = argv[1];
//		//gs_omc_passwd = argv[2];
//		//gs_filename = argv[3];
//		break;
//		
//	default:
//		fprintf(stderr,"Usage: getfile [front-name yymmdd]\n");
//		exit(1);
//	}
//	
//	FD_ZERO(&all_fds);
//	ctrl_sock = create_telnet_cli(host, service);
//	FD_SET(ctrl_sock, &all_fds);
//	printf("ctrl_sock:%d\n", ctrl_sock);
//	
//	//send_command(ctrl_sock);
//	exit(0);
//}

 TelnetClient *telnet_curr = 0;
 Rlogin *rlogin_curr = 0;
 RExec *rexec_curr = 0;

bool idaapi remote_conn_execute_line(SocketClientT *curr, const char *line)
{
	//msg("%s\n", line);
	bool terminate = false;

	try {
		if(strncmp(line, "dis", 3) == 0) {
			terminate = true;
		} else {
			std::string line_s(line);
			if (!line_s.empty()) {
				curr->SendLine(line_s);
			}
		}
	}
	catch (const char *s ) {
		msg("Error: %s\n", s);
		terminate = true;
	} 
	catch (std::string s) {
		msg("Error: %s\n", s.c_str());
		terminate = true;
	}
	catch (...) {
		msg("Unhandled...\n");
		terminate = true;
	}

	if (terminate == true) {
		curr->Terminate();
		curr->Close();
	}
	return true;
	
};

bool idaapi remote_conn_execute_line_telnet(const char *line)
{
	bool ret = remote_conn_execute_line(telnet_curr, line);
	if (telnet_curr->Terminated()) { 
		delete telnet_curr;
		telnet_curr = 0;
	}
	return ret;
}

bool idaapi remote_conn_execute_line_rlogin(const char *line)
{
	bool ret = remote_conn_execute_line(rlogin_curr, line);
	if (rlogin_curr->Terminated()) {
		delete rlogin_curr;
		rlogin_curr = 0;
	}
	return ret;
}

bool idaapi remote_conn_execute_line_rexec(const char *line)
{
	bool ret = remote_conn_execute_line(rexec_curr, line);
	if (rexec_curr->Terminated()) {
		delete rexec_curr;
		rexec_curr = 0;
	}
	return ret;
}

static int idaapi telnet_handler(void *ud)
{
	TelnetClient *t = (TelnetClient *)ud;
	while (!t->Terminated()) {
		SocketSelect sel(t);
		while(sel.Readable(t)) {
			t->ParseAndRespondServer();
		}
	}
	return 0;
}

TelnetClient::TelnetClient(const std::string &host, int dbg)
	: SocketClientT(host, 23),
	debug(dbg)
//	int create_telnet_cli(const char *host, const char *service)
{
	unsigned char rbuf[LINELEN]; // buffer for one line of text
	unsigned char wbuf[LINELEN]; // buffer for one line of text
//	int outchars, inchars; // characters sent and received

	if (debug) msg("entering into telnet_cli\n");
	if (debug) msg("li_socket is: %d\n", s_);
	memset(wbuf, 0, LINELEN);
	memset(rbuf, 0, LINELEN);
	
#if 1	
	TelnetCommand("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
		IAC, DODO, GO_AHEAD,
		IAC, WILL, TERMINAL,
		IAC, WILL, NAWS,
		IAC, WILL, TSPEED, 
		IAC, WILL, FLOW, 
		IAC, WILL, LINEMODE, 
		IAC, WILL, NEW_ENVIRON, 
		IAC, DODO, STATUS);
#endif
	Running();
	StartCli(remote_conn_execute_line_telnet);
}
/*VARARGS1*/

int TelnetClient::ParseAndRespondServer(void)
{
	int li_count;
	unsigned char rbuf[LINELEN];
	unsigned char ch;
	unsigned char lcOption;

	if ( (li_count = ReceiveByte(&ch)) > 0) {
		switch (ch) {
		case 10:
			break;
		case 13:
			msg("\n");
			break;
		case 27:
			if (debug) msg("escape sequence...\n");
			break;
		case IAC:
			ReceiveBytes(rbuf, 2);
			ch = rbuf[0];
			lcOption = rbuf[1];
			if (debug) msg(" ch: %d lcOption: %d\n", ch, lcOption);
			switch (ch) {
			case DODO:
				switch(lcOption) {
				case AUTH:
					if (debug) msg("RECV DO AUTHENTICATION (37)\n");
					//sleep(5);
					TelnetCommand("%c%c%c", IAC, WONT, AUTH);
					break;
				case NAWS:
					if (debug) msg("RECV DO NAWS (31)\n");
					//sleep(5);
					//////SEND IAC SB NAWS 0 132 (132) 0 52 (52)
					//the key point is to find SB syntax.
					TelnetCommand("%c%c%c%d%d%d%d%c%c",
						IAC, SB, NAWS, 0,80,0,24, IAC,SE);
					break;
				case  XDISPLOC:
					if (debug) msg("RECV DO XDISPLOC (35)\n");
					//sleep(5);
					TelnetCommand("%c%c%c\n", IAC, WONT, XDISPLOC);
					break;
				case OLD_ENVIRON:
					if (debug) msg("RECV DO OLD-ENVIRON (36)\n");
					//sleep(5);
					TelnetCommand("%c%c%c\n", IAC, WONT, OLD_ENVIRON);
					break;
				case ECHO_ON:
					if (debug) msg("RECV DO ECHO (1)\n");
					//sleep(5);
					TelnetCommand("%c%c%c", IAC, WONT, ECHO_ON);
					break;
				default:
					TelnetCommand("%c%c%c", IAC, WONT, lcOption);
					break;
				}
				break;
				
			case DONT:
				if (debug) msg("ch is DONT\n");
				break;
			case WILL:
				if (lcOption == ECHO_ON) {
					if (debug) msg("RECV WILL ECHO (1)\n");
					//sleep(5);
					TelnetCommand("%c%c%c", IAC, DODO, ECHO_ON);
				}
				break;
			case WONT:
				break;
			case SB:
				if (debug) msg("ch is SB\n");
				switch(lcOption) {
					case TSPEED:
						if (debug) msg("RECV IAC SB TERMINAL SPEED SEND (32)\n");
						ReceiveBytes(rbuf, 3);
						if (debug) msg("%d%d%d", rbuf[0], rbuf[1], rbuf[2]);
						//i = i + 3;
						//sleep(5);
						TelnetCommand("%c%c%c%c%c%c%c%c%c%c",
							IAC, SB, TSPEED, IS,
							38400 >> 8, (38400 & 0xFF),
							38400 >> 8, (38400 & 0xFF),
							IAC,SE);
						break;
					case NEW_ENVIRON:
						ReceiveBytes(rbuf, 3);
						msg("RECV IAC SB NEW-ENVIRON SEND (39)\n");
						if (debug) msg("%d%d%d\n", rbuf[0], rbuf[1], rbuf[2]);
						//i = i + 3;
						//sleep(5);
						TelnetCommand("%c%c%c%c%c%c\n",
							IAC, SB, NEW_ENVIRON, IS, IAC, SE);
						break;
					case TERMINAL:
						msg("RECV IAC SB TERMINAL TYPE SEND (24)\n");
						ReceiveBytes(rbuf, 3);
						if (debug) msg("%d%d%d\n", rbuf[0], rbuf[1], rbuf[2]);
						//i = i + 3;
						//sleep(5);
						TelnetCommand("%c%c%c%c%s%c%c",
							IAC, SB, NEW_ENVIRON, IS, "ANSI", IAC, SE);
						if (debug) msg("okokok\n");
						TelnetCommand("%c%c%c", IAC, DODO,ECHO_ON);
						if (debug) msg("okokok\n");
						break;
					default:
						break;
				} // end of SB
			} // end of switch(ch)
			break;
		default:
			msg("%c", ch);
			break;
        }
	}
	return li_count;
}

int errexit(const char *format,...)
{
	va_list args;
	va_start(args,format);
	vfprintf(stderr,format,args);
	va_end(args);
	exit(1);
}

static const char telnet_tbl_args[] = { 0 };
static error_t idaapi telnet_tbl_idc(idc_value_t *argv, idc_value_t *res)
{
	return telnet_connect();
}

error_t telnet_connect()
{
	if (telnet_curr!=0) {
		msg("please current telnet connection first\n");
	} else {
		char *connect_dg =
			"STARTITEM 0\n"
			"Connect to a remote system\n\n"
			"System name\n"
			"<hostname:A:32:32::>\n"  // hostname
			"<Debug:C>>\n"; // port
		char hostname[MAXSTR] = "nlbautd3";
		ushort debug = 0;
		if (AskUsingForm_c(connect_dg, hostname, &debug) == 1) {
			msg("hostname: %s\n", hostname);
			std::string hostname_s(hostname);
			try {
				telnet_curr = new TelnetClient(hostname_s, int(debug));
			}
			catch(...) {
				msg("Telnet Failed\n");
				return 1;
			}
			telnet_curr->CreateThread(telnet_handler);
		}
	}
	return 0;
}

//
// Rlogin
//

int idaapi Rlogin__WorkingThread(void *ud)
{
	Rlogin *r = (Rlogin*)ud;
	while (!r->Terminated()) {
		SocketSelect sel(r);
		while(sel.Readable(r)) {
			r->ParseAndRespondServer();
		}
	}
	return 0;
}

Rlogin::Rlogin(const std::string &host, int dbg)
	: SocketClientT(host, 513),
	debug(dbg)
{

}

void Rlogin::RloginLogin(const std::string& username,
	const std::string& password,
	const std::string& term,
	const std::string& baud)
{
	SendByte(0);	SendBytes(username);
	SendByte(0);	SendBytes(password);
	SendByte(0);	SendBytes(term);
	SendByte('/');	SendBytes(baud);
	SendByte(0);
	unsigned char ch;
	int reply = ReceiveByte(&ch);
	Running();
	StartCli(remote_conn_execute_line_rlogin);
	
}

static const char rlogin_tbl_args[] = { 0 };
static error_t idaapi rlogin_tbl_idc(idc_value_t *argv, idc_value_t *res)
{
	return rlogin_connect();
}
error_t rlogin_connect() {

	if (rlogin_curr!=0) {
		msg("please current rlogin connection first\n");
	} else {
		char *connect_dg =
			"STARTITEM 0\n"
			"Connect to a remote system\n\n"
			"System name\n"
			"<hostname:A:32:32::>\n"  // hostname
			"<username:A:32:32::>\n"  // username
			"<password:A:32:32::>\n"  // password
			"<Debug:C>>\n"; // 
		char hostname[MAXSTR] = "nlbautd3";
		char username[MAXSTR] = "acohens";
		char password[MAXSTR] = "12yerusala";
		char term[MAXSTR] = "vt100";
		char baud[MAXSTR] = "9600";
		ushort debug = 0;
		if (AskUsingForm_c(connect_dg,
			hostname, username, password, &debug) == 1) {
			msg("hostname: %s\n", hostname);
			std::string hostname_s(hostname);
			std::string username_s(username);
			std::string password_s(password);
			std::string term_s(term);
			std::string baud_s(baud);
			try {
				rlogin_curr = new Rlogin(hostname_s, (int)debug);
			}
			catch(...) {
				msg("Rlogin failed\n");
				return 1;
			};
			rlogin_curr->RloginLogin(username_s, 
				password_s,
				term_s,
				baud_s);
			rlogin_curr->CreateThread(Rlogin__WorkingThread);
		}
	}
	return 0;
}


int Rlogin::ParseAndRespondServer(void)
{
	int li_count;
	unsigned char ch;
	li_count = ReceiveByte(&ch);
	if (li_count>0) msg("%c", ch);
	return li_count;
}


//
// RExec
//

int idaapi RExec__WorkingThread(void *ud)
{
	RExec *r = (RExec*)ud;
	while (!r->Terminated()) {
		SocketSelect sel(r);
		while( (sel.Readable(r) &&
			(r->ParseAndRespondServer()>0) ) );
	}
	return 0;
}

RExec::RExec(const std::string &host, int dbg)
	: SocketClientT(host, 512),
	debug(dbg)
{

}

void RExec::RExecExec(const std::string& username,
	const std::string& password,
	const std::string& cmd)
{
	SendByte(0);	SendBytes(username);
	SendByte(0);	SendBytes(password);
	SendByte(0);	SendBytes(cmd);
	SendByte(0);
	unsigned char ch;
	int reply = ReceiveByte(&ch);
	Running();
	StartCli(remote_conn_execute_line_rexec);
	
}

static const char rexec_tbl_args[] = { 0 };
static error_t idaapi rexec_tbl_idc(idc_value_t *argv, idc_value_t *res)
{
	return rexec_connect();
}

error_t rexec_connect()
{
	if (rexec_curr!=0) {
		msg("please current rlogin connection first\n");
	} else {
		char *connect_dg =
			"STARTITEM 0\n"
			"Connect to a remote system\n\n"
			"System name\n"
			"<hostname:A:32:32::>\n"  // hostname
			"<username:A:32:32::>\n"  // username
			"<password:A:32:32::>\n"  // password
			"<execute:A:32:32::>\n"   // exec
			"<Debug:C>>\n"; // 
		char hostname[MAXSTR] = "nlbautd3";
		char username[MAXSTR] = "acohens";
		char password[MAXSTR] = "12yerusala";
		char cmd[MAXSTR] = "/bin/ksh";
		ushort debug = 0;
		if (AskUsingForm_c(connect_dg,
			hostname, username, password, cmd, &debug) == 1) {
			msg("hostname: %s\n", hostname);
			std::string hostname_s(hostname);
			std::string username_s(username);
			std::string password_s(password);
			std::string cmd_s(cmd);

			try {
				rexec_curr = new RExec(hostname_s, (int)debug);
			}
			catch(...) {
				msg("RExec failed\n");
				return 1;
			};
			rexec_curr->RExecExec(username_s, 
				password_s,
				cmd_s);
			rexec_curr->CreateThread(RExec__WorkingThread);
		}
	}
	return 0;
}


int RExec::ParseAndRespondServer(void)
{
	int li_count;
	unsigned char ch;
	li_count = ReceiveByte(&ch);
	if (li_count>0) msg("%c", ch);
	return li_count;
}


void telnet_helpers::setup_telnet(void)
{

}

void telnet_helpers::remove_telnet(void)
{
	if (telnet_curr) {
		telnet_curr->Terminate();
		telnet_curr->Close();
		delete telnet_curr;
		telnet_curr = 0;
	}
	if (rlogin_curr) {
		rlogin_curr->Terminate();
		rlogin_curr->Close();
		delete rlogin_curr;
		rlogin_curr = 0;
	}
	if (rexec_curr) {
		rexec_curr->Terminate();
		rexec_curr->Close();
		delete rexec_curr;
		rexec_curr = 0;
	}

}

telnet_helpers::telnet_helpers()
{
	set_idc_func_ex("telnet", telnet_tbl_idc, telnet_tbl_args, 0);
	set_idc_func_ex("rlogin", rlogin_tbl_idc, rlogin_tbl_args, 0);
	set_idc_func_ex("rexec", rexec_tbl_idc, rexec_tbl_args, 0);
	setup_telnet();
}

telnet_helpers::~telnet_helpers()
{
	set_idc_func_ex("telnet", NULL, NULL, 0);
	set_idc_func_ex("rlogin", NULL, NULL, 0);
	set_idc_func_ex("rexec", NULL, NULL, 0);
	remove_telnet();
}