#include "parser.h"

#include <stdio.h>
#include <ctype.h>

void parse_input_file(char *filename, task_t *tasks, int *size) {
    FILE *f = fopen(filename, "r");
    char *buff = (char *)malloc(sizeof(char) * FILE_BUFF);

    *size = 0;

    while (fgets(buff, FILE_BUFF, f) != NULL) {
        if (*buff == '#') continue;

        if (!isalpha(*buff)) continue;

        sscanf(buff, "%s%d%d",
             tasks[*size].name,
            &tasks[*size].frequency,
            &tasks[*size].duration);

        *size += 1;
    }

    free(buff);
    fclose(f);
    return;
}
