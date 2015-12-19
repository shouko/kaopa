#include <iostream>
#include <string.h>
#include <thread>
#include <map>
#include "util.h"
using namespace std;

SafeQueue<vector<string>*> cmdQueue;

class User{
public:
	User(string username) : username(username), port(0), online(0) {
	}
	~User(){
	}
	int adjust_balance(int amount){
		return 0;
	}
	int get_balance(){
		return 0;
	}
	int is_online(){
		return online;
	}
	int set_port(int port){
		this->port = port;
		this->online = true;
		return 0;
	}
private:
	string username;
	int balance;
	string ip;
	unsigned short port;
	bool online;
};

map<string, User> users;

int connection_process(Socket* c){
	c->send("Hello!");
	string cmd;
	User* current_user;
	while(1){
		cmd = c->recv();
		vector<string>* cmd_tokens = split(cmd, "#");
		if(cmd_tokens->size() != 2){
			c->send("220 UNKNOWN COMMAND\n");
		}else if((*cmd_tokens)[0] == "REGISTER"){
			// register
			if(users.find((*cmd_tokens)[1]) != users.end()){
				c->send("220 DUPLICATE USERNAME\n");
			}else{
				current_user = new User((*cmd_tokens)[1]);
				users.insert(make_pair((*cmd_tokens)[1], *(current_user)));
				c->send("100 SUCCESS\n");
				break;
			}
		}else{
			// login
			// fetch user data from user database
			map<string, User>::iterator it = users.find((*cmd_tokens)[1]);
			if(it != users.end()){
				current_user = &(it->second);
				current_user->set_port(stoi((*cmd_tokens)[1].c_str()));
				break;
			}
		}
		delete cmd_tokens;
	}
/*
	while(1){
		cmd = c->recv();
		vector<string>* cmd_tokens = split(cmd);
		switch((*cmd_tokens)[0]){
			case "REGISTER":
				break;
			case "List":
				break;
			case "Exit":
				break;
			default:
				if(cmd_tokens->size() == 2){

				}
		}
	}
*/
	delete c;
	return 0;
}

int connection_accept(Socket* s){
	while(1){
		thread (connection_process, s->accept()).detach();
	}
}

int main(int argc, char* argv[]){
	Socket* s = new Socket();
	s->listen("8889");
	int local_port = s->getlocalport();
	thread (connection_accept, s).detach();
	cout << "Listening on local port " << local_port << endl;

	return 0;
}
