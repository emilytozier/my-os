#include "fs.h"
#include "string.h"
#include "kheap.h"

#define MAX_FILES 100

typedef struct {
    char name[MAX_NAME_LENGTH];
    char* data;
    size_t size;
    size_t capacity;
    int is_directory;
    int parent_index;
    int in_use;
    Permissions perms;      
    int owner_id;           
    int group_id;          
} FileEntry;

static Permissions default_file_perms = {1, 1, 0, 1, 0, 0, 1, 0, 0};  // rw-r--r--
static Permissions default_dir_perms  = {1, 1, 1, 1, 0, 1, 1, 0, 1};  // rwxr-xr-x

static FileEntry files[MAX_FILES];
static int file_count = 0;
static int current_dir = 0;

void fs_init(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        files[i].in_use = 0;
    }

 
    files[0].in_use = 1;
    files[0].is_directory = 1;
    strcpy(files[0].name, "/");
    files[0].parent_index = -1;
    files[0].data = NULL;
    files[0].size = 0;
    files[0].capacity = 0;

    file_count = 1;
    current_dir = 0;

    // Тестовые данные
    fs_create_file("readme.txt", 0);
    fs_write_file("readme.txt", "Welcome to MyOS!", 0);

    fs_create_dir("bin", 0);
    fs_create_dir("home", 0);

    int home = fs_find_dir("home");
    if (home >= 0) {
        fs_create_file("hello.txt", home);
        fs_write_file("hello.txt", "Hello from home directory!", home);
    }

}

int fs_rename_entry(const char* old_name, const char* new_name, int parent_dir) {
    int idx = fs_find_entry(old_name, parent_dir);
    if (idx == -1) return -1;
    if (fs_find_entry(new_name, parent_dir) != -1) return -2;

    strncpy(files[idx].name, new_name, MAX_NAME_LENGTH - 1);
    files[idx].name[MAX_NAME_LENGTH - 1] = '\0';
    return 0;
}

static int fs_match_pattern(const char* name, const char* pattern) {
    return strstr(name, pattern) != NULL;
}

int fs_mkdir_p(const char* path) {
    if (!path || !*path) return -1;

    int cur = (path[0] == '/') ? 0 : current_dir;
    const char* p = path;
    if (*p == '/') p++;

    while (*p) {      
        while (*p == '/') p++;
        if (*p == '\0') break;

        char name[MAX_NAME_LENGTH];
        int n = 0;
        while (*p && *p != '/' && n < MAX_NAME_LENGTH - 1) {
            name[n++] = *p++;
        }
        name[n] = '\0';

        if (n == 0) continue;
        if (strcmp(name, ".") == 0) continue;
        if (strcmp(name, "..") == 0) {
            if (files[cur].parent_index != -1) cur = files[cur].parent_index;
            continue;
        }

        int next = fs_find_entry(name, cur);
        if (next != -1) {
            if (!files[next].is_directory) {
                return -1;  
            }
            cur = next;
            continue;
        }

        int r = fs_create_dir(name, cur);
        if (r < 0) return -1;
        cur = r;
    }

    return 0;
}


int fs_copy_file(const char* src_name, const char* dst_name, int parent_dir) {
    int src_idx = fs_find_entry(src_name, parent_dir);
    if (src_idx == -1) return -1;

    if (files[src_idx].is_directory) return -3;

    int dst_idx = fs_find_entry(dst_name, parent_dir);
    if (dst_idx != -1) {
        if (files[dst_idx].is_directory) return -4;      
    } else {
        dst_idx = fs_create_file(dst_name, parent_dir);
        if (dst_idx < 0) return -6;
    }

    size_t data_len = files[src_idx].size;

    if (files[dst_idx].data != NULL) {
        kfree(files[dst_idx].data);
        files[dst_idx].data = NULL;
        files[dst_idx].capacity = 0;
    }

    if (data_len > 0) {
        files[dst_idx].data = (char*)kmalloc(data_len + 1);
        if (files[dst_idx].data == NULL) {
            files[dst_idx].size = 0;
            files[dst_idx].capacity = 0;
            return -5;
        }
        memcpy(files[dst_idx].data, files[src_idx].data, data_len);
        files[dst_idx].data[data_len] = '\0';
        files[dst_idx].size = data_len;
        files[dst_idx].capacity = data_len + 1;
    } else {
        files[dst_idx].size = 0;
        files[dst_idx].capacity = 0;
    }

    files[dst_idx].perms = files[src_idx].perms;

    return 0;
}

