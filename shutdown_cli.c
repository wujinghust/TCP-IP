#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<sys/wait.h>      //*进程用的头文件*/
#include<netinet/in.h>
#include<arpa/inet.h>

#define MAXBYTEMUN   1024


ssize_t readn(int fd, void *buf, size_t count)
{
        ssize_t nleft=count;
        ssize_t nread;
        char *charbuf=(char*) buf;

        while(nleft>0)
        {
                nread=read(fd,charbuf,nleft);
                if(nread<0)
                  {
                        if(errno==EINTR)
                           continue;
                        return -1;
                  }
                else if(nread==0)
                       return count-nleft;

                charbuf +=nread;
                nleft=count-nread;
        }

        return count;
}

ssize_t writen(int fd, const void *buf, size_t count)
{
        ssize_t nleft=count;
        ssize_t nwrite;
        char *charbuf=(char*) buf;

        while(nleft>0)
        {
                nwrite=write(fd,charbuf,nleft);
                if(nwrite<0)
                  {
                        if(errno==EINTR)
                           continue;
                        return -1;
                  }
                else if(nwrite==0)
                       return count-nleft;

        charbuf +=nwrite;
        nleft=count-nwrite;
    }
    return count;
}
 
 ssize_t recv_peek(int sockfd, void *buf, size_t len)
{
    int ret;
    while(1)
    {
        ret=recv(sockfd,buf,len,MSG_PEEK);
        if(ret==-1&& errno==EINTR)
            continue;
        return ret;
    }
}

 ssize_t readline(int sockfd, void *buf, size_t len)
{ 
    ssize_t nleft=len,nread;
    int ret;
    char* bufchar=buf;
    while(1)
    {
         ret=recv_peek(sockfd,bufchar,len);
        if(ret<0||ret==0)
            return ret;
        nread=ret;
        int i;
        for(i=0;i<nread;i++)
        {
            if(bufchar[i]=='\n')
            {
                ret=readn(sockfd,bufchar,i+1);
                if(ret!=i+1)
                    exit(EXIT_FAILURE);
                return ret;
            }
        }
        if(nread>nleft)
            exit(EXIT_FAILURE);
        nleft-=nread;
        ret=readn(sockfd,bufchar,nread);
        if(ret!=nread)
            exit(EXIT_FAILURE);
        bufchar+=nread;
    }
    return -1;

}


int main(int argc,char *argv[])
{
    int sock_fd,numbytes,maxfd,fd_stdin,nready;
//    char buf[MAXBYTEMUN];
    struct hostent;
    struct sockaddr_in client_addr;//客户机的地址信息
    ssize_t ret;
    char recvbuf[1024]={'0'},sendbuf[1024]={'0'};
    fd_set  rset;
        int stdineof;
    
    if(argc!=2)
    {
        fprintf(stderr,"usage: client IPAddress\n");   //执行客户端程序时，输入客户端程序名称和其IP地址
        exit(1);    
    }
    
    /*创建套接字*/
    sock_fd=socket(AF_INET,SOCK_STREAM,0);//采用IPv4协议
    if(sock_fd==-1)
    {
        perror("creat socket failed");
        exit(1);
    }
    
    /*服务器地址参数*/
    client_addr.sin_family=AF_INET;  
    client_addr.sin_port=htons(3490);
    client_addr.sin_addr.s_addr=inet_addr(argv[1]);
    bzero(&client_addr.sin_zero,sizeof(struct sockaddr_in));//bzero位清零函数，将sin_zero清零，sin_zero为填充字段，必须全部为零
    
    
    /*连接到服务器*/
    if(connect(sock_fd,(struct sockaddr*)&client_addr,sizeof(struct sockaddr))==-1)
    {
        perror("connect failed");
        exit(1);
    }
    if((numbytes=recv(sock_fd,recvbuf,MAXBYTEMUN,0))==-1)
        {       
            perror("receive failed");
             exit(1);
        }
    recvbuf[numbytes]='\0';//在字符串末尾加上\0，否则字符串无法输出
    printf("Received: %s\n",recvbuf);
    
    fd_stdin=fileno(stdin);
    if(sock_fd>fd_stdin)
        maxfd=sock_fd;
    else
        maxfd=fd_stdin;
        stdineof=0;
        
            while(1)
            {
                FD_SET(fd_stdin,&rset);
                FD_SET(sock_fd,&rset);
                nready=select(maxfd+1,&rset,NULL,NULL,NULL);
                if(nready==-1)
                    perror("nready\n");
                else if(nready==0)
                    continue;
                
                if(FD_ISSET(sock_fd,&rset))
                {
                    memset(recvbuf,0,sizeof(recvbuf));
                    ret=readline(sock_fd,recvbuf,1024);
                    if(ret<0)
                        perror("read from server error");
                    else if(ret==0)
                    {
                        if(stdineof==1)    //如果是因为输入已经完毕造成的读到的数据个数为0，则正常终止
                                break;
                        else               //否则是服务器端关闭
                        {
                            printf("server closed\n");
                            break;
                        }
                    }
                    writen(fileno(stdout),recvbuf,ret);
                }
                
                if(FD_ISSET(fd_stdin,&rset))
                {
                    if(fgets(sendbuf,sizeof(sendbuf),stdin)==NULL)
                        {
                            stdineof=1;                     //表示输入已经完毕
                            shutdown(sock_fd,SHUT_WR);      //关闭sock_fd的写端
                            FD_CLR(fileno(stdin),&rset);
                            continue;
                        }     
                    else
                        {
                             writen(sock_fd,sendbuf,strlen(sendbuf));
                            memset(sendbuf,0,sizeof(sendbuf));    //清空，以免和下一次混淆
                        }
                }
            }
    close(sock_fd);
    return 0;
}
