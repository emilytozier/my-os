#include "shell.h"
#include "fs.h"
#include "screen.h"
#include "serial.h"
#include "string.h"
#include "memory.h"
#include "kheap.h"
#include "process.h"

#define MAX_INPUT_LENGTH 256
#define MAX_HISTORY 20

static char history[MAX_HISTORY][MAX_INPUT_LENGTH];
static int history_count = 0;
static int history_index = -1;
static char saved_input[MAX_INPUT_LENGTH];

static char input_buffer[MAX_INPUT_LENGTH];
static int input_pos = 0;

static void add_to_history(const char* cmd);
static void history_up(void);
static void history_down(void);
static void restore_history_command(void);
static void clear_input_line(void);
static void show_history(void);
static void shell_execute_command(const char* cmd);

static void clear_input_line(void) {
    for (int i = 0; i < input_pos + 6; i++) {
        kprintf("\b \b");
    }
    input_pos = 0;
}

static void restore_history_command(void) {
    clear_input_line();

    if (history_index == -1) {
        strcpy(input_buffer, saved_input);
    } else {
        strcpy(input_buffer, history[history_index]);
    }
    input_pos = strlen(input_buffer);
    kprintf("MyOS> %s", input_buffer);
}

static void add_to_history(const char* cmd) {
    if (strlen(cmd) == 0) return;
    if (history_count > 0 && strcmp(history[0], cmd) == 0) return;

    for (int i = MAX_HISTORY - 1; i > 0; i--) {
        strcpy(history[i], history[i - 1]);
    }
    strcpy(history[0], cmd);

    if (history_count < MAX_HISTORY) history_count++;

    history_index = -1;
    saved_input[0] = '\0';
}

static void history_up(void) {
    if (history_index < history_count - 1) {
        if (history_index == -1) {
            input_buffer[input_pos] = '\0';
            strcpy(saved_input, input_buffer);
        }
        history_index++;
        restore_history_command();
    }
}

static void history_down(void) {
    if (history_index > -1) {
        history_index--;
        restore_history_command();
    }
}


static void show_history(void) {
    kprintf("\nCommand history (%d/%d):\n", history_count, MAX_HISTORY);
    for (int i = 0; i < history_count; i++) {
        kprintf("  %d: %s\n", i + 1, history[i]);
    }
}

static void handle_echo_redirect(const char* cmd) {
  
    const char* rest = cmd + 5;

    const char* p = rest;
    const char* gt = NULL;
    while (*p) {
        if (p[0] == ' ' && p[1] == '>' && p[2] == ' ') {
            gt = p;
            break;
        }
        p++;
    }

    if (!gt) {       
        kprintf("%s\n", rest);
        return;
    }

    char text[256];
    int len = (int)(gt - rest);
    if (len > 255) len = 255;
    for (int i = 0; i < len; i++) text[i] = rest[i];
    text[len] = '\0';

    const char* filename = gt + 3;
    if (*filename == '\0') {
        kprintf("echo: missing filename\n");
        return;
    }

    int r = fs_write_file(filename, text, fs_get_current_dir());
    if (r != 0) {
        kprintf("echo: cannot write to %s\n", filename);
    }
}



