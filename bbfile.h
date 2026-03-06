#ifndef BBFILE_H
#define BBFILE_H

int bb_init(const char *bbfile_path);
int bb_write(const char *poster, const char *message);
int bb_read(const long message_number, char **message);
int bb_replace(const char *username, const long message_number, const char *new_message);

#endif
