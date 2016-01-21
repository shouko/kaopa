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

bool SecureSocket::ssl_lib_loaded = false;
bool SecureSocket::ssl_certs_loaded = false;
SSL_CTX* SecureSocket::ctx_client = NULL;
SSL_CTX* SecureSocket::ctx_server = NULL;

SecureSocket::SecureSocket(){
	if(!ssl_lib_loaded){
		init_ssl_lib();
	}
}

SecureSocket::~SecureSocket(){
	SSL_free(ssl);
}

SecureSocket::SecureSocket(int sockfd){

}

SecureSocket::SecureSocket(const char* hostname, const char* port){
	if(!ssl_lib_loaded){
		init_ssl_lib();
	}
	this->connect(hostname, port);
}

int SecureSocket::connect(const char* hostname, const char* port){
	Socket::connect(hostname, port);
	ssl = SSL_new(ctx_client);      // create new SSL connection state
	SSL_set_fd(ssl, sockfd);    // attach the socket descriptor
	if(SSL_connect(ssl) == FAIL)   // perform the connection
		ERR_print_errors_fp(stderr);
	cipher_name = SSL_get_cipher(ssl);
	X509 *cert = SSL_get_peer_certificate(ssl);
	if(cert != NULL)
	{
		cert_subject = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
		cert_issuer = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
		X509_free(cert);
	}
	return 0;
}

int SecureSocket::init_ssl_lib(){
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	ctx_client = SSL_CTX_new(SSLv3_client_method());
	ctx_server = SSL_CTX_new(SSLv3_server_method());
	if(ctx_client == NULL || ctx_server == NULL){
		ERR_print_errors_fp(stderr);
		abort();
	}
	ssl_lib_loaded = true;
	return 0;
}

int SecureSocket::init_ssl_certs(){
	return init_ssl_certs("server.crt.pem", "server.key.pem");
}

int SecureSocket::init_ssl_certs(const char* cert_fn, const char* key_fn){
	//set local certificate
	if(SSL_CTX_use_certificate_file(ctx_server, cert_fn, SSL_FILETYPE_PEM) <= 0){
		ERR_print_errors_fp(stderr);
		abort();
	}
	//set private key
	if(SSL_CTX_use_PrivateKey_file(ctx_server, key_fn, SSL_FILETYPE_PEM) <= 0){
		ERR_print_errors_fp(stderr);
		abort();
	}
	//verify private key
	if(!SSL_CTX_check_private_key(ctx_server)){
		fprintf(stderr, "Private key does not match the public certificate\n");
		abort();
	}
	return 0;
}

int SecureSocket::listen(const unsigned short port){
	if(!ssl_certs_loaded){
		init_ssl_certs();
	}
	return Socket::listen(port);
}

int SecureSocket::listen(){
	return this->listen(((const unsigned short)0));
}

SecureSocket* SecureSocket::accept(){
	Socket* client_socket = Socket::accept();
	int client_sockfd = client_socket->sockfd;
	SSL* ssl = SSL_new(ctx_server);              // get new SSL state with context
	SSL_set_fd(ssl, client_sockfd);
	switch(SSL_accept(ssl)){
		case 0:
			throw new SocketException();
		case 1:
			return new SecureSocket(client_sockfd, ssl);
		case FAIL:
		default:
			ERR_print_errors_fp(stderr);
			throw new SocketException();
	}
}

const char* SecureSocket::recv(){
	if(SSL_read(ssl, recv_buf, MAX_BUF) > 0){
		return recv_buf;
	}else{
		return "";
		ERR_print_errors_fp(stderr);
	}
}

int SecureSocket::send(const string msg){
	return this->send(msg.c_str());
}

int SecureSocket::send(const char* msg){
	return SSL_write(ssl, msg, strlen(msg));
}

const string SecureSocket::get_cipher_name(){
	return cipher_name;
}

const string SecureSocket::get_cert_subject(){
	return cert_subject;
}

const string SecureSocket::get_cert_issuer(){
	return cert_issuer;
}
