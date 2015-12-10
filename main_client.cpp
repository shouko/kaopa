#include <iostream>
#include <string.h>
#include "socket.h"
using namespace std;

int main(int argc, char* argv[]){
	if(argc < 3){
		cerr << "Usage: " << argv[0] << " <hostname> <port>" << endl;
		return 0;
	}
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
