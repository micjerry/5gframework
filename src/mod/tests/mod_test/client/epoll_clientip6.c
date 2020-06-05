#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
//#include <linux/if_link.h>

#define MAXSIZE     1024
#define IPADDRESS   "::1"
#define SERV_PORT   9000
#define FDSIZE        1024
#define EPOLLEVENTS 20

static void handle_connection(int sockfd);
static void 
handle_events(int epollfd, struct epoll_event *events, int num, int sockfd, char *buf);

static void do_read(int epollfd, int fd, int sockfd, char *buf);
static void do_write(int epollfd, int fd, int sockfd, char *buf);

static void add_event(int epollfd, int fd, int state);
static void delete_event(int epollfd, int fd, int state);
static void modify_event(int epollfd, int fd, int state);

int main(int argc, char *argv[])
{
	int sockfd;
	struct sockaddr_in6 servaddr;
    struct ifaddrs *addrs = NULL;

    if (getifaddrs(&addrs) == 0) {
        struct ifaddrs *iter = addrs;
        while(iter) {
            printf("eth: %s  %s %d\n", iter->ifa_name, (iter->ifa_addr->sa_family == AF_INET) ? "IPV4" : "IPV6", if_nametoindex(iter->ifa_name));
            iter = iter->ifa_next;
        }
        freeifaddrs(addrs);
    } else {
        printf("no addrs\n");
    }
	
	sockfd = socket(AF_INET6, SOCK_STREAM, 0);
	
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin6_family = AF_INET6;
	servaddr.sin6_port = htons(SERV_PORT);
	inet_pton(AF_INET6,IPADDRESS,&servaddr.sin6_addr);
	
	connect(sockfd,(struct sockaddr*)&servaddr, sizeof(servaddr));
	handle_connection(sockfd);
	close(sockfd);
	return 0;
}

static void handle_connection(int sockfd)
{
	int epollfd;
	struct epoll_event events[EPOLLEVENTS];
	char buf[MAXSIZE];
	int ret;
	
	//memset(buf, 'a', MAXSIZE);
	epollfd = epoll_create(FDSIZE);
	//add_event(epollfd, sockfd, EPOLLOUT);
	add_event(epollfd, STDIN_FILENO, EPOLLIN);
	for (;;) {
		ret = epoll_wait(epollfd, events, EPOLLEVENTS, -1);
		handle_events(epollfd, events, ret, sockfd, buf);
	}
	close(epollfd);
}

static void 
handle_events(int epollfd, struct epoll_event *events, int num, int sockfd, char *buf)
{
	int fd;
	int i;
	for (i = 0; i < num; i++) {
		fd = events[i].data.fd;
		if (events[i].events & EPOLLIN) {
			do_read(epollfd, fd, sockfd, buf);
		} else {
			do_write(epollfd, fd, sockfd, buf);
		}
	}
}

static void do_read(int epollfd, int fd, int sockfd, char *buf)
{
	int nread;
	nread = read(fd, buf, MAXSIZE);
	
	if (nread == -1) {
		perror("read error:");
		close(fd);
	} else if (nread == 0) {
		fprintf(stderr,"server close.\n");
		close(fd);
	} else {
		printf("read fd=%d,%s\n",fd, buf);
		if (fd == STDIN_FILENO) {
			add_event(epollfd, sockfd, EPOLLOUT);
		} else {
			delete_event(epollfd, sockfd,EPOLLIN);
			add_event(epollfd,STDOUT_FILENO,EPOLLOUT);
		}
	}
}

static void do_write(int epollfd,int fd,int sockfd,char *buf)
{
	int nwrite;
    nwrite = write(fd,buf,strlen(buf));
	
	if (nwrite == -1)
    {
        perror("write error:");
        close(fd);
    } else {
		printf("write fd=%d,%s\n",fd, buf);
		//modify_event(epollfd,fd,EPOLLIN);
		if (fd == STDOUT_FILENO) {
			delete_event(epollfd,fd,EPOLLOUT);
		} else {
			modify_event(epollfd,fd,EPOLLIN);
		}
	}
	
	memset(buf,0,MAXSIZE);
}

static void add_event(int epollfd, int fd, int state)
{
	struct epoll_event ev;
	ev.events = state;
	ev.data.fd = fd;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
}

static void delete_event(int epollfd, int fd, int state)
{
	struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
	
	epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,&ev);
}

static void modify_event(int epollfd, int fd, int state)
{
	struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
	
	epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}
