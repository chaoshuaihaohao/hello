#include<stdio.h>
#include<stdlib.h>
#include<sys/epoll.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
 
#define MAXLEN 1024
 
int main(int argc,char *argv[])
{
	if(3 != argc)
	{
		printf("param error\n");
		exit(EXIT_FAILURE);
	}
	
	int max_open_fd = atoi(argv[2]);
	
    int connfd = 0;
    char buffer[MAXLEN]={0};
    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);
 
    int listenfd = socket(AF_INET,SOCK_STREAM,0);
 
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));
    
	bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr));
    
	listen(listenfd,20);
	
    int epollfd = epoll_create(max_open_fd);
	
    struct epoll_event  stevent, events[max_open_fd];
	stevent.events = EPOLLIN;
	stevent.data.fd = listenfd;
 
    int ret = epoll_ctl(epollfd,EPOLL_CTL_ADD,listenfd,&stevent);
 
    for (;;)
    {
        size_t nready = epoll_wait(epollfd,events,max_open_fd,-1);
		if (-1 == nready) 
		{
			perror("epoll_pwait error ");
			exit(EXIT_FAILURE);
		}
		
        for (int i = 0; i < nready; ++i)
        {
            if (events[i].data.fd == listenfd )
            {
                connfd = accept(listenfd,(struct sockaddr*)&cliaddr,&clilen);
				if (-1 == connfd) 
				{
				   perror("accept error");
				   exit(EXIT_FAILURE);
			    }
				
                stevent.data.fd = connfd;				
                stevent.events = EPOLLIN | EPOLLET;
                epoll_ctl(epollfd,EPOLL_CTL_ADD,connfd,&stevent);
            }
			else if(events[i].events & EPOLLIN)
			{
                int cli_fd = events[i].data.fd;
                int nread = read(cli_fd, buffer, sizeof(buffer));
				if (nread ==0)
				{
					epoll_ctl(epollfd,EPOLL_CTL_DEL,cli_fd,NULL);
					close(cli_fd);
				}
				else if (nread <0)
				{
					perror("accept error");
					return -1;
				}
				
				stevent.data.fd = cli_fd;
                stevent.events = EPOLLOUT | EPOLLET;
                epoll_ctl(epollfd, EPOLL_CTL_MOD, cli_fd, &stevent);
			}
			else if(events[i].events & EPOLLOUT)
			{
                int cli_fd = events[i].data.fd;
                write(cli_fd, buffer, sizeof(buffer));
				
				stevent.data.fd = cli_fd;
                stevent.events = EPOLLIN | EPOLLET;
                epoll_ctl(epollfd, EPOLL_CTL_MOD, cli_fd, &stevent);	
			}
            else 
            {
 
            }
        }
    }
	
    return 0;
}
