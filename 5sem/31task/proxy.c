#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include "http_parser.h"
#include <string.h>

#define TIMEOUT 10000
#define PORT 9000
#define MAX_CLIENTS_NUM 100
#define BLOCK_SIZE 4096
#define MAX_CACHE_NUM 100
#define MAX_HEADERS 100

//TODO проверять cache_size, выслать клиенту результат
//TODO проверять конец запроса от клиента через \0 или eof
//TODO проверять количество ссылок на кэш при скачке страницы, если 0 ссылок можно перезаписать в него
//TODO проверять write обязательно!!!

typedef struct CacheRecord{
    char *path;
    char *data;
    bool is_upload_completed;
    bool is_response_analyzed;
    int links_num;
}CacheRecord;

typedef struct Cache{
    CacheRecord *cache_records;
    int cache_size;
}Cache;

typedef struct Proxy{
    struct sockaddr_in serv_addr;
    int sock_fd;

    // 0 - фд от сокета, принимающего соединенния
    // 1..MAX_CLIENTS_NUM - фд от клиентов,
    // MAX_CLIENTS_NUM+1...2*MAX_CLIENTS_NUM - фд от серверов,
    struct pollfd fds[2*MAX_CLIENTS_NUM + 1];

    Cache *cache;
    char *buf_arr[MAX_CLIENTS_NUM];
    http_request_t requests[MAX_CLIENTS_NUM];
}Proxy;

Proxy proxy;

int is_cached(char *url);
int send_error_to_client(int fd, const char* msg);
void init_proxy();
void create_new_connection();
void delete_connection(int i);
int connect_to_host(http_request_t request);
int handle_client_request(int i);
int handle_server_response(int j);
int send_data(int i);
int find_free_fd();
int find_cache(char *url);

int main(int argc, char **argv){
    int status, code;

    init_proxy();

    if(listen(proxy.sock_fd, MAX_CLIENTS_NUM) == -1){
        perror("Error: listen() call failed");
        exit(EXIT_FAILURE);
    }

    while(1) {
        code = poll(proxy.fds, 2*MAX_CLIENTS_NUM+1, TIMEOUT);
        if (code < 0) {
            perror("Error poll() call failed");
            exit(EXIT_FAILURE);
        }
        if (code == 0) {
            printf("Timeout\n");
            continue;
        }
        if(proxy.fds[0].revents == POLLIN){
            proxy.fds[0].revents = 0;
            create_new_connection();
        }
        for (int i = 1; i < MAX_CLIENTS_NUM + 1; i++) { //клиенты
            if (proxy.fds[i].revents == POLLIN) {
                proxy.fds[i].revents = 0;
                status = handle_client_request(i);
                if (status < 0) {
                    proxy.fds[i].events = POLLHUP;
                }
                else if (status == 1) { //клиент продолжает писать байты
                    continue;
                }
                else if(status ==2){
                    continue;
                }
                else if (status == 0) {
                    proxy.fds[i].events = POLLOUT; //??
                }
            }
            if (proxy.fds[i].revents == POLLOUT) { //пишем клиенту
                proxy.fds[i].revents = 0;
                if (send_data(i) == -1) {
                    perror("Error: sending page to client failed");
                    proxy.fds[i].events = POLLHUP;
                }
            }
            if (proxy.fds[i].revents == POLLHUP) {
                if (proxy.fds[i+MAX_CLIENTS_NUM].fd != -1) {
                    delete_connection(i+MAX_CLIENTS_NUM);
                }
                delete_connection(i);
            }
        }

        for (int j = MAX_CLIENTS_NUM+1; j < 2*MAX_CLIENTS_NUM + 1; j++) { //сервера
            if (proxy.fds[j].revents == POLLIN) { //сервер ответил нам, можно качать в кэш, писать клиенту и т.д.
                proxy.fds[j].revents = 0;
                status = handle_server_response(j);
                if (status < 0) {
                    send_error_to_client(j - MAX_CLIENTS_NUM, "Error: uploading page from server failed");
                    proxy.fds[j].events = POLLHUP;
                    continue;
                }
                else if (status == 1) {
                    continue;
                }
                else { //закачка страницы с сервера прошла успешно
                    proxy.fds[j - MAX_CLIENTS_NUM].events = POLLOUT;
                }
            }
            else if (proxy.fds[j].revents == POLLOUT) { // надо попросить страницу у сервера
                proxy.fds[j].revents = 0;
                if (send_data(j) == -1) { //поменять метод
                    perror("Error: sending page to client failed");
                    proxy.fds[j].events = POLLHUP;
                }
            }
            if (proxy.fds[j].revents == POLLHUP) {
                if (proxy.fds[j-MAX_CLIENTS_NUM].fd != -1) {
                    delete_connection(j-MAX_CLIENTS_NUM);
                }
                delete_connection(j);
            }
        }
    }

    return 0;
}


