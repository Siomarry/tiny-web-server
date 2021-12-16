#ifndef __CSAPP_H__
#define __CSAPP_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#define RIO_BUFSIZE 8192
#define MAXLINE 8192
#define MAXBUF  8192
#define MAX_LISTEN 128
typedef struct {
    int rio_fd;     /* Descriptor for this internal buf */
    int rio_cnt;    /* unread bytes in internal buf */
    char *rio_bufptr;   /* Next unread byte in internal buf */
    char rio_buf[RIO_BUFSIZE];  /* Internal buffer */
} rio_t;

/* read without the buffer */
ssize_t rio_readn(int fd, void* usrbuf, size_t n);
ssize_t rio_writen(int fd, void* usrbuf, size_t n);

/* read with the buffer */
void rio_readinitb(rio_t* rp, int fd);
ssize_t rio_readlineb(rio_t* rp, void* usrbuf, size_t maxlen);
ssize_t rio_readnb(rio_t* rp, void* usrbuf, size_t n);
