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
 
 #define MAXLINE  50  //通信内容的最大长度
int main()
{
	int sock_fd,new_fd;//sock_fd用于监听，new_fd用于连接
	struct sockaddr_in srv_addr;//服务器的地址信息
	struct sockaddr_in client_addr;//客户机的地址信息
	int size; //地址结构数据的长度
	pid_t  pid;  //子进程id
	ssize_t n;
	char buf[MAXLINE]; //用于存放通信的内容
	
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
	
	/*连接到服务器
	if(connect(sock_fd,(struct sockaddr*)&srv_addr,sizeof(sock_fd))==-1)
	{
		perror("bind failed");
		exit(1);
	}*/
	
	/*设置监听模式，等待客户机的监听*/
	if((listen(sock_fd,5))==-1)
	{
		perror("listen failed");
		exit(1);
	}
	
	/*接受连接,采用非阻塞是的模式调用accep*/
	//while(1)
	//{
		size=sizeof(struct sockaddr_in);
		new_fd=accept(sock_fd,(struct sockaddr*)&client_addr,&size);
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

		pid=fork();  //父进程建立套接字的连接之后，创建子进程用于通信
	    if(pid<0)
               perror("fork error\n");
	    if(!pid)//创建新的子进程,用于发送数据
	    {  
		   // close(sock_fd);//子进程不需要监听，所以子进程关闭监听套接字
	        while(fgets(buf,sizeof(buf),stdin)!=NULL)
		    {
				write(new_fd,buf,strlen(buf));
				memset(buf,0,sizeof(buf));   //清空，以免和下一次混淆
		   // exit(EXIT_SUCCESS);
            }
			exit(EXIT_SUCCESS);
	    }
		else   //f=父进程用于接收数据
		{
			char recvbuf[50];
			//close(new_fd);//父进程不需要连接，所以关闭连接套接字
			while(1)
			{
				memset(recvbuf,0,sizeof(recvbuf));
				n=read(new_fd,recvbuf,MAXLINE);
				if(n==0)
				{	
					printf("peer closed\n");	
					break;
				}
				else if(n<0)
					perror("read from client error");	
			 	fputs(recvbuf,stdout);
 			}
			exit(EXIT_SUCCESS);
			 
		}
	       

	    //while(waitpid(-1,NULL,WNOHANG)>0);//等待子进程结束，进行新的连接
	//}
	return 0;
}
