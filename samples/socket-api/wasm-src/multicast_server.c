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
test_multicast_server(int is_inet6) {
    struct in_addr localInterface;
    struct sockaddr_in6 addr = { 0 };
    int sd;
    char * databuf = "Test message";
    int datalen = strlen(databuf);
    sd = guard(socket(AF_INET6, SOCK_DGRAM, 0), "Failed to open socket");

    // guard(set_and_get_bool_opt(sd, SOL_SOCKET, SO_REUSEADDR, 1), "Failed setting SO_REUSEADDR");


    ((struct sockaddr_in6 *)&addr)->sin6_family = AF_INET6;
    ((struct sockaddr_in6 *)&addr)->sin6_port = htons(1234);
    ((struct sockaddr_in6 *)&addr)->sin6_addr = in6addr_any;

    struct addrinfo   *reslocal, *resmulti, hints;

    struct sockaddr_in6 address = {AF_INET6, htons(1234)};
    inet_pton(AF_INET6, "FF02:113D:6FDD:2C17:A643:FFE2:1BD1:3CD2", &address.sin6_addr);
    // guard(getaddrinfo("FF02:113D:6FDD:2C17:A643:FFE2:1BD1:3CD2", NULL, NULL, &resmulti), "Failed getting addr info 2");
    //     ((struct sockaddr_in6 *)&addr)->sin6_addr = ((struct sockaddr_in6 *)resmulti->ai_addr)->sin6_addr;
    localInterface.s_addr = 1;

    for (int i = 0; i < 16; i++) {
        printf("%d\n",  address.sin6_addr.__u6_addr.__u6_addr8[i]);
    }

    guard(setsockopt(sd, IPPROTO_IPV6, IPV6_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface)), "Failed setting local interface");
    guard(sendto(sd, databuf, datalen, 0, (struct sockaddr*)&address, sizeof(address)), "Failed sending datagram");

    return EXIT_SUCCESS;
}