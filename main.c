#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <ctype.h>
#include <fcntl.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#define MKDIR(path) mkdir(path, 0777)
#endif


#define MAX_FILES 32
#define MAX_TOTAL_SIZE (200 * 1024 * 1024) // 200 MB

void print_usage() {
    printf("Kullanim:\n");
    printf("  Arsivleme: tarsau -b dosya1 dosya2 ... -o arsiv.sau\n");
    printf("  Arsiv Acma: tarsau -a arsiv.sau [hedef_dizin]\n");
}

int is_ascii_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return 0; // Dosya acilamadi
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (c > 127) { // Standart ASCII karakter araligi: 0-127
            fclose(f);
            return 0; // ASCII degil
        }
    }
    fclose(f);
    return 1;
}

void archive_files(int file_count, char *files[], const char *output_file) {
    if (file_count > MAX_FILES) {
        printf("Hata: En fazla %d dosya birlestirilebilir.\n", MAX_FILES);
        exit(EXIT_FAILURE);
    }

    long total_size = 0;
    struct stat st;
    
    // Boyut kontrolleri ve ASCII kontrolü
    for (int i = 0; i < file_count; i++) {
        if (stat(files[i], &st) != 0) {
            printf("Hata: %s dosyasi bulunamadi veya erisilemiyor.\n", files[i]);
            exit(EXIT_FAILURE);
        }
        
        if (!is_ascii_file(files[i])) {
            printf("%s giris dosyasinin formati uyumsuzdur!\n", files[i]);
            exit(EXIT_FAILURE);
        }
        
        total_size += st.st_size;
    }

    if (total_size > MAX_TOTAL_SIZE) {
        printf("Hata: Giris dosyalarinin toplam boyutu 200 MB'i gecemez.\n");
        exit(EXIT_FAILURE);
    }

    // Metadata olusturma: |Dosya_adi,izinler,boyut|
    char metadata[8192] = ""; 
    for (int i = 0; i < file_count; i++) {
        stat(files[i], &st);
        char entry[512];
        // Linux/Unix izinlerini 3 haneli octal (sekizlik) formatta aliyoruz (orn: 0644)
        int perms = st.st_mode & 0777;
        sprintf(entry, "|%s,%04o,%ld|", files[i], perms, (long)st.st_size);
        strcat(metadata, entry);
    }

    // Arsiv dosyasina yazma
    FILE *out = fopen(output_file, "wb");
    if (!out) {
        printf("Hata: %s dosyasi olusturulamadi.\n", output_file);
        exit(EXIT_FAILURE);
    }

    int metadata_len = strlen(metadata);
    // İlk 10 bayt, ilk bölümün (metadata) toplam boyutunu içermeli.
    // Kendi boyutu (10 bayt) da dahil mi? Genelde hayır, sadece okunacak metadata uzunluğu
    // daha mantıklıdır, ancak isterdeki "ilk bölümün ... boyutu" ifadesinden 10 + metadata_len alıyoruz.
    fprintf(out, "%010d%s", metadata_len + 10, metadata);

    // Dosyaların içeriklerini arsive ekleme
    for (int i = 0; i < file_count; i++) {
        FILE *in = fopen(files[i], "rb");
        if (in) {
            char buffer[8192];
            size_t bytes;
            while ((bytes = fread(buffer, 1, sizeof(buffer), in)) > 0) {
                fwrite(buffer, 1, bytes, out);
            }
            fclose(in);
        }
    }
    
    fclose(out);
    printf("Dosyalar birlestirildi.\n");
}

