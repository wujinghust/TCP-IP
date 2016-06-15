# TCP-IP
TCP/IP套接字编程
服务器与客户端进行通讯,实现并发回射服务器

select_srv.c为用select实现的并发服务器程序，客户端与前面一样

shutdown_cli.c为在客户端程序中将close()函数改为了shutdown()函数，服务器程序可以使用select_srv.c程序
