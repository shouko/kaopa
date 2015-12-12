#include <iostream>
#include <string.h>
#include <thread>
#include <vector>
#include "socket.h"
using namespace std;

int client_worker(Socket* client){
	client->send("HELLO");
	client->recv();
	return 0;
}

int main(int argc, char* argv[]){
	if(argc < 2){
		cerr << "Usage: " << argv[0] << " <port>" << endl;
		return 0;
	}
	Socket s;
	s.listen(argv[1]);
	vector<Socket*> client_sockets;
	vector<thread*> client_threads;
	while(1){
		client_sockets.push_back(s.accept());
		client_threads.push_back(new thread(client_worker, client_sockets.back()));
	}
	return 0;
}
