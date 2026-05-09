# Tarsau Arşivleme Programı

Bu proje, Sistem Programlama dersi için geliştirilmiş bir arşivleme (sıkıştırmasız) programıdır. `tar` komutuna benzer şekilde çalışır.

## Özellikler
- `-b` parametresi ile birden fazla metin (ASCII) dosyasını tek bir `.sau` formatında arşivler.
- `-a` parametresi ile `.sau` arşiv dosyasını belirtilen dizine çıkartır ve orijinal dosya izinlerini (permissions) korur.

## Kullanım
Derlemek için:
```bash
make
```

Arşivleme:
```bash
./tarsau -b file1.txt file2.txt -o archive.sau
```

Arşiv Açma:
```bash
./tarsau -a archive.sau output_dir
```
