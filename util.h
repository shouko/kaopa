#include <iostream>
#include <string.h>
using namespace std;

#ifndef __SOCKET__
#define __SOCKET__
#define MAX_BUF 1024

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdexcept>

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
private:
	int sockfd;
	char recv_buf[MAX_BUF];
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
