#include <iostream>
#include <string.h>
#include <thread>
#include <map>
#include <sstream>
#include <ncurses.h>
#include <locale.h>
#include <time.h>
#include <unistd.h>
#include "util.h"
#include "ascii_art.h"
#define UNSPEC_BALANCE -100000
#define EMPTY_LINE "                                                                                "
#define EMPTY_LINE_50 "                                                  "
#define KEEP_TRANSACTION_STATUS 0
using namespace std;
typedef Socket PeerSocket;
typedef SecureSocket ServerSocket;

struct User{
	string username;
	string ip;
	string port;
};

struct Payment{
public:
	Payment(){}
	string user_from;
	string amount;
	string user_to;
};

enum PayResult{
	Success,
	PeerNotOnline,
	PeerConnIssue,
	InsufficientBalance,
	IllegalAmount,
	GeneralFailure
};

SafeQueue<Payment> paymentQueue;
vector<Transaction> transactionHistory;

class Client{
private:
	ServerSocket* s;
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
	Client(const char* remote_host, const char* port) : remote_host(remote_host), username(""), balance(UNSPEC_BALANCE), local_port(0){
		s = new ServerSocket(this->remote_host.c_str(), port);
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
	const string* get_info(){
		const string info[3] = {
			s->get_cipher_name(),
			s->get_cert_subject(),
			s->get_cert_issuer()
		};
		return info;
	}
	const int get_balance(){
		return balance;
	}
	PayResult pay(string to_user, int amount){
		map<string, User>::iterator it = users.find(to_user);
		if(it == users.end()){
			return PeerNotOnline;
		}
		if(amount > get_balance()){
			return InsufficientBalance;
		}
		if(amount < 0){
			return IllegalAmount;
		}
		try{
			//			SecureSocket s_to(it->second.ip, it->second.port);
			PeerSocket sto(it->second.ip.c_str(), it->second.port.c_str());
			sto.recv(); // get Hello
			string request = "0#" + this->username + "#" + to_string(amount) + "#" + to_user + "#\n";
			sto.send(request);
			string result(sto.recv());
			if(result[0] != '1'){
				return PeerConnIssue;
			}
		}catch(SocketException e){
			return PeerConnIssue;
		}
#if not KEEP_TRANSACTION_STATUS
		Transaction trans;
		trans.user_from = "我";
		trans.user_to = to_user;
		trans.amount = to_string(amount);
		transactionHistory.push_back(trans);
#endif
		return Success;
	}
friend void show_list();
};

Client *c;

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
	init_pair(8, COLOR_WHITE, COLOR_GREEN); // success trans
	init_pair(9, COLOR_WHITE, COLOR_RED); // fail trans
}

