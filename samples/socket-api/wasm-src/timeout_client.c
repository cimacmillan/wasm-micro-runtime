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
test_send_and_recv_timeout_client()
{
    int socket_fd;
    struct sockaddr_in addr;
    struct timeval tv = to_timeval(0, 1);
    const int snd_buf_len = 8;
    const int data_buf_len = 1000000;
    char *buffer = (char *)malloc(sizeof(char) * data_buf_len);
    int result;
    socklen_t opt_len = sizeof(snd_buf_len);
    struct timeval snd_start_time, snd_end_time;

    /* 127.0.0.1:1234 */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socket_fd = guard(socket(AF_INET, SOCK_STREAM, 0), "Create socket failed");
    guard(set_and_get_bool_opt(socket_fd, SOL_SOCKET, SO_REUSEADDR, 1),
          "Failed to set REUSEADDR");
    guard(setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)),
          "Failed to set SO_RCVTIMEO");
    guard(setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)),
          "Failed to set SO_SNDTIMEO");
    guard(setsockopt(socket_fd, SOL_SOCKET, SO_SNDBUF, &data_buf_len,
                     sizeof(data_buf_len)),
          "Failed to set buffer length");
    guard(connect(socket_fd, (struct sockaddr *)&addr, sizeof(addr)),
          "Connect failed");
    getsockopt(socket_fd, SOL_SOCKET, SO_SNDBUF, (void *)&data_buf_len,
               &opt_len);

    printf("Waiting on recv, which should timeout\n");
    result = recv(socket_fd, buffer, 1, 0);
    OPTION_ASSERT(result, -1, "recv timeout");
    OPTION_ASSERT(errno, EAGAIN, "errno EAGAIN");

    printf("Waiting on send, which should timeout\n");
    gettimeofday(&snd_start_time, NULL);
    result = send(socket_fd, buffer, data_buf_len, 0);
    gettimeofday(&snd_end_time, NULL);

    OPTION_ASSERT((result < data_buf_len), 1,
                  "expect partial send transmission");
    OPTION_ASSERT((snd_start_time.tv_sec == snd_end_time.tv_sec), 1,
                  "expected quick send return");

    close(socket_fd);
    printf("Closing socket \n");
    return EXIT_SUCCESS;
}
