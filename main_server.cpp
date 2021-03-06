#include <iostream>
#include <string.h>
#include <thread>
#include <map>
#include <sstream>
#include <fstream>
#include <assert.h>
#include "util.h"
using namespace std;
typedef Socket PeerSocket;
typedef SecureSocket ServerSocket;

class User{
public:
	string username;
	int balance;
	string ip;
	string port;
	bool online;
	User(string username, int balance = 1000) : username(username), balance(balance), port(""), online(0) {
	}
	~User(){
	}
	int adjust_balance(int amount){
		balance += amount;
		return balance;
	}
	int set_port(string port){
		this->port = port;
		this->online = true;
		return 0;
	}
};

map<string, User> users;
map<string, User*> users_online;

int import_users(string fn){
	ifstream ifs(fn);
	string user_data = "";
	while(ifs >> user_data){
		stringstream ss(user_data);
		string username;
		int balance;
		getline(ss, username, '#');
		ss >> balance;
		User user(username, balance);
		users.insert(make_pair(username, user));
	}
	return 0;
}

int export_users(string fn){
	ofstream ofs(fn);
	for(map<string, User>::iterator it = users.begin(); it != users.end(); it++){
		ofs << it->second.username << '#' << it->second.balance << '\n';
	}
	return 0;
}

void notify_sender(Transaction* trans, User* user){
	try{
		PeerSocket s(user->ip, user->port);
		s.recv(); // get Hello
		string msg;
		if(trans->success){
			msg = "100 OK#";
		}else{
			msg = "201 INSUFFICIENT_FUND#";
		}
		msg +=  trans->user_to + "#" + trans->amount + "#\n";
		s.send(msg);
		s.recv();
	}catch(SocketException e){
		cout << "Error: Failed to notify sender" << endl;
	}
}

int send_list(User* current_user, ServerSocket* c){
	string uol_str = "";
	int uol = 0;
	for(map<string, User*>::iterator it = users_online.begin(); it != users_online.end(); it++){
		User* user = it->second;
		uol_str += user->username + "#" + user->ip + "#" + user->port + "\n";
		uol++;
	}
	return c->send(to_string(current_user->balance)+ "\n" + to_string(uol) + "\n" + uol_str);
}

int connection_process(ServerSocket* c){
	User* current_user = 0;
	try{
		c->send("Hello!");
		string cmd;
		while(1){
			stringstream ss(c->recv());
			getline(ss, cmd, '#');
			if(cmd == "REGISTER"){
				getline(ss, cmd, '\n');
				if(cmd == ""){
					// fail
					continue;
				}
				// register
				if(users.find(cmd) != users.end()){
					c->send("203 DUPLICATE USERNAME");
				}else{
					current_user = new User(cmd);
					users.insert(make_pair(cmd, *(current_user)));
					cout << "New user name: " << cmd << endl;
					cout << users.size() << " th" << endl;
					c->send("100 SUCCESS");
					continue;
				}
			}else{
				// login
				// fetch user data from user database
				cout << "LOGIN: " << cmd << endl;
				map<string, User>::iterator it = users.find(cmd);
				cout << it->second.username << endl;
				if(it != users.end()){
					string port;
					ss >> port;
					current_user = &(it->second);
					current_user->set_port(port);
					current_user->ip = c->getremoteip();
					users_online.insert(make_pair(current_user->username, current_user));
					send_list(current_user, c);
					break;
				}else{
					c->send("200 BAD REQUEST");
				}
			}
		}
		while(1){
			stringstream ss(c->recv());
			getline(ss, cmd);
			if(cmd == "List"){
				send_list(current_user, c);
			}else if(cmd == "Exit"){
				c->send("Bye");
				break;
			}else if(cmd == "Pay"){
				// user received payment
				string from_str;
				string amount_str;
				string to_str;
				User* from_user;
				User* to_user;
				int amount = 0;
				getline(ss, from_str, '#');
				getline(ss, amount_str, '#');
				getline(ss, to_str, '#');
				Transaction trans(from_str, to_str, amount_str);
				stringstream ass(amount_str);
				ass >> amount;
				cout << current_user->username << " in transaction " << from_str << "#" << amount_str << "#" << to_str << endl;
				map<string, User*>::iterator from_user_it = users_online.find(from_str);
				if(from_user_it == users_online.end()){
					// inexist sender or not online
					cout << "Error: Fail to find sender " << from_str << endl;
					c->send("200 SENDER_NOT_AVAIL\n");
					c->recv();
					continue;
				}
				from_user = from_user_it->second;
				if(from_user->balance < amount){
					// insufficient fund
					trans.success = false;
					thread (notify_sender, &trans, from_user).detach();
					c->send("201 INSUFFICIENT_FUND\n");
					c->recv();
					continue;
				}
				map<string, User*>::iterator to_user_it = users_online.find(to_str);
				if(to_user_it == users_online.end()){
					cout << "Error: In exist user " << to_str << endl;
					cout << endl;
					cout << endl;
					continue; // inexist receiver
				}
				to_user = to_user_it->second;
				if(to_user != current_user){
					cout << "Error: Receiver " << to_user->username << " " << to_user << " " << current_user->username << " " << current_user << " does not match current user" << to_str << endl;
					continue; // receiver does not match current user
				}
				from_user->adjust_balance(-1*amount);
				to_user->adjust_balance(amount);
				trans.success = true;
				thread (notify_sender, &trans, from_user).detach();
				c->send("100 OK\n");
			}
		}
	}catch(SocketException e){
		cerr << "Error: Connection with client interrupted." << endl;
	}
	map<string, User*>::iterator it = users_online.find(current_user->username);
	if(it != users_online.end()){
		users_online.erase(it);
	}
	current_user->online = false;
	delete c;
	return 0;
}

int connection_accept(ServerSocket* s){
	while(1){
		try{
			thread (connection_process, s->accept()).detach();
		}catch(SocketException e){
			cout << "Error: Incoming client failed to establish secure connection." << endl;
		}
	}
}

int main(int argc, char* argv[]){
	cout << "Importing user data..." << endl;
	import_users("data_server.txt");
	cout << "Imported user data." << endl;
	ServerSocket* s = new ServerSocket();
	s->listen(8889);
	int local_port = s->getlocalport();
	thread (connection_accept, s).detach();
	cout << "Listening on local port " << local_port << endl;
	char x;
	while(cin >> x){
		if(x == 'q'){
			cout << "Server shutting down..." << endl;
			cout << "Exporting user data..." << endl;
			export_users("data_server.txt");
			cout << "Exported user data." << endl;
			break;
		}
	}
	return 0;
}
