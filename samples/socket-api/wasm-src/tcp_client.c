/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#ifdef __wasi__
#include <wasi_socket_ext.h>
#endif

int
main(int argc, char *argv[])
{
    int socket_fd, ret, total_size = 0;
    char buffer[1024] = { 0 };
    char ip_string[16] = { 0 };
    struct sockaddr_in server_address = { 0 };
    struct sockaddr_in local_address = { 0 };
    socklen_t len;

    printf("[Client] Create socket\n");
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("Create socket failed");
        return EXIT_FAILURE;
    }

    /* 127.0.0.1:1234 */
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(1234);
    server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

#ifdef __wasi__
    printf("Set rcv timeout\n");
    uint64_t recv_timeout = 5000000;
    __wasi_sock_set_recv_timeout(socket_fd, &recv_timeout);

    printf("Get rcv timeout\n");
    __wasi_sock_get_recv_timeout(socket_fd, &recv_timeout);

    printf("Timeout is %llu\n", recv_timeout);
#endif
    printf("[Client] Connect socket\n");
    if (connect(socket_fd, (struct sockaddr *)&server_address,
                sizeof(server_address))
        == -1) {
        perror("Connect failed");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    len = sizeof(local_address);
    ret = getsockname(socket_fd, (struct sockaddr *)&local_address, &len);
    if (ret == -1) {
        perror("Failed to retrieve socket address");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    inet_ntop(AF_INET, &local_address.sin_addr, ip_string,
              sizeof(ip_string) / sizeof(ip_string[0]));

    printf("[Client] Local address is: %s:%d\n", ip_string,
           ntohs(local_address.sin_port));

    printf("[Client] Client receive\n");
    while (1) {
        ret = recv(socket_fd, buffer + total_size, sizeof(buffer) - total_size,
                   0);

        printf("recv return was %d\n", ret);
        if (ret <= 0)
            break;
        total_size += ret;
    }

    printf("[Client] %d bytes received:\n", total_size);
    if (total_size > 0) {
        printf("Buffer recieved:\n%s\n", buffer);
    }

    close(socket_fd);
    printf("[Client] BYE \n");
    return EXIT_SUCCESS;
}
