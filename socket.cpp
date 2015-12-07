#include "socket.h"

Socket::Socket(){
	/*
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(argv[1], argv[2], &hints, &res);
	s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if(connect(s, res->ai_addr, res->ai_addrlen)){
		cerr << "Error connecting to host " << argv[1] << " port " << argv[2] << endl;
		return 0;
	}
	*/
}

Socket::~Socket(){

}

int recv(){
	return 0;
}

int send(){
	return 0;
}