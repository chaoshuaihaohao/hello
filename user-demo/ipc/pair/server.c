#include "apue.h"
#include "opend.h"

/*
* This function is called by buf_args(), which is called by
* handle_request(). buf_args() has broken up the client’s
* buffer into an argv[]-style array, which we now process.
*/
int cli_args(int argc, char **argv)
{
	if (argc != 3 || strcmp(argv[0], CL_OPEN) != 0) {
		strcpy(errmsg, "usage: <pathname> <oflag>\n");
		return (-1);
	}
	pathname = argv[1];	/* save ptr to pathname to open */
	oflag = atoi(argv[2]);
	return (0);
}

#define MAXARGC 50		/* max number of arguments in buf */
#define WHITE " \t\n"		/* white space for tokenizing arguments */
/*
* buf[] contains white-space-separated arguments. We convert it to an
* argv-style array of pointers, and call the user’s function (optfunc)
* to process the array. We return -1 if there’s a problem parsing buf,
* else we return whatever optfunc() returns. Note that user’s buf[]
* array is modified (nulls placed after each token).
*/
int buf_args(char *buf, int (*optfunc)(int, char **))
{
	char *ptr, *argv[MAXARGC];
	int argc;
	if (strtok(buf, WHITE) == NULL)	/* an argv[0] is required */
		return (-1);
	argv[argc = 0] = buf;
	while ((ptr = strtok(NULL, WHITE)) != NULL) {
		if (++argc >= MAXARGC - 1)	/* -1 for room for NULL at end */
			return (-1);
		argv[argc] = ptr;
	}
	argv[++argc] = NULL;
/*
* Since argv[] pointers point into the user’s buf[],
* user’s function can just copy the pointers, even
* though argv[] array will disappear on return.
*/
	return ((*optfunc) (argc, argv));
}

ssize_t				/* Write "n" bytes to a descriptor */
writen(int fd, const void *ptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;
	nleft = n;
	while (nleft > 0) {
		if ((nwritten = write(fd, ptr, nleft)) < 0) {
			if (nleft == n)
				return (-1);	/* error, return -1 */
			else
				break;	/* error, return amount written so far */
		} else if (nwritten == 0) {
			break;
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	return (n - nleft);	/* return >= 0 */
}

#include <sys/socket.h>
/* size of control buffer to send/recv one file descriptor */
#define CONTROLLEN CMSG_LEN(sizeof(int))
static struct cmsghdr *cmptr = NULL;	/* malloc’ed first time */
/*
* Pass a file descriptor to another process.
* If fd<0, then -fd is sent back instead as the error status.
*/
int send_fd(int fd, int fd_to_send)
{
	struct iovec iov[1];
	struct msghdr msg;
	char buf[2];		/* send_fd()/recv_fd() 2-byte protocol */
	iov[0].iov_base = buf;
	iov[0].iov_len = 2;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	if (fd_to_send < 0) {
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
		buf[1] = -fd_to_send;	/* nonzero status means error */
		if (buf[1] == 0)
			buf[1] = 1;	/* -256, etc. would screw up protocol */
	} else {
		if (cmptr == NULL && (cmptr = malloc(CONTROLLEN)) == NULL)
			return (-1);
		cmptr->cmsg_level = SOL_SOCKET;
		cmptr->cmsg_type = SCM_RIGHTS;
		cmptr->cmsg_len = CONTROLLEN;
		msg.msg_control = cmptr;
		msg.msg_controllen = CONTROLLEN;
		*(int *)CMSG_DATA(cmptr) = fd_to_send;	/* the fd to pass */
		buf[1] = 0;	/* zero status means OK */
	}
	buf[0] = 0;		/* null byte flag to recv_fd() */
	if (sendmsg(fd, &msg, 0) != 2)
		return (-1);
	return (0);
}

/*
* Used when we had planned to send an fd using send_fd(),
* but encountered an error instead. We send the error back
* using the send_fd()/recv_fd() protocol.
*/
int send_err(int fd, int errcode, const char *msg)
{
	int n;
	if ((n = strlen(msg)) > 0)
		if (writen(fd, msg, n) != n)	/* send the error message */
			return (-1);
	if (errcode >= 0)
		errcode = -1;	/* must be negative */
	if (send_fd(fd, errcode) < 0)
		return (-1);
	return (0);
}

#include <fcntl.h>
void handle_request(char *buf, int nread, int fd)
{
	int newfd;
	printf("%s\n", buf);
	if (buf[nread - 1] != 0) {
		snprintf(errmsg, MAXLINE - 1,
			 "request not null terminated: %*.*s\n", nread, nread,
			 buf);
		send_err(fd, -1, errmsg);
		return;
	}
	if (buf_args(buf, cli_args) < 0) {	/* parse args & set options */
		send_err(fd, -1, errmsg);
		return;
	}
	if ((newfd = open(pathname, oflag)) < 0) {
		snprintf(errmsg, MAXLINE - 1, "can’t open %s: %s\n", pathname,
			 strerror(errno));
		send_err(fd, -1, errmsg);
		return;
	}
	if (send_fd(fd, newfd) < 0)	/* send the descriptor */
		printf("send_fd error");
	close(newfd);		/* we’re done with descriptor */
}

char errmsg[MAXLINE];
int oflag;
char *pathname;
int main(void)
{
	int nread;
	char buf[MAXLINE];
	for (;;) {		/* read arg buffer from client, process request */
		if ((nread = read(STDIN_FILENO, buf, MAXLINE)) < 0)
			printf("read error on stream pipe");
		else if (nread == 0)
			break;	/* client has closed the stream pipe */
		handle_request(buf, nread, STDOUT_FILENO);
	}
	exit(0);
}