void extract_archive(const char *archive_file, const char *target_dir) {
    const char *ext = strrchr(archive_file, '.');
    if (!ext || strcmp(ext, ".sau") != 0) {
        printf("Arsiv dosyasi uygunsuz veya bozuk!\n");
        exit(EXIT_FAILURE);
    }

    FILE *in = fopen(archive_file, "rb");
    if (!in) {
        printf("Arsiv dosyasi uygunsuz veya bozuk!\n");
        exit(EXIT_FAILURE);
    }

    char size_buf[11] = {0};
    if (fread(size_buf, 1, 10, in) != 10) {
        printf("Arsiv dosyasi uygunsuz veya bozuk!\n");
        fclose(in);
        exit(EXIT_FAILURE);
    }
    
    long total_section1_size = atol(size_buf);
    if (total_section1_size <= 10) {
        printf("Arsiv dosyasi uygunsuz veya bozuk!\n");
        fclose(in);
        exit(EXIT_FAILURE);
    }

    long metadata_len = total_section1_size - 10;
    
    char *metadata = malloc(metadata_len + 1);
    if (!metadata) {
        printf("Hata: Bellek ayrilamadi.\n");
        fclose(in);
        exit(EXIT_FAILURE);
    }
    
    if (fread(metadata, 1, metadata_len, in) != (size_t)metadata_len) {
        printf("Arsiv dosyasi uygunsuz veya bozuk!\n");
        free(metadata);
        fclose(in);
        exit(EXIT_FAILURE);
    }
    metadata[metadata_len] = '\0';

    if (strcmp(target_dir, ".") != 0) {
        MKDIR(target_dir);
    }

    char *ptr = metadata;
    int files_extracted = 0;
    
    while (*ptr == '|') {
        ptr++; 
        char *end = strchr(ptr, '|');
        if (!end) break;
        
        *end = '\0'; 
        
        char filename[256];
        int perms;
        long file_size;
        
        if (sscanf(ptr, "%255[^,],%o,%ld", filename, &perms, &file_size) != 3) {
            printf("Arsiv dosyasi uygunsuz veya bozuk!\n");
            free(metadata);
            fclose(in);
            exit(EXIT_FAILURE);
        }
        
        char filepath[1024];
        if (strcmp(target_dir, ".") == 0) {
            sprintf(filepath, "%s", filename);
        } else {
            sprintf(filepath, "%s/%s", target_dir, filename);
        }
        
        FILE *out = fopen(filepath, "wb");
        if (!out) {
            printf("Hata: %s olusturulamadi.\n", filepath);
            free(metadata);
            fclose(in);
            exit(EXIT_FAILURE);
        }
        
        char buffer[8192];
        long remaining = file_size;
        while (remaining > 0) {
            size_t to_read = remaining < (long)sizeof(buffer) ? (size_t)remaining : sizeof(buffer);
            size_t bytes = fread(buffer, 1, to_read, in);
            if (bytes == 0) break;
            fwrite(buffer, 1, bytes, out);
            remaining -= bytes;
        }
        fclose(out);
        
        chmod(filepath, perms);
        files_extracted++;
        
        ptr = end + 1; 
    }

    free(metadata);
    fclose(in);
    printf("%s dizininde %d dosya acildi.\n", target_dir, files_extracted);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage();
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "-b") == 0) {
        char *input_files[MAX_FILES + 5];
        int file_count = 0;
        char *output_file = "a.sau"; // Varsayilan arsiv adi
        
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0) {
                if (i + 1 < argc) {
                    output_file = argv[i + 1];
                    i++; // Sonraki argumani atla (arsiv adi)
                }
            } else {
                input_files[file_count++] = argv[i];
            }
        }
        
        if (file_count == 0) {
            printf("Hata: Birlestirilecek dosya belirtilmedi.\n");
            return EXIT_FAILURE;
        }
        
        archive_files(file_count, input_files, output_file);
    } 
    else if (strcmp(argv[1], "-a") == 0) {
        if (argc < 3) {
            printf("Hata: Arsiv dosyasi belirtilmedi.\n");
            return EXIT_FAILURE;
        }
        if (argc > 4) {
            printf("Hata: -a parametresinden sonra en fazla 2 arguman alabilir.\n");
            return EXIT_FAILURE;
        }
        const char *archive_file = argv[2];
        const char *target_dir = "."; // Varsayılan mevcut dizin
        if (argc >= 4) {
            target_dir = argv[3];
        }
        extract_archive(archive_file, target_dir);
    } 
    else {
        printf("Hata: Gecersiz parametre '%s'\n", argv[1]);
        print_usage();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
