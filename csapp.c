#include "csapp.h"


/* Robust I/O */
ssize_t rio_readn(int fd, void* usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char* bufp = usrbuf;

    while (nleft > 0) {
        if ( (nread = read(fd, bufp, nleft)) < 0) {
            if (errno == EINTR) /* interrupted by sig handler return */
                nread = 0;
            else 
                return -1;      /* errno set by read() */
        } else if (nread == 0) 
            break;      /* EOF */
        nleft = nleft - nread;
        bufp = bufp + nread;
    }
    return n - nleft;   /* 返回成功读入的数量 */
}

ssize_t rio_writen(int fd, void* usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten;
    char* bufp = usrbuf;

    while (nleft > 0) {
        if ((nwritten = write(fd, bufp, nleft)) <= 0) {
            if (errno == EINTR)     /* interrupted by sig handler return */
                nwritten = 0;       /* and call write() again */
            else
                return -1;          /* errno set by write() */
        }
        nleft = nleft - nwritten;
        bufp = bufp + nwritten;
    }
    
    return n;   /* 返回成功写入的字节数 */
}

void rio_readinitb(rio_t *rp, int fd)
{
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

/* rio_read函数, 若rio_t的缓冲区为空，则从fp中读取BUFSIZE填充缓冲区. 若不为空, 则读取min(n, rio_cnt)大小的数据. */
/* static 静态成员函数，为csapp.c拥有，不需要声明*/
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    int cnt;
    /* 当缓冲区为空时，首先从fd中读取BUFSIZE大小至缓冲区进行初始化 */
    while (rp->rio_cnt <= 0) {
        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));
        if (rp->rio_cnt < 0) {
            if (errno != EINTR) /* interrupted by sig handler return */
                return -1;
        } else if (rp->rio_cnt == 0)    /* EOF */
            return 0;
        else
            rp->rio_bufptr = rp->rio_buf;  /* reset buffer ptr */
    }
    cnt = n;
    if (rp->rio_cnt < n)
        cnt = rp->rio_cnt;
    
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;  /* 更新unread的位置和大小 */
    rp->rio_cnt -= cnt;
    return cnt;     /* 成功拷贝的字节个数  */
}

/* rio_readlineb ：读取一行或者读取最大maxlen - 1的字节数 */

ssize_t rio_readlineb(rio_t *rp, void* usrbuf, size_t maxlen)
{
    int n, rc;
    char c, *bufp = usrbuf;
    for (n = 1; n < maxlen; n++) {
        if ((rc = rio_read(rp, &c, 1)) == 1) {
            *bufp++ = c;
            if (c == '\n')
                break;
        } else if (rc == 0) {
            if (n == 1)
                return 0;   /* EOF, no data read */
            else 
                break;      /* EOF, some data was read */
        } else 
                return -1;
    }
    *bufp = 0;
    return n; /* 返回成功读取的个数 */
}

ssize_t rio_readnb(rio_t *rp, void* usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char* bufp = usrbuf;
    while (nleft > 0) {
        if ((nread = rio_read(rp, bufp, nleft)) < 0) {
            if (errno == EINTR) /* interrupted by sig handler return */
                nread = 0;
            else
                return -1;      /* errno set by read() */
        } else if (nread == 0)
            break;              /* EOF */
        nleft -= nread;
        bufp += nread;
    }
    return n - nleft;   /* 成功读取的字节个数 */
}


