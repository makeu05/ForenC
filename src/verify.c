
#include "verify.h"
#include "hash.h"

#include <json-c/json.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

int verify_manifest(const char *manifest_path)
{
    struct json_object *root = json_object_from_file(manifest_path);

    if (root == NULL) {
        fprintf(stderr, "ForenC: impossible de lire le manifeste JSON\n");
        return -1;
    }

    struct json_object *files = NULL;
    struct json_object *algorithm = NULL;

    if (!json_object_object_get_ex(root, "files", &files) ||
        json_object_get_type(files) != json_type_array ||
        !json_object_object_get_ex(root, "algorithm", &algorithm) ||
        json_object_get_type(algorithm) != json_type_string ||
        strcmp(json_object_get_string(algorithm), "SHA-256") != 0) {
        fprintf(stderr, "ForenC: format de manifeste invalide\n");
        json_object_put(root);
        return -1;
    }

    int errors = 0;
    size_t count = json_object_array_length(files);

    for (size_t i = 0; i < count; i++) {
        struct json_object *entry = json_object_array_get_idx(files, i);
        struct json_object *path_obj = NULL;
        struct json_object *hash_obj = NULL;
        struct json_object *size_obj = NULL;

        if (entry == NULL ||
            !json_object_object_get_ex(entry, "path", &path_obj) ||
            !json_object_object_get_ex(entry, "sha256", &hash_obj) ||
            !json_object_object_get_ex(entry, "size", &size_obj) ||
            json_object_get_type(path_obj) != json_type_string ||
            json_object_get_type(hash_obj) != json_type_string ||
            json_object_get_type(size_obj) != json_type_int) {
            fprintf(stderr,
                    "ForenC: entrée invalide à l'index %zu\n", i);
            errors++;
            continue;
        }

        const char *path = json_object_get_string(path_obj);
        const char *expected_hash = json_object_get_string(hash_obj);
        long long expected_size =
            (long long)json_object_get_int64(size_obj);

        if (strlen(expected_hash) != 64) {
            fprintf(stderr, "INVALID  %s (empreinte invalide)\n", path);
            errors++;
            continue;
        }

        struct stat info;

        if (stat(path, &info) != 0) {
            printf("MISSING  %s\n", path);
            errors++;
            continue;
        }

        if (!S_ISREG(info.st_mode)) {
            printf("INVALID  %s (pas un fichier régulier)\n", path);
            errors++;
            continue;
        }

        char actual_hash[65];

        if (hash_file_sha256(path, actual_hash) != 0) {
            printf("ERROR    %s (lecture impossible)\n", path);
            errors++;
            continue;
        }

        if ((long long)info.st_size != expected_size ||
            strcmp(actual_hash, expected_hash) != 0) {
            printf("MODIFIED %s\n", path);
            errors++;
        } else {
            printf("OK       %s\n", path);
        }
    }

    printf("\nFichiers vérifiés : %zu\n", count);
    printf("Anomalies détectées : %d\n", errors);

    json_object_put(root);
    return errors == 0 ? 0 : 1;
}
