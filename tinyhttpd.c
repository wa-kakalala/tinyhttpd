#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <ctype.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include <pthread.h>
#include <sys/stat.h>

// if space 
#define ISspace(x) isspace((int)(x))
#define SERVER_STRING "Server: wkk's WebServer/0.1\r\n"

/**
* print the error info and exit
* @param sc error info 
*/
void error(const char *sc){
 	perror(sc);
 	exit(1);
}

/**
* create server 's socket fd, and come into listen state
* @param port spectific port ( if 0,assign by system)
* @return socket fd and port 
*/
int createSockAndListen(u_short *port)
{
	int httpd = 0;
 	struct sockaddr_in inaddr;

	// use tcp protocol
 	httpd = socket(AF_INET, SOCK_STREAM, 0);
 	if (httpd == -1) error("[webserver]: create socket failed");
 	memset(&inaddr, 0, sizeof(inaddr));
 	inaddr.sin_family = AF_INET;
 	inaddr.sin_port = htons(*port);
 	inaddr.sin_addr.s_addr = htonl(INADDR_ANY);
 	//bind address info
 	if (bind(httpd, (struct sockaddr *)&inaddr, sizeof(inaddr)) < 0)
  	error("[webserver]: bind address failed");
	// dynamically allocating a port 
 	if (*port == 0){
  		socklen_t  addrlen = sizeof(inaddr);
  		if (getsockname(httpd, (struct sockaddr *)&inaddr, &addrlen) == -1)
  			error("[webserver]: getsockname failed");
  		*port = ntohs(inaddr.sin_port);
	}
 	if (listen(httpd, 5) < 0) error("[webserver]: listen failed");
 	return httpd;
}

/**
* read one line data: 1. end with \r\n 2. end with \r 3. recv data len < 0
* @param sock client fd
* @param buf save line data
* @param size buf length
* @return one line data --> buf and data_len
*/
int getLine(int sock, char *buf, int size)
{
 	int i = 0, n = 0;
 	char c = '\0';

 	while ((i < size - 1) && (c != '\n')){
  		n = recv(sock, &c, 1, 0);
  		if (n > 0){
   			if (c == '\r'){
				// look for next char whether is \n
    			n = recv(sock, &c, 1, MSG_PEEK);
  				if ((n > 0) && (c == '\n')) // must remove the next char \n
     				recv(sock, &c, 1, 0);
    			else  // do not remove the next char
     				c = '\n';
   			}
			buf[i++] = c;
  		}else
			c = '\n';
 	}
 	buf[i] = '\0';
 	return i;
}

/**
* add http headers
* @param client client fd
*/
void headers(int client)
{
 	char buf[1024];
 	strcpy(buf, "HTTP/1.0 200 OK\r\n");
 	send(client, buf, strlen(buf), 0);
 	strcpy(buf, SERVER_STRING);
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "Content-Type: text/html\r\n");
 	send(client, buf, strlen(buf), 0);
 	strcpy(buf, "\r\n");
 	send(client, buf, strlen(buf), 0);
}

/**
* the file is unavailable , response 404
*/
void notFindFile(int client){
 	char buf[1024];
 	sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, SERVER_STRING);
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "Content-Type: text/html\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "\r\n"); // empty line is a must
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "your request because the resource specified\r\n");
	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "is unavailable or nonexistent.\r\n");
 	send(client, buf, strlen(buf), 0);
	sprintf(buf, "</BODY></HTML>\r\n");
 	send(client, buf, strlen(buf), 0);
}

/**
* HTTP request method not supported
*/
void unImplemented(int client){
 	char buf[1024];

 	sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, SERVER_STRING);
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "Content-Type: text/html\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "</TITLE></HEAD>\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
 	send(client, buf, strlen(buf), 0);
 	sprintf(buf, "</BODY></HTML>\r\n");
 	send(client, buf, strlen(buf), 0);
}

/**
* get file content and send
*/
void cat(int client, FILE *resource){
 	char buf[1024];
	fgets(buf, sizeof(buf), resource);
 	while (!feof(resource)){
  		send(client, buf, strlen(buf), 0);
  		fgets(buf, sizeof(buf), resource);
 	}
}

