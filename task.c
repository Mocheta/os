#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>  
#include <fcntl.h>
#include <errno.h>

void snapshot(const char *dirname, const char *parent, FILE *output_file) {
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    char path[1024];

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

        fprintf(output_file, "%s/%s\n", parent, entry->d_name);
        fprintf(output_file, "  Size: %lld bytes\n", (long long)statbuf.st_size);
        fprintf(output_file, "  Permissions: %o\n", statbuf.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
        fprintf(output_file, "  Type: %s\n", (S_ISDIR(statbuf.st_mode)) ? "Directory" : "File");

        if (S_ISDIR(statbuf.st_mode)) {
            snapshot(path, path, output_file);
        }
    }

    closedir(dir);
}

int main(void) {
    FILE *output_file = fopen("metadata.txt", "w");
    if (output_file == NULL) {
        perror("Error opening file");
        return 1;
    }

    snapshot(".", ".", output_file);

    fclose(output_file);

    return 0;
}
