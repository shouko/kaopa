#include <iostream>
#include <string.h>
#include <thread>
#include "util.h"
using namespace std;

SafeQueue<vector<string>*> cmdQueue;

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

int main(int argc, char* argv[]){
	Socket* p = new Socket();
	p->listen();
	int local_port = p->getlocalport();
	thread (payment_accept, p).detach();
	cout << "Listening on local port " << local_port << endl;

	string remote_host;
	cout << "Please enter remote hostname: ";
	cin >> remote_host;
	Socket s(remote_host.c_str(), "8889");
	s.recv();
	string username;
	cout << "Please enter your username: ";
	cin >> username;
	s.send(username + "#" + to_string(local_port) + "\n");
	s.recv();
	string input, toSend;
	int withLf;
	while(cin >> input >> withLf){
		toSend = input;
		if(withLf){
			toSend += '\n';
		}
		s.send(toSend);
		s.recv();
	}
	return 0;
}
