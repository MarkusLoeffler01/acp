#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>


#define BUFFER_SIZE 1024


//Funktion zur Berechnung der Prüfsumme mit SHA-256
void calculate_checksum(const char *filename, unsigned char *output) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Fehler beim Öffnen der Datei");
        exit(EXIT_FAILURE);
    }

    EVP_MD_CTX *mdctx;
    const EVP_MD *md;
    unsigned char buffer[BUFFER_SIZE];
    size_t bytesRead = 0;

    mdctx = EVP_MD_CTX_new();
    md = EVP_sha256();

    if (!EVP_DigestInit_ex(mdctx, md, NULL)) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (!EVP_DigestUpdate(mdctx, buffer, bytesRead)) {
            ERR_print_errors_fp(stderr);
            exit(EXIT_FAILURE);
        }
    }

    unsigned int length;
    if (!EVP_DigestFinal_ex(mdctx, output, &length)) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    EVP_MD_CTX_free(mdctx);
    fclose(file);
}

// Funktion zum Vergleichen der Prüfsummen
int compare_checksums(unsigned char *checksum1, unsigned char *checksum2) {
    return memcmp(checksum1, checksum2, SHA256_DIGEST_LENGTH) == 0;
}

// Funktion zum Kopieren einer Datei
void copy_file(const char *source, const char *destination) {
    int src_fd = open(source, O_RDONLY);
    if (src_fd < 0) {
        perror("Fehler beim Öffnen der Quelldatei");
        exit(EXIT_FAILURE);
    }

    int dest_fd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd < 0) {
        perror("Fehler beim Öffnen der Zieldatei");
        close(src_fd);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;
    while ((bytesRead = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
        write(dest_fd, buffer, bytesRead);
    }

    close(src_fd);
    close(dest_fd);
}

// Funktion, um eine Datei im Zielverzeichnis zu prüfen und ggf. zu ersetzen
void process_file(const char *source_file, const char *target_file) {
    unsigned char source_checksum[SHA256_DIGEST_LENGTH];
    unsigned char target_checksum[SHA256_DIGEST_LENGTH];

    calculate_checksum(source_file, source_checksum);

    if (access(target_file, F_OK) != -1) {
        // Datei existiert im Zielverzeichnis, also Prüfsumme berechnen
        calculate_checksum(target_file, target_checksum);

        // Wenn Prüfsummen gleich sind, nichts tun
        if (compare_checksums(source_checksum, target_checksum)) {
            printf("Die Datei %s ist bereits aktuell.\n", target_file);
            return;
        }
    }

    // Datei kopieren, wenn sie nicht existiert oder die Prüfsummen unterschiedlich sind
    printf("Ersetze %s durch %s\n", target_file, source_file);
    copy_file(source_file, target_file);
}

// Funktion, um die Dateien im Quellverzeichnis zu verarbeiten
void process_directory(const char *source_dir, const char *target_dir) {
    DIR *dir = opendir(source_dir);
    if (!dir) {
        perror("Fehler beim Öffnen des Quellverzeichnisses");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Ignoriere "." und ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char source_path[1024], target_path[1024];
        snprintf(source_path, sizeof(source_path), "%s/%s", source_dir, entry->d_name);
        snprintf(target_path, sizeof(target_path), "%s/%s", target_dir, entry->d_name);

        struct stat statbuf;
        if (stat(source_path, &statbuf) == 0) {
            // Prüfe, ob es sich um eine reguläre Datei handelt
            if (S_ISREG(statbuf.st_mode)) {
                process_file(source_path, target_path);
            }
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Verwendung: %s <source_dir> <target_dir>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *source_dir = argv[1];
    const char *target_dir = argv[2];

    struct stat st;
    if (stat(target_dir, &st) == -1) {
        if (mkdir(target_dir, 0755) == -1) {
            perror("Fehler beim Erstellen des Zielverzeichnisses");
            return EXIT_FAILURE;
        }
    }

    process_directory(source_dir, target_dir);

    return EXIT_SUCCESS;
}