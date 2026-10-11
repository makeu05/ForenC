#include "hash.h"

#include <openssl/evp.h>
#include <stdio.h>

int hash_file_sha256(const char *path, char output[65])
{
    if (path == NULL || output == NULL) {
        return -1;
    }

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        perror("ForenC: impossible d'ouvrir le fichier");
        return -1;
    }

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (ctx == NULL) {
        fprintf(stderr, "ForenC: impossible de créer le contexte SHA-256\n");
        fclose(file);
        return -1;
    }

    int result = -1;
    unsigned char buffer[8192];
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    size_t bytes_read;

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) != 1) {
        fprintf(stderr, "ForenC: initialisation SHA-256 échouée\n");
        goto cleanup;
    }

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (EVP_DigestUpdate(ctx, buffer, bytes_read) != 1) {
            fprintf(stderr, "ForenC: calcul SHA-256 échoué\n");
            goto cleanup;
        }
    }

    if (ferror(file)) {
        perror("ForenC: erreur de lecture");
        goto cleanup;
    }

    if (EVP_DigestFinal_ex(ctx, digest, &digest_len) != 1 ||
        digest_len != 32) {
        fprintf(stderr, "ForenC: finalisation SHA-256 échouée\n");
        goto cleanup;
    }

    for (unsigned int i = 0; i < digest_len; i++) {
        snprintf(&output[i * 2], 3, "%02x", digest[i]);
    }

    output[64] = '\0';
    result = 0;

cleanup:
    EVP_MD_CTX_free(ctx);
    fclose(file);
    return result;
}
