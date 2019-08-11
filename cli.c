#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

//下载
int recv_file(int sockfd, char* name)//下载，客户端接收文件
{
    char buff[128] = {0};
    if(recv(sockfd,buff,127,0) <= 0)//没有接收到文件
    {
        return -1;
    }

    if(strncmp(buff,"ok",2) !=0 )//字符串比较
    {
        printf("%s\n",buff);
        return 0;
    }

    int size = 0;
    sscanf(buff+3,"%d",&size);

    printf("file(%s):%d\n",name,size);

    int fd = open(name,O_WRONLY | O_CREAT, 0600);//以只读方式打开接收的文件，若无文件则进行创建
    if(fd == -1)
    {
        send(sockfd,"err",3,0);
        return;
    }
    send(sockfd,"ok",2,0);
    
    int num = 0;
    int cur_size = 0;
    char data[256] = {0};
    while( 1 )
    {
        num = recv(sockfd,data,256,0);
        if(num <= 0)
        {
            return -1;
        }

        write(fd,data,num);
        cur_size = cur_size + num;

        float f = cur_size * 100.0 / size;
        printf("\033[?25l");//去掉光标闪烁
        printf("download:%.2f%%\r",f);
        fflush(stdout);
        if(cur_size >= size)
        {
            break;
        }
    }
    printf("\n");
    return 0;
}

//上传
void send_file(int c, char *myargv[])//客户端上传文件
{
    if(myargv[1] == NULL)//
    {
        send(c,"no file name",12,0);
        return;
    }
    int fd = open(myargv[1],O_RDONLY);
    if(fd == -1)//没有找到文件
    {
        send(c,"not found!",10,0);
        return;
    }

    int size = lseek(fd,0,SEEK_END);
    lseek(fd,0,SEEK_SET);//求文件的大小

    char status[32] = {0};//保存客户端目前的状态
    sprintf(status,"ok#%d",size);
    send(c,status,strlen(status),0);//把文件大小保存到c中
    ////
    char ser_status[32] = {0};
    if(recv(c,ser_status,31,0) <= 0)
    {
        return;
    }

   else if(strncmp(ser_status,"ok",2) != 0)
    {
        return;
    }
    else
    {
        char data[256] = {0};
        int num = 0;

        while(1)
        {
            num = read(fd,data,256);
            if(num <= 0)
            {
                return;
            }
        int cur_size = cur_size + num;
  
        float f = cur_size * 100.0 / size;
        printf("\033[?25l");//去掉光标闪烁
        send(c,data,strlen(data),0);
        printf("upload:%.2f%%\r",f);
        fflush(stdout);
        }

        close(fd);
        return;
    } 
}


int main()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);//创建套接字
    assert(sockfd != -1);

    struct sockaddr_in saddr;
    memset(&saddr,0,sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(6001);
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");//创建协议族端口号和ip地址

    int res = connect(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));//命名套接字
    assert(res != -1);

    while(1)
    {
        char buff[128] = {0};//接收键盘内容
        printf("Connect success ~]$");//与服务器连接成功
        fflush(stdout);
        fgets(buff,128,stdin);

        if(strncmp(buff,"end",3) == 0)//断开连接
        {
            break;
        }

        buff[strlen(buff)-1] = 0;

        if(buff[0] == 0)//没有接收到get,put等
        {
            continue;
        }

        char tmp[128] = {0};
        strcpy(tmp, buff);

        char* myargv[10] = {0};//客户端发送的文件请求
        char* s = strtok(tmp," ");//分割字符串

        int i = 0;
        while(s != NULL)
        {
            myargv[i++] = s;
            s = strtok(NULL, " ");
        }
        
        if(strcmp(myargv[0],"get") == 0)  //下载
        {
            send(sockfd,buff,strlen(buff),0);//发送get a.c文件名进行发送
            recv_file(sockfd,myargv[1]);//接收a.c的内容
        }
        
        else if(strcmp(myargv[0],"put") == 0)  //上传
        {
            send_file(sockfd,myargv);//发送文件内容
            recv(sockfd,buff,strlen(buff),0);//接收成功消息
        
        }
        
        else
        {
            send(sockfd,buff,strlen(buff),0);

            char read_buff[1024] = {0};
            recv(sockfd,read_buff,1023,0);

            printf("%s",read_buff+3);
        }
    }
    close(sockfd);

    exit(0);
}

