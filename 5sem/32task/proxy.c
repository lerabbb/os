#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <termios.h>
#include <ctype.h>

#include <pthread.h>

#define PORT 7778
#define CACHE_SIZE 10
#define MAX_CLIENTS_NUM 100
#define BUFFER_SIZE 2048
#define ADDRESS_LEN 256
#define PORT_LEN 5
#define DELAY 20000
#define TTL 30

typedef struct cache {
    int size;
    char* title;
    char* data;
    pthread_mutex_t mutex;
    long lastTime;
} cache_t;

typedef struct url {
    char * host;
    char * path;
    int port;
} url_t;

typedef struct thread_arg{
    int client_fd;
    int client_index;
} thread_arg_t;


int clients[MAX_CLIENTS_NUM];
int servers[MAX_CLIENTS_NUM];
int cache_to_client[MAX_CLIENTS_NUM];
int bytes_sent[MAX_CLIENTS_NUM];
fd_set lfds, cfds;
int cache_size;

pthread_t tids[MAX_CLIENTS_NUM];
cache_t cache[CACHE_SIZE];

pthread_mutex_t cache_size_mutex;
pthread_mutex_t realloc_mutex;

void free_url(url_t *pUrl);
url_t *parse_url(char *url_buf);
void send_err_to_client(int client, char *msg);
void init_mutex(pthread_mutex_t *mutex);


int find_free_fd() {
    for(int i = 0; i < MAX_CLIENTS_NUM; i++) {
        if(tids[i] == 0) {
            return i;
        }
    }
    return -1;
}

int connect_to_host(char *host, in_port_t port){
    struct hostent *hp = gethostbyname(host);
    if(hp == NULL){
        perror("Fail while calling gethostbyname");
        return -1;
    }

    struct sockaddr_in addr;
    memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;

    int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock == -1){
        perror("socket");
        return -1;
    }
    int on = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));

    if(connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1){
        perror("Fail while connecting to host");
        return -1;
    }

    return sock;
}

char *make_request(const url_t *url) {
    char* request = (char*)malloc(sizeof(char) * (ADDRESS_LEN + 16));
    strcpy(request, "GET /");
    strcat(request, url->path);
    strcat(request, "\r\n");
    return request;
}

int send_request(int i, const url_t *url) {
    if(url->path == NULL){
        int n = write(servers[i], "GET /\r\n", strlen("GET /\r\n"));
        if(n < 0){
            send_err_to_client(clients[i], "fail while writing to server");
            return -1;
        }
    }
    else{
        char *request = make_request(url);
        int n = write(servers[i], request, strlen(request));
        if(n < 0){
            send_err_to_client(clients[i], "fail while writing to server");
            return -1;
        }
        free(request);
    }
    return 0;
}

int find_in_cache(const char *url_buf) {
    for(int i = 0; i < cache_size; i++) {
        if(strcmp(cache[i].title, url_buf) == 0) {
            return i;
        }
    }
    return -1;
}

int read_from_server(int index, int fd) {
    int offset = 0;
    int read_bytes = 0;
    while(1) {
        read_bytes = read(fd, &((cache[index].data)[offset]), BUFFER_SIZE);
        if(read_bytes < 0){
            perror("fail while reading bytes from server");
            return -1;
        }
        if(read_bytes == 0){
            break;
        }
        offset += read_bytes;
        cache[index].size = offset;

        pthread_mutex_lock(&realloc_mutex);
        cache[index].data = (char*)realloc(cache[index].data, offset + BUFFER_SIZE + 1);
        pthread_mutex_unlock(&realloc_mutex);
    }
    return 0;
}

void disconnect_client(int fd, int i) {
    printf("client with fd %d disconnected\n", fd);
    tids[i] = 0;
    close(fd);
}

int writeToClient(int client_fd, int socket){
    int read_bytes = 0;
    char buffer[BUFFER_SIZE + 1] = {0};
    while(1) {
        read_bytes = read(socket, buffer, BUFFER_SIZE);
        if(read_bytes < 0){
            perror("fail while reading bytes from server");
            return -1;
        }
        if(read_bytes == 0){
            break;
        }

        int n = write(client_fd, buffer, read_bytes);
        if(n < 0){
            perror("fail while writing bytes to client");
            return -1;
        }
        if(n < 1) {
            break;
        }
    }
    return 0;
}

void *handle_client(void *args){
    thread_arg_t* arg = (thread_arg_t *) args;
    int fd = arg->client_fd;
    int index = arg->client_index;
    free(arg);

    char* url_buf = calloc(BUFFER_SIZE + 1, sizeof(char));
    int read_bytes = -1;

    while(1) {
        read_bytes = read(fd, url_buf, BUFFER_SIZE);
        if (read_bytes < 1) {
            perror("fail while reading bytes from client");
            disconnect_client(fd, index);
            break;
        }

        url_buf[read_bytes] = '\0';
        if (strcmp(url_buf, "/exit") == 0) {
            disconnect_client(fd, index);
            break;
        }
        int cache_ind = find_in_cache(url_buf);
        if (cache_ind != -1) {
            printf("Page found at cache!\n");
        } else {
            url_t *url = parse_url(url_buf);
            if (url == NULL) {
                printf("invalid url=%s\n", url_buf);
                continue;
            }
            int client_sock = connect_to_host(url->host, url->port);
            if (client_sock == -1) {
                //write(fd, "Failed connect!\n", strlen("Failed connect!\n"));
                continue;
            }
            send_request(client_sock, url);
            if (cache_size > CACHE_SIZE - 1) {
                //write(clientFd, "Cache is full!\n", strlen("Cache is full!\n"));
                writeToClient(fd, client_sock);
                printf("Cache is full\n");
                continue;
            }

            pthread_mutex_lock(&cache_size_mutex);
            cache_ind = cache_size;
            cache_size++;
            pthread_mutex_unlock(&cache_size_mutex);


            pthread_mutex_lock(&(cache[index].mutex));

            cache[index].title = (char *) malloc(strlen(url_buf) + 1);
            strcpy(cache[index].title, url_buf);
            read_from_server(index, client_sock);

            pthread_mutex_unlock(&(cache[index].mutex));
        }
        cache[index].lastTime = time(NULL);
        int n = write(fd, cache[index].data, cache[index].size);
        if(n < 0){
            perror("fail while writing bytes to client");
            break;
        }
        if(n == 0) {
            break;
        }
    }

    close(fd);
    pthread_exit((void *) 1);
}