static void shell_execute_command(const char* cmd) {
    if (strcmp(cmd, "history") == 0) {
        show_history();
        return;
    }

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        kprintf("Available commands:\n");
        kprintf("  help      - show help\n");
        kprintf("  clear/cls - clear screen\n");
        kprintf("  memory    - memory info\n");
        kprintf("  echo      - print text\n");
        kprintf("  reboot    - reboot\n");
        kprintf("  history   - command history\n");
        kprintf("\nFile system commands:\n");
        kprintf("  ls              - list directory\n");
        kprintf("  pwd             - print working directory\n");
        kprintf("  cd <dir>        - change directory\n");
        kprintf("  cd ..           - go up\n");
        kprintf("  mkdir <dir>     - create directory\n");
        kprintf("  ps              - list processes\n");
        kprintf("  run <name>      - create process (e.g. run process1)\n");
        kprintf("  kill <pid>      - kill process by pid\n");
        kprintf("  touch <file>    - create empty file\n");
        kprintf("  cat <file>      - show file contents\n");
        kprintf("  rm <file>       - remove file/dir\n");
        kprintf("  echo text > file - write to file\n");
        kprintf("  stat <file>     - show file info\n");
        kprintf("  chmod <mode> <file> - change permissions\n");
        kprintf("  find <path> <pat> - find files by pattern\n");
        kprintf("  tree            - show directory tree\n");
        kprintf("  tree -a         - show tree including hidden\n");
        kprintf("  tree <path>     - show tree of path\n");
        kprintf("  mv <old> <new>  - rename file\n");
        kprintf("  cp <src> <dst>  - copy file\n");
       kprintf("  sleep <ms>      - sleep in milliseconds (1000 = 1s)\n");
      kprintf("  uptime          - time since boot\n"); 
          kprintf("\nMemory:\n");
    kprintf("  memory           - memory info\n");
       kprintf("\nUP/DOWN arrows for history\n");
       return;
    }

    if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "cls") == 0) {
        clear_screen();
        return;
    }

    if (strcmp(cmd, "memory") == 0) {
      kprintf("Memory: 4 MB total\n");
      kprintf("Used: %d KB\n", memory_used() / 1024);
      kprintf("Free: %d KB\n", memory_free() / 1024);
      return;
    }

    if (strcmp(cmd, "reboot") == 0) {
        kprintf("Reboot command (not implemented yet)\n");
        return;
    }

    if (starts_with(cmd, "echo ")) {
        handle_echo_redirect(cmd);
        return;
    }    
      
    if (strcmp(cmd, "ps") == 0) {
        print_process_list();
        return;
    }

if (starts_with(cmd, "run ")) {
    const char* name = cmd + 4;
    
    if (strlen(name) == 0) {
        kprintf("Usage: run <name>\n");
        kprintf("Example: run process1\n");
        return;
    }
  
    if (strlen(name) >= 32) {
        kprintf("run: name too long (max 31 chars)\n");
        return;
    }
    
    extern void dummy_process(void);
    Process* p = process_create_kernel(name, dummy_process);
    
    if (p) {
        p->time_slice = 20;
        enqueue_ready(p);
        kprintf("Started '%s' with pid %d\n", name, p->pid);
    } else {
        kprintf("run: failed to create process (out of memory or too many processes)\n");
    }
    return;
}


if (starts_with(cmd, "sleep ")) {
    const char* args = cmd + 6;
    while (*args == ' ') args++;
   
    int pid = 0;
    while (*args >= '0' && *args <= '9') {
        pid = pid * 10 + (*args - '0');
        args++;
    }
    while (*args == ' ') args++;
    
    
    int ticks = 0;
    while (*args >= '0' && *args <= '9') {
        ticks = ticks * 10 + (*args - '0');
        args++;
    }
    
    if (ticks <= 0) {
        kprintf("Usage: sleep <pid> <ticks> (100 ticks = 1 sec)\n");
        kprintf("Example: sleep 2 300\n");
        return;
    }
    
    int r = process_sleep_pid(pid, ticks);
    switch (r) {
        case 0:  kprintf("pid %d sleeping %d ticks\n", pid, ticks); break;
        case -1: kprintf("sleep: invalid pid\n"); break;
        case -2: kprintf("sleep: no such process\n"); break;
        case -3: kprintf("sleep: cannot sleep idle\n"); break;
        case -4: kprintf("sleep: cannot sleep current process\n"); break;
        case -5: kprintf("sleep: ticks must be > 0\n"); break;
    }
    return;
}

