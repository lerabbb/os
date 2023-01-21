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

#define PORT 7777
#define CACHE_SIZE 10
#define MAX_CLIENTS_NUM 100
#define BUFFER_SIZE 2048
#define ADDRESS_LEN 256
#define PORT_LEN 5
typedef struct cache {
    int size;
    char* title;
    char* data;
} cache_t;

typedef struct url {
    char * host;
    char * path;
    int port;
} url_t;

int clients[MAX_CLIENTS_NUM];
int servers[MAX_CLIENTS_NUM];
int cache_to_client[MAX_CLIENTS_NUM];
int bytes_sent[MAX_CLIENTS_NUM];
fd_set lfds, cfds;

void free_url(url_t *pUrl);
url_t * parse_url(char *url_buf);
void send_err_to_client(int client, char *msg);

int find_free_fd() {
    int i = 0;
    while (i < MAX_CLIENTS_NUM && clients[i] >= 0) {
        i++;
    }
    return i;
}

int create_new_connection(int listenfd, int clients_num, struct timeval *timeout){
    if(clients_num < MAX_CLIENTS_NUM) {
        if(select(4, &lfds, NULL, NULL, timeout)){
            int free_fd = find_free_fd();
            clients[free_fd] = accept(listenfd, (struct sockaddr*)NULL, NULL);
            char send_buf[BUFFER_SIZE + 1];
            write(clients[free_fd], send_buf, strlen(send_buf));
            clients_num++;
        }
    }
    return clients_num;
}

int find_in_cache(const cache_t *cache, int cache_size, int clientIndex, const char *url_buf) {
    for(int i = 0; i < cache_size; i++) {
        if(strcmp(cache[i].title, url_buf) == 0) {
            cache_to_client[clientIndex] = i;
            bytes_sent[clientIndex] = 0;
            return 1;
        }
    }
    return 0;
}

