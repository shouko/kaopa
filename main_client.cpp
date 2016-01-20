#include <iostream>
#include <string.h>
#include <thread>
#include <map>
#include <sstream>
#include <ncurses.h>
#include <locale.h>
#include <time.h>
#include "util.h"
#include "ascii_art.h"
#define UNSPEC_BALANCE -100000
#define EMPTY_LINE "                                                                                "
using namespace std;

struct User{
	string username;
	string ip;
	string port;
};

struct Payment{
	string user_from;
	string amount;
	string user_to;
};

SafeQueue<Payment> paymentQueue;

class Client{
private:
	SecureSocket* s;
	string remote_host;
	string username;
	int balance;
	int local_port;
	map<string,User> users;
	void parse_list(const char* list){
		stringstream ss(list);
		int online_users;
		ss >> balance >> online_users;
		string user_info;
		User user;
		users.erase(users.begin(), users.end());
		while(ss >> user_info){
			stringstream uss(user_info);
			getline(uss, user.username, '#');
			getline(uss, user.ip, '#');
			getline(uss, user.port, '\n');
			users.insert(make_pair(user.username, user));
		}
	}

public:
	Client(string remote_host, const char* port) : remote_host(remote_host), username(""), balance(UNSPEC_BALANCE), local_port(0){
		s = new SecureSocket(this->remote_host.c_str(), port);
		s->recv();
	}
	~Client(){
		delete s;
	}
	int setlocalport(int local_port){
		this->local_port = local_port;
		return 0;
	}
	int send(const char* command){
		return s->send(command);
	}
	const char* recv(){
		return s->recv();
	}
	const char* exec(const char* command){
		s->send(command);
		return s->recv();
	}
	const char* exec(string command){
		return exec(command.c_str());
	}
	int login(){
		const char* list = exec(username + "#" + to_string(local_port) + "\n");
		if(list[3] == ' '){
			return -1;
		}
		parse_list(list);
		return 0;
	}
	int reg(string username){
		if(exec("REGISTER#" + username + "\n")[0] == '1'){
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
	void bye(){
		s->send("Exit\n");
		s->recv();
	}
	bool is_loggedin(){
		return (balance != UNSPEC_BALANCE);
	}
	int get_onlineusers(){
		return users.size();
	}
	void fetch_list(){
		parse_list(exec("List\n"));
	}
};

void tui_init(){
	setlocale(LC_ALL,"");
	initscr();
	keypad(stdscr, true);
	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLUE); // title
	init_pair(2, COLOR_YELLOW, COLOR_BLUE); // app title
	init_pair(3, COLOR_BLACK, COLOR_WHITE); // reverse
	init_pair(4, COLOR_BLUE, COLOR_CYAN); // statusbar time
	init_pair(5, COLOR_RED, COLOR_WHITE); // reverse important
	init_pair(6, COLOR_YELLOW, COLOR_MAGENTA); // weather
	init_pair(7, COLOR_CYAN, COLOR_BLACK); // menu
}

void get_weather(char* weather){
	Socket s("w.ntu.im", "80");
	s.send("GET /~b102020/kaopa.php?f=weather HTTP/1.0\n\n");
	string data(s.recv());
	stringstream wss(data);
	string wstr;
	getline(wss, wstr, '#');
	getline(wss, wstr, '#');
	strncpy(weather, wstr.c_str(), 19);
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

void print_title(const char* title){
	attron(COLOR_PAIR(1));
	mvprintw(0, 0, "【主功能表】");
	attroff(COLOR_PAIR(1));
}

void print_header(const char* title){
	attron(COLOR_PAIR(1));
	mvprintw(0, 0, EMPTY_LINE);
	attroff(COLOR_PAIR(1));
	print_title(title);
	attron(COLOR_PAIR(2));
	mvprintw(0, 37, "カオパ");
	attroff(COLOR_PAIR(2));
}

char* weekday_str[7] = {"日", "一", "二", "三", "四", "五", "六"};

void print_statusbar(const int online_users, const char* username){
	char* weather = new char[20];
	thread weather_thread(get_weather, weather);
	attron(COLOR_PAIR(3));
	mvprintw(23, 0, EMPTY_LINE);
	attroff(COLOR_PAIR(3));
	attron(COLOR_PAIR(4));
	time_t rawtime;
	struct tm* timeinfo;
	char time_buffer[80];
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(time_buffer, 80, "%w", timeinfo);
	int weekday_index = (int)time_buffer[0] - (int)'0';
	strftime(time_buffer, 80, "[%m/%d 星期%%s %R]", timeinfo);
	mvprintw(23, 0, time_buffer, weekday_str[weekday_index]);
	attroff(COLOR_PAIR(4));
	attron(COLOR_PAIR(6));
	printw("                 ");
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
	weather_thread.join();
	attron(COLOR_PAIR(6));
	mvprintw(23, 20, " [ %s ]  ", weather);
	attroff(COLOR_PAIR(6));
}

void payment_accept(SecureSocket* s){
	while(1){
		SecureSocket* c = s->accept();
		stringstream ps(c->recv());
		Payment payment_data;
		getline(ps, payment_data.user_from, '#');
		getline(ps, payment_data.amount, '#');
		getline(ps, payment_data.user_to, '\n');
		paymentQueue.push(payment_data);
		c->send("100 OK");
		delete c;
	}
}

void mvprint_menukey(const int y, const int x, const char key){
	move(y, x);
	attron(COLOR_PAIR(7));
	printw("%c", key);
	attroff(COLOR_PAIR(7));
}

bool ask_leave(){
	char yes[2] = {0};
	mvprintw(22, 0, "您確定要離開 [ カオパ ] 嗎(Y/N)？[N] ");
	attron(COLOR_PAIR(3));
	mvprintw(22, 37, "   ");
	move(22, 37);
	getnstr(yes, 1);
	attroff(COLOR_PAIR(3));
	if(yes[0] == 'y' || yes[0] == 'Y'){
		return 1;
	}else{
		move(22, 0);
		clrtoeol();
		return 0;
	}
}

int menu_command(char menu[][2][16], int items, int initial = 0){
	noecho();
	int jump_table[26] = {0};
	for(int i = 0; i < items; i++){
		int y = 13 + i;
		mvprintw(y, 22, "( )%s", menu[i][0] + 1);
		mvprintw(y, 39, "[          ]");
		mvprintw(y, 41, "%s", menu[i][1]);
		mvprint_menukey(y, 23, menu[i][0][0]);
		jump_table[menu[i][0][0] - 'A'] = i;
	}
	move(13, 20);
	int curpos = initial;
	bool selecting = 1;
	while(selecting){
		printw(" ");
		mvprintw(13 + curpos, 20, ">");
		move(13 + curpos, 20);
		int key = getch();
		switch(key){
			default:
				if(key >= 'a' && key <= 'z'){
					key -= 'a';
				}else if(key >= 'A' && key <= 'Z'){
					key -= 'A';
				}else{
					break;
				}
				curpos = jump_table[key];
				break;
			case KEY_UP:
				curpos += (-1 + items);
				break;
			case KEY_DOWN:
				curpos += 1;
				break;
			case KEY_RIGHT:
			case '\r':
			case '\n':
			case KEY_ENTER:
				printw(" ");
				selecting = 0;
				echo();
				return curpos;
		}
		curpos %= items;
	}
}

int main(int argc, char* argv[]){

	SecureSocket* p = new SecureSocket();
	p->listen();
	int local_port = p->getlocalport();
	thread (payment_accept, p).detach();

	char host[31], username[21];

	tui_init();
	print_welcome();
	mvprintw(19, 0, "請輸入要連線的主機: ");
	attron(COLOR_PAIR(3));
	mvprintw(19, 20, "          ");
	move(19, 20);
	getnstr(host, 30);
	attroff(COLOR_PAIR(3));
	char port[6] = "8889";
	if(argc > 1){
		strncpy(port, argv[1], 5);
	}
	Client client(host, port);
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
	print_header("主功能表");
	print_welcome();
//	mvprintw(9, 5, "\033[1;33m草蜢\033[m\n");
	print_statusbar(client.get_onlineusers(), username);

	char main_menu[5][2][16] = {
		{"Announce", "系統公告"},
		{"Pay", "發起付款"},
		{"History", "交易紀錄"},
		{"List", "用戶列表"},
		{"Goodbye", "離開"}
	};

	int cmd = 0;
	bool run = 1;
	while(run){
		cmd = menu_command(main_menu, 5, cmd);
		switch(cmd){
			default:
			case 0:
				break;
			case 3:
				client.fetch_list();
				print_statusbar(client.get_onlineusers(), username);
				break;
			case 4:
				if(ask_leave()){
					client.bye();
					run = 0;
				}
				break;
		}
	}

/*
	getnstr(command, 20);
	client.send(command);
	printw(client.recv());
*/
  getch();
	endwin();
	return 0;
}
