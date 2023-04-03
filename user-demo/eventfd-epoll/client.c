#include<stdio.h>
#include<stdlib.h>
#include <stdbool.h>
#include <string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include <unistd.h>
#include <time.h>
 
int num =0;
 
int main(int argc, char* argv[]){
        int sock;
        int i = 0;
		
		if(argc != 2)
		{
			printf("param error\n");
		}
		
		time_t t1 = time(NULL);
			
        while(1)
		{	
			sock = socket(AF_INET,SOCK_STREAM,0);
			if(-1 == sock)
			{
				perror("socket create error");
				break;
			}
 
			struct sockaddr_in servaddr;
			memset(&servaddr,0,sizeof(servaddr));
			servaddr.sin_family = AF_INET;
			servaddr.sin_port = htons(atoi(argv[2]));
			servaddr.sin_addr.s_addr = inet_addr(argv[1]);
 
			if(connect(sock,(struct sockaddr*)&servaddr,sizeof(servaddr))<0)
			{
				perror("connect error");
				break;
			}
			struct sockaddr_in cliaddr;
			socklen_t addrlen;
			getsockname(sock,(struct sockaddr*)&cliaddr,&addrlen);
			printf("ip = %s ,port = %d\n",inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port));
			
			++num;
		}
		
		time_t t2 = time(NULL);
 
		printf("num=%d, time=%ld\n, su=%f",++num, t2-t1, (float)num/t2-t1);
 
	close(sock);
	
	while(true)
		sleep(5);
	
	return 0;
}