if (starts_with(cmd, "kill ")) {
    const char* arg = cmd + 5;
    
   
    while (*arg == ' ') arg++;
    
    if (*arg == '\0') {
        kprintf("Usage: kill <pid>\n");
        kprintf("Example: kill 2\n");
        return;
    }
    
    int pid = 0;
    while (*arg >= '0' && *arg <= '9') {
        pid = pid * 10 + (*arg - '0');
        arg++;
    }
    
    int r = process_kill(pid);
    switch (r) {
        case 0:
            kprintf("Killed pid %d\n", pid);
            break;
        case -1:
            kprintf("kill: invalid pid\n");
            break;
        case -2:
            kprintf("kill: no such process (pid %d)\n", pid);
            break;
        case -3:
            kprintf("kill: cannot kill idle\n");
            break;
        case -4:
            kprintf("kill: cannot kill current process\n");
            break;
        default:
            kprintf("kill: unknown error %d\n", r);
            break;
    }
    return;
}


if (starts_with(cmd, "sleep ")) {
    const char* arg = cmd + 6;
    while (*arg == ' ') arg++;

    int ms = 0;
    while (*arg >= '0' && *arg <= '9') {
        ms = ms * 10 + (*arg - '0');
        arg++;
    }

    if (ms <= 0) {
        kprintf("Usage: sleep <ms> (1000 = 1 sec)\n");
        return;
    }

    kprintf("Sleeping %d ms...\n", ms);
    process_sleep_ms(ms);
    kprintf("Awake!\n");
    return;
}


