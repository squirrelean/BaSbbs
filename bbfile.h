#ifndef BBFILE_H
#define BBFILE_H

typedef struct {
    long file_offset;
    char *backup;
    long previous_id;
    int status;
} BbMeta;

int bb_init();
BbMeta bb_write(const char *poster, const char *message);
int bb_read(const long message_number, char **message);
BbMeta bb_replace(const char *username, const long message_number, const char *new_message);
void bb_rollback(BbMeta meta);
void delete_backup(BbMeta meta);

#endif