void init_proxy(){
    proxy.sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(proxy.sock_fd == -1){
        perror("Error: socket wasn't created");
        exit(EXIT_FAILURE);
    }

    memset(&proxy.serv_addr, 0, sizeof(proxy.serv_addr));
    proxy.serv_addr.sin_family = AF_INET;
    proxy.serv_addr.sin_port = htons((u_short)PORT);
    proxy.serv_addr.sin_addr.s_addr = INADDR_ANY;

    memset(proxy.fds, 0, (2*MAX_CLIENTS_NUM + 1) * sizeof(struct pollfd));
    proxy.cache = (Cache *) malloc(sizeof(Cache));
    proxy.cache->cache_size = 0;

    for(int i=1; i<2*MAX_CLIENTS_NUM+1; i++){
        proxy.fds[i].fd = -1;
    }
    proxy.fds[0].fd = proxy.sock_fd;
    proxy.fds[0].events = POLLIN;

    if(bind(proxy.sock_fd, (struct sockaddr *)&proxy.serv_addr, sizeof(proxy.serv_addr)) == -1){
        perror("Error: socket wasn't binded");
        exit(EXIT_FAILURE);
    }

    printf("Proxy is ready: %s:%d\n", inet_ntoa(proxy.serv_addr.sin_addr), PORT);
}

void create_new_connection(){
    struct sockaddr_in client_candidate;
    int size, new_sock_fd, free_fd;
    size = sizeof(client_candidate);

    new_sock_fd = accept(proxy.fds[0].fd, (struct sockaddr *)&client_candidate, &size);
    if(new_sock_fd < 0){
        perror("Error: accept() call failed");
        exit(EXIT_FAILURE);
    }

    printf("Request for new connection from %s:%d\n", inet_ntoa(client_candidate.sin_addr), ntohs(client_candidate.sin_port));

    free_fd = find_free_fd();
    if(free_fd < 0){
        perror("Maximum number of connections reached. There's no sockets for clients\n");
        close(new_sock_fd);
        return;
    }

    proxy.fds[free_fd].fd = new_sock_fd;
    proxy.fds[free_fd].events = POLLIN;
    //proxy.clnt_addrs[free_fd] = client_candidate;

    printf("New client %s:%d connected\n", inet_ntoa(client_candidate.sin_addr), ntohs(client_candidate.sin_port));
}

