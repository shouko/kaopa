#include <iostream>
#include <string.h>
#include <thread>
#include "util.h"
using namespace std;

void process_payment(Socket* s){
	s->recv();
	s->send("Good!");
	delete s;
}

void wait_payment(Socket* s){
	while(1){
		thread (process_payment, s->accept()).detach();
	}
}

int main(int argc, char* argv[]){
	if(argc < 3){
		cerr << "Usage: " << argv[0] << " <hostname> <port>" << endl;
		return 0;
	}

	Socket* p = new Socket();
	p->listen();
	int local_port = p->getlocalport();
	thread (wait_payment, p).detach();

	cout << "Listening on local port " << local_port;

	Socket s(argv[1], argv[2]);
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
