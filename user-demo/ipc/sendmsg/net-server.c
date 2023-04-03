#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/un.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>



void usage()
{
	printf("Usage:\n\t./net-server <mysocket path>\n");
}


int main(int argc, char *argv[])
{
	int fd, new_fd, numbytes, i;
	struct sockaddr_un server_addr;

	char send_buf[BUFSIZ];
	char recv_buf[BUFSIZ];
	char *mysocketpath = argv[1];
	struct msghdr msg;
	struct iovec io;


	if (!argv[1]) {
		usage();
		return -1;
	}
		

	bzero(&server_addr, sizeof(server_addr));
	server_addr.sun_family = AF_UNIX;
	strncpy(server_addr.sun_path, mysocketpath,
		sizeof(server_addr.sun_path));
//	while ((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) ;
	while ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) ;
	if (bind(fd, (struct sockaddr *)(&server_addr), sizeof(struct sockaddr_un)) < 0) {
		fprintf(stderr, "bind fail\n");
		exit(EXIT_FAILURE);
	}
	printf("Bind Success!\n");

	    // listen
    if( listen(fd, SOMAXCONN) < 0){
        perror("listen socket failed");
        return -1;
    }
int sockaddr_len;
    while(1) {
	int sock_fd = accept(fd, (struct sockaddr*)&server_addr, &sockaddr_len);

	while (1) {
#if 0
		/* Build recv msg */
		bzero(&msg, sizeof(struct msghdr));
		msg.msg_name = &server_addr;
		msg.msg_namelen = sizeof(struct sockaddr_un);
		io.iov_base = recv_buf;
		io.iov_len = sizeof(recv_buf);
		msg.msg_iov = &io;
		msg.msg_iovlen = 1;

#if 0
		while (numbytes = recvmsg(fd, &msg, 0) > 0) {
			char *temp = msg.msg_iov[0].iov_base;	//Get the addr of recv msg buf
			temp[numbytes] = '\0';	//为数据末尾添加结束符Add end char for the recv msg.
			printf("get %d \n", numbytes);
			memcpy(recv_buf, temp, numbytes);
			printf("server recv:%s\n", recv_buf);
		}
#else
		numbytes = recvmsg(fd, &msg, 0);
		if (numbytes < 0) {
			if (errno == EINTR)
			       continue;
			fprintf(stderr, "recvmsg fail\n");
			exit(EXIT_FAILURE);
		}
			char *temp = msg.msg_iov[0].iov_base;	//Get the addr of recv msg buf
			temp[numbytes] = '\0';	//为数据末尾添加结束符Add end char for the recv msg.
			printf("get %d \n", numbytes);
			memcpy(recv_buf, temp, numbytes);
			printf("server recv:%s\n", recv_buf);
#if 0
			char *temp = msg.msg_iov[0].iov_base;	//Get the addr of recv msg buf
			printf("server recv:%s\n", temp);
			/* Build send msg */
			strncpy(send_buf, "This is the server want to send msg", sizeof(send_buf));
			bzero(&msg, sizeof(struct msghdr));
			msg.msg_name = &server_addr;
			msg.msg_namelen = sizeof(struct sockaddr_un);
			io.iov_base = send_buf;
			io.iov_len = sizeof(send_buf);
			msg.msg_iov = &io;
			msg.msg_iovlen = 1;
#endif
#endif
#endif
			strcpy(send_buf, "This is the server want to send msg");
		bzero(&msg, sizeof(struct msghdr));
		msg.msg_name = &server_addr;
		msg.msg_namelen = sizeof(struct sockaddr_un);
		io.iov_base = send_buf;
		io.iov_len = sizeof(send_buf);
		msg.msg_iov = &io;
		msg.msg_iovlen = 1;
			sendmsg(sock_fd, &msg, 0);
			printf("server send:%s\n", send_buf);
			sleep(2);
	}
	close(sock_fd);
    }
	close(fd);
	return 0;
}