static void fs_find_recursive(int dir_index, const char* pattern,
                              int* count) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].in_use && files[i].parent_index == dir_index) {
               if (fs_match_pattern(files[i].name, pattern)){
                char path[256];           
                int indices[32], cnt = 0;
                int x = i;
                while (x != -1 && x != 0 && cnt < 32) {
                    indices[cnt++] = x;
                    x = files[x].parent_index;
                }
                char* ptr = path;
                *ptr++ = '/';
                for (int k = cnt - 1; k >= 0; k--) {
                    const char* nm = files[indices[k]].name;
                    while (*nm) *ptr++ = *nm++;
                    if (k > 0) *ptr++ = '/';
                }
                *ptr = '\0';

                kprintf("%s\n", path);
                (*count)++;
            }

            if (files[i].is_directory) {
                fs_find_recursive(i, pattern, count);
            }
        }
    }
}

int fs_find_files(const char* start_path, const char* pattern) {
    int start = current_dir;
    if (start_path && *start_path) {
        if (fs_resolve_path(start_path, &start) != 0) return -1;
    }
    int count = 0;
    fs_find_recursive(start, pattern, &count);
    return count;
}

int fs_resolve_path(const char* path, int* out_index) {
    if (!path || !*path) return -1;

    int cur = (path[0] == '/') ? 0 : current_dir;
    const char* p = path;

    if (*p == '/') p++; 

    while (*p) {
        while (*p == '/') p++;
        if (*p == '\0') break;

        char name[MAX_NAME_LENGTH];
        int n = 0;
        while (*p && *p != '/' && n < MAX_NAME_LENGTH - 1) {
            name[n++] = *p++;
        }
        name[n] = '\0';

        if (n == 0) continue;

        if (strcmp(name, ".") == 0) {
            continue;
        }
        if (strcmp(name, "..") == 0) {
            if (files[cur].parent_index != -1) {
                cur = files[cur].parent_index;
            }
            continue;
        }

        int next = fs_find_entry(name, cur);
        if (next == -1) return -1;
        if (!files[next].is_directory) return -1;

        cur = next;
    }

    *out_index = cur;
    return 0;
}

FileInfo fs_get_info(const char* name, int parent_dir) {
    FileInfo info;
    memset(&info, 0, sizeof(FileInfo));

    int file_index = fs_find_entry(name, parent_dir);
    if (file_index == -1) {
        info.exists = 0;
        return info;
    }

    info.exists = 1;
    info.is_directory = files[file_index].is_directory;
    info.size = files[file_index].size;
    strlcpy(info.name, files[file_index].name, MAX_NAME_LENGTH);
    info.perms = files[file_index].perms;
    info.owner_id = files[file_index].owner_id;
    info.group_id = files[file_index].group_id;

    return info;
}

static int find_free_slot(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].in_use) return i;
    }
    return -1;
}

int fs_set_permissions(const char* name, int parent_dir, Permissions perms) {
    int idx = fs_find_entry(name, parent_dir);
    if (idx == -1) return -1;
    files[idx].perms = perms;
    return 0;
}

Permissions fs_get_permissions(const char* name, int parent_dir) {
    int idx = fs_find_entry(name, parent_dir);
    Permissions empty = {0};
    if (idx == -1) return empty;
    return files[idx].perms;
}

