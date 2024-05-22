# Sisop-4-2024-MH-IT07

## Soal 2
Masih dengan Ini Karya Kita, sang CEO ingin melakukan tes keamanan pada folder sensitif Ini Karya Kita. Karena Teknologi Informasi merupakan departemen dengan salah satu fokus di Cyber Security, maka dia kembali meminta bantuan mahasiswa Teknologi Informasi angkatan 2023 untuk menguji dan mengatur keamanan pada folder sensitif tersebut. Untuk mendapatkan folder sensitif itu, mahasiswa IT 23 harus kembali mengunjungi website Ini Karya Kita pada www.inikaryakita.id/schedule . Silahkan isi semua formnya, tapi pada form subject isi dengan nama kelompok_SISOP24 , ex: IT01_SISOP24 . Lalu untuk form Masukkan Pesanmu, ketik “Mau Foldernya” . Tunggu hingga 1x24 jam, maka folder sensitif tersebut akan dikirimkan melalui email kalian. Apabila folder tidak dikirimkan ke email kalian, maka hubungi sang CEO untuk meminta bantuan.   
- Pada folder "pesan" Adfi ingin meningkatkan kemampuan sistemnya dalam mengelola berkas-berkas teks dengan menggunakan fuse.
  
	- Jika sebuah file memiliki prefix "base64," maka sistem akan langsung mendekode isi file tersebut dengan algoritma Base64.
	- Jika sebuah file memiliki prefix "rot13," maka isi file tersebut akan langsung di-decode dengan algoritma ROT13.
	- Jika sebuah file memiliki prefix "hex," maka isi file tersebut akan langsung di-decode dari representasi heksadesimalnya.
	- Jika sebuah file memiliki prefix "rev," maka isi file tersebut akan langsung di-decode dengan cara membalikkan teksnya.

- Pada folder “rahasia-berkas”, Adfi dan timnya memutuskan untuk menerapkan kebijakan khusus. Mereka ingin memastikan bahwa folder dengan prefix "rahasia" tidak dapat diakses tanpa izin khusus. 
	- Jika seseorang ingin mengakses folder dan file pada “rahasia”, mereka harus memasukkan sebuah password terlebih dahulu (password bebas). 
- Setiap proses yang dilakukan akan tercatat pada logs-fuse.log dengan format : [SUCCESS/FAILED]::dd/mm/yyyy-hh:mm:ss::[tag]::[information]

	Ex:
	[SUCCESS]::01/11/2023-10:43:43::[moveFile]::[File moved successfully]

### Langkah Pengerjaan :
- Download dan extract `folder rahasia`
- Masuk direktori `pesam` ( hasil dari ekstrak `folder rahasia`
- buat script `pastibisa.c` menggunakan editor nano denga command `nano pastibisa.c`
- Isi script `pastibisa.c` 
  ```
  #define FUSE_USE_VERSION 31

  #include <fuse.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <errno.h>
  #include <ctype.h>

  // decode base64
  char *base64_decode(const char *input) {
    static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int len = strlen(input);
    char *decoded = (char *)malloc(len * 3 / 4 + 1);
    char *p = decoded;
    int val = 0, valb = -8;

    for (int i = 0; i < len; i++) {
        unsigned char c = input[i];
        if (isspace(c)) continue;
        if (c == '=') break;
        if (strchr(base64_table, c) == NULL) continue;

        val = (val << 6) + (strchr(base64_table, c) - base64_table);
        valb += 6;
        if (valb >= 0) {
            *p++ = (val >> valb) & 0xFF;
            valb -= 8;
        }
    }
    *p = '\0';
    return decoded;
  }

  // decode rot13
  char *rot13_decode(const char *input) {
    char *decoded = strdup(input);
    for (int i = 0; decoded[i]; i++) {
        if ((decoded[i] >= 'A' && decoded[i] <= 'Z') || (decoded[i] >= 'a' && decoded[i] <= 'z')) {
            if ((decoded[i] >= 'A' && decoded[i] <= 'M') || (decoded[i] >= 'a' && decoded[i] <= 'm')) {
                decoded[i] += 13;
            } else {
                decoded[i] -= 13;
            }
        }
    }
    return decoded;
  }

  // decode hex
  char *hex_decode(const char *input) {
    int len = strlen(input);
    char *decoded = (char *)malloc(len / 2 + 1);
    for (int i = 0; i < len / 2; i++) {
        sscanf(input + 2 * i, "%2hhx", &decoded[i]);
    }
    decoded[len / 2] = '\0';
    return decoded;
  }

  // rev
  char *reverse_text(const char *input) {
    int len = strlen(input);
    char *reversed = (char *)malloc(len + 1);
    for (int i = 0; i < len; i++) {
        reversed[i] = input[len - i - 1];
    }
    reversed[len] = '\0';
    return reversed;
  }

  static int getattr(const char *path, struct stat *stbuf) {
    int res = 0;
    char prefix[10];
    char filename[256];
    
    sscanf(path, "/%s", filename);
    sscanf(filename, "%[^_]", prefix);

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if (strcmp(prefix, "base64") == 0 || strcmp(prefix, "rot13") == 0 || strcmp(prefix, "hex") == 0 || 
  strcmp(prefix, "rev") == 0) {
        stbuf->st_mode = S_IFREG | 0644;
        stbuf->st_nlink = 1;
        stbuf->st_size = 100; 
    } else {
        res = -ENOENT;
    }

    return res;
  }    

  static int readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void)offset;
    (void)fi;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    // file
    filler(buf, "enkripsi_rot13.txt", NULL, 0);
    filler(buf, "halo.txt", NULL, 0);
    filler(buf, "new-hex.txt", NULL, 0);
    filler(buf, "notes-base64.txt", NULL, 0);
    filler(buf, "rev-text.txt", NULL, 0);

    return 0;
  }

  static int sens_open(const char *path, struct fuse_file_info *fi) {
    char prefix[10];
    char filename[256];
    
    sscanf(path, "/%s", filename);
    sscanf(filename, "%[^_]", prefix);

    if (strcmp(prefix, "base64") != 0 && strcmp(prefix, "rot13") != 0 && strcmp(prefix, "hex") != 0 && strcmp(prefix, "rev") != 0) {
        return -ENOENT;
    }

    return 0;
  }

  static struct fuse_operations oper = {
    .getattr = getattr,
    .readdir = readdir,
    .open = sens_open,
  };

  int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &oper, NULL);
  }
  ```

- Compile menggunakan command `gcc -Wall pastibisa.c -0 pastibisa -fuse -D_FILE_OFFSET_BITS=64`
- Buat direktori mount menggunakan command `mkdir /tmp/mount`
- Run fuse menggunakan `./pastibisa /tmp/mount`
- Tampilkan file menggunakan `ls /tmp/mount`
- Untuk membaca file gunakan command `cat /tmp/mount/[file.txt]`
- Unmount menggunakan `fusermount -u /tmp/mount`

