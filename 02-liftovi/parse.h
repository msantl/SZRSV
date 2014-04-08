/*
 * Parsing commad line arguments and configuration file
 */

#ifndef __PARSE_H
#define __PARSE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <err.h>

/*
 * ParseHostnameAndPort - finds configuration for given name
 *                      - exits on error
 */
void ParseHostnameAndPort(char *, char**, char**);

#endif