void *clean_cache(void * arg){
    while (1) {
        usleep(DELAY);
        pthread_mutex_lock(&cache_size_mutex);
        int cacheIndex = cache_size;
        pthread_mutex_unlock(&cache_size_mutex);
        for (int i = cacheIndex - 1; i >= 0; i--) {
            pthread_mutex_lock(&(cache[i].mutex));
            if (time(NULL) - cache[i].lastTime > 30) {
                free(cache[i].data);
                cache[i].data = (char *)calloc(BUFFER_SIZE, sizeof(char));
                printf("Cache ent=%s  removed\n", cache[i].title);
                cache[i].title = NULL;
                cache[i].size = 0;
                pthread_mutex_lock(&cache_size_mutex);
                cache_size--;
                pthread_mutex_unlock(&cache_size_mutex);
            }
            pthread_mutex_unlock(&(cache[i].mutex));
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    struct sockaddr_in proxy_addr;
    int sock_fd = 0, new_fd = -1;
    pthread_t clean_tid;

    init_mutex(&realloc_mutex);
    init_mutex(&cache_size_mutex);
    for(int i=0; i<CACHE_SIZE; i++){
        init_mutex(&cache[i].mutex);
        cache[i].lastTime = time(NULL);
    }

    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    proxy_addr.sin_port = htons(PORT);

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd != 0){
        perror("fail while opening socket");
        exit(EXIT_FAILURE);
    }
    if(bind(sock_fd, (struct sockaddr*)&proxy_addr, sizeof(proxy_addr))){
        perror("fail while binding socket");
        exit(EXIT_FAILURE);
    }
    if(listen(sock_fd, MAX_CLIENTS_NUM)){
        perror("fail while listening socket");
        exit(EXIT_FAILURE);
    }

    if(pthread_create(&clean_tid, NULL, clean_cache, NULL)){
        perror("fail while creating thread");
        exit(EXIT_FAILURE);
    }

    while(1){
        new_fd = accept(sock_fd,(struct sockaddr*)NULL, NULL);
        if(new_fd < 0){
            perror("fail while accepting new connection");
            continue;
        }
        else{
            int free_fd = find_free_fd();
            if(free_fd < 0){
                perror("No available sockets for new connection");
                continue;
            }
            thread_arg_t *arg = (thread_arg_t*) malloc(sizeof(thread_arg_t));
            arg->client_fd = new_fd;
            arg->client_index = free_fd;

            while(1){
                if(pthread_create(&tids[free_fd], NULL, handle_client, arg)){
                    perror("fail while creating thread");
                    continue;
                } else{
                    break;
                }
            }
        }
    }

    return 0;
}

void init_mutex(pthread_mutex_t *mutex){
    if(pthread_mutex_init(mutex, NULL)){
        perror("fail while initializing mutex");
        exit(EXIT_FAILURE);
    }
}

url_t * parse_url(char *url_buf) {
    url_t * url = (url_t *) malloc(sizeof(url_t));
    url->path = NULL;
    url->host = NULL;
    url->port = 80;
    int url_size = strlen(url_buf);
    int start = 0;
    for (int i = 0; i < url_size; i++){
        if (url_buf[i] == ':'){
            if (start){
                free_url(url);
                return NULL;
            }
            start = i;
            char port[PORT_LEN + 1] = {0};
            int port_pos = i + 1;
            int portIndex = 0;
            while (portIndex <  PORT_LEN && port_pos < url_size && isdigit(url_buf[port_pos])){
                port[portIndex++] = url_buf[port_pos];
                port_pos++;
            }
            url->port = atoi(port);
        }
        if (url_buf[i] == '/') {
            if (i + 1 == url_size) {
                url_buf[i] = '\0';
                break;
            }
            url->host = (char *) malloc(sizeof(char) * (start + 1));
            url->path = (char *) malloc(sizeof(char) * (url_size - i));
            strncpy(url->host, url_buf, start ? start : i);
            strncpy(url->path, &(url_buf[i + 1]), url_size - i - 1);
            break;
        }
    }
    return url;
}
void free_url(url_t *pUrl) {
    free(pUrl->path);
    free(pUrl->host);
    free(pUrl);
}
void send_err_to_client(int client, char *msg){
    int n = write(client, msg, strlen(msg));
    if(n < 0){
        perror("fail while sending error to client");
        exit(EXIT_FAILURE);
    }
}
