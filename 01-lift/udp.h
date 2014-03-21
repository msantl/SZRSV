/*
 * Wrapper functions for UDP
 */
#ifndef __UDP_H
#define __UDP_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>

#include <err.h>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <arpa/inet.h>

/*
 * Socket   - create a new socket with given properties
 *          - exits on failure
 *
 * @return  - file descriptor of created socket
 */
int Socket(int, int, int);

/*
 * Bind     - binds given socket to address/port
 *          - exits on failure
 */
void Bind(int, const struct sockaddr*, int);

/*
 * CloseUDPServer   - close given socket for read/write
 */
void CloseUDPServer(int);

/*
 * CloseUDPClient   - close given socket for read/write
 */
void CloseUDPClient(int);

/*
 * InitUDPServer    - initialize UDP server on given port
 *                  - exits on failure
 *
 * @return          - file descriptor of created socket
 */
int InitUDPServer(char *);

/*
 * InitUDPClient    - initialize UDP client with given host and port
 *                  - exits on failure
 *
 * @return          - file descriptor of created socket
 */
int InitUDPClient(char *, char *, struct sockaddr *);

#endif
