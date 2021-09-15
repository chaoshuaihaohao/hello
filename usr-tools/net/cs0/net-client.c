#include <stdio.h>
#include <string.h>	//bzero
#include <stdlib.h>	//exit

#include <netinet/in.h>	//IPPROTO_TCP
#include <arpa/inet.h>	//htons,htonl

#include <unistd.h>	//read,close
#include <sys/types.h>
#include <sys/socket.h>	//socket,bind
#include <linux/in.h>

int main(int argc, char *argv[])
{
	unsigned short port = 8000;        	// 服务器的端口号
	char *server_ip = "10.20.53.139";    	// 服务器ip地址

	int sockfd;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);// 创建通信端点：套接字
	if(sockfd < 0)
	{
		perror("socket");
		exit(-1);
	}

	struct sockaddr_in server_addr;
	bzero(&server_addr,sizeof(server_addr)); // 初始化服务器地址
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	//将点分格式ip转换为二进制格式
	inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

	int err_log = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));      // 主动连接服务器
	if(err_log != 0)
	{
		perror("connect");
		close(sockfd);
		exit(-1);
	}

	char send_buf[100]="this is for test";
	send(sockfd, send_buf, strlen(send_buf), 0);   // 向服务器发送信息

	close(sockfd);

	return 0;
}
