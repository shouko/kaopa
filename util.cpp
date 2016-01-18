#include "util.h"

Socket::Socket(){
	sockfd = -1;
}

Socket::Socket(const int sockfd){
	this->sockfd = sockfd;
}

Socket::Socket(const char* hostname, const char* port){
	this->connect(hostname, port);
}

int Socket::connect(const char* hostname, const char* port){
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	::getaddrinfo(hostname, port, &hints, &res);
	struct addrinfo *p;
	for(p = res; p != NULL; p = p->ai_next){
		if((sockfd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
			// perror("connect");
			continue;
		}
		if (::connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			::close(sockfd);
			// perror("connect");
			continue;
		}
		break;
	}

	if(p == NULL){
		throw SocketException();
	}

	freeaddrinfo(res);
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
	if(!this->isconnected()){
		throw SocketException();
	}
#ifdef DEBUG
	cout << ">> " << msg << endl;
#endif
	return ::send(sockfd, msg, strlen(msg), 0);
}

int Socket::listen(){
	return this->listen(((const unsigned short)0));
}

int Socket::listen(const char* port){
	return this->listen(atoi(port));
}

int Socket::listen(const unsigned short port){
	struct sockaddr_in dest;
	sockfd = ::socket(PF_INET, SOCK_STREAM, 0);
	bzero(&dest, sizeof(dest));
	dest.sin_family = AF_UNSPEC;
	dest.sin_port = htons(port);
	dest.sin_addr.s_addr = INADDR_ANY;
	::bind(sockfd, (struct sockaddr*)&dest, sizeof(dest));
	if(::listen(sockfd, 20)){
		throw SocketException();
	}else{
		return 0;
	}
}

Socket* Socket::accept(){
	if(!this->isconnected()){
		throw SocketException();
	}
	int clientfd;
	struct sockaddr_in client_addr;
	unsigned int addrlen = sizeof(client_addr);
	clientfd = ::accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);

	return new Socket(clientfd);
}

bool Socket::isconnected(){
	return sockfd != -1;
}

const unsigned short Socket::getlocalport(){
	if(!this->isconnected()){
		throw SocketException();
	}
	struct sockaddr local_sockaddr;
	memset(&local_sockaddr, 0, sizeof local_sockaddr);
	socklen_t len = sizeof(struct sockaddr_in);
	if(::getsockname(sockfd, &local_sockaddr, &len)==-1){
	  throw SocketException();
	}
	return ntohs(((struct sockaddr_in*)&local_sockaddr)->sin_port);
}

const char* Socket::getremoteip(){
  struct sockaddr_in addr;
  socklen_t addr_size = sizeof(struct sockaddr_in);
  ::getpeername(sockfd, (struct sockaddr *)&addr, &addr_size);
  return inet_ntoa(addr.sin_addr);
}

bool SecureSocket::openssl_lib_loaded = false;

SecureSocket::SecureSocket(){
	if(!openssl_lib_loaded){
		init_ssl_ctx();
		openssl_lib_loaded = true;
	}
}

SecureSocket::~SecureSocket(){
	SSL_free(ssl);         /* release SSL state */
	Socket::~Socket();
}

SecureSocket::init_ssl_ctx(char* cert_fn, char* key_fn){
	SSL_library_init();
	const SSL_METHOD* method = SSLv3_server_method();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	ctx = SSL_CTX_new(method);
	if(ctx == NULL){
		ERR_print_errors_fp(stderr);
		abort();
	}
	//set local certificate
	if(SSL_CTX_use_certificate_file(ctx, cert_fn, SSL_FILETYPE_PEM) <= 0){
		ERR_print_errors_fp(stderr);
		abort();
	}
	//set private key
	if(SSL_CTX_use_PrivateKey_file(ctx, key_fn, SSL_FILETYPE_PEM) <= 0){
		ERR_print_errors_fp(stderr);
		abort();
	}
	//verify private key
	if(!SSL_CTX_check_private_key(ctx)){
		fprintf(stderr, "Private key does not match the public certificate\n");
		abort();
	}
}

SecureSocket* SecureSocket::accept(){
	Socket* client_socket = Socket::accept();
	int client_sockfd = client_socket->sockfd;
	SSL* ssl = SSL_new(ctx);              // get new SSL state with context
	SSL_set_fd(ssl, client_sockfd);
	switch(SSL_accept(ssl)){
		case 0:
			throw new SocketException();
		case 1:
			return new SecureSocket(client_sockfd, ssl);
		default:
			ERR_print_errors_fp(stderr);
			throw new SocketException();
	}
}

const char* SecureSocket:recv(){
	if(SSL_read(ssl, recv_buf, MAX_BUF) > 0){
		return recv_buf;
	}else{
		return "";
		ERR_print_errors_fp(stderr);
	}
}

int SecureSocket::send(const char* msg){
	return SSL_write(ssl, msg, strlen(msg));
}
