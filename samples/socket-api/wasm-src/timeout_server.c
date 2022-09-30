int
guard(int n, char *err)
{
    if (n == -1) {
        perror(err);
        exit(1);
    }
    return n;
}

int
test_send_and_recv_timeout_server()
{
    int socket_fd;
    int client_socket_fd;
    struct sockaddr_in addr = { 0 };
    int addrlen = sizeof(addr);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    socket_fd = guard(socket(AF_INET, SOCK_STREAM, 0), "Create socket failed");
    guard(set_and_get_bool_opt(socket_fd, SOL_SOCKET, SO_REUSEADDR, 1),
          "Failed to set REUSEADDR");
    guard(bind(socket_fd, (struct sockaddr *)&addr, addrlen),
          "Bind socket failed");
    guard(listen(socket_fd, 1), "Listen failed");

    printf("Wait for client to connect\n");
    client_socket_fd = guard(
        accept(socket_fd, (struct sockaddr *)&addr, (socklen_t *)&addrlen),
        "Accept failed");

    printf("Client connected, sleeping for 10s\n");
    sleep(10);

    printf("Shuting down\n");
    shutdown(client_socket_fd, SHUT_RDWR);
    shutdown(socket_fd, SHUT_RDWR);
    return EXIT_SUCCESS;
}
