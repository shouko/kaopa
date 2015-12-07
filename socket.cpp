#include "socket.h"

Socket::Socket(){
	sockfd = -1;
}

Socket::Socket(const char* hostname, const char* port){
	this->connect(hostname, port);
}

int Socket::connect(const char* hostname, const char* port){
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	::getaddrinfo(hostname, port, &hints, &res);
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if(::connect(sockfd, res->ai_addr, res->ai_addrlen)){
		throw SocketException();
	}
	return 0;
}

Socket::~Socket(){
	close(sockfd);
}

const char* Socket::recv(){
	if(::recv(sockfd, recv_buf, MAX_BUF, 0) > 0){
#ifdef DEBUG
		cout << "<< " << recv_buf << endl;
#endif
		return recv_buf;
	}else{
		return "";
	}
}

int Socket::send(const string msg){
	return this->send(msg.c_str());
}

int Socket::send(const char* msg){
	if(!this->isConnected()){
		throw SocketException();
	}
#ifdef DEBUG
	cout << ">> " << msg << endl;
#endif
	return ::send(sockfd, msg, strlen(msg), 0);
}

bool Socket::isConnected(){
	return sockfd != -1;
}