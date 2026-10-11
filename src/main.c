
#include "hash.h"
#include "manifest.h"
#include "verify.h"
#include <stdio.h>
#include <string.h>

#define FORENC_VERSION "0.1.0"

static void print_help(const char *program)
{
    printf("ForenC - Digital Forensics CLI\n\n");
    printf("Usage: %s <command> [arguments]\n\n", program);
    printf("Commands:\n");
    printf("  --help          Display this help message\n");
    printf("  --version       Display program version\n");
    printf("  hash <file>     Calculate the SHA-256 hash of a file\n");
    printf("  scan <dir> -o <file.json>  Create a JSON evidence manifest\n");
	printf("  verify <manifest.json>  Verify file integrity\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--help") == 0) {
        print_help(argv[0]);
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0) {
        printf("ForenC version %s\n", FORENC_VERSION);
        return 0;
    }

    if (strcmp(argv[1], "hash") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s hash <file>\n", argv[0]);
            return 1;
        }

        char hash[65];

        if (hash_file_sha256(argv[2], hash) != 0) {
            return 1;
        }

        printf("SHA-256  %s  %s\n", hash, argv[2]);
        return 0;
    }

    if (strcmp(argv[1], "scan") == 0) {
        if (argc != 5 || strcmp(argv[3], "-o") != 0) {
            fprintf(stderr,
                    "Usage: %s scan <directory> -o <manifest.json>\n",
                    argv[0]);
            return 1;
        }

        if (create_manifest(argv[2], argv[4]) != 0) {
            fprintf(stderr, "ForenC: création du manifeste échouée\n");
            return 1;
        }

        return 0;
    }
	if (strcmp(argv[1], "verify") == 0) {
    	if (argc != 3) {
        	fprintf(stderr, "Usage: %s verify <manifest.json>\n", argv[0]);
        	return 1;
    }

    return verify_manifest(argv[2]) == 0 ? 0 : 1;
}

    fprintf(stderr, "Error: unknown command '%s'\n", argv[1]);
    fprintf(stderr, "Use '%s --help' for usage.\n", argv[0]);

    return 1;
}

