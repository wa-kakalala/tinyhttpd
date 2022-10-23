#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include <pthread.h>

#define SERVER_STRING "Server: wkk/0.1.0\r\n"
typedef unsigned short u16;
enum METHOD{
    METHOD_GET,
    METHOD_POST,
    METHOD_UNKNOWN
};

/**
* 输出错误信息，并退出
* @param  str  错误提示信息
*/
void err_Exit(const char * str){
    perror(str);
    exit(1);
}

/**
* 接收一行数据
* @param sock 对端socket id 
* @param buf  缓存区
* @param size 缓存区大小
* 返回一行的字符个数
*/
int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n = 0;

    while ((i < size - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);
        if (n > 0)
        {
            if (c == '\r')  // 一行已经结束了， 如果下个字符是\n 要将其读走
            {
                // 使用MSG_PEEK 方式查看下一个字符
                n = recv(sock, &c, 1, MSG_PEEK);
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else{
                    c = '\n';
                }
            }
            buf[i++] = c;
        }else{
            c = '\n';
        }
    }
    buf[i] = '\0';
    return i;
}

/**
* 资源找不到 404 错误
* @param client  client socket id
*/
void not_found_404(int client)
{
 char buf[1024];

 sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, SERVER_STRING);
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "Content-Type: text/html\r\n");
 send(client, buf, strlen(buf), 0);
 sprintf(buf, "\r\n"); //  <---注意点
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
* 处理GET 请求
*/
void get_processor(int client){
    printf("处理 get 请求\r\n");
    not_found_404(client);

}

/**
* 处理POST 请求
*/
void post_processor(){
    printf("处理 post 请求\r\n");

}



/* *
 * 处理客户端的连接请求
 * @param arg client socket id
*/
void processor(void *arg)
{
    int client = (intptr_t)arg;
    char buf[1024];
    int numchars;
    u16 i,j;
    char methodStr[20];
    enum METHOD method = METHOD_UNKNOWN;
    char url[255];

    // 获取第一行信息
    // "GET / HTTP/1.1\n"
    // "GET /index.html HTTP/1.1"
    // buf的最后一位是'\n'(不考虑 '\0')
    numchars = get_line(client, buf, sizeof(buf));
    printf("header : %s", buf); 
    // 提取请求方法
    i = j = 0;
    while(buf[i] != ' ' && i < sizeof(method)){
        methodStr[j++] = buf[i++];
    }
    methodStr[j] = '\0';
    printf("method : %s\r\n", methodStr); 

    if(strcmp(methodStr,"GET") == 0 ) method = METHOD_GET;
    if(strcmp(methodStr,"POST") == 0 ) method = METHOD_POST;
    printf("method value : %d \r\n", method);

    // 继续处理后面的信息
    // 略过空格
    while(buf[i] != ' ' && i < sizeof(buf)){
        i++;
    }
    // 得到请求地址
    j = 0;
    while(buf[i] != ' ' && i < sizeof(buf)){
        url[j++] = buf[i++];
    }
    url[j] = '\0';
    switch(method){
        case METHOD_GET:
            get_processor(client);
            break;
        case METHOD_POST:
            post_processor();
            break;
        case METHOD_UNKNOWN:
            break;
    }










//  //判断Get请求
//  if (strcasecmp(method, "GET") == 0)
//  {
//   query_string = url;
//   while ((*query_string != '?') && (*query_string != '\0'))
//    query_string++;
//   if (*query_string == '?')
//   {
//    cgi = 1;
//    *query_string = '\0';
//    query_string++;
//   }
//  }

//  //路径
//  sprintf(path, "htdocs%s", url);

//  //默认地址，解析到的路径如果为/，则自动加上index.html
//  if (path[strlen(path) - 1] == '/')
//   strcat(path, "index.html");

//  //获得文件信息
//  if (stat(path, &st) == -1) {
//   //把所有http信息读出然后丢弃
//   while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
//    numchars = get_line(client, buf, sizeof(buf));

//   //没有找到
//   not_found(client);
//  }
//  else
//  {
//   if ((st.st_mode & S_IFMT) == S_IFDIR)
//    strcat(path, "/index.html");
//   //如果你的文件默认是有执行权限的，自动解析成cgi程序，如果有执行权限但是不能执行，会接受到报错信号
//   if ((st.st_mode & S_IXUSR) ||
//       (st.st_mode & S_IXGRP) ||
//       (st.st_mode & S_IXOTH)    )
//    cgi = 1;
//   if (!cgi)
//    //接读取文件返回给请求的http客户端
//    serve_file(client, path);
//   else
//    //执行cgi文件
//    execute_cgi(client, path, method, query_string);
//  }
    //执行完毕关闭socket
    sleep(1);
    close(client);
}


/**
* 创建服务器socket 并 监听   
* @param  port 监听端口
* 返回 socket_fd
*/
int start_listen(u16  *port)
{
    int socket_fd = 0;
    struct sockaddr_in servaddr;

    socket_fd = socket(PF_INET,SOCK_STREAM,0);
    if (socket_fd == -1) err_Exit("socket error");
  
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(*port);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    //绑定socket
    if (bind(socket_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        err_Exit("bind error");
    //监听
    if (listen(socket_fd, 5) < 0)
        err_Exit("listen error");
    return socket_fd;
}

int main(void)
{
    int server_sock = -1;
    u16 port = 8000;
    int client_sock = -1;

    pthread_t newthread;

    server_sock = start_listen(&port);
    printf("tiny http running on port %d\n", port);

    while (1)
    {  
        client_sock = accept(server_sock,(struct sockaddr*)NULL,NULL);
        if (client_sock == -1)
	        err_Exit("accept error");
        printf("接入 client\r\n");

        // 创建线程
        if (pthread_create(&newthread, NULL, (void *)processor, (void *)(intptr_t)client_sock) != 0)
	       perror("pthread_create");
    }
    close(server_sock);
    return 0;
}