if (strcmp(cmd, "uptime") == 0) {
    uint32_t ms = uptime_ms();
    uint32_t sec = ms / 1000;
    uint32_t min = sec / 60;
    uint32_t hr  = min / 60;

    kprintf("Uptime: %d ms", ms);
    kprintf(" (%d:%d:%d)\n",
            hr,
            min % 60,
            sec % 60);
    return;
}
       
    if (starts_with(cmd, "mv ")) {
        const char* args = cmd + 3;
        const char* p = args;
        while (*p && *p != ' ') p++;
        if (*p == '\0') {
            kprintf("Usage: mv <old> <new>\n");
            return;
        }
        char old_name[128], new_name[128];
        int n = (int)(p - args);
        if (n > 127) n = 127;
        for (int i = 0; i < n; i++) old_name[i] = args[i];
        old_name[n] = '\0';
        strlcpy(new_name, p + 1, sizeof(new_name));

        int r = fs_rename_entry(old_name, new_name, fs_get_current_dir());
        if (r == -1) kprintf("mv: no such file: %s\n", old_name);
        else if (r == -2) kprintf("mv: target exists: %s\n", new_name);
        return;
    }

      
    if (starts_with(cmd, "cp ")) {
        const char* args = cmd + 3;

        const char* p = args;
        while (*p && *p != ' ') p++;

        if (*p == '\0') {
            kprintf("Usage: cp <source> <dest>\n");
            kprintf("Example: cp a.txt b.txt\n");
            return;
        }

        char src_name[128];
        int n = (int)(p - args);
        if (n > 127) n = 127;
        for (int i = 0; i < n; i++) src_name[i] = args[i];
        src_name[n] = '\0';

        const char* dst_name = p + 1;
        if (*dst_name == '\0') {
            kprintf("cp: missing destination\n");
            return;
        }

        int r = fs_copy_file(src_name, dst_name, fs_get_current_dir());

        switch (r) {
            case 0:
                kprintf("'%s' -> '%s'\n", src_name, dst_name);
                break;
            case -1:
                kprintf("cp: cannot stat '%s': No such file\n", src_name);
                break;
            case -2:
                kprintf("cp: '%s' already exists\n", dst_name);
                break;
            case -3:
                kprintf("cp: '%s' is a directory (use -r)\n", src_name);
                break;
            case -4:
                kprintf("cp: '%s' is a directory\n", dst_name);
                break;
            case -5:
                kprintf("cp: out of memory\n");
                break;
            case -6:
                kprintf("cp: cannot create '%s'\n", dst_name);
                break;
            default:
                kprintf("cp: unknown error %d\n", r);
                break;
        }
        return;
    }

  
    if (strcmp(cmd, "tree") == 0) {
        fs_tree(NULL, 0);
        return;
    }
    if (strcmp(cmd, "tree -a") == 0) {
        fs_tree(NULL, 1);
        return;
    }
    if (starts_with(cmd, "tree ")) {
        fs_tree(cmd + 5, 0);
        return;
    }

    if (starts_with(cmd, "find ")) {
        const char* args = cmd + 5;
        const char* p = args;
        while (*p && *p != ' ') p++;
        if (*p == '\0') {
            kprintf("Usage: find <path> <pattern>\n");
            return;
        }
        char path[128];
        int n = (int)(p - args);
        if (n > 127) n = 127;
        for (int i = 0; i < n; i++) path[i] = args[i];
        path[n] = '\0';
        const char* pattern = p + 1;

        int found = fs_find_files(path, pattern);
        if (found <= 0) kprintf("No files found matching '%s'\n", pattern);
        return;
    }

 
    if (strcmp(cmd, "ls") == 0) {
        char buffer[1024];
        fs_list_directory(fs_get_current_dir(), buffer, sizeof(buffer));
        kprintf("%s\n", buffer);
        return;
    }

    
    if (strcmp(cmd, "pwd") == 0) {
        char path[256];
        fs_get_current_path(path, sizeof(path));
        kprintf("%s\n", path);
        return;
    }

      
    if (strcmp(cmd, "cd") == 0) {
        fs_change_dir("/");
        return;
    }
    if (starts_with(cmd, "cd ")) {
        const char* dir = cmd + 3;
        int r = fs_change_dir(dir);
        if (r == -2) kprintf("cd: no such directory: %s\n", dir);
        return;
    }

    if (starts_with(cmd, "mkdir ")) {
        const char* path = cmd + 6;
        int r = fs_mkdir_p(path);
        if (r == -1) kprintf("mkdir: cannot create '%s'\n", path);
        return;
    }
   
    if (starts_with(cmd, "touch ")) {
        const char* filename = cmd + 6;
        int r = fs_create_file(filename, fs_get_current_dir());
        if (r == -2) kprintf("touch: already exists: %s\n", filename);
        else if (r < 0) kprintf("touch: failed\n");
        return;
    }

    
    if (starts_with(cmd, "cat ")) {
        const char* filename = cmd + 4;
        const char* content = fs_read_file(filename, fs_get_current_dir());
        if (!content) kprintf("cat: no such file: %s\n", filename);
        else kprintf("%s\n", content);
        return;
    }

   
    if (starts_with(cmd, "rm ")) {
        const char* filename = cmd + 3;
        int r = fs_delete_entry(filename, fs_get_current_dir());
        if (r == -1) kprintf("rm: no such file: %s\n", filename);
        else if (r == -5) kprintf("rm: directory not empty: %s\n", filename);
        else if (r != 0) kprintf("rm: failed\n");
        return;
    }

    if (starts_with(cmd, "stat ")) {
      const char* filename = cmd + 5;
      FileInfo info = fs_get_info(filename, fs_get_current_dir());

      if (!info.exists) {
          kprintf("stat: no such file: %s\n", filename);
          return;
      }

      kprintf("File: %s\n", info.name);
      kprintf("Type: %s\n", info.is_directory ? "directory" : "regular file");
      kprintf("Size: %d bytes\n", info.size);
      kprintf("Permissions: ");
      kprintf("%c", info.perms.owner_read    ? 'r' : '-');
      kprintf("%c", info.perms.owner_write   ? 'w' : '-');
      kprintf("%c", info.perms.owner_execute ? 'x' : '-');
      kprintf("%c", info.perms.group_read    ? 'r' : '-');
      kprintf("%c", info.perms.group_write   ? 'w' : '-');
      kprintf("%c", info.perms.group_execute ? 'x' : '-');
      kprintf("%c", info.perms.others_read   ? 'r' : '-');
      kprintf("%c", info.perms.others_write  ? 'w' : '-');
      kprintf("%c", info.perms.others_execute? 'x' : '-');
      kprintf("\n");
      return;
    }

   
    if (starts_with(cmd, "chmod ")) {
        const char* args = cmd + 6;

       
        const char* p = args;
        while (*p && *p != ' ') p++;

        if (*p == '\0') {
            kprintf("Usage: chmod <octal> <file>\n");
            kprintf("Example: chmod 755 script.sh\n");
            return;
        }

        int len = (int)(p - args);
        if (len != 3) {
            kprintf("chmod: mode must be 3 octal digits (e.g. 644, 755)\n");
            return;
        }

        int digits[3];
        for (int i = 0; i < 3; i++) {
            char ch = args[i];
            if (ch < '0' || ch > '7') {
                kprintf("chmod: invalid digit '%c'\n", ch);
                return;
            }
            digits[i] = ch - '0';
        }

        const char* filename = p + 1;
        if (*filename == '\0') {
            kprintf("chmod: missing filename\n");
            return;
        }

        Permissions perms;
        perms.owner_read     = (digits[0] & 4) ? 1 : 0;
        perms.owner_write    = (digits[0] & 2) ? 1 : 0;
        perms.owner_execute  = (digits[0] & 1) ? 1 : 0;
        perms.group_read     = (digits[1] & 4) ? 1 : 0;
        perms.group_write    = (digits[1] & 2) ? 1 : 0;
        perms.group_execute  = (digits[1] & 1) ? 1 : 0;
        perms.others_read    = (digits[2] & 4) ? 1 : 0;
        perms.others_write   = (digits[2] & 2) ? 1 : 0;
        perms.others_execute = (digits[2] & 1) ? 1 : 0;

        int r = fs_set_permissions(filename, fs_get_current_dir(), perms);
        if (r == -1) kprintf("chmod: no such file: %s\n", filename);
        return;
    }
    kprintf("Unknown command: %s\n", cmd);
    kprintf("Type 'help' for commands\n");
}

