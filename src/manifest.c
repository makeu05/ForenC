
#define _XOPEN_SOURCE 700

#include "manifest.h"
#include "hash.h"
#include "scanner.h"

#include <json-c/json.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

struct manifest_context {
    struct json_object *files;
    const char *output_path;
    int failed;
};

static int add_file(const char *path, void *data)
{
    struct manifest_context *ctx = data;

    /*
     * Ne pas inclure le manifeste lui-même dans l'inventaire.
     */
    if (ctx->output_path != NULL &&
        strcmp(path, ctx->output_path) == 0) {
        return 0;
    }

    struct stat info;

    if (stat(path, &info) != 0) {
        perror(path);
        ctx->failed = 1;
        return -1;
    }

    char digest[65];

    if (hash_file_sha256(path, digest) != 0) {
        ctx->failed = 1;
        return -1;
    }

    struct json_object *file = json_object_new_object();

    if (file == NULL) {
        fprintf(stderr, "ForenC: impossible de créer l'objet JSON\n");
        ctx->failed = 1;
        return -1;
    }

    json_object_object_add(
        file, "path", json_object_new_string(path));
    json_object_object_add(
        file, "size", json_object_new_int64((int64_t)info.st_size));
    json_object_object_add(
        file, "sha256", json_object_new_string(digest));

    if (json_object_array_add(ctx->files, file) != 0) {
        json_object_put(file);
        fprintf(stderr, "ForenC: impossible d'ajouter le fichier au manifeste\n");
        ctx->failed = 1;
        return -1;
    }

    printf("Inventorié : %s\n", path);
    return 0;
}

int create_manifest(const char *directory, const char *output_path)
{
    if (directory == NULL || output_path == NULL ||
        directory[0] == '\0' || output_path[0] == '\0') {
        fprintf(stderr, "ForenC: répertoire ou fichier de sortie invalide\n");
        return -1;
    }

    struct json_object *root = json_object_new_object();
    struct json_object *files = json_object_new_array();

    if (root == NULL || files == NULL) {
        fprintf(stderr, "ForenC: allocation JSON impossible\n");
        if (root != NULL) {
            json_object_put(root);
        }
        if (files != NULL) {
            json_object_put(files);
        }
        return -1;
    }

    json_object_object_add(root, "tool",
                           json_object_new_string("ForenC"));
    json_object_object_add(root, "version",
                           json_object_new_string("0.1.0"));
    json_object_object_add(root, "algorithm",
                           json_object_new_string("SHA-256"));
    json_object_object_add(root, "files", files);

    struct manifest_context ctx = {
        .files = files,
        .output_path = output_path,
        .failed = 0
    };

    if (scan_directory(directory, add_file, &ctx) != 0 ||
        ctx.failed) {
        json_object_put(root);
        return -1;
    }

    /*
     * Écrire d'abord dans un fichier temporaire, puis remplacer
     * la destination uniquement si l'écriture a réussi.
     */
    size_t temp_len = strlen(output_path) + sizeof(".tmp");
    char temp_path[temp_len];

    snprintf(temp_path, temp_len, "%s.tmp", output_path);

    FILE *out = fopen(temp_path, "w");

    if (out == NULL) {
        perror(temp_path);
        json_object_put(root);
        return -1;
    }

    const char *json = json_object_to_json_string_ext(
        root, JSON_C_TO_STRING_PRETTY);

    int failed = json == NULL;

    if (!failed &&
        fprintf(out, "%s\n", json) < 0) {
        failed = 1;
    }

    if (fclose(out) != 0) {
        failed = 1;
    }

    if (failed) {
        fprintf(stderr, "ForenC: erreur d'écriture du manifeste\n");
        unlink(temp_path);
        json_object_put(root);
        return -1;
    }

    if (rename(temp_path, output_path) != 0) {
        perror(output_path);
        unlink(temp_path);
        json_object_put(root);
        return -1;
    }

    printf("Manifeste créé : %s\n", output_path);
    printf("Nombre de fichiers : %zu\n",
           json_object_array_length(files));

    json_object_put(root);
    return 0;
}

