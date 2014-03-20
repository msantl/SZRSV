#include <stdio.h>

#include "udp.h"
#include "parse.h"
#include "constants.h"

int main(int argc, char **argv) {
    char upr_hostname[MAX_BUFF], upr_port[MAX_BUFF];
    char tipke_hostname[MAX_BUFF], tipke_port[MAX_BUFF];

    int server_socket;

    ParseHostnameAndPort("UPR", &upr_hostname, &upr_port);
    printf("upr = %s %s\n", upr_hostname, upr_port);

    ParseHostnameAndPort("TIPKE", &tipke_hostname, &tipke_port);
    printf("tipke = %s %s\n", tipke_hostname, tipke_port);

    server_socket = InitUDPServer(tipke_port);


    CloseUDPServer(server_socket);
    return 0;
}
