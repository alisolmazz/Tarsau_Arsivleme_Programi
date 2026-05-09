#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage() {
    printf("Kullanim:\n");
    printf("  Arsivleme: tarsau -b dosya1 dosya2 ... -o arsiv.sau\n");
    printf("  Arsiv Acma: tarsau -a arsiv.sau [hedef_dizin]\n");
}

int main(int argc, char *argv[]) {
    // Argüman kontrolü
    if (argc < 2) {
        print_usage();
        return EXIT_FAILURE;
    }

    // -b parametresi (Arşivleme işlemi)
    if (strcmp(argv[1], "-b") == 0) {
        printf("Arsivleme (-b) islemi secildi.\n");
        // TODO: Boyut limitleri, dosya türü doğrulama ve arşive yazma işlemi eklenecek
    } 
    // -a parametresi (Arşivden çıkarma işlemi)
    else if (strcmp(argv[1], "-a") == 0) {
        printf("Arsiv acma (-a) islemi secildi.\n");
        // TODO: .sau doğrulaması, dizin oluşturma ve dosyaları ayrıştırma işlemi eklenecek
    } 
    // Hatalı parametre
    else {
        printf("Hata: Gecersiz parametre '%s'\n", argv[1]);
        print_usage();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