int fs_find_entry(const char* name, int dir_index) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].in_use &&
            files[i].parent_index == dir_index &&
            strcmp(files[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int fs_find_dir(const char* name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].in_use &&
            files[i].is_directory &&
            strcmp(files[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int fs_create_file(const char* name, int parent_dir) {
    if (parent_dir < 0 || parent_dir >= MAX_FILES || !files[parent_dir].is_directory) {
        return -1;
    }
    if (fs_find_entry(name, parent_dir) != -1) {
        return -2;
    }
    int slot = find_free_slot();
    if (slot == -1) return -3;

    files[slot].in_use = 1;
    files[slot].is_directory = 0;
    strncpy(files[slot].name, name, MAX_NAME_LENGTH - 1);
    files[slot].name[MAX_NAME_LENGTH - 1] = '\0';
    files[slot].parent_index = parent_dir;
    files[slot].data = NULL;
    files[slot].size = 0;
    files[slot].capacity = 0;
    files[slot].perms = default_file_perms;
    files[slot].owner_id = 0;
    files[slot].group_id = 0;

    file_count++;
    return slot;
}

int fs_create_dir(const char* name, int parent_dir) {
    if (parent_dir < 0 || parent_dir >= MAX_FILES || !files[parent_dir].is_directory) {
        return -1;
    }
    if (fs_find_entry(name, parent_dir) != -1) {
        return -2;
    }
    int slot = find_free_slot();
    if (slot == -1) return -3;

    files[slot].in_use = 1;
    files[slot].is_directory = 1;
    strncpy(files[slot].name, name, MAX_NAME_LENGTH - 1);
    files[slot].name[MAX_NAME_LENGTH - 1] = '\0';
    files[slot].parent_index = parent_dir;
    files[slot].data = NULL;
    files[slot].size = 0;
    files[slot].capacity = 0;
    files[slot].perms = default_dir_perms;
    files[slot].owner_id = 0;
    files[slot].group_id = 0;
    file_count++;
    return slot;
}

static void fs_tree_recursive(int dir_index, int depth, int show_hidden) {
    if (depth > 10) return;

    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].in_use && files[i].parent_index == dir_index) {
            if (!show_hidden && files[i].name[0] == '.') continue;

            // Отступы
            for (int j = 0; j < depth; j++) {
                kprintf("  ");
            }

            kprintf("|-- %s", files[i].name);

            if (files[i].is_directory) {
                kprintf("/\n");
                fs_tree_recursive(i, depth + 1, show_hidden);
            } else {
                kprintf(" (%d bytes)\n", files[i].size);
            }
        }
    }
}

int fs_tree(const char* path, int show_hidden) {
    int start = current_dir;
    if (path && *path) {
        if (fs_resolve_path(path, &start) != 0) return -1;
    }

    char buf[256];
    fs_get_current_path(buf, sizeof(buf)); 
    kprintf("%s\n", buf);

    fs_tree_recursive(start, 0, show_hidden);
    return 0;
}

int fs_write_file(const char* name, const char* data, int parent_dir) {
    int file_index = fs_find_entry(name, parent_dir);
    if (file_index == -1) {
        file_index = fs_create_file(name, parent_dir);
        if (file_index < 0) return file_index;
    }

    if (files[file_index].is_directory) {
        return -4;
    }

    size_t data_len = strlen(data);

    if (files[file_index].data != NULL) {
        kfree(files[file_index].data);
        files[file_index].data = NULL;
        files[file_index].capacity = 0;
    }

    files[file_index].data = (char*)kmalloc(data_len + 1);
    if (files[file_index].data == NULL) {
        files[file_index].size = 0;
        files[file_index].capacity = 0;
        return -6; 
    }

 
    memcpy(files[file_index].data, data, data_len);
    files[file_index].data[data_len] = '\0';
    files[file_index].size = data_len;
    files[file_index].capacity = data_len + 1;

    return 0;
}

const char* fs_read_file(const char* name, int parent_dir) {
    int file_index = fs_find_entry(name, parent_dir);
    if (file_index == -1) return NULL;
    if (files[file_index].is_directory) return NULL;
    return files[file_index].data;
}

size_t fs_file_size(const char* name, int parent_dir) {
    int file_index = fs_find_entry(name, parent_dir);
    if (file_index == -1 || files[file_index].is_directory) return 0;
    return files[file_index].size;
}

int fs_delete_entry(const char* name, int parent_dir) {
    int file_index = fs_find_entry(name, parent_dir);
    if (file_index == -1) return -1;

    if (files[file_index].is_directory) {
        for (int i = 0; i < MAX_FILES; i++) {
            if (files[i].in_use && files[i].parent_index == file_index) {
                return -5; 
            }
        }
    }

    if (files[file_index].data != NULL) {
        kfree(files[file_index].data);
        files[file_index].data = NULL;
    }
    files[file_index].capacity = 0;

    files[file_index].in_use = 0;
    file_count--;
    return 0;
}
void fs_list_directory(int dir_index, char* buffer, size_t buffer_size) {
    if (dir_index < 0 || !files[dir_index].is_directory) {
        buffer[0] = '\0';
        return;
    }

    char temp[1024];
    char* ptr = temp;
    char* end = temp + sizeof(temp) - 1;

    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].in_use && files[i].parent_index == dir_index) {
            const char* name = files[i].name;
            while (*name && ptr < end) *ptr++ = *name++;

            if (files[i].is_directory && ptr < end) {
                *ptr++ = '/';
            }
            if (ptr < end) *ptr++ = ' ';
            if (ptr < end) *ptr++ = ' ';
        }
    }

  
    if (ptr >= temp + 2) {
        ptr -= 2;
    }
    *ptr = '\0';

    strlcpy(buffer, temp, buffer_size);
}

int fs_get_current_dir(void) {
    return current_dir;
}

int fs_change_dir(const char* path) {
    int idx;
    if (fs_resolve_path(path, &idx) != 0) {
        return -2;
    }
    current_dir = idx;
    return 0;
}

void fs_get_current_path(char* buffer, size_t buffer_size) {
    if (current_dir == 0) {
        strlcpy(buffer, "/", buffer_size);
        return;
    }

    int indices[50];
    int count = 0;
    int index = current_dir;

    while (index != -1 && index != 0 && count < 50) {
        indices[count++] = index;
        index = files[index].parent_index;
    }

    char temp[256];
    char* ptr = temp;
    char* end = temp + sizeof(temp) - 1;

    *ptr++ = '/';

    for (int i = count - 1; i >= 0; i--) {
        const char* name = files[indices[i]].name;
        while (*name && ptr < end) *ptr++ = *name++;
        if (i > 0 && ptr < end) *ptr++ = '/';
    }
    *ptr = '\0';

    strlcpy(buffer, temp, buffer_size);
}
