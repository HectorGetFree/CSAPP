#include <stdio.h>
#include "csapp.h" 
#include "cache.h" // 引入缓存模块
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

/* 自定义string类型，便于字符串构造 */
typedef char string[MAXLINE];
typedef struct {
    string host;
    string port;
    string path;
} url_t;

/* 自定义函数签名 */
void* thread(void* vargp);
void do_get(rio_t* client_rio_p, string url);
int parse_url(string url, url_t* url_info);
int parse_header(rio_t* client_rio_p, string header_info, string host);

/* 
 * main函数
 * 创建监听套接字，循环接受请求，创建线程处理请求
*/
int main(int argc, char **argv)
{
    // 忽略SIGPIPE信号
    // 原因是这个信号默认会终止这个进程，而在一些特定情况下可能会引发该信号
    // 处理该信号是比较复杂的，所以这里先直接忽略，详见课本p677的旁注
    signal(SIGPIPE, SIG_IGN);

    // 剩下的代码部分基本仿照课本p695,另外有一些修改
    int listenfd, * connfd;
    // char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    // 检查命令行参数
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // 创建套接字，使用csapp.h中的包装函数，
    listenfd = Open_listenfd(argv[1]);

    // 初始化缓存
    init_cache();
    while(1) {
        clientlen = sizeof(clientaddr);
        // 每次循环使用malloc从而实现基于线程的并发服务器
        // 不使用局部变量，因为局部变量会导致线程间共享同一块内存，从而导致竞争
        connfd = (int *)malloc(sizeof(int));
        *connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
        if (*connfd < 0) {
            fprintf(stderr, "Accept Error: %s\n", strerror(errno));
            continue;
        }

        // 创建线程处理请求
        pthread_create(&tid, NULL, thread, connfd);
    }
    close(listenfd);
    return 0;
}

/*
 * 使用线程处理请求，实现并发
 * @param vargp 指向客户套接字描述符的指针
*/
void *thread(void* vargp) {
    // 分离自身线程
    pthread_detach(pthread_self());

    // 将局部变量复制存储在线程栈，释放动态分配的参数，防止内存泄露
    int client_fd = *((int *)vargp);
    free(vargp);

    // 处理请求
    // 用rio_readinitb()初始化客户端缓冲区
    rio_t client_rio;
    string buf;
    rio_readinitb(&client_rio, client_fd);

    // 读取客户端内容到buf
    // rio_readlineb()从缓冲区中读取一行数据
    // 处理遇到EOF或者读取出错的情况
    if (rio_readlineb(&client_rio, buf, MAXLINE) <= 0) {
        fprintf(stderr, "Read request line error: %s\n", strerror(errno));
        close(client_fd);
        return NULL;
    }

    // 参考请求行格式
    string method, url, http_version;
    // 解析
    if (sscanf(buf, "%s %s %s", method, url, http_version) != 3) {
        fprintf(stderr, "Parse request line error: %s\n", strerror(errno));
        close(client_fd);
        return NULL;
    }

    // 检查是否为GET方法
    // strcasecmp()不区分大小写地比较两个字符串
    if (!strcasecmp(method, "GET")) {
        do_get(&client_rio, url);
    }

    // 关闭连接
    close(client_fd);
    return NULL;
}

/*
 * 解析url
 * @param url 请求的url
 * @param ulr_info 解析结果的存储位置
*/
int parse_url(string url, url_t* url_info) {
    // 检查是否为http协议
    const int http_prefix_len = strlen("http://");
    if (strncasecmp(url, "http://", http_prefix_len)) {
        fprintf(stderr, "Not http protocol: %s\n", url);
        return -1;
    }
    // 检查是否为合法的url http://<host>:<port>/<path>
    // 这里用的是指针运算先得到各个组成部分
    char* host_start = url + http_prefix_len;
    // 从字符串 host_start 中查找第一个字符 ':' 出现的位置
    // 返回位置指针
    char* port_start = strchr(host_start, ':');
    char* path_start = strchr(host_start, '/');

    // 非法url
    if (path_start == NULL) {
        return -1;
    }

    // 没有端口号，默认设置为80
    if (port_start == NULL) {
        // 先截断，便于分割出port部分
        *path_start = '\0';
        strcpy(url_info->host, host_start);
        strcpy(url_info->port, "80");
        // 恢复
        *path_start = '/';
        strcpy(url_info->path, path_start);
    }
    else {
        // 有端口号
        *port_start = '\0';
        strcpy(url_info->host, host_start);
        *port_start = ':';
        *path_start = '\0';
        strcpy(url_info->port, port_start + 1);
        *path_start = '/';
        strcpy(url_info->path, path_start);
    }
    return 0;
}

