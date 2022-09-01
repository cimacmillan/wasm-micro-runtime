

#include <stdio.h> /* printf, sprintf */
#include <stdlib.h> /* exit */
#include <unistd.h> /* read, write, close */
#include <string.h> /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#ifdef __wasi__
#include <wasi_socket_ext.h>
#endif

typedef struct hostent {
    char* h_addr;
    short h_length;
} hostent;

__wasi_addr_t gethostbyname(const char *name) {
   __wasi_addr_info_hints_t hints;
    hints.hints_enabled = 1;
    hints.type = SOCKET_STREAM;
    hints.family = INET6;
    __wasi_addr_info_t info[4];
    __wasi_size_t max_info;
    __wasi_errno_t error = __wasi_sock_addr_resolve(name, "http", &hints, info, 4, &max_info);
    if (error) {
        printf("host address resolution failed %d\n", error);
    }
    printf("Address found for %s\n", name);
    for (int i = 0; i < 1; i++) {
        printf("%u.%u.%u.%u.%u.%u.%u.%u\n", 
            info[i].addr.addr.ip6.addr.n0,
            info[i].addr.addr.ip6.addr.n1, 
            info[i].addr.addr.ip6.addr.n2,
            info[i].addr.addr.ip6.addr.n3,
            info[i].addr.addr.ip6.addr.h0,
            info[i].addr.addr.ip6.addr.h1, 
            info[i].addr.addr.ip6.addr.h2,
            info[i].addr.addr.ip6.addr.h3
            );
    }

    return info[0].addr;
}

void error(const char *msg) { perror(msg); exit(0); }

__wasi_errno_t
wasi_addr_to_sockaddr(const __wasi_addr_t *wasi_addr,
                      struct sockaddr *sock_addr, socklen_t *addrlen);


int does_char_buffer_contain_between_indexes(char* buffer, const char* does_contain, int start_index, int end_index) {
    size_t string_length = strlen(does_contain);
    end_index = end_index - string_length;

    for (int i = start_index; i < end_index; i++) {
        int is_equal = 1;
        for (int character_index = 0; character_index < string_length; character_index++) {
            is_equal = is_equal && (buffer[i + character_index] == does_contain[character_index]);
        }
        if (is_equal == 1) {
            return i;
        }
    }
    return -1;
}   


int main()
{
    /* first what are we going to send and where are we going to send it? */
    int portno =        80;
    char *host =        "google.com";
    char *message = "GET /index.html HTTP/1.1\r\nHost: www.google.com\r\n\r\n";
    // char *message = "GET /index.html HTTP/1.1";


    __wasi_addr_t addr;
    struct sockaddr_in serv_addr;
    int sockfd, bytes, sent, received, total;
    char* response = (char*)calloc(0, sizeof(char) * 128000);


    /* create the socket */
    sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");

    /* lookup the ip address */
    addr = gethostbyname(host);

    /* connect the socket */
    socklen_t addr_length;
    wasi_addr_to_sockaddr(&addr, &serv_addr, &addr_length);
    // serv_addr.sin_family = AF_INET6;
    // serv_addr.sin_port = htons(portno);
    if (connect(sockfd, &serv_addr, addr_length) != 0)
        error("ERROR connecting");

    /* send the request */
    total = strlen(message);

    printf("Connected, sending request of %d bytes\n", total);

    sent = 0;
    do {
        bytes = send(sockfd, message, total, 0);
        if (bytes < 0)
            error("ERROR writing message to socket");
        if (bytes == 0)
            break;
        sent+=bytes;
    } while (sent < total);

    printf("Sent request, receiving response. \n");

    /* receive the response */
    total = 100000;
    received = 0;
    do {
        bytes = recv(sockfd,response+received,total-received, 0);
        if (bytes < 0)
            error("ERROR reading response from socket");
        if (bytes == 0)
            break;
        
        // for(int i = received; i < received + bytes; i++) {
        //     printf("%c", response[i], response[i]);
        // }

        received+=bytes;
        // printf("Received %d bytes\n", received);

        if (does_char_buffer_contain_between_indexes(response, "</html>", 0, received) >= 0) {
            break;
        }
    } while (received < total);

    /*
     * if the number of received bytes is the total size of the
     * array then we have run out of space to store the response
     * and it hasn't all arrived yet - so that's a bad thing
     */
    if (received == total)
        error("ERROR storing complete response from socket");

    /* close the socket */
    close(sockfd);

    /* process response */
    printf("Response:\n------------\n%s\n-------------\n",response);

    int start_index = does_char_buffer_contain_between_indexes(response, "<!doctype html>", 0, received);
    int end_index = does_char_buffer_contain_between_indexes(response, "</html>", 0, received) + strlen("</html>");

    printf("Received %d bytes from GET google.com/index.html \n", received);

    return 0;
}