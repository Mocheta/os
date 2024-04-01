#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>

void snapshot(const char *name, int indent, FILE *file)
{
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(name)))
        return;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            char path[1024];
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
            fprintf(file, "%*s[%s]\n", indent, "", entry->d_name);
            snapshot(path, indent + 2, file);
        } else {
            fprintf(file, "%*s- %s\n", indent, "", entry->d_name);
        }
    }
    closedir(dir);
}

int main(void) {
    FILE *output_file = fopen("tree_output.txt", "w");
    if (output_file == NULL) {
        perror("Error opening file");
        return 1;
    }

    snapshot(".", 0, output_file);

    fclose(output_file);

    return 0;
}