void serveFile(int client, const char *filename){
 	FILE *resource = NULL;
 	int numchars = 1;
 	char buf[1024];
 	buf[0] = 'A'; buf[1] = '\0';
	/* read & discard headers */
 	while ((numchars > 0) && strcmp("\n", buf))  
  		numchars = getLine(client, buf, sizeof(buf));

 	resource = fopen(filename, "r");
 	if (resource == NULL)
  		notFindFile(client);
	else{
  		headers(client);
  		cat(client, resource);
 	}
 	fclose(resource);
}

// GET / HTTP/1.1
// Host: 192.168.0.23:47310
// Connection: keep-alive
// Upgrade-Insecure-Requests: 1
// --------------------------------
// ------- HTTP POST
// POST /color1.cgi HTTP/1.1
// Host: 192.168.0.23 : 47310
// Connection : keep-alive
// Content-Length : 10

/**
* deal with client 's request  --> just support GET now 
* @param arg client fd
*/
void acceptRequest(void *arg)
{
  	//get client socket fd from ptr struct
 	int client = (intptr_t)arg;
 	int numchars;
	/*
	 method : request method GET ? POST
	 url : request url 
	path : request file path
	*/
	char buf[1024],method[20],url[255],path[512];

 	unsigned int i = 0, j = 0;
 	struct stat st;   
 	char *query_string = NULL;

 	//GET / HTTP/1.1\n
 	numchars = getLine(client, buf, sizeof(buf));
	//extract GET / POST	
 	while (!ISspace(buf[j]) && (i < sizeof(method) - 1))
  		method[i++] = buf[j++];
 	method[i] = '\0';

 	//judge GET or POST --> both not to response umImplemented
 	if (strcasecmp(method, "GET") && strcasecmp(method, "POST")){
  		unImplemented(client);
  		return;
 	}
 	i = 0;
 	//skip space
 	while (ISspace(buf[j]) && (j < sizeof(buf))) j++;

    //url : http://192.168.0.23:47310/index.html
 	//http info : GET /index.html HTTP/1.1，
 	while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
  		url[i++] = buf[j++];
	url[i] = '\0';  // for example : /index.html

	//POST , Not currently support
 	if (strcasecmp(method, "POST") == 0)
  		return ; 
 	//GET
 	if (strcasecmp(method, "GET") == 0){
  		query_string = url;
		// find ? 
  		while ((*query_string != '?') && (*query_string != '\0'))
   			query_string++;
  		if (*query_string == '?'){
			// url content between / and ?
   			*query_string = '\0';
			// after ? is query_string
   			query_string++;
  		}
 	}
 	//get file path
 	sprintf(path, "data%s", url);
 	//whether path is /,add index.html
 	if (path[strlen(path) - 1] == '/')
  		strcat(path, "index.html");

 	//get file info 
 	if (stat(path, &st) == -1) {
		// get all the http info ,and then discard
  		while ((numchars > 0) && strcmp("\n", buf)) 
   			numchars = getLine(client, buf, sizeof(buf));
  		notFindFile(client);
		return ;
 	}else{
		// is directory
  		if ((st.st_mode & S_IFMT) == S_IFDIR)
   			strcat(path, "/index.html");

  		if (!(st.st_mode & S_IRUSR) &&
      		!(st.st_mode & S_IRGRP) &&
      		!(st.st_mode & S_IROTH)){
			printf("[WebServer]: no permission to read the file\r\n");
		}
		serveFile(client, path);
 	}
 	close(client);
}

/**
* @param argv can give one parameter --> port 
*/
int main(int argc ,char * argv[]){
	int server_sock = -1,client_sock = -1;
	u_short port = (argc == 2)?atoi(argv[1]):0;
	struct sockaddr_in client_name;
 	socklen_t client_name_len = sizeof(client_name);
	// create thread variable
 	pthread_t newthread;
 	server_sock = createSockAndListen(&port);
 	printf("[webserver]: WebServer running on port %d\r\n", port);

 	while (1){
	 	client_sock = accept(server_sock,
                       (struct sockaddr *)&client_name,
                       &client_name_len);
  		if (client_sock == -1)	error("[webserver]: accept failed");
		//create thread to deal with new request
		//convert client fd to address parameter ( intptr_t is safe )
  		if (pthread_create(&newthread, NULL,(void* (*)(void*))acceptRequest,
				 (void *)(intptr_t)client_sock) != 0)
	  		error("pthread_create failed");
 	}
	close(server_sock);
	return 0;
}
