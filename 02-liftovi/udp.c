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

    return;
}

void CloseUDPClient(int sockfd) {
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);

    return;
}

int InitUDPServer(char *port) {
    int status, sockfd;
    struct addrinfo hints;
    struct addrinfo *server_res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    status = getaddrinfo(NULL, port, &hints, &server_res);

    if (status != 0) {
        errx(UDP_FAILURE, "Could not get address info :(");
    }

    sockfd = Socket(server_res->ai_family, server_res->ai_socktype, server_res->ai_protocol);

    Bind(sockfd, server_res->ai_addr, server_res->ai_addrlen);

    freeaddrinfo(server_res);

    return sockfd;
}

int InitUDPClient(char *host, char *port, struct sockaddr *server) {
    int status, sockfd;
    struct addrinfo hints;
    struct addrinfo *client_res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    status = getaddrinfo(host, port, &hints, &client_res);

    if (status != 0) {
        errx(UDP_FAILURE, "Could not get address info :(");
    }

    sockfd = Socket(client_res->ai_family, client_res->ai_socktype, client_res->ai_protocol);

    *server = *(client_res->ai_addr);

    freeaddrinfo(client_res);

    return sockfd;
}
