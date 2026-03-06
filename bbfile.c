#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "globals.h"
#include "lock.h"

long global_next_id = 0;

int bb_init()
{
    FILE *fp = fopen(global_config.bbfile, "a+");
    if (!fp)
        return -1;

    long highest_id = 0;
    char line[1024];

    while (fgets(line, sizeof(line), fp)) {
        long current_id;
        if (sscanf(line, "%ld/", &current_id) == 1) {
            if (current_id > highest_id)
                highest_id = current_id;
        }
    }

    global_next_id = highest_id + 1;

    fclose(fp);
    return 0;
}

int bb_write(const char *poster, const char *message)
{
    write_lock();

    if (global_config.fdebug) {
        printf("Write debug 6s sleep\n");
        sleep(6);
    }

    FILE *fp = fopen(global_config.bbfile, "a");
    if (!fp) {
        write_unlock();
        return -1;
    }

    fprintf(fp, "%ld/%s/%s\n", global_next_id, poster, message);
    global_next_id++;

    fflush(fp);
    fclose(fp);

    if (global_config.fdebug)
        printf("Write debug End\n");

    write_unlock();

    return global_next_id - 1;
}

int bb_read(const long message_number, char **message)
{
    read_lock();

    if (global_config.fdebug) {
        printf("Read debug 3s sleep\n");
        sleep(3);
    }

    FILE *fp = fopen(global_config.bbfile, "r");
    if (!fp) {
        read_unlock();
        return -1;
    }

    *message = NULL;

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        long id;
        if (sscanf(line, "%ld/", &id) == 1 && id == message_number) {
            char *corresponding_msg = strchr(line, '/');
            if (corresponding_msg) {
                corresponding_msg++;
                *message = strdup(corresponding_msg);
            }
            break;
        }
    }

    fclose(fp);

    if (global_config.fdebug)
        printf("Read debug end\n");

    read_unlock();

    if (!*message)
        return -2;
    return 0;
}

int bb_replace(const char *username, const long message_number, const char *new_message)
{
    write_lock();

    if (global_config.fdebug) {
        printf("Replace debug 6s sleep\n");
        sleep(6);
    }

    FILE *og_fp = fopen(global_config.bbfile, "r");
    if (!og_fp) {
        write_unlock();
        return -1;
    }

    char temp_filename[strlen(global_config.bbfile) + 8];
    snprintf(temp_filename, sizeof(temp_filename), "%s_tmp", global_config.bbfile);
    FILE *temp_fp = fopen(temp_filename, "w");
    if (!temp_fp) {
        fclose(og_fp);
        write_unlock();
        return -1;
    }

    char line[1024];
    char found_message = 0;
    while (fgets(line, sizeof(line), og_fp)) {
        long id;
        if (sscanf(line, "%ld/", &id) == 1 && id == message_number) {
            fprintf(temp_fp, "%ld/%s/%s\n", message_number, username, new_message);
            found_message = 1;
        } else
            fputs(line, temp_fp);
    }

    fflush(temp_fp);
    fclose(og_fp);
    fclose(temp_fp);

    if (global_config.fdebug)
        printf("Replace debug end\n");

    if (found_message) {
        if (rename(temp_filename, global_config.bbfile) != 0) {
            perror("bb_replace: file rename failed");
            remove(temp_filename);
            write_unlock();
            return -3;
        }
    } else {
        remove(temp_filename);
        write_unlock();
        return -2;
    }

    write_unlock();
    return 0;
}
