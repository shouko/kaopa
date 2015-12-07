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
	cout << "<< " << s.recv() << endl;
	s.send("REGISTER#GOOGLE\n");
	cout << s.recv() << endl;
	int a;
	cin >> a;
	return 0;
}