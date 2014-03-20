#include "udp.h"

#include "constants.h"

extern struct addrinfo hints, *res;

int Socket(int ai_family, int ai_socktype, int ai_protocol) {
    int fd = socket(ai_family, ai_socktype, ai_protocol);
    if (fd < 0) {
        errx(UDP_FAILURE, "Could not create socket :(");
    }
    return fd;
}

void Bind(int sockfd, const struct sockaddr* myaddr, int addrlen) {
    if (bind(sockfd, myaddr, addrlen) < 0) {
        errx(UDP_FAILURE, "Could not bind socket");
    }
    return;
}

void CloseUDPServer(int sockfd) {
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);

    freeaddrinfo(res);

    return;
}

int InitUDPServer(char *port) {
    int status, sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    status = getaddrinfo(NULL, port, &hints, &res);

    if (status != 0) { errx(1, gai_strerror(status)); }

    sockfd = Socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    Bind(sockfd, res->ai_addr, res->ai_addrlen);

    return sockfd;
}
