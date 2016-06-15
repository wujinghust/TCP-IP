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
#include<unistd.h>
#include<sys/select.h>  //
#include<sys/time.h>
#define MAXLINE  1024  //通信内容的最大长度

#ifndef FD_SETSIZE
#define FD_SETSIZE 25  //select最多能处理的文件描述符
#endif

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

 ssize_t readline(int sockfd, void *buf, size_t len)       //按行读取输入
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


int main()
{
    int sock_fd,new_fd,fd;//sock_fd用于监听，new_fd用于连接
    int maxi,maxfd；
    int client[FD_SETSIZE];//用于存放客户端描述符
    int nready;//检测到的事件数
    struct sockaddr_in srv_addr;//服务器的地址信息
    struct sockaddr_in client_addr;//客户机的地址信息
    int i,size; //地址结构数据的长度
    ssize_t n;
    fd_set rset,allset;     
    char sendbuf[1024],recvbuf[1024];
    memset(sendbuf,0,sizeof(sendbuf));
    memset(recvbuf,0,sizeof(recvbuf));

    /*创建套接字*/
    sock_fd=socket(AF_INET,SOCK_STREAM,0);//采用IPv4协议
    if(sock_fd==-1)
    {
        perror("creat socket failed");
        exit(1);
    }
    
    /*服务器地址参数*/
    srv_addr.sin_family=AF_INET;  
    srv_addr.sin_port=htons(3490);
    srv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    bzero(&srv_addr.sin_zero,sizeof(struct sockaddr_in));//bzero位清零函数，将sin_zero清零，sin_zero为填充字段，必须全部为零
    
    int on=1; //表示开启reuseaddr
    if(setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on))<0)  //打开地址、端口重用
        perror("setsockopt");
    
    /*绑定地址和端口*/
    if(bind(sock_fd,(struct sockaddr*)&srv_addr,sizeof(struct sockaddr))==-1)
    {
        perror("bind failed");
        exit(1);
    }
    
    /*设置监听模式，等待客户机的监听*/
     if((listen(sock_fd,5))==-1)
    {
        perror("listen failed");
        exit(1);
    }
    
    maxfd=sock_fd;
    maxi=-1;
    for(i=0;i<FD_SETSIZE;i++)
        client[i]=-1;    //描述符为-1表示空闲
 
    FD_ZERO(&rset);
    FD_ZERO(&allset);
    FD_SET(sock_fd,&allset);    

    //使用select实现并发服务器
    while(1)   
    {
        rset=allset;
        nready=select(maxfd+1,&rset,NULL,NULL,NULL);        
        if(nready==-1)
        {
            if(errno==EINTR)
                continue；
            ERR_EXIT("select");
        }
        
        if(FD_ISSET(sock_fd,&rset)) //监听套接口不在阻塞
        {
            size=sizeof(struct sockaddr_in);
            new_fd=accept(sock_fd,(struct sockaddr*)&client_addr,&size);  /*接受连接,采用非阻塞是的模式调用accep*/
            if(new_fd==-1)
            {
                perror("accept failed");
            //continue;//restart accept when EINTR
            }
        
                printf("server:got connection from IP= %s prot= %d \n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));//连接成功，打印客户机IP地址和端口号
            /*char *inet_nota(struct sockaddr_in in);
            头文件：
            arpa/inet.h
            Winsock2.h
            参数：
            一个网络上的IP地址
            返回值：
            如果正确，返回一个字符指针，指向一块存储着点分格式IP地址的静态缓冲区（同一线程内共享此内存）；错误，返回NULL。
            uint31_t ntohs(uint32_t net32bitvalue);
            头文件：
            #include<netinet/in.h>
            把net32bitvalue有网络字节序转换为主机字节序。
            */
                    if(send(new_fd,"Hello client,I am 192.168.229.125!\n",50,0)==-1)  //192.168.229.125为子进程IP，可更改
                        perror("send failed");
            for(i=0;i<FD_SETSIZE;i++)  
                if(client[i]<0)
                {
                    client[i]=new_fd;         //将描述符保存在某一个空闲的位置
                    break;
                }
            if(i==FD_SETSIZE)                //没有找到空闲的位置，即描述符个数达到上限
                perror("too many client");
            FD_SET(new_fd,&allset);
            if(new_fd>maxfd)           //更新最大描述符
                maxfd=new_fd;
            if(i>maxi)
                maxi=i;
            if(--nready<=0)         //若检测到的套接口已经处理完，则继续用select监听
                continue;
        }
    
        for(i=0;i<=maxi;i++)
        {
            if((fd=client[i])<0)
                continue;
            if(FD_ISSET(fd,&rset))
            {
                if((n=readline(fd,recvbuf,MAXLINE))==0)
                {
                    printf("client closed\n");
                    close(fd);
                    FD_CLR(fd,&allset);  //客户端关闭，将其清除
                    client[i]=-1;
                }                
                else if(n==-1)
                    perror("readline\n");
                else
                {
                    writen(fd,recvbuf,n);
                    fputs(stdout,recvbuf,n);
                }
    
                if(--nready<=0)
                    break;                    
            }
        }

    }
 }
