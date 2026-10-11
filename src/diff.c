#include "diff.h"

#include <json-c/json.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct file_entry {
    const char *path;
    const char *sha256;
    int64_t size;
};

struct manifest_data {
    struct json_object *root;
    struct file_entry *entries;
    size_t count;
};

static void free_manifest(struct manifest_data *manifest)
{
    free(manifest->entries);
    if (manifest->root != NULL) {
        json_object_put(manifest->root);
    }
    manifest->entries = NULL;
    manifest->root = NULL;
    manifest->count = 0;
}

static int valid_sha256(const char *hash)
{
    if (hash == NULL || strlen(hash) != 64) {
        return 0;
    }

    for (size_t i = 0; i < 64; i++) {
        char c = hash[i];
        if (!((c >= '0' && c <= '9') ||
              (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F'))) {
            return 0;
        }
    }

    return 1;
}

static int load_manifest(const char *path,
                         struct manifest_data *manifest)
{
    memset(manifest, 0, sizeof(*manifest));

    manifest->root = json_object_from_file(path);
    if (manifest->root == NULL ||
        json_object_get_type(manifest->root) != json_type_object) {
        fprintf(stderr, "ForenC: impossible de lire le manifeste %s\n",
                path);
        free_manifest(manifest);
        return -1;
    }

    struct json_object *algorithm = NULL;
    struct json_object *files = NULL;

    if (!json_object_object_get_ex(manifest->root, "algorithm",
                                   &algorithm) ||
        json_object_get_type(algorithm) != json_type_string ||
        strcmp(json_object_get_string(algorithm), "SHA-256") != 0 ||
        !json_object_object_get_ex(manifest->root, "files", &files) ||
        json_object_get_type(files) != json_type_array) {
        fprintf(stderr, "ForenC: format de manifeste invalide : %s\n",
                path);
        free_manifest(manifest);
        return -1;
    }

    manifest->count = json_object_array_length(files);

    if (manifest->count > SIZE_MAX / sizeof(*manifest->entries)) {
        fprintf(stderr, "ForenC: manifeste trop volumineux\n");
        free_manifest(manifest);
        return -1;
    }

    if (manifest->count > 0) {
        manifest->entries = calloc(manifest->count,
                                   sizeof(*manifest->entries));
        if (manifest->entries == NULL) {
            fprintf(stderr, "ForenC: mémoire insuffisante\n");
            free_manifest(manifest);
            return -1;
        }
    }

    for (size_t i = 0; i < manifest->count; i++) {
        struct json_object *entry =
            json_object_array_get_idx(files, i);
        struct json_object *path_obj = NULL;
        struct json_object *hash_obj = NULL;
        struct json_object *size_obj = NULL;

        if (entry == NULL ||
            json_object_get_type(entry) != json_type_object ||
            !json_object_object_get_ex(entry, "path", &path_obj) ||
            !json_object_object_get_ex(entry, "sha256", &hash_obj) ||
            !json_object_object_get_ex(entry, "size", &size_obj) ||
            json_object_get_type(path_obj) != json_type_string ||
            json_object_get_type(hash_obj) != json_type_string ||
            json_object_get_type(size_obj) != json_type_int) {
            fprintf(stderr,
                    "ForenC: entrée invalide dans %s à l'index %zu\n",
                    path, i);
            free_manifest(manifest);
            return -1;
        }

        const char *file_path = json_object_get_string(path_obj);
        const char *hash = json_object_get_string(hash_obj);
        int64_t size = json_object_get_int64(size_obj);

        if (file_path == NULL || file_path[0] == '\0' ||
            !valid_sha256(hash) || size < 0) {
            fprintf(stderr,
                    "ForenC: données invalides dans %s à l'index %zu\n",
                    path, i);
            free_manifest(manifest);
            return -1;
        }

        manifest->entries[i].path = file_path;
        manifest->entries[i].sha256 = hash;
        manifest->entries[i].size = size;

        for (size_t j = 0; j < i; j++) {
            if (strcmp(manifest->entries[j].path, file_path) == 0) {
                fprintf(stderr,
                        "ForenC: chemin dupliqué dans %s : %s\n",
                        path, file_path);
                free_manifest(manifest);
                return -1;
            }
        }
    }

    return 0;
}

static const struct file_entry *find_file(
    const struct manifest_data *manifest,
    const char *path)
{
    for (size_t i = 0; i < manifest->count; i++) {
        if (strcmp(manifest->entries[i].path, path) == 0) {
            return &manifest->entries[i];
        }
    }

    return NULL;
}

int diff_manifests(const char *baseline_path,
                   const char *followup_path)
{
    if (baseline_path == NULL || followup_path == NULL ||
        baseline_path[0] == '\0' || followup_path[0] == '\0') {
        fprintf(stderr, "ForenC: chemins de manifestes invalides\n");
        return -1;
    }

    struct manifest_data baseline = {0};
    struct manifest_data followup = {0};

    if (load_manifest(baseline_path, &baseline) != 0) {
        return -1;
    }

    if (load_manifest(followup_path, &followup) != 0) {
        free_manifest(&baseline);
        return -1;
    }

    size_t added = 0;
    size_t deleted = 0;
    size_t modified = 0;
    size_t unchanged = 0;

    printf("Comparaison des manifestes\n");
    printf("Référence : %s\n", baseline_path);
    printf("Nouveau   : %s\n\n", followup_path);

    for (size_t i = 0; i < baseline.count; i++) {
        const struct file_entry *old_file = &baseline.entries[i];
        const struct file_entry *new_file =
            find_file(&followup, old_file->path);

        if (new_file == NULL) {
            printf("DELETED   %s\n", old_file->path);
            deleted++;
        } else if (old_file->size != new_file->size ||
                   strcmp(old_file->sha256, new_file->sha256) != 0) {
            printf("MODIFIED  %s\n", old_file->path);
            modified++;
        } else {
            printf("UNCHANGED %s\n", old_file->path);
            unchanged++;
        }
    }

    for (size_t i = 0; i < followup.count; i++) {
        const struct file_entry *new_file = &followup.entries[i];

        if (find_file(&baseline, new_file->path) == NULL) {
            printf("ADDED     %s\n", new_file->path);
            added++;
        }
    }

    printf("\nRésumé de la comparaison\n");
    printf("Ajoutés      : %zu\n", added);
    printf("Supprimés    : %zu\n", deleted);
    printf("Modifiés     : %zu\n", modified);
    printf("Inchangés    : %zu\n", unchanged);
    printf("Référence    : %zu fichier(s)\n", baseline.count);
    printf("Nouveau      : %zu fichier(s)\n", followup.count);

    free_manifest(&baseline);
    free_manifest(&followup);
    return 0;
}
