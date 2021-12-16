/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the 
 * GET method to serve static and dynamic content.
 */

#include "csapp.h"

void doit(int fd);      /* hanlde the request */
void read_requesthdrs(rio_t *rp);
int parse_uri(char* uri, char* filename, char* cgiargs);
void serve_static(int fd, char* filename, int filesize);
void serve_dynamic(int fd, char* filename, char* cgiargs);
void clienterror(int fd, char* cause, char* errnum, char* shortmsg, char* longmsg);
void get_filetype(char* filename, char* filetype);
int open_listenfd(int port);

int main(int argc, char** argv)
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port> \n", argv[0]);
        exit(1);
    }

    port = atoi(argv[1]);       /* get the port */
    
    listenfd = open_listenfd(port);
    while (1) {
        clientlen = sizeof(clientaddr);
        /* 当连接建立完成时，客户端的发送数据已经保存在了fd的发送缓存之中*/
        connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
        doit(connfd);
        close(connfd);
    }
}

/* 处理浏览器事务 */
void doit(int fd)
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    /* Read request line and headers */
    rio_readinitb(&rio, fd);
    rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET")) {
        clienterror(fd, method, "501", "Not Implemented", "Tiny does not implement this method");
        return ;
    }
    read_requesthdrs(&rio);

    /* Parse URI from GET request */

    is_static = parse_uri(uri, filename, cgiargs);
    if (stat(filename, &sbuf) < 0) {    /* stat 通过filename文件名获取文件信息 */
        clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
        return ;
    }
    if (is_static) {    /* Server static content */
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
            return ;
        }
        serve_static(fd, filename, sbuf.st_size);/* 文件大小 */
    } else {
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
        }
        serve_dynamic(fd, filename, cgiargs);
    }
}

void clienterror(int fd, char* cause, char* errnum, char* shortmsg, char* longmsg)
{
    char buf[MAXLINE], body[MAXBUF];
    
    /* Build the HTTP response body */
    sprintf(body, "<html><title> Tiny Error </title>");
    sprintf(body, "%s<body bgcolor = ""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg,cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /*Print the HTTP response*/
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type:text/html\r\n");
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", strlen(body));
    rio_writen(fd, buf, strlen(buf));
    rio_writen(fd, buf, strlen(body));
}


void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];
    rio_readlineb(rp, buf, MAXLINE);
    while (strcmp(buf, "\r\n"))
        rio_readlineb(rp, buf, MAXLINE);
    return;
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;
    if (!strstr(uri, "cgi-bin")) {  /* Static content */
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);

        if (uri[strlen(uri) - 1] == '/')
            strcat(filename, "/html/index.html");
        return 1;
    }
    else {  /* Dynamic content */
        ptr = index(uri, '?');
        if (ptr) {
            strcpy(cgiargs, ptr+1);
            *ptr = '\0';
        } else 
            strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        return 0;
    }
}
void serve_static(int fd, char* filename, int filesize)
{
    int srcfd;
    char* srcp, filetype[MAXLINE], buf[MAXBUF];
    /* Send response headers to client */
    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    rio_writen(fd, buf, strlen(buf));

    /* Send response body to client */
    srcfd = open(filename, O_RDONLY, 0);
    srcp = mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    close(srcfd);
    rio_writen(fd, srcp, filesize);
    munmap(srcp, filesize);
}

void get_filetype(char* filename, char* filetype)
{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, ".jpg");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else
        strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char* filename, char* cgiargs)
{
    char buf[MAXLINE], *emptylist[] = { NULL };
    /* Return first part to HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    rio_writen(fd, buf, strlen(buf));
    if (fork() == 0) {  /* child */
        /* Real server would set all CGI vars here */
        setenv("QUERY_STRING", cgiargs, 1);
        dup2(fd, STDOUT_FILENO);
        execve(filename, emptylist, __environ);
    }
    wait(NULL);     /* parent waits for and reaps child */
}

int open_listenfd(int port)
{
    int listenfd, optval = 1;
    struct sockaddr_in serveraddr;

    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
        return -1;

    /* Eliminates "Address already in use" errno from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int)) < 0)
        return -1;
    
    /* Listenfd will be an endpoint for all requests to port on any IP address for this host */
    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);

    if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;
    
    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, MAX_LISTEN) < 0)
        return -1;
    return listenfd;
}