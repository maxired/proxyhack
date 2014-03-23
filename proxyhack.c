/*	
	A simple LD_PRELOAD hack to let you specify the source address
	for all outbound connections or if you want to limit a process
	to only listening on one IP

	Copyright (C) 2005 Robert J. McKay <robert@mckay.com>

License: You can do whatever you want with it.


Compile:

gcc -fPIC -static -shared -o bindhack.so bindhack.c -lc -ldl

You can add -DDEBUG to see debug output.

Usage:

LD_PRELOAD=/path/to/bindhack.so <command>

eg:

LD_PRELOAD=/home/rm/bindhack.so telnet example.com

you can also specify the address to use at runtime like so:

LD_PRELOAD=/home/rm/bindhack.so BIND_SRC=192.168.0.1 telnet example.com

 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <dlfcn.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

/* 
   This is the address you want to force everything to use. It can be
   overriden at runtime by specifying the BIND_SRC environment 
   variable.
 */
#define PROXY_ADDR	"127.0.0.1"
#define PROXY_PORT	"3128"

/* 
   LIBC_NAME should be the name of the library that contains the real
   bind() and connect() library calls. On Linux this is libc, but on
   other OS's such as Solaris this would be the socket library
 */
#define LIBC_NAME	"libc.so.6" 

#define YES	1
#define NO	0

//#define DEBUG 	1
/* 
   Sometimes (alot of times) programs don't bother to call bind() 
   if they're just making an outgoing connection. To take care of
   these cases, we need to call bind when they call connect 
   instead. And of course, then call connect as well...
 */


int type_of_sockfd( int sockfd){
	int type;
	int length = sizeof( int );
	getsockopt( sockfd, SOL_SOCKET, SO_TYPE, &type, &length );
	return type;
}

void * origin_from_libc(char* function_name){

	void	*libc;
	libc = dlopen(LIBC_NAME, RTLD_LAZY);
	return dlsym(libc, function_name);
}
	int
connect(int  sockfd, const struct sockaddr *serv_addr, socklen_t addrlen)
{
	int	(*connect_ptr)(int, void *, int);
	void	*libc;
	int	ret;

	char * proxy_addr = getenv("PROXY_ADDR");
	char * proxy_port = getenv("PROXY_PORT");

	if(!proxy_addr){
		proxy_addr = PROXY_ADDR;
	}

	if(!proxy_port){
		proxy_port = PROXY_PORT;
	}


	int type;
	int length = sizeof( int );

	struct sockaddr_in proxy_addr_in ;
	memcpy(&proxy_addr_in , serv_addr , sizeof(struct sockaddr));
		

	getsockopt( sockfd, SOL_SOCKET, SO_TYPE, &type, &length );

	struct sockaddr_in* serv_addr_in= (void*) serv_addr; 

	if(serv_addr->sa_family == AF_INET && type == SOCK_STREAM){
		//printf("socket inet and TCP\n");
		memcpy(&proxy_addr_in , serv_addr , sizeof(struct sockaddr));
		inet_pton(AF_INET, proxy_addr, &proxy_addr_in.sin_addr);
		proxy_addr_in.sin_port = htons( atoi(proxy_port )); 
		//proxy_addr_in->sin_family = AF_INET;
	}else {
		//printf("socket not inet or not tcp\n");
	}	

#ifdef DEBUG
	fprintf(stderr, "connect() override called for addr: %s and port %s \n", proxy_addr , proxy_port);
#endif

	/* Before we call connect, let's call bind() and make sure we're
	   using our preferred source address.
	 */


	libc = dlopen(LIBC_NAME, RTLD_LAZY);

	if (!libc)
	{
		fprintf(stderr, "Unable to open libc!\n");
		exit(-1);
	}

	*(void **) (&connect_ptr) = dlsym(libc, "connect");

	if (!connect_ptr)
	{
		fprintf(stderr, "Unable to locate connect function in lib\n");
		exit(-1);
	}


	/* Call real connect function */
	ret = (int)(*connect_ptr)(sockfd, (void *) &proxy_addr_in, addrlen);

	//we want send CONNECT methd DIRECTLY
	if(serv_addr->sa_family == AF_INET && type == SOCK_STREAM){

		struct timeval tv;
		fd_set writefds;
		fd_set readfds;

		tv.tv_sec = 2;
		tv.tv_usec = 500000;

		FD_ZERO(&writefds);
		FD_SET(sockfd, &writefds);

		FD_ZERO(&readfds);
		FD_SET(sockfd, &readfds);


		// don't care about writefds and exceptfds:
		select(sockfd+1, NULL, &writefds, NULL, &tv);
		ssize_t (*send_ptr)(int sockfd, const void *buf, size_t len, int flags) = origin_from_libc("send");

		char tosend[1024];
		char str[INET_ADDRSTRLEN];
		
		inet_ntop(AF_INET, &(serv_addr_in->sin_addr), str, INET_ADDRSTRLEN);
		short pport = ntohs( serv_addr_in->sin_port);
	
		int tosendlenght = sprintf(tosend, "CONNECT %s:%d HTTP/1.1\r\nHost: %s:%d\r\n\r\n",  str, pport,str,pport);

		int rr = send_ptr(sockfd, tosend, tosendlenght, 16384);
		ssize_t (*recv_ptr)(int sockfd, void *buf, size_t len, int flags)= origin_from_libc("recv");
		select(sockfd+1, NULL, &writefds, NULL, &tv);
		char data[1024];
		while(recv_ptr(sockfd,data, 1023, 0) ==-1){
		usleep(200);
			}
	}	
	/* Clean up */
	dlclose(libc);

	return ret;	

}

int write(int fd, const void *buf, size_t count){

	return 0;
}

