#include <stdio.h>
#include <string.h>		//bzero
#include <stdlib.h>		//exit

#include <unistd.h>		//read,close
#include <netinet/in.h>		//IPPROTO_TCP
#include <arpa/inet.h>		//htons,htonl

#include <sys/types.h>
#include <sys/socket.h>		//socket,bind

#include <getopt.h>

void usage()
{
	printf("Usage:\n"
	       "\t-i <host ip>\t\t:set the server ip\n"
	       "\t-p <port>\t\t:set the server port\n"
	       "\t-m <message string>\t:set the message to send\n");
}

char *l_opt_arg;
char* const short_options = "i:p:m:";
struct option long_options[] = {
	{"host ip", 1, NULL, 'i' },
	{"port", 1, NULL, 'p' },
	{"msg", 1, NULL, 'm' },
	{0, 0, 0, 0 },
};

int main(int argc, char *argv[])
{
	if (argc < 4) {
	//	fprintf(stderr, "Usage: %s host port msg...\n", argv[0]);
		usage();
		exit(EXIT_FAILURE);
	}

	int c;
	char *server_ip = "127.0.0.1";	// 服务器ip地址
	unsigned short port = 8000;	// 服务器的端口号
	char *msg = NULL;
	while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) != -1)
	{
		switch (c)
		{
		case 'i':
			l_opt_arg = optarg;
			printf("i:%s\n", l_opt_arg);
			server_ip = l_opt_arg;
			break;
		case 'p':
			l_opt_arg = optarg;
			printf("p:%s\n", l_opt_arg);
			port = atoi(l_opt_arg);
			break;
		case 'm':
			l_opt_arg = optarg;
			printf("m:%s\n", l_opt_arg);
			msg = l_opt_arg;
			break;
		default:
			usage();
			exit(EXIT_FAILURE);
		}
	}


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

	send(sockfd, msg, strlen(msg), 0);	// 向服务器发送信息

	close(sockfd);

	return 0;
}
