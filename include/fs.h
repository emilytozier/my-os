#ifndef FS_H
#define FS_H

#include <stddef.h>

#define MAX_NAME_LENGTH 32
#define MAX_PATH_LENGTH 256

typedef struct {
    unsigned int owner_read    : 1;
    unsigned int owner_write   : 1;
    unsigned int owner_execute : 1;
    unsigned int group_read    : 1;
    unsigned int group_write   : 1;
    unsigned int group_execute : 1;
    unsigned int others_read   : 1;
    unsigned int others_write  : 1;
    unsigned int others_execute: 1;
} Permissions;

typedef struct {
    int exists;
    int is_directory;
    size_t size;
    char name[MAX_NAME_LENGTH];
    Permissions perms;
    int owner_id;
    int group_id;
} FileInfo;

void fs_init(void);
int fs_rename_entry(const char* old_name, const char* new_name, int parent_dir);

int fs_copy_file(const char* src_name, const char* dst_name, int parent_dir);

int fs_tree(const char* path, int show_hidden);

int fs_mkdir_p(const char* path);

int fs_create_file(const char* name, int parent_dir);

int fs_find_files(const char* start_path, const char* pattern);

int fs_create_dir(const char* name, int parent_dir);

int fs_write_file(const char* name, const char* data, int parent_dir);

int fs_resolve_path(const char* path, int* out_index);

const char* fs_read_file(const char* name, int parent_dir);

size_t fs_file_size(const char* name, int parent_dir);

int fs_delete_entry(const char* name, int parent_dir);

void fs_list_directory(int dir_index, char* buffer, size_t buffer_size);

int fs_find_entry(const char* name, int dir_index);

int fs_find_dir(const char* name);

int fs_get_current_dir(void);

int fs_change_dir(const char* name);

void fs_get_current_path(char* buffer, size_t buffer_size);

// Права: set/get
int fs_set_permissions(const char* name, int parent_dir, Permissions perms);

Permissions fs_get_permissions(const char* name, int parent_dir);


FileInfo fs_get_info(const char* name, int parent_dir);

#endif
