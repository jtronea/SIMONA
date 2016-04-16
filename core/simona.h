#ifndef _SIMONA_H_INCLUDED_
#define _SIMONA_H_INCLUDED_

#include	"sm_base.h"

#define MAXEVENTS 64  
#define	SA	struct sockaddr
#define MAXLEN 1024
#define	LISTENQ		1024
#define MAXCONNECTION 10000//最大连接数
#define MEMPOOLSIZE 4096//内存池中单个内存的大小
#define WORKERTHREADS 10//工作线程数
int make_socket_non_blocking(int sfd);

int create_and_bind(const char * port);

int add_read_event(int fd);

int add_write_event(int fd);

int init_event(int fd);

#endif