/*
 * 解析请求头
 * 基本结构是：
 * Host: hostname
 * User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3
 * Connection: close
 * Proxy-Connection: close
 * @param clinet_rio_p 指向客户端rio的指针
 * @param header_info 解析结果的存储位置
 * @param host 先前解析出的请求的host，作为Host头的默认值
*/
int parse_header(rio_t* client_rio_p, string header_info, string host) {
    string buf;
    int has_host_flag = 0;
    while (1) {
        rio_readlineb(client_rio_p, buf, MAXLINE);
        // 遇到结束行
        if (strcmp(buf, "\r\n") == 0) {
            break;
        }
        // 如果遇到Host头就记录下来，后续不再添加
        if (!strncasecmp(buf, "Host:", strlen("Host:"))) {
            has_host_flag = 1;
        }
        // 如果遇到Connection 头， Proxy-Connection头，User-Agent 头，直接跳过，后续替换为默认值
        if (!strncasecmp(buf, "Connection:", strlen("Connection:"))) {
            continue;
        }
        if (!strncasecmp(buf, "Proxy-Connection:", strlen("Proxy-Connection:"))) {
            continue;
        }
        if (!strncasecmp(buf, "User-Agent:", strlen("User-Agent:"))) {
            continue;
        }
        // 其他头和Host头直接添加
        strcat(header_info, buf);
    }
    // 如果没有Host头，添加Host头
    if (!has_host_flag) {
        sprintf(buf, "Host: %s\r\n", host);
        strcpy(header_info, buf);
    }
    // 添加Connection头，Proxy-Connection头，User-Agent头
    strcat(header_info, "Connection: close\r\n");
    strcat(header_info, "Proxy-Connection: close\r\n");
    strcat(header_info, user_agent_hdr);
    // 添加结束行
    strcat(header_info, "\r\n");
    return 0;
}

/*
 * 处理get请求
 * @param client_rio_p 指向客户端rio的指针
 * @param url 请求的url
*/
void do_get(rio_t* client_rio_p, string url) {
    // 检查是否在缓存中，如果命中缓存，直接返回
    if (query_cache(client_rio_p, url)) {
        return;
    }
    // 解析url
    url_t url_info;
    if (parse_url(url, &url_info) < 0) {
        fprintf(stderr, "Parse url error\n");
        return;
    }

    // 解析header
    string header_info;
    parse_header(client_rio_p, header_info, url_info.host);

    // 启动与host的连接
    int server_fd = open_clientfd(url_info.host, url_info.port);
    if (server_fd < 0) {
        fprintf(stderr, "Open connect to %s:%s error\n", url_info.host, url_info.port);
        return;
    }
    // 初始化服务端缓冲区rio
    rio_t server_rio;
    rio_readinitb(&server_rio, server_fd);

    // 准备请求行和请求头
    string buf;
    sprintf(buf, "GET %s HTTP/1.0\r\n%s", url_info.path, header_info);

    // 发送请求行和请求头
    if (rio_writen(server_fd, buf, strlen(buf)) != strlen(buf)) {
        fprintf(stderr, "Send request line and header error\n");
        close(server_fd);
        return;
    }

    // 接受相应行
    int resp_total = 0, resp_current = 0;
    char file_cache[MAX_OBJECT_SIZE];
    int client_fd = client_rio_p->rio_fd;

    // 从服务端读取相应
    // server可能会写多次，所以需要循环读取直到遇到EOF（即 resp_current == 0）
    while ((resp_current = rio_readnb(&server_rio, buf, MAXLINE))) {
        if (resp_current < 0) {
            fprintf(stderr, "Read server response error\n");
            close(server_fd);
            return;
        }
        // 缓存到局部变量 file_cache 中， 供缓存使用
        if (resp_total + resp_current < MAX_OBJECT_SIZE) {
            memcpy(file_cache + resp_total, buf, resp_current);
        }
        resp_total += resp_current;
        // 发送给客户端
        if (rio_writen(client_fd, buf, resp_current) != resp_current) {
            fprintf(stderr, "Send response to client error\n");
            close(server_fd);
            return;
        }
    }
    // 如果响应小于 MAX_OBJECT_SIZE，缓存到本地
    if (resp_total < MAX_OBJECT_SIZE) {
        add_cache(url, file_cache, resp_total);
    }
    close(server_fd);
    return;
}