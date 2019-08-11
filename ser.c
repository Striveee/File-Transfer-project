#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include "work_thread.h"

#define PORT 6002
#define IPSTR "127.0.0.1"

#define LIS_MAX 5

//创建监听套接字
int create_socket();

//接受链接
int accept_fun(int sockfd);

int main()
{
    int sockfd = create_socket();//创建描述符
    if(sockfd == -1)
    {
        printf("create socket failed\n");
        exit(0);
    }

    while(1)
    {
        int c= accept_fun(sockfd);//接收描述符
        if(c == -1)
        {
            printf("accept error\n");
            continue;
        }
        thread_start(c);//开始创建线程
    }
}

int accept_fun(int sockfd)//接受链接
{
    struct sockaddr_in caddr;
    int len = sizeof(caddr);
    int c = accept(sockfd,(struct sockaddr*)&caddr,&len);

    return c;
}

int create_socket()//创建监听套接字
{
    int sockfd = socket(AF_INET,SOCK_STREAM,0);//创建套接字
    if(sockfd == -1)
    {
        return -1;
    }

    struct sockaddr_in saddr;
    memset(&saddr,0,sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(PORT);
    saddr.sin_addr.s_addr = inet_addr(IPSTR);//设置协议族端口号和IP地址

    int res = bind(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));//命名套接字
    if(res == -1)
    {
        perror("bind error");
        return -1;
    }

    listen(sockfd,LIS_MAX);//监听套接字

    return sockfd;
}

