#define _XOPEN_SOURCE 700

#include "scanner.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int scan_recursive(
    const char *directory,
    file_callback callback,
    void *context
)
{
    DIR *dir = opendir(directory);

    if (dir == NULL) {
        perror(directory);
        return -1;
    }

    struct dirent *entry;
    int result = 0;

    errno = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        size_t length = strlen(directory) + strlen(entry->d_name) + 2;
        char *path = malloc(length);

        if (path == NULL) {
            fprintf(stderr, "ForenC: mémoire insuffisante\n");
            result = -1;
            break;
        }

        int written = snprintf(
            path,
            length,
            "%s/%s",
            directory,
            entry->d_name
        );

        if (written < 0 || (size_t)written >= length) {
            fprintf(stderr, "ForenC: chemin trop long\n");
            free(path);
            result = -1;
            break;
        }

        struct stat info;

        /*
         * lstat() examine le lien symbolique lui-même.
         * Les liens symboliques ne sont pas suivis.
         */
        if (lstat(path, &info) != 0) {
            perror(path);
            free(path);
            result = -1;
            continue;
        }

        if (S_ISDIR(info.st_mode)) {
            if (scan_recursive(path, callback, context) != 0) {
                result = -1;
            }
        } else if (S_ISREG(info.st_mode)) {
            if (callback(path, context) != 0) {
                free(path);
                result = -1;
                break;
            }
        }

        free(path);
        errno = 0;
    }

    if (errno != 0) {
        perror(directory);
        result = -1;
    }

    if (closedir(dir) != 0) {
        perror(directory);
        result = -1;
    }

    return result;
}

int scan_directory(
    const char *root,
    file_callback callback,
    void *context
)
{
    if (root == NULL || callback == NULL) {
        fprintf(stderr, "ForenC: argument invalide\n");
        return -1;
    }

    struct stat info;

    if (lstat(root, &info) != 0) {
        perror(root);
        return -1;
    }

    if (!S_ISDIR(info.st_mode)) {
        fprintf(stderr, "ForenC: le chemin n'est pas un répertoire\n");
        return -1;
    }

    return scan_recursive(root, callback, context);
}
