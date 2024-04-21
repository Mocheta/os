#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <bits/getopt_core.h>
#include <sys/wait.h>

#define MAX_DIRS 10
#define MAX_PATH_LEN 1024

bool file_changed(const char *filepath) {
    static struct stat last_stat;
    struct stat current_stat;

    if (stat(filepath, &current_stat) == -1) {
        return true;
    }

    if (last_stat.st_dev != current_stat.st_dev || last_stat.st_ino != current_stat.st_ino ||
        last_stat.st_mtime != current_stat.st_mtime || last_stat.st_size != current_stat.st_size) {
        return true;
    }

    return false;
}

void take_snapshot(const char *dir, const char *output_dir) {
    char snapshot_file[MAX_PATH_LEN];
    struct dirent *entry;
    DIR *dir_ptr;
    int output_fd;

    dir_ptr = opendir(dir);
    if (dir_ptr == NULL) {
        perror("opendir");
        return;
    }

    snprintf(snapshot_file, sizeof(snapshot_file), "%s/%s_snapshot.txt", output_dir, dir);

    output_fd = open(snapshot_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output_fd == -1) {
        perror("Error opening snapshot file");
        closedir(dir_ptr);
        return;
    }

    dprintf(output_fd, "Snapshot for directory: %s\n", dir);
    dprintf(output_fd, "Timestamp: %ld\n", (long)time(NULL));

    while ((entry = readdir(dir_ptr)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char filepath[MAX_PATH_LEN];
        snprintf(filepath, sizeof(filepath), "%s/%s", dir, entry->d_name);

        if (file_changed(filepath)) {
            struct stat file_stat;
            if (stat(filepath, &file_stat) == -1) {
                perror("stat");
                continue;
            }

            dprintf(output_fd, "\n// Old snapshot file for: %s\n", entry->d_name);
            dprintf(output_fd, "Timestamp: %ld\n", (long)time(NULL));
            dprintf(output_fd, "Entry: %s\n", entry->d_name);
            dprintf(output_fd, "Size: %lld bytes\n", (long long)file_stat.st_size);
            dprintf(output_fd, "Last Modified: %s", ctime(&file_stat.st_mtime));
            dprintf(output_fd, "Permissions: %o\n", file_stat.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
            dprintf(output_fd, "Inode no: %ld\n", (long)file_stat.st_ino);
        }
    }

    closedir(dir_ptr);
    close(output_fd);
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > MAX_DIRS + 2) {
        fprintf(stderr, "Usage: %s [-o output_dir] dir1 dir2 ... dirN\n", argv[0]);
        return 1;
    }

    char *output_dir = ".";
    int opt;
    while ((opt = getopt(argc, argv, "o:")) != -1) {
        switch (opt) {
            case 'o':
                output_dir = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s [-o output_dir] dir1 dir2 ... dirN\n", argv[0]);
                return 1;
        }
    }

    struct stat output_stat;
    if (stat(output_dir, &output_stat) == -1) {
        perror("Output directory stat");
        return 1;
    }
    if (!S_ISDIR(output_stat.st_mode)) {
        fprintf(stderr, "%s is not a directory.\n", output_dir);
        return 1;
    }

    for (int i = optind; i < argc; ++i) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return 1;
        } else if (pid == 0) { 
            take_snapshot(argv[i], output_dir);
            return 0;
        }
    }

    int status;
    pid_t child_pid;
    while ((child_pid = wait(&status)) != -1) {
        printf("The process with Pid %d has ended with code %d\n", child_pid, WEXITSTATUS(status));
    }

    return 0;
}
