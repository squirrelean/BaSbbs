#ifndef READ_CONFIG_H
#define READ_CONFIG_H

typedef struct {
    int thmax;
    int thincr;
    int bbport;
    int fdebug;
    char *bbfile;
} ServerConfig;

void read_config_file(ServerConfig *config, char *config_path);

#endif
