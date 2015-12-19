#include <iostream>
#include <string.h>
#include <thread>
#include <map>
#include <sstream>
#include <ncurses.h>
#include <locale.h>
#include "util.h"
#include "ascii_art.h"
#define UNSPEC_BALANCE -100000
#define EMPTY_LINE "                                                                                "
using namespace std;

SafeQueue<vector<string>*> cmdQueue;

struct User{
	string username;
	string ip;
	string port;
};

class Client{
private:
	Socket* s;
	string remote_host;
	string username;
	int balance;
	int local_port;
	map<string,User> users;

public:
	Client(string remote_host, const char* port) : remote_host(remote_host), username(""), balance(UNSPEC_BALANCE), local_port(0){
		s = new Socket(this->remote_host.c_str(), port);
		s->recv();
	}
	~Client(){
		delete s;
	}
	int setlocalport(int local_port){
		this->local_port = local_port;
		return 0;
	}
	int login(){
		s->send(username + "#" + to_string(local_port) + "\n");
		getch();
		return 0;
		/*
		if(list[3] == ' '){
			return -1;
		}
		stringstream ss(list);
		int online_users;
		ss >> balance >> online_users;
		string user_info;
		User user;
		while(ss >> user_info){
			stringstream uss(user_info);
			getline(uss, user.username, '#');
			getline(uss, user.ip, '#');
			getline(uss, user.port, '\n');
			users.insert(make_pair(user.username, user));
		}
		return 0;
		*/
	}
	int reg(string username){
		s->send("REGISTER#" + username + "\n");
		if(s->recv()[0] == '1'){
			this->username = username;
			return login();
		}else{
			return -1;
		}
	}
	int login(string username){
		this->username = username;
		return login();
	}
	bool is_loggedin(){
		return (balance != UNSPEC_BALANCE);
	}
	int send(const char* command){
		return s->send(command);
	}
	const char* recv(){
		return s->recv();
	}
};

void tui_init(){
	setlocale(LC_ALL,"");
	initscr();
	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLUE); // title
	init_pair(2, COLOR_YELLOW, COLOR_BLUE); // app title
	init_pair(3, COLOR_BLACK, COLOR_WHITE); // reverse
	init_pair(4, COLOR_BLUE, COLOR_CYAN); // statusbar time
	init_pair(5, COLOR_RED, COLOR_WHITE); // reverse important
	init_pair(6, COLOR_YELLOW, COLOR_MAGENTA); // weather
	init_pair(7, COLOR_CYAN, COLOR_BLACK); // menu
}

void print_block(int y, int x, int height, string graph[]){
	for(int i = 0; i < height; i++){
		mvprintw(y+i, x, "%s", graph[i].c_str());
	}
}

void print_welcome(){
	print_block(1, 3, 5, ascii_art::cat);
	print_block(2, 15, 3, ascii_art::rabbit);
	print_block(6, 23, 5, ascii_art::cat);
	print_block(7, 35, 3, ascii_art::rabbit);
	print_block(4, 43, 5, ascii_art::cat);
	print_block(5, 55, 3, ascii_art::rabbit);
}

void print_statusbar(const int online_users, const char* username){
	attron(COLOR_PAIR(3));
	mvprintw(23, 0, EMPTY_LINE);
	attroff(COLOR_PAIR(3));
	attron(COLOR_PAIR(4));
	mvprintw(23, 0, "[12/19 星期六 22:24]");
	attroff(COLOR_PAIR(4));
	attron(COLOR_PAIR(6));
	printw(" [ 晴時多雲 ]  ");
	attroff(COLOR_PAIR(6));
	attron(COLOR_PAIR(3));
	printw(" 線上");
	attroff(COLOR_PAIR(3));
	attron(COLOR_PAIR(5));
	printw("%d", online_users);
	attroff(COLOR_PAIR(5));
	attron(COLOR_PAIR(3));
	printw("人, 我是");
	attroff(COLOR_PAIR(3));
	attron(COLOR_PAIR(5));
	printw("%s", username);
	attroff(COLOR_PAIR(5));
}

void payment_accept(Socket* s){
	while(1){
		Socket* c = s->accept();
		vector<string>* payment_data = split(c->recv(), "#");
		if(payment_data->size() != 3){
			cmdQueue.push(payment_data);
			c->send("100 OK");
		}
		delete c;
	}
}

void mvprint_menukey(const int y, const int x, const char key){
	move(y, x);
	attron(COLOR_PAIR(7));
	printw("%c", key);
	attroff(COLOR_PAIR(7));
}

int main(int argc, char* argv[]){

	Socket* p = new Socket();
	p->listen();
	int local_port = p->getlocalport();
	thread (payment_accept, p).detach();

	char host[20], command[20], username[20];

	tui_init();
	print_welcome();
	mvprintw(19, 0, "請輸入要連線的主機: ");
	attron(COLOR_PAIR(3));
	mvprintw(19, 20, "          ");
	move(19, 20);
	getnstr(host, 20);
	attroff(COLOR_PAIR(3));
	Client client(host, "8889");
	client.setlocalport(local_port);

	while(1){
		mvprintw(20, 0, "請輸入登入代號，或以 new 註冊: ");
		attron(COLOR_PAIR(3));
		mvprintw(20, 31, "          ");
		move(20, 31);
		getnstr(username, 20);
		attroff(COLOR_PAIR(3));
		if(strcmp(username, "new") == 0){
			mvprintw(21, 0, "請輸入註冊代號: ");
			attron(COLOR_PAIR(3));
			printw("          ");
			move(21, 16);
			getnstr(username, 20);
			attroff(COLOR_PAIR(3));
			mvprintw(21, 0, "註冊並登入中...");
			client.reg(username);
		}else{
			mvprintw(21, 0, "登入中...");
			client.login(username);
		}
		if(!client.is_loggedin()){
			mvprintw(22, 0, "註冊或登入失敗...");
		}else{
			break;
		}
	}

	erase();
	attron(COLOR_PAIR(1));
	mvprintw(0, 0, EMPTY_LINE);
	mvprintw(0, 1, "【主功能表】");
	attroff(COLOR_PAIR(1));
	attron(COLOR_PAIR(2));
	mvprintw(0, 37, "カオパ");
	attroff(COLOR_PAIR(2));
	print_welcome();
//	mvprintw(9, 5, "\033[1;33m草蜢\033[m\n");
	mvprintw(13, 20, "(A)nnounce    [ 系統公告   ]");
	mvprint_menukey(13, 21, 'A');
	mvprintw(14, 20, "(L)ist        [ 使用者列表 ]");
	mvprint_menukey(14, 21, 'L');
	mvprintw(15, 20, "(G)oodbye     [ 離開       ]");
	mvprint_menukey(15, 21, 'G');
	print_statusbar(112, username);
	mvprintw(14, 18, ">");
	move(14, 18);
	while(1){
		getnstr(command, 20);
		client.send(command);
		printw(client.recv());
	}
  getch();
	endwin();
	return 0;
}
