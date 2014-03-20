#include <stdio.h>

#include "udp.h"
#include "parse.h"
#include "constants.h"

int main(int argc, char **argv) {
    char upr_hostname[MAX_BUFF], upr_port[MAX_BUFF];
    char lift_hostname[MAX_BUFF], lift_port[MAX_BUFF];
    char tipke_hostname[MAX_BUFF], tipke_port[MAX_BUFF];

    int server_socket;

    ParseHostnameAndPort("UPR", (char **) &upr_hostname, (char **) &upr_port);
    printf("upr = %s %s\n", upr_hostname, upr_port);

    ParseHostnameAndPort("LIFT1", (char **) &lift_hostname, (char **) &lift_port);
    printf("lift = %s %s\n", lift_hostname, lift_port);

    ParseHostnameAndPort("TIPKE", (char **) &tipke_hostname, (char **) &tipke_port);
    printf("tipke = %s %s\n", tipke_hostname, tipke_port);

    server_socket = InitUDPServer(upr_port);


    CloseUDPServer(server_socket);
    return 0;
}
