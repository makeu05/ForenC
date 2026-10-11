#ifndef FORENC_SCANNER_H
#define FORENC_SCANNER_H

#include <stddef.h>

typedef int (*file_callback)(const char *path, void *context);

int scan_directory(
    const char *root,
    file_callback callback,
    void *context
);

#endif
