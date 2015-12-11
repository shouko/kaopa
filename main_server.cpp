#include <iostream>
#include <string.h>
#include <thread>
#include <vector>
#include "socket.h"
using namespace std;

int client_worker(Socket client){
	client.send("HELLO");
	client.recv();
	return 0;
}

int main(int argc, char* argv[]){
	if(argc < 3){
		cerr << "Usage: " << argv[0] << " <port>" << endl;
		return 0;
	}
	Socket s();
	s.listen(argv[1]);
	vector<Socket> client_sockets;
	vector<thread*> client_threads;
	while(1){
		Socket client_socket = s.accept();
		client_sockets.push_back(client_socket);
		thread* client_thread = new thread(client_worker, client_socket);
	}
	return 0;
}
