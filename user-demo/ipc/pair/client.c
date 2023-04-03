#include "apue.h"
#include "open.h"

#include <fcntl.h>

#include <sys/socket.h>

#include <sys/socket.h>		/* struct msghdr */
/* size of control buffer to send/recv one file descriptor */
#define CONTROLLEN CMSG_LEN(sizeof(int))
static struct cmsghdr *cmptr = NULL;	/* malloc’ed first time */
/*
* Receive a file descriptor from a server process. Also, any data
* received is passed to (*userfunc)(STDERR_FILENO, buf, nbytes).
* We have a 2-byte protocol for receiving the fd from send_fd().
*/
int recv_fd(int fd, ssize_t(*userfunc) (int, const void *, size_t))
{
	int newfd, nr, status;
	char *ptr;
	char buf[MAXLINE];
	struct iovec iov[1];
	struct msghdr msg;
	status = -1;
	for (;;) {
		iov[0].iov_base = buf;
		iov[0].iov_len = sizeof(buf);
		msg.msg_iov = iov;
		msg.msg_iovlen = 1;
		msg.msg_name = NULL;
		msg.msg_namelen = 0;
		if (cmptr == NULL && (cmptr = malloc(CONTROLLEN)) == NULL)
			return (-1);
		msg.msg_control = cmptr;
		msg.msg_controllen = CONTROLLEN;
		if ((nr = recvmsg(fd, &msg, 0)) < 0) {
			printf("recvmsg error");
			return (-1);
		} else if (nr == 0) {
			printf("connection closed by server");
			return (-1);
		}
/*
* See if this is the final data with null & status. Null
* is next to last byte of buffer; status byte is last byte.
* Zero status means there is a file descriptor to receive.
*/
		for (ptr = buf; ptr < &buf[nr];) {
			if (*ptr++ == 0) {
				if (ptr != &buf[nr - 1])
					printf("message format error");
				status = *ptr & 0xFF;	/* prevent sign extension */
				if (status == 0) {
					if (msg.msg_controllen != CONTROLLEN)
						printf("status = 0 but no fd");
					newfd = *(int *)CMSG_DATA(cmptr);
				} else {
					newfd = -status;
				}
				nr -= 2;
			}
		}
		if (nr > 0 && (*userfunc) (STDERR_FILENO, buf, nr) != nr)
			return (-1);
		if (status >= 0)	/* final data has arrived */
			return (newfd);	/* descriptor, or -status */
	}
}

/*
* Returns a full-duplex pipe (a UNIX domain socket) with
* the two file descriptors returned in fd[0] and fd[1].
*/
int fd_pipe(int fd[2])
{
	return (socketpair(AF_UNIX, SOCK_STREAM, 0, fd));
}

#define BUFFSIZE 8192

#include <sys/uio.h>		/* struct iovec */
/*
* Open the file by sending the "name" and "oflag" to the
* connection server and reading a file descriptor back.
*/
int csopen(char *name, int oflag)
{
	pid_t pid;
	int len;
	char buf[10];
	struct iovec iov[3];
	static int fd[2] = { -1, -1 };
	if (fd[0] < 0) {	/* fork/exec our open server first time */
		if (fd_pipe(fd) < 0) {
			printf("fd_pipe error");
			return (-1);
		}
		if ((pid = fork()) < 0) {
			printf("fork error");
			return (-1);
		} else if (pid == 0) {	/* child */
			close(fd[0]);
			if (fd[1] != STDIN_FILENO &&
			    dup2(fd[1], STDIN_FILENO) != STDIN_FILENO)
				printf("dup2 error to stdin");
			if (fd[1] != STDOUT_FILENO &&
			    dup2(fd[1], STDOUT_FILENO) != STDOUT_FILENO)
				printf("dup2 error to stdout");
			if (execl("./opend", "opend", (char *)0) < 0)
				printf("execl error");
		}
		close(fd[1]);	/* parent */
	}
	sprintf(buf, " %d", oflag);	/* oflag to ascii */
	iov[0].iov_base = CL_OPEN " ";	/* string concatenation */
	iov[0].iov_len = strlen(CL_OPEN) + 1;
	iov[1].iov_base = name;
	iov[1].iov_len = strlen(name);
	iov[2].iov_base = buf;
	iov[2].iov_len = strlen(buf) + 1;	/* +1 for null at end of buf */
	len = iov[0].iov_len + iov[1].iov_len + iov[2].iov_len;
	if (writev(fd[0], &iov[0], 3) != len) {
		printf("writev error");
		return (-1);
	}
/* read descriptor, returned errors handled by write() */
	return (recv_fd(fd[0], write));
}

int main(int argc, char *argv[])
{
	int n, fd;
	char buf[BUFFSIZE];
	char line[MAXLINE];
/* read filename to cat from stdin */
	while (fgets(line, MAXLINE, stdin) != NULL) {
		if (line[strlen(line) - 1] == '\n')
			line[strlen(line) - 1] = 0;	/* replace newline with null */
/* open the file */
		if ((fd = csopen(line, O_RDONLY)) < 0)
			continue;	/* csopen() prints error from server */
/* and cat to stdout */
		while ((n = read(fd, buf, BUFFSIZE)) > 0)
			if (write(STDOUT_FILENO, buf, n) != n)
				printf("write error");
		if (n < 0)
			printf("read error");
		close(fd);
	}
	exit(0);
}
