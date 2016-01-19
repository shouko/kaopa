#include <iostream>
#include <string.h>
using namespace std;

#ifndef __SOCKET__
#define __SOCKET__
#define MAX_BUF 1024
#define FAIL -1

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdexcept>
#include "openssl/ssl.h"
#include "openssl/err.h"

class SocketException : public exception{
public:
	virtual const char* what() const throw(){
		return "SocketException!";
	}
};

class Socket{
public:
	Socket();
	Socket(const int sockfd);
	Socket(const char* hostname, const char* port);
	~Socket();
	int connect(const char* hostname, const char* port);
	int send(const string msg);
	int send(const char* msg);
	const char* recv();
	int listen();
	int listen(const char* port);
	int listen(const unsigned short port);
	Socket* accept();
	bool isconnected();
	const unsigned short getlocalport();
	const char* getremoteip();
protected:
	int sockfd;
	char recv_buf[MAX_BUF];
friend class SecureSocket;
};

class SecureSocket : public Socket{
public:
	SecureSocket();
	SecureSocket(const char* hostname, const char* port);
	SecureSocket(const int sockfd, const SSL* ssl) : Socket(sockfd), ssl((SSL*)ssl) {}
	~SecureSocket();
	int send(const char* msg);
	const char* recv();
	SecureSocket* accept();
private:
	static bool openssl_lib_loaded;
	static SSL_CTX* ctx_client;
	static SSL_CTX* ctx_server;
	SSL* ssl;
	int init_ssl_ctx(const char* cert_fn, const char* key_fn);
};
#endif

#ifndef __SAFE_QUEUE__
#define __SAFE_QUEUE__

#include <queue>
#include <mutex>
#include <condition_variable>

template <class T>
class SafeQueue
{
public:
  SafeQueue(void) : q(), m(), c() {}
  ~SafeQueue(void) {}
	void push(T t){
		std::lock_guard<std::mutex> lock(m);
		q.push(t);
		c.notify_one();
	}

	T pop(void){
		std::unique_lock<std::mutex> lock(m);
		while(q.empty())
		{
			c.wait(lock);
		}
		T val = q.front();
		q.pop();
		return val;
	}
private:
  std::queue<T> q;
  mutable std::mutex m;
  std::condition_variable c;
};
#endif

#ifndef __PAYMENT__
#define __PAYMENT__
#define PAYMENT_FROM 0
#define PAYMENT_AMOUNT 1
#define PAYMENT_TO 2
#endif