void draw_borders(WINDOW *screen) {
  int x, y, i;
  getmaxyx(screen, y, x);
  // 4 corners
  mvwprintw(screen, 0, 0, "+");
  mvwprintw(screen, y - 1, 0, "+");
  mvwprintw(screen, 0, x - 1, "+");
  mvwprintw(screen, y - 1, x - 1, "+");
  // sides
  for (i = 1; i < (y - 1); i++) {
    mvwprintw(screen, i, 0, "|");
    mvwprintw(screen, i, x - 1, "|");
  }
  // top and bottom
  for (i = 1; i < (x - 1); i++) {
    mvwprintw(screen, 0, i, "-");
    mvwprintw(screen, y - 1, i, "-");
  }
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

char const* weekday_str[7] = {"日", "一", "二", "三", "四", "五", "六"};

void print_statusbar(const int online_users, const char* username, const int balance){
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
	attron(COLOR_PAIR(3));
	printw(", 帳戶餘額");
	attroff(COLOR_PAIR(3));
	attron(COLOR_PAIR(5));
	printw("%d", balance);
	attroff(COLOR_PAIR(5));
	attron(COLOR_PAIR(3));
	printw("元");
	attroff(COLOR_PAIR(3));
	weather_thread.join();
	attron(COLOR_PAIR(6));
	mvprintw(23, 20, " [ %s ]  ", weather);
	attroff(COLOR_PAIR(6));
}

void payment_ack(Payment& payment_data){
	// redeem the incoming payment
	string request = "Pay\n" + payment_data.user_from + "#" + payment_data.amount + "#" + payment_data.user_to + "#";
	const char* result = c->exec(request);
	Transaction trans;
	trans.user_to = "我";
	trans.user_from = payment_data.user_from;
	trans.amount = payment_data.amount;
	trans.success = (result[0] == '1');
	transactionHistory.push_back(trans);
}

void payment_accept(PeerSocket* s){
	while(1){
		try{
			PeerSocket* c = s->accept();
			c->send("Hello\n");
			string request(c->recv());
			stringstream ps(request);
			Payment payment_data;
			string action;
			getline(ps, action, '#');
			if(action[0] == '0'){
				// payment from peer
				getline(ps, payment_data.user_from, '#');
				getline(ps, payment_data.amount, '#');
				getline(ps, payment_data.user_to, '#');
				c->send("100 OK\n");
				payment_ack(payment_data);
			}else if(action[0] == '1' || action[0] == '2'){
				// ACK from server about my outgoing payments
#if KEEP_TRANSACTION_STATUS
				Transaction trans;
				trans.user_from = "我";
				getline(ps, trans.user_to, '#');
				getline(ps, trans.amount, '#');
				trans.success = action[0] == '1';
				transactionHistory.push_back(trans);
#endif
				c->send("100 OK\n");
			}
			delete c;
		}catch(SocketException e){
			continue;
		}
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

void init_payment(){
	WINDOW* info_window = newwin(12, 50, 8, 15);
	draw_borders(info_window);
	mvwprintw(info_window, 0, 19, "[ 發起付款 ]");
	wattron(info_window, COLOR_PAIR(3));
	for(int i = 0; i < 10; i++){
		mvwprintw(info_window, 1 + i, 0, EMPTY_LINE_50);
	}
//	mvwprintw(info_window, 10, 18, "[ 任意鍵關閉 ]");
	mvwprintw(info_window, 4, 4, "收款帳號：");
	mvwprintw(info_window, 6, 4, "交易金額：");
	wattroff(info_window, COLOR_PAIR(3));
	wattron(info_window, COLOR_PAIR(4));
	mvwprintw(info_window, 4, 14, "               ");
	mvwprintw(info_window, 6, 14, "               ");
	wrefresh(info_window);
	char to_user[21];
	int amount = 0;
	mvwscanw(info_window, 4, 14, "%s", to_user);
	mvwscanw(info_window, 6, 14, "%d", &amount);
	wattroff(info_window, COLOR_PAIR(4));
	wattron(info_window, COLOR_PAIR(3));
	mvwprintw(info_window, 8, 4, "交易進行中...                      ");
	wrefresh(info_window);
	const char* msg;
	switch(c->pay(to_user, amount)){
		case Success:
			msg = "交易送出！請等待目標客戶端向伺服器兌換";
			break;
		case PeerNotOnline:
			msg = "交易失敗：目標帳號不存在，或不在線上";
			break;
		case InsufficientBalance:
			msg = "交易失敗：餘額不足";
			break;
		case IllegalAmount:
			msg = "交易失敗：非法金額";
			break;
		case PeerConnIssue:
			msg = "交易失敗：目標客戶端連線問題";
			break;
		default:
			msg = "交易失敗！";
			break;
	}
	mvwprintw(info_window, 8, 4, msg);
	mvwprintw(info_window, 10, 18, "[ 任意鍵關閉 ]");
	wrefresh(info_window);
	wgetch(info_window);
	wattroff(info_window, COLOR_PAIR(3));
	delwin(info_window);
	touchwin(stdscr);
}

void show_info(const string* info){
	WINDOW* info_window = newwin(12, 50, 8, 15);
	draw_borders(info_window);
	mvwprintw(info_window, 0, 19, "[ 系統資訊 ]");
	curs_set(0);
	wattron(info_window, COLOR_PAIR(3));
	for(int i = 0; i < 10; i++){
		mvwprintw(info_window, 1 + i, 0, EMPTY_LINE_50);
	}
	mvwprintw(info_window, 10, 18, "[ 任意鍵關閉 ]");
	mvwprintw(info_window, 1, 0, "加密方法：%s", info[0].c_str());
	mvwprintw(info_window, 2, 0, "憑證資訊：%s", info[1].c_str());
	wrefresh(info_window);
	getch();
	wattroff(info_window, COLOR_PAIR(3));
	delwin(info_window);
	curs_set(1);
	touchwin(stdscr);
}

void show_history(){
	WINDOW* info_window = newwin(15, 50, 5, 15);
	draw_borders(info_window);
	mvwprintw(info_window, 0, 19, "[ 交易紀錄 ]");
	curs_set(0);
	wattron(info_window, COLOR_PAIR(3));
	for(int i = 0; i < 13; i++){
		mvwprintw(info_window, 1 + i, 0, EMPTY_LINE_50);
	}
	mvwprintw(info_window, 1, 1, "#");
	mvwprintw(info_window, 1, 4, "付款方");
	mvwprintw(info_window, 1, 18, "收款方");
	mvwprintw(info_window, 1, 32, "金額");
#if KEEP_TRANSACTION_STATUS
	mvwprintw(info_window, 1, 40, "狀態");
#endif
	int i = 2;
	for(vector<Transaction>::iterator it = transactionHistory.begin(); it != transactionHistory.end(); it++){
		mvwprintw(info_window, i, 1, "%d", i - 1);
		mvwprintw(info_window, i, 4, it->user_from.c_str());
		mvwprintw(info_window, i, 18, it->user_to.c_str());
		mvwprintw(info_window, i, 32, it->amount.c_str());
#if KEEP_TRANSACTION_STATUS
		wattroff(info_window, COLOR_PAIR(3));
		if(it->success){
			wattron(info_window, COLOR_PAIR(8));
			mvwprintw(info_window, i, 40, "[ 成功 ]");
			wattroff(info_window, COLOR_PAIR(8));
		}else{
			wattron(info_window, COLOR_PAIR(9));
			mvwprintw(info_window, i, 40, "[ 失敗 ]");
			wattroff(info_window, COLOR_PAIR(9));
		}
		wattron(info_window, COLOR_PAIR(3));
#endif
		i++;
	}
	mvwprintw(info_window, 13, 18, "[ 任意鍵關閉 ]");
	wrefresh(info_window);
	getch();
	wattroff(info_window, COLOR_PAIR(3));
	delwin(info_window);
	curs_set(1);
	touchwin(stdscr);
}

void show_list(){
	WINDOW* info_window = newwin(12, 50, 8, 15);
	draw_borders(info_window);
	mvwprintw(info_window, 0, 19, "[ 線上用戶 ]");
	curs_set(0);
	wattron(info_window, COLOR_PAIR(3));
	for(int i = 0; i < 10; i++){
		mvwprintw(info_window, 1 + i, 0, EMPTY_LINE_50);
	}
	mvwprintw(info_window, 1, 1, "用戶名稱");
	mvwprintw(info_window, 1, 14, "位址");
	int i = 2;
	for(map<string, User>::iterator it = c->users.begin(); it != c->users.end(); it++){
		mvwprintw(info_window, i, 1, "%s", it->second.username.c_str());
		mvwprintw(info_window, i, 14, "%s:%s", it->second.ip.c_str(), it->second.port.c_str());
		i++;
	}
	mvwprintw(info_window, 10, 18, "[ 任意鍵關閉 ]");
	wrefresh(info_window);
	getch();
	wattroff(info_window, COLOR_PAIR(3));
	delwin(info_window);
	curs_set(1);
	touchwin(stdscr);
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
				selecting = 0;
				echo();
				return curpos;
		}
		curpos %= items;
	}
	return 0;
}

int main(int argc, char* argv[]){

	PeerSocket* p = new PeerSocket();
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
	c = &client;
	erase();
	print_header("主功能表");
	print_welcome();
//	mvprintw(9, 5, "\033[1;33m草蜢\033[m\n");
	print_statusbar(client.get_onlineusers(), username, client.get_balance());

	char main_menu[6][2][16] = {
		{"Announce", "系統公告"},
		{"Pay", "發起付款"},
		{"History", "交易紀錄"},
		{"List", "用戶列表"},
		{"Info", "系統資訊"},
		{"Goodbye", "離開"}
	};

	int cmd = 0;
	bool run = 1;
	while(run){
		cmd = menu_command(main_menu, 6, cmd);
		switch(cmd){
			default:
			case 0:
				break;
			case 1:
				init_payment();
				break;
			case 2:
				show_history();
				break;
			case 3:
				client.fetch_list();
				print_statusbar(client.get_onlineusers(), username, client.get_balance());
				show_list();
				break;
			case 4:
				show_info(client.get_info());
				break;
			case 5:
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
