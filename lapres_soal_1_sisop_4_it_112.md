# Laporan Resmi Praktikum Sistem Operasi Modul 4

## Identitas

- **Nama:** A. Algifari Rantiga Isdar
- **NRP:** 5027251112
- **Repository:** SISOP-4-2026-IT-112

---

# Soal 1 - Save Asisten Kenz

## Deskripsi Soal

Pada soal ini praktikan diminta membuat filesystem sederhana menggunakan FUSE (Filesystem in Userspace). Filesystem tersebut harus bersifat passthrough terhadap file asli pada source directory serta memiliki satu file virtual bernama `tujuan.txt`.

File virtual tersebut tidak boleh ada secara fisik pada source directory dan harus dibuat secara dinamis (on-the-fly) dengan menggabungkan seluruh fragmen `KOORD:` dari file `1.txt` sampai `7.txt`.

---

# Struktur Direktori

```plaintext
soal_1/
├── kenz_rescue.c
├── amba_files/
│   ├── 1.txt
│   ├── 2.txt
│   ├── 3.txt
│   ├── 4.txt
│   ├── 5.txt
│   ├── 6.txt
│   └── 7.txt
└── mnt/
```

---

# Penjelasan Program

Program dibuat menggunakan FUSE dengan callback utama:

- `getattr`
- `readdir`
- `open`
- `read`

Filesystem bekerja sebagai mirror/passthrough terhadap file pada source directory.

Selain itu ditambahkan file virtual `tujuan.txt` yang dibuat secara dinamis ketika file dibaca.

---

# Source Code

## Library dan Global Variable

```c
#define FUSE_USE_VERSION 28
#define _FILE_OFFSET_BITS 64

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

char source_dir[1024];
```

Digunakan library FUSE dan beberapa library standar Linux.

Variabel `source_dir` digunakan untuk menyimpan path directory asli.

---

## Fungsi get_full_path

```c
void get_full_path(char fpath[1024], const char *path)
{
    sprintf(fpath, "%s%s", source_dir, path);
}
```

Fungsi ini digunakan untuk menggabungkan path virtual dengan source directory asli.

---

## Fungsi is_tujuan

```c
int is_tujuan(const char *path)
{
    return strcmp(path, "/tujuan.txt") == 0;
}
```

Digunakan untuk mengecek apakah file yang diakses adalah `tujuan.txt`.

---

## Fungsi generate_tujuan

```c
void generate_tujuan(char *result)
{
    char temp[4096] = "";
    char line[1024];

    strcat(temp, "Tujuan Mas Amba: ");

    for (int i = 1; i <= 7; i++) {

        char filepath[1024];
        sprintf(filepath, "%s/%d.txt", source_dir, i);

        FILE *fp = fopen(filepath, "r");

        if (!fp)
            continue;

        while (fgets(line, sizeof(line), fp)) {

            if (strncmp(line, "KOORD:", 6) == 0) {

                char *frag = line + 6;

                while (*frag == ' ')
                    frag++;

                frag[strcspn(frag, "\n")] = 0;

                strcat(temp, frag);
            }
        }

        fclose(fp);
    }

    strcat(temp, "\n");

    strcpy(result, temp);
}
```

Fungsi ini membaca seluruh file `1.txt` sampai `7.txt`, mengambil bagian setelah prefix `KOORD:`, kemudian menggabungkannya menjadi isi file virtual `tujuan.txt`.

File virtual dibuat secara on-the-fly tanpa membuat file fisik baru.

---

## Callback getattr

```c
static int xmp_getattr(const char *path, struct stat *stbuf)
```

Callback ini digunakan untuk:

- membaca metadata file
- menentukan ukuran file
- menentukan permission

Untuk file virtual `tujuan.txt`, ukuran file ditentukan berdasarkan hasil generate string.

---

## Callback readdir

```c
static int xmp_readdir(const char *path,
                       void *buf,
                       fuse_fill_dir_t filler,
                       off_t offset,
                       struct fuse_file_info *fi)
```

Callback ini digunakan untuk membaca isi directory.

Semua file pada `amba_files` ditampilkan kembali sebagai passthrough, lalu ditambahkan file virtual `tujuan.txt`.

---

## Callback open

```c
static int xmp_open(const char *path, struct fuse_file_info *fi)
```

Callback ini digunakan untuk membuka file.

Untuk file biasa dilakukan passthrough ke source directory.

---

## Callback read

```c
static int xmp_read(const char *path,
                    char *buf,
                    size_t size,
                    off_t offset,
                    struct fuse_file_info *fi)
```

Callback ini digunakan untuk membaca isi file.

- File biasa dibaca langsung dari source directory.
- File `tujuan.txt` dibuat secara dinamis menggunakan fungsi `generate_tujuan()`.

---

# Cara Compile

```bash
gcc -Wall -D_FILE_OFFSET_BITS=64 `pkg-config fuse --cflags` kenz_rescue.c -o kenz_rescue `pkg-config fuse --libs`
```

---

# Cara Menjalankan

Membuat mount point:

```bash
mkdir -p mnt
```

Mount filesystem:

```bash
./kenz_rescue amba_files mnt
```

---

# Pengujian

## Menampilkan Isi Mount

```bash
ls mnt
```

Output:

```plaintext
1.txt
2.txt
3.txt
4.txt
5.txt
6.txt
7.txt
tujuan.txt
```

---

## Pengujian Passthrough

```bash
cat mnt/1.txt
```

Isi file identik dengan source directory.

---

## Pengujian Virtual File

```bash
cat mnt/tujuan.txt
```

Output:

```plaintext
Tujuan Mas Amba: -7.957382728443728, 112.4698688227961, 23:59 WIB
```

---

## Membuktikan tujuan.txt Tidak Ada di Source

```bash
ls amba_files
```

Output:

```plaintext
1.txt 2.txt 3.txt 4.txt 5.txt 6.txt 7.txt
```

Hal ini membuktikan bahwa `tujuan.txt` hanya merupakan file virtual.

---

# Kendala

Kendala utama saat pengerjaan adalah penggunaan FUSE pada WSL yang sempat mengalami error mountpoint dan parsing argument. Solusi dilakukan dengan memperbaiki fungsi `main()` serta memastikan filesystem dijalankan pada environment Linux yang sesuai.

---

# Kesimpulan

Program berhasil membuat filesystem FUSE sederhana yang:

- melakukan passthrough terhadap file asli
- menambahkan file virtual `tujuan.txt`
- menghasilkan isi file virtual secara dinamis
- tidak mengubah source directory asli

Seluruh requirement soal berhasil dipenuhi.

