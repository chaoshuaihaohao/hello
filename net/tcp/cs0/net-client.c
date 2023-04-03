#include <stdio.h>
#include <string.h>		//bzero
#include <stdlib.h>		//exit

#include <netinet/in.h>		//IPPROTO_TCP
#include <arpa/inet.h>		//htons,htonl

#include <unistd.h>		//read,close
#include <sys/types.h>
#include <sys/socket.h>		//socket,bind


enum {
	ZERO,
	ONE,
	TWO,
	THREE,
	FOUR,
	MAX
};


int main(int argc, char *argv[])
{
	unsigned short port = 8000;	// 服务器的端口号
	char *server_ip = "192.168.11.219";	// 服务器ip地址

	int sockfd;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);	// 创建通信端点：套接字
	if (sockfd < 0) {
		perror("socket");
		exit(-1);
	}

	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));	// 初始化服务器地址
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	//将点分格式ip转换为二进制格式
	inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

	int err_log = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));	// 主动连接服务器
	if (err_log != 0) {
		perror("connect");
		close(sockfd);
		exit(-1);
	}

#if 0
	char send_buf[100] = "this is for test";
	send(sockfd, send_buf, strlen(send_buf), 0);	// 向服务器发送信息
#else
	int send_buf = ZERO;
	int recv_buf = 0;
	while(1) {
		send_buf %= 4;
		printf("send data:%d\n", send_buf);
		send(sockfd, &send_buf, sizeof(send_buf), 0);	// 向服务器发送信息
		send_buf++;
		//If we need ack, block recv here.
		recv(sockfd, &recv_buf, sizeof(recv_buf), 0);
		printf("recv data:%d\n", recv_buf);
		sleep(1);
	}
#endif

	close(sockfd);

	return 0;
}
