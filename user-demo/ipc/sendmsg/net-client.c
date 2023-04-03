#include <stdio.h>
#include <string.h>		//bzero
#include <stdlib.h>		//exit

#include <unistd.h>		//read,close

#include <sys/types.h>
#include <sys/socket.h>		//socket,bind

#include <linux/un.h>
#include <errno.h>


#define USE_TCP   // udp有问题

#ifdef USE_TCP
static int domain = AF_UNIX;
static int type = SOCK_STREAM;
static int protocol = 0;



int SOCKET_SOCK_TYPE = SOCK_STREAM;
int SOCKET_PROTO_TYPE = 0;
#else
int SOCKET_SOCK_TYPE = SOCK_DGRAM;
int SOCKET_PROTO_TYPE = 0;
#endif


int main(int argc, char *argv[])
{
	int c, numbytes;
	char *mysocketpath = argv[1];
	struct msghdr msg;
	struct iovec io;

	int sockfd;
	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);	// 创建通信端点：套接字
	if (sockfd < 0) {
		perror("socket");
		exit(-1);
	}

	struct sockaddr_un server_addr;
	bzero(&server_addr, sizeof(server_addr));	// 初始化服务器地址
	server_addr.sun_family = AF_UNIX;
	strncpy(server_addr.sun_path, mysocketpath,
		sizeof(server_addr.sun_path));
	printf("%s\n", server_addr.sun_path);

	char recv_buf[BUFSIZ];
	char send_buf[BUFSIZ];
	strncpy(send_buf, argv[2], sizeof(send_buf));

	if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
	{
	        perror("connect socket failed");
		return -1;
	}

//	while (1) {
#if 0
		/* Sendmsg */
		bzero(&msg, sizeof(struct msghdr));
		//msg.msg_name = NULL;
		msg.msg_name = &server_addr;
		msg.msg_namelen = sizeof(struct sockaddr_un);
		io.iov_base = send_buf;
		io.iov_len = sizeof(send_buf);
		msg.msg_iov = &io;
		msg.msg_iovlen = 1;
		printf("client sendmsg:%s\n", send_buf);
		sendmsg(sockfd, &msg, 0);	// 向服务器发送信息
#if 0
		bzero(&msg, sizeof(struct msghdr));
		//msg.msg_name = NULL;
		msg.msg_name = &server_addr;
		msg.msg_namelen = sizeof(struct sockaddr_un);
		io.iov_base = recv_buf;
		io.iov_len = sizeof(recv_buf);
		msg.msg_iov = &io;
		msg.msg_iovlen = 1;
		recvmsg(sockfd, &msg, 0);
		char *temp = msg.msg_iov[0].iov_base;   //Get the addr of recv msg buf
		printf("client recv:%s\n", temp);
		printf("client recv_buf:%s\n", recv_buf);
#endif
#endif
		bzero(&msg, sizeof(struct msghdr));
		//msg.msg_name = NULL;
		msg.msg_name = &server_addr;
		msg.msg_namelen = sizeof(struct sockaddr_un);
		io.iov_base = recv_buf;
		io.iov_len = sizeof(recv_buf);
		msg.msg_iov = &io;
		msg.msg_iovlen = 1;
		recvmsg(sockfd, &msg, 0);
		char *temp = msg.msg_iov[0].iov_base;   //Get the addr of recv msg buf
		printf("client recv:%s\n", temp);
		printf("client recv_buf:%s\n", recv_buf);

//	}
	close(sockfd);

	return 0;
}
