#ifndef __SOCKET_H__
#define __SOCKET_H__
#define MAX_BUF 1024
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

class Socket{
public:
	Socket();
	~Socket();
	int send();
	int recv();
private:
	int sockfd;
	struct addrinfo hints, *res;
	char recv_buf[MAX_BUF];
};
#endif

/*
	int s;
	
	

	
	if(recv(s, recv_buf, MAX_BUF, 0) > 0){
		if(strncmp(recv_buf, "Hello!!!", 8) == 0){
//			send(s, buffer, sizeof(buffer), 0);
		}
	}

*/