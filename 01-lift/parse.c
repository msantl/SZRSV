#include "parse.h"
#include "constants.h"

void ParseHostnameAndPort(char *name, char **hostname, char **port) {
    char buff[MAX_BUFF];
    int found = 0;
    FILE *f = fopen(CONFIGURATION, "r");

    if (!f) {
        errx(PARSE_FAILURE, "Could not open configuration file :(");
    }

    while (fgets(buff, MAX_BUFF, f) != NULL) {
        /* skip comments */
        if (*buff == '#') continue;

        /* we have a match */
        if (strncmp(buff, name, strlen(name)) == 0) {
            found = 1;
            sscanf(buff, "%*s %s %s", hostname, port);
            break;
        }

    }

    fclose(f);

    if (!found) {
        errx(PARSE_FAILURE, "Could not find given configuration :(");
    }
    return;
}