int connect_to_host(char *host, in_port_t port){
    printf("host %s\n", host);
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

void disconnect_client(int i){
    printf("client %d disconnected", i);
    close(clients[i]);
    close(servers[i]);
    clients[i] = -1;
    servers[i] = -1;
    cache_to_client[i] = -1;
    bytes_sent[i] = 0;
}



int read_from_server(cache_t *cache, int cache_size, int i) {
    int offset = 0;
    int read_bytes = 0;
    while(1) {
        read_bytes = read(servers[i], &((cache[cache_size].data)[offset]), BUFFER_SIZE);
        if(read_bytes < 0){
            send_err_to_client(clients[i], "fail while uploading page from server");
            return -1;
        }
        if(read_bytes == 0){
            break;
        }
        offset += read_bytes;
        cache[cache_size].size = offset;
        cache[cache_size].data = (char*)realloc(cache[cache_size].data, offset + BUFFER_SIZE + 1);
    }
    return 0;
}

int resend_from_cache(const cache_t *cache, int clients_num, int i) {
    int written_bytes = 0;
    if(cache_to_client[i] != -1) {
        if (bytes_sent[i] < cache[cache_to_client[i]].size) {
            int bytes_left = cache[cache_to_client[i]].size - bytes_sent[i];
            int bytes_len = BUFFER_SIZE < bytes_left ? BUFFER_SIZE : bytes_left;
            written_bytes = write(clients[i], &(cache[cache_to_client[i]].data[bytes_sent[i]]), bytes_len);
            if (written_bytes == -1) {
                cache_to_client[i] = -1;
                clients[i] = -1;
                clients_num--;
            }
            else if (written_bytes != 0) {
                bytes_sent[i] += written_bytes;
            }
        }
    }
    return clients_num;
}


int main(int argc, char **argv) {
    int sock_fd = 0, clients_num = 0, cache_size = 0;
    struct sockaddr_in proxy_addr;
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    memset(&proxy_addr, 0, sizeof(proxy_addr));
    cache_t * cache = (cache_t*)malloc(sizeof(cache_t) * CACHE_SIZE);
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    for(int k = 0; k < MAX_CLIENTS_NUM; k++) {
        clients[k] = -1;
        servers[k] = -1;
        cache_to_client[k] = -1;
        bytes_sent[k] = -1;
    }

    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    proxy_addr.sin_port = htons(PORT);

    bind(sock_fd, (struct sockaddr*)&proxy_addr, sizeof(proxy_addr));
    listen(sock_fd, MAX_CLIENTS_NUM);

    while(1){
        FD_ZERO(&lfds);
        FD_SET(sock_fd, &lfds);

        clients_num = create_new_connection(sock_fd, clients_num, &timeout);

        for(int i=0; i< MAX_CLIENTS_NUM; i++){
            if(clients[i] < 2){
                continue;
            }

            FD_ZERO(&cfds);
            FD_SET(clients[i], &cfds);

            if(servers[i] == -1 && select(clients[i] + 1, &cfds, NULL, NULL, &timeout)){
                char url_buf[ADDRESS_LEN];
                int bytes_read = read(clients[i], &url_buf, ADDRESS_LEN);
                if(bytes_read < 0){
                    perror("Fail while reading from client");
                    exit(EXIT_FAILURE);
                }
                if(strcmp(url_buf, "/exit") == 0){
                    clients_num--;
                    disconnect_client(i);
                    continue;
                }

                url_buf[bytes_read] = '\0';
                url_t * url = parse_url(url_buf);
                if (url == NULL){
                    perror("Fail while parsing url");
                }

                int index = find_in_cache(cache, cache_size, i, url_buf);
                if(index == 0){
                    servers[i] = connect_to_host(url->host, url->port);

                    if(servers[i] < 0){
                        send_err_to_client(clients[i], "Fail while connecting to host");
                        continue;
                    }
                    if(send_request(i, url) < 0){
                        perror("Fail while sending request");
                        continue;
                    }

                    cache[cache_size].title = (char*)malloc(sizeof(char) * strlen(url_buf));
                    strcpy(cache[cache_size].title, url_buf);
                    cache[cache_size].size = BUFFER_SIZE;
                    cache[cache_size].data = (char*)malloc(BUFFER_SIZE + 1);
                    cache_to_client[i] = cache_size;
                    bytes_sent[i] = 0;

                    if(read_from_server(cache, cache_size, i) < 0){
                        perror("Fail while reading data from server");
                        continue;
                    }
                    close(servers[i]);

                    cache[cache_size].data[cache[cache_size].size + 1] = '\0';
                    cache_size++;
                }
                free_url(url);
            }
            clients_num = resend_from_cache(cache, clients_num, i);
        }
    }

    return 0;
}


url_t * parse_url(char *urlBuffer) {
    url_t * url = (url_t *) malloc(sizeof(url_t));
    url->path = NULL;
    url->host = NULL;
    url->port = 80;
    size_t urlBufferSize = strlen(urlBuffer);
    int startPortIndex = 0;
    for (size_t strIndex = 0; strIndex < urlBufferSize; strIndex++){
        if (urlBuffer[strIndex] == ':'){
            if (startPortIndex){
                free_url(url);
                return NULL;
            }
            startPortIndex = strIndex;
            char port[PORT_LEN + 1] = {0};
            size_t portStrIndex = strIndex + 1;
            int portIndex = 0;
            while (portIndex <  PORT_LEN && portStrIndex < urlBufferSize && isdigit(urlBuffer[portStrIndex])){
                port[portIndex++] = urlBuffer[portStrIndex];
                portStrIndex++;
            }
            url->port = atoi(port);
        }
        if (urlBuffer[strIndex] == '/') {
            if (strIndex + 1 == urlBufferSize) {
                urlBuffer[strIndex] = '\0';
                break;
            }
            url->host = (char *) malloc(sizeof(char) * (startPortIndex + 1));
            url->path = (char *) malloc(sizeof(char) * (urlBufferSize - strIndex));
            strncpy(url->host, urlBuffer, startPortIndex  ? startPortIndex : strIndex);
            strncpy(url->path, &(urlBuffer[strIndex + 1]), urlBufferSize - strIndex - 1);
            printf("host = %s\n", url->host);
            printf("path = %s\n", url->path);
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
   //   exit(EXIT_FAILURE);
    }
}
