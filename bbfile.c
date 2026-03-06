#include <stdio.h>
#include <string.h>

long global_next_id = 0;
static char global_bbfile_path[1024];

int bb_init(const char *bbfile_path)
{
    FILE *fp = fopen(bbfile_path, "a+");
    if (!fp)
        return -1;

    long highest_id = 0;
    char line[1024];

    while (fgets(line, sizeof(line), fp)) {
        int current_id;
        if (sscanf(line, "%d/", &current_id) == 1) {
            if (current_id > highest_id)
                highest_id = current_id;
        }
    }

    global_next_id = highest_id + 1;
    strncpy(global_bbfile_path, bbfile_path, sizeof(global_bbfile_path));

    fclose(fp);
    return 0;
}

int bb_write(const char *poster, const char *message)
{
    FILE *fp = fopen(global_bbfile_path, "a");
    if (!fp)
        return -1;

    fprintf(fp, "%ld/%s/%s\n", global_next_id, poster, message);
    global_next_id++;

    fflush(fp);
    fclose(fp);

    return global_next_id - 1;
}

int bb_read(const long message_number, char **message)
{
    FILE *fp = fopen(global_bbfile_path, "r");
    if (!fp)
        return -1;

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

    if (!*message)
        return -2;
    return 0;
}

int bb_replace(const char *username, const long message_number, const char *new_message)
{
    FILE *og_fp = fopen(global_bbfile_path, "r");
    if (!og_fp)
        return -1;

    char temp_filename[strlen(global_bbfile_path) + 8];
    snprintf(temp_filename, sizeof(temp_filename), "%s_tmp", global_bbfile_path);
    FILE *temp_fp = fopen(temp_filename, "w");

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

    fflush(og_fp);
    fflush(temp_fp);
    fclose(og_fp);
    fclose(temp_fp);

    if (found_message) {
        if (rename(temp_filename, global_bbfile_path) != 0) {
            perror("bb_replace: file rename failed");
            remove(temp_filename);
            return -3;
        }
    } else {
        remove(temp_filename);
        return -2;
    }

    return 0;
}
