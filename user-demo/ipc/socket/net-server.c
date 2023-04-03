#include <stdio.h>
#include <string.h>		//bzero
#include <stdlib.h>		//exit

#include <unistd.h>		//read,close
#include <sys/types.h>
#include <sys/socket.h>		//socket,bind

#include <linux/un.h>

int main(int argc, char *argv[])
{
	if (!argv[1]) {
		printf("Error: No socket path\n");
		return -1;
	}
	char * mysocketpath = argv[1];

	int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("socket");
		exit(-1);
	}

	struct sockaddr_un my_addr;
	bzero(&my_addr, sizeof(my_addr));
	my_addr.sun_family = AF_UNIX;
	memcpy(my_addr.sun_path, mysocketpath, sizeof(my_addr.sun_path));

	int err_log =
	    bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr));
	if (err_log != 0) {
		perror("binding");
		close(sockfd);
		exit(-1);
	}

	err_log = listen(sockfd, 0);	// 等待队列为1
	if (err_log != 0) {
		perror("listen");
		close(sockfd);
		exit(-1);
	}

	int i = 0;

		int connfd;
	while (1) {
		struct sockaddr_un client_addr;
		socklen_t cliaddr_len = sizeof(client_addr);

		connfd =
		    accept(sockfd, (struct sockaddr *)&client_addr,
			   &cliaddr_len);
		if (connfd < 0) {
			perror("accept");
			continue;
		}

		while(1) {
		printf("-----------%d------\n", ++i);

		char recv_buf[500] = { 0 };
		while (recv(connfd, recv_buf, sizeof(recv_buf), 0) > 0) {
			printf("recv data: %s\n", recv_buf);
			send(connfd, "Server recv data", strlen("Server recv data"), 0);
			break;
		}
		}

		close(connfd);	//关闭已连接套接字
		printf("client closed!\n");
	}
	close(sockfd);		//关闭监听套接字
	return 0;
}
