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
test_multicast_client(int is_inet6) {
    struct ipv6_mreq ipv6_group;
    struct addrinfo   *reslocal, *resmulti, hints;
    int sd;
    int datalen;
    char databuf[1024];



    sd = guard(socket(AF_INET6, SOCK_DGRAM, 0), "Failed opening socket");
    guard(set_and_get_bool_opt(sd, SOL_SOCKET, SO_REUSEADDR, 1), "Failed setting SO_REUSEADDR");

    struct sockaddr_in6 addr = { 0 };
    ((struct sockaddr_in6 *)&addr)->sin6_family = AF_INET6;
    ((struct sockaddr_in6 *)&addr)->sin6_port = htons(1234);
    // ((struct sockaddr_in6 *)&addr)->sin6_addr = in6addr_any;

     memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    getaddrinfo(NULL, "5150", &hints, &reslocal);
    // bind(sd, reslocal->ai_addr, reslocal->ai_addrlen);  
    // freeaddrinfo(reslocal);

    guard(bind(sd, reslocal->ai_addr, reslocal->ai_addrlen), "Failed binding socket");

    // Resolve the link-local interface

    // guard(getaddrinfo("fe80::250:4ff:fe7c:7036", NULL, NULL, &reslocal), "failed getting addr info 1");

    // Resolve the multicast address
        
    // guard(getaddrinfo("F012:113D:6FDD:2C17:A643:FFE2:1BD1:3CD2", NULL, NULL, &resmulti), "Failed getting addr info 2");

    // ipv6_group.ipv6mr_multiaddr = ((struct sockaddr_in6 *)resmulti->ai_addr)->sin6_addr;
    inet_pton(AF_INET6, "FF02:113D:6FDD:2C17:A643:FFE2:1BD1:3CD2", &(ipv6_group.ipv6mr_multiaddr));

    ipv6_group.ipv6mr_interface = 1;

    for (int i = 0; i < 16; i++) {
        printf("%d\n",  ipv6_group.ipv6mr_multiaddr.__u6_addr.__u6_addr8[i]);
    }
    printf("%d\n",  ipv6_group.ipv6mr_interface);
    printf("Adding group\n");
 
    guard(setsockopt(sd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &ipv6_group, sizeof(ipv6_group)), "Failed adding multicast group");

    printf("Added group\n");

    datalen = sizeof(databuf);
    int result = read(sd, databuf, datalen);

    OPTION_ASSERT((result < 0), 0, "read response");

    printf("Reading datagram message...OK.\n");
    printf("The message from multicast server is: \"%s\"\n", databuf);

    return EXIT_SUCCESS;
}