void shell_process_input(char c) {
    int code = (unsigned char)c;

    if (code == 0x80) { history_up(); return; }
    if (code == 0x81) { history_down(); return; }

    if (c == '\n' || c == '\r') {
        input_buffer[input_pos] = '\0';

        if (input_pos > 0) {
            add_to_history(input_buffer);

            int has_semicolon = 0;
            for (int i = 0; i < input_pos; i++) {
                if (input_buffer[i] == ';') { has_semicolon = 1; break; }
            }

            kprintf("\n");

            if (has_semicolon) {               
                char tmp[MAX_INPUT_LENGTH];
                strcpy(tmp, input_buffer);
                int start = 0;
                for (int i = 0; ; i++) {
                    if (tmp[i] == ';' || tmp[i] == '\0') {
                        char cmd[MAX_INPUT_LENGTH];
                        int clen = i - start;
                        int j = start, k = 0;
                        while (j < start + clen && tmp[j] == ' ') j++;
                        while (j < start + clen && k < MAX_INPUT_LENGTH - 1) {
                            cmd[k++] = tmp[j++];
                        }
                        while (k > 0 && cmd[k-1] == ' ') k--;
                        cmd[k] = '\0';
                        if (k > 0) shell_execute_command(cmd);
                        start = i + 1;
                        if (tmp[i] == '\0') break;
                    }
                }
            } else {
                shell_execute_command(input_buffer);
            }
        } else {
            kprintf("\n");
        }

        input_pos = 0;
        input_buffer[0] = '\0';
        history_index = -1;

        kprintf("MyOS> ");
    } else if (c == '\b' || c == 127) {
        if (input_pos > 0) {
            input_pos--;
            kprintf("\b \b");
            if (history_index != -1) {
                history_index = -1;
                saved_input[0] = '\0';
            }
        }
    } else if (c >= 32 && c <= 126 && input_pos < MAX_INPUT_LENGTH - 1) {
        input_buffer[input_pos++] = c;
        print_char(c);
        if (history_index != -1) {
            history_index = -1;
            saved_input[0] = '\0';
        }
    }
}

void shell_init(void) {
    clear_screen();
    kprintf("=== MyOS Microkernel ===\n");
    kprintf("Type 'help' for available commands\n\n");

    input_buffer[0] = '\0';
    input_pos = 0;
    history_count = 0;
    history_index = -1;
    saved_input[0] = '\0';

    add_to_history("help");
    add_to_history("clear");

    kprintf("MyOS> ");
}

void shell_run(void) {
    
}
