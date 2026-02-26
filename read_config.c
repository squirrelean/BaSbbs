#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "read_config.h"

void read_config_file(ServerConfig *config, char *config_path)
{
    FILE *fp = fopen(config_path, "r");
    if (!fp) {
        perror("Could not open file");
        return;
    }

    char buffer[256], key[256], value[256];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (sscanf(buffer, "%255s %255s", key, value) == 2) {
            if (!strcmp(key, "THMAX"))
                config->thmax = atoi(value);
            else if (!strcmp(key, "THINCR"))
                config->thincr = atoi(value);
            else if (!strcmp(key, "BBPORT"))
                config->bbport = atoi(value);
            else if (!strcmp(key, "FDEBUG")) {
                if (!strcmp(value, "true") || atoi(value) == 1)
                    config->fdebug = true;
                else if (!strcmp(value, "false") || !atoi(value))
                    config->fdebug = false;
            } else if (!strcmp(key, "BBFILE"))
                config->bbfile = strdup(value);
        }
    }

    fclose(fp);
}
