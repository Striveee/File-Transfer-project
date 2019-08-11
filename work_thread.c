#include "work_thread.h"
#include <fcntl.h>

#define ARGC 10

//启动线程
void thread_start(int c)
{
    pthread_t id;
    pthread_create(&id,NULL,work_thread,(void*)c);
}

void get_argv(char buff[], char* myargv[])//分割字符串
{
    char *p = NULL;
    char *s = strtok_r(buff," ",&p);
    int i = 0;

    while(s != NULL)
    {
        myargv[i++] = s;
        s = strtok_r(NULL," ",&p);
    }
}

//下载
void send_file(int c, char *myargv[])//服务器发送文件
{
    //myargv[1]表示文件名,myargv[0]表示get或者put命令
    if(myargv[1] == NULL)//客户端没输入需下载的文件名
    {
        send(c,"get:no file name!",17,0);
        return;
    }
    int fd = open(myargv[1],O_RDONLY);//打开文件描述符
    if(fd == -1)//没有找到文件
    {
        send(c,"not found!",10,0);
        return;
    }
    
    int size = lseek(fd,0,SEEK_END);//求文件fd的大小
    lseek(fd,0,SEEK_SET);
    
    char ser_status[32] = {0};//保存服务器目前的状态
    sprintf(ser_status,"ok#%d",size);
    send(c,ser_status,strlen(ser_status),0);//将status的大小保存到c里,发送给客户端

    char cli_status[32] = {0};//客户端给服务器返回的状态
    if(recv(c,cli_status,31,0) <= 0)//如果接收到客户端状态异常
    {
        return;
    }
    
    if(strncmp(cli_status,"ok",2) != 0)//客户端没有发送ok
    {
        return;
    }
    char data[256] = {0};//每次读文件大小
    int num = 0;//实际发送的文件大小

    while((num = read(fd,data,256)) > 0)
    {
        send(c,data,num,0);//发送文件内容
    }
    close(fd);
    return;
}


//上传
int recv_file(int sockfd, char * name)
{
    char buff[128] = {0};

   if(recv(sockfd,buff,127,0) <= 0)//链接异常
   {
        return -1;
   }
    if(strncmp(buff,"ok",2) != 0)//客户端发送ok
   {
        printf("%s\n",buff);
        return 0;
    }
    int size = 0;
    sscanf(buff+3,"%d",&size);
    printf("file(%s):%d\n",name,size);
    int fd = open(name,O_WRONLY | O_CREAT, 0600);

    if(fd == -1)//文件创建失败
    {
        send(sockfd,"err",3,0);
        return;
    }

    if(strncmp(buff,"ok",2) != 0)
    {
        return;
    }
    send(sockfd,"ok",2,0);//确认收到客户端发送的请求及文件大小
    int num = 0;
    char data[256] = {0};

     while(1)
    {
        num = recv(sockfd,data,256,0);
        if(num <= 0)//没有接收到文件
        {
            return;
        }
        
        int cur_size = cur_size + num;
        write(fd,data,num);
    }
    printf("sucessful!\n");
    return ;
}


//工作线程
void* work_thread(void* arg)
{
    int c = (int)arg;

    //测试
    while(1)
    {
        char buff[128] = {0};
        int n = recv(c, buff, 127, 0);
        if(n <= 0)
        {
            close(c);
            printf("one client over\n");
            break;
        }

        printf("recv:%s\n",buff);
        char * myargv[ARGC] = {0};
        get_argv(buff,myargv);

        if(strcmp(myargv[0],"get") == 0)//下载
        {
            send_file(c,myargv);
        }
        
        else if(strcmp(myargv[0],"put") == 0)//上传
        {
            recv_file(c, myargv[1]);
        }
        
        else
        {
            int pipefd[2];
            pipe(pipefd);

            pid_t pid = fork();
            if(pid == 0)
            {
                dup2(pipefd[1], 1);
                dup2(pipefd[1], 2);
                execvp(myargv[0],myargv);
                perror("exec error");
                exit(0);
            }
            close(pipefd[1]);
            wait(NULL);

            char read_buff[1024] = {0};
            strcpy(read_buff,"ok#");
            read(pipefd[0],read_buff+strlen(read_buff),1000);

            send(c,read_buff,strlen(read_buff),0);
        }
    }
}

