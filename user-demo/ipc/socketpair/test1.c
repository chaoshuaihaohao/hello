#include <stdio.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/*
* Returns a full-duplex pipe (a UNIX domain socket) with
* the two file descriptors returned in fd[0] and fd[1].
*/
int fd_pipe(int fd[2])
{
	return (socketpair(AF_UNIX, SOCK_STREAM, 0, fd));
}


int main()
{
	int fd[2] = {-1};
	int ret = 0;

	printf("pid = %d\n", getpid());
	for (int i = 0; i < 2; i++) {
		printf("fd[%d]=%d\n", i, fd[i]);
	}

	ret = fd_pipe(fd);
	printf("ret=%d\n", ret);
	for (int i = 0; i < 2; i++) {
		printf("fd[%d]=%d\n", i, fd[i]);
	}
	pause();
}