int connect_to_host(http_request_t request){
    struct addrinfo web_server;
    struct addrinfo *result, *rp;
    int s, sfd;

    memset(&web_server, 0, sizeof(web_server));
    web_server.ai_family = AF_INET;
    web_server.ai_socktype = SOCK_STREAM;
    web_server.ai_protocol = 0;

    printf("request path %s\n", request.path);
    s = getaddrinfo(request.path, NULL, &web_server, &result);
    if(s != 0){
        printf("Error: connection to web server failed with error code = %d\n", s);
        return -1;
    }

    for(rp = result; rp != NULL; rp = rp->ai_next){
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(sfd == -1){
            continue;
        }
        if(bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;
    }

    freeaddrinfo(result);
    freeaddrinfo(rp);
    return sfd;
}


int handle_client_request(int i){
    int offset, bytes_read;
    if(proxy.buf_arr[i]){
        offset = strlen(proxy.buf_arr[i]);
    } else{
        offset = 0;
    }
    proxy.buf_arr[i] = (char*)realloc(proxy.buf_arr[i], (offset + BLOCK_SIZE) * sizeof(char));
    bytes_read = read(proxy.fds[i].fd, proxy.buf_arr[i] + offset, BLOCK_SIZE);

    if(bytes_read < 0 || bytes_read < BLOCK_SIZE && !strstr(proxy.buf_arr[i], "\0")){
        send_error_to_client(proxy.fds[i].fd, "Error: proxy failed while calling read()");
        return -1;
    }
    if(!strstr(proxy.buf_arr[i], "\0")){ //клиент пишет байты
        proxy.fds[i].events = POLLIN;
        return 1;
    }

    httpParseRequest(proxy.buf_arr[i], &proxy.requests[i]);
    printf("http request url: %s\n", proxy.requests[i].path);
    if(strcmp(proxy.requests[i].http_method, "GET") != 0){
        perror("invalid http method from client");
        send_error_to_client(proxy.fds[i].fd, "Error: invalid method in header. Only \'GET\' is available");
        return -1;
    }

    int index = is_cached(proxy.requests[i].path);
    if(index == -1){
        proxy.fds[i].events = 0;

        int web_sock_fd = connect_to_host(proxy.requests[i]);
        if(web_sock_fd < 0){
            send_error_to_client(proxy.fds[i].fd, "Error: connection to web server failed");
            return -1;
        }
        else{
            proxy.fds[MAX_CLIENTS_NUM+i].fd = web_sock_fd;
            proxy.fds[MAX_CLIENTS_NUM+i].events = POLLOUT;
        }
    }

    else if(proxy.cache->cache_records[index].is_upload_completed){
        proxy.fds[i].events = POLLOUT;
        proxy.buf_arr[i] = (char*)realloc(proxy.buf_arr[i], strlen(proxy.cache->cache_records[index].data));
        strcpy(proxy.buf_arr[i], proxy.cache->cache_records[index].data);
    }
    else{ //страница не полностью закачана
/*
        if(proxy.buf_arr[i]){
            offset = strlen(proxy.buf_arr[i]);
        } else{
            offset = 0;
        }
*/
    return 2;
    }

    return 0;
}

int handle_server_response(int j){
    int offset, bytes_read;
    int cache_num = find_cache(proxy.requests[j-MAX_CLIENTS_NUM].path);
    http_response_t response;

    if(cache_num == -1){
        //error
    }

    if(proxy.cache->cache_records[cache_num].data){
        offset = strlen(proxy.cache->cache_records[cache_num].data);
    } else{
        offset = 0;
    }
    proxy.cache->cache_records[cache_num].data = (char*)realloc(proxy.cache->cache_records[cache_num].data, (offset + BLOCK_SIZE) * sizeof(char));
    proxy.cache->cache_records[cache_num].is_upload_completed = false;
    proxy.cache->cache_records[cache_num].is_response_analyzed = false;

    bytes_read = read(proxy.fds[j].fd, proxy.cache->cache_records[cache_num].data + offset, BLOCK_SIZE);

    if(bytes_read < 0 || bytes_read < BLOCK_SIZE && !strstr(proxy.cache->cache_records[cache_num].data, "\0")){
        send_error_to_client(proxy.fds[j].fd, "Error: server failed while calling read()");
        return -1;
    }

    if(strstr(proxy.cache->cache_records[cache_num].data, "\r\n") &&
        !proxy.cache->cache_records[cache_num].is_response_analyzed)
    {
        httpParseResponse(proxy.cache->cache_records[cache_num].data, &response);
        proxy.cache->cache_records[cache_num].is_response_analyzed = true;
        if(strcmp(response.status_code, "200") != 0){
            char *msg = NULL;
            strcpy(msg, response.status_code);
            strcpy(msg, ": ");
            strcpy(msg, response.status_text);

            send_error_to_client(proxy.fds[j+1].fd, msg);

            free(proxy.cache->cache_records[cache_num].data);
            free(proxy.cache->cache_records[cache_num].path);
            return -1;
        }
    }

    proxy.cache->cache_size++;
    if(cache_num >= proxy.cache->cache_size) {
        strcpy(proxy.cache->cache_records[cache_num].path, proxy.requests[j].path);
    }

    if(!strstr(proxy.cache->cache_records[cache_num].data, "\0")){
        proxy.fds[j].events = POLLIN;
        return 1;
    }

    httpParseResponse(proxy.cache->cache_records[cache_num].data, &response);

    proxy.cache->cache_records[cache_num].is_upload_completed = true;

    delete_connection(j);
    return 0;
}

int send_data(int i){
    int index = i <= MAX_CLIENTS_NUM ? i : i-MAX_CLIENTS_NUM;
    int n = write(proxy.fds[i].fd, proxy.buf_arr[index], BLOCK_SIZE);
    if(n < 0){
        return -1;
    }
    //n<BLOCK_SIZE
    if(n != strlen(proxy.buf_arr[index])){
        proxy.buf_arr[index] += n;
    }
    return 0;
}
int send_error_to_client(int fd, const char* msg){
    int n = write(fd, msg, strlen(msg));
    if(n<0 || n!= strlen(msg)){
        perror("Error: sending error to client failed");
        return -1;
    }
    return 0;
}

void delete_connection(int i){
    printf("Connection with fd %d closed\n", proxy.fds[i].fd);
    close(proxy.fds[i].fd);
    proxy.fds[i].fd = -1;
}

int is_cached(char *url){
    for(int i=0; i<proxy.cache->cache_size; i++){
        if(!strcmp(url, proxy.cache->cache_records[i].path)){
            return i;
        }
    }
    return -1;
}

int find_cache(char *url){
    int i;
    for(i=0; i<proxy.cache->cache_size; i++){
        if(strcmp(proxy.cache->cache_records[i].path, url) && !proxy.cache->cache_records[i].is_upload_completed){
            return i;
        }
    }
    if(i < MAX_CACHE_NUM){
        return i;
    }
    return -1;
}

int find_free_fd(){
    for(int i=1;MAX_CLIENTS_NUM+1;i++){
        if(proxy.fds[i].fd == -1) {
            return i;
        }
    }
    return -1;
}

