#include <stdio.h>
#include <string.h>		//bzero
#include <stdlib.h>		//exit

#include <unistd.h>		//read,close

#include <sys/types.h>
#include <sys/socket.h>		//socket,bind

#include <linux/un.h>

int main(int argc, char *argv[])
{
	int c;
	char * mysocketpath = argv[1];
	char *msg = argv[2];

	int sockfd;
	sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);	// 创建通信端点：套接字
	if (sockfd < 0) {
		perror("socket");
		exit(-1);
	}

	struct sockaddr_un server_addr;
	bzero(&server_addr, sizeof(server_addr));	// 初始化服务器地址
	server_addr.sun_family = AF_UNIX;
	memcpy(server_addr.sun_path, mysocketpath, sizeof(server_addr.sun_path));

	int err_log = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));	// 主动连接服务器
	if (err_log != 0) {
		perror("connect");
		close(sockfd);
		exit(-1);
	}

	char recv_buf[500] = { 0 };
	while(1) {
		send(sockfd, msg, strlen(msg), 0);	// 向服务器发送信息
		while (recv(sockfd, recv_buf, sizeof(recv_buf), 0) > 0) {
			printf("recv data: %s\n", recv_buf);
			break;
		}
		sleep(1);
	}
	close(sockfd);

	return 0;
}
