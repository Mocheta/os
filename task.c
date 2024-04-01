#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

void snapshot(const char *dirname, const char *parent, int output_file) {
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    char path[1024];
    char buffer[1024];

    if (!(dir = opendir(dirname))) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(path, sizeof(path), "%s/%s", dirname, entry->d_name);

        if (lstat(path, &statbuf) < 0) {
            perror("lstat");
            continue;
        }

        sprintf(buffer, "%s/%s\n", parent, entry->d_name);
        write(output_file, buffer, strlen(buffer));
        sprintf(buffer, "  Size: %lld bytes\n", (long long)statbuf.st_size);
        write(output_file, buffer, strlen(buffer));
        sprintf(buffer, "  Permissions: %o\n", statbuf.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
        write(output_file, buffer, strlen(buffer));
        sprintf(buffer, "  Type: %s\n", (S_ISDIR(statbuf.st_mode)) ? "Directory" : "File");
        write(output_file, buffer, strlen(buffer));

        if (S_ISDIR(statbuf.st_mode)) {
            snapshot(path, path, output_file);
        }
    }

    closedir(dir);
}

int main() {
    int output_file = open("metadata.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644); 
    if (output_file == -1) {
        perror("Error opening file");
        return 1;
    }

    snapshot(".", ".", output_file);

    if (close(output_file) == -1) { 
        perror("Error closing file");
        return 1;
    }

    return 0;
}