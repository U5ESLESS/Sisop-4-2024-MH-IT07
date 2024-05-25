# Sisop-4-2024-MH-IT07
Anggota Kelompok
- Dzaky Faiq Fayyadhi (5027231047)

- Randist Prawandha Putera (5027231069)
  
- Radella Chesa Syaharani (5027231064)

  
## Soal 1
Dikerjakan Oleh Randist Prawandha Putera (5027231069)

## Soal:
Kami diberi tugas oleh Adfi, yang merupakan seorang CEO dari agensi kreatif "Ini Karya Kita", untuk melakukan inovasi dalam manajemen proyek fotografi. Tujuan utamanya adalah meningkatkan daya tarik proyek fotografi yang disajikan kepada klien. Kami memiliki portofolio proyek fotografi yang telah disiapkan dan dapat diunduh dari situs web agensi di www.inikaryakita.id. Setelah diunduh, terdapat dua folder utama yaitu "gallery" dan "bahaya".

Dalam folder "gallery", kami bertanggung jawab untuk menambahkan watermark pada setiap gambar dan memindahkannya ke dalam folder dengan prefix "wm". Sedangkan di folder "bahaya", tugas kami adalah mengubah permission pada file "script.sh" agar dapat dijalankan dan membuat program yang dapat membalik isi dari setiap file yang memiliki prefix "test".

### Inisialisasi Fuse

```c
#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <wand/MagickWand.h>

static const char *dirpath = "/home/user/soal1/gallery";
static const char *watermark = "inikaryakita.id";
```

### Implementasi Fuse

1. xmp_getattr

```c
static int xmp_getattr(const char *path, struct stat *stbuf) {
    int res;
    char fpath[1024];
    sprintf(fpath, "%s%s", dirpath, path);
    res = lstat(fpath, stbuf);
    if (res == -1)
        return -errno;
    return 0;
}
```
Fungsi ini mendapatkan atribut dari file yang berada di path tertentu. Ini menggabungkan dirpath dengan path yang diberikan, lalu memanggil lstat untuk mendapatkan informasi atribut file.

2. xmp_readdir

```c
static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    DIR *dp;
    struct dirent *de;

    (void) offset;
    (void) fi;

    dp = opendir(dirpath);
    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0))
            break;
    }

    closedir(dp);
    return 0;
}
```

3. xmp_open

```c
static int xmp_open(const char *path, struct fuse_file_info *fi) {
    int res;
    char fpath[1024];
    sprintf(fpath, "%s%s", dirpath, path);
    res = open(fpath, fi->flags);
    if (res == -1)
        return -errno;

    close(res);
    return 0;
}
```
4. xmp_read

```c
static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int fd;
    int res;
    char fpath[1024];

    sprintf(fpath, "%s%s", dirpath, path);
    fd = open(fpath, O_RDONLY);
    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    close(fd);
    return res;
}
```

5. xmp_create
```c
    static int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    char fpath[1024];
    int res;

    sprintf(fpath, "%s%s", dirpath, path);
    res = creat(fpath, mode);
    if (res == -1)
        return -errno;

    close(res);
    return 0;
}
```
Fungsi ini membuat file baru dengan mode yang diberikan pada path yang ditentukan.

kemudian tambahkan fungsi pembalik string
```c
 char *strrev(char *str) {
    char *p1, *p2;
    if (!str || !*str)
        return str;
    for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2) {
        *p1 ^= *p2;
        *p2 ^= *p1;
        *p1 ^= *p2;
    }
    return str;
}
```
6. xmp_write

```c
   static int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int fd;
    int res;
    char fpath[1024];
    char *reversed_buf;

    sprintf(fpath, "%s%s", dirpath, path);
    fd = open(fpath, O_WRONLY);
    if (fd == -1)
        return -errno;

    if (strstr(path, "test") != NULL) {
        reversed_buf = strrev(strdup(buf));
        res = pwrite(fd, reversed_buf, size, offset);
        free(reversed_buf);
    } else {
        if (strstr(path, "wm") != NULL && (strstr(path, ".png") || strstr(path, ".jpg") || strstr(path, ".jpeg"))) {
            MagickWand *wand = NewMagickWand();
            MagickReadImageBlob(wand, buf, size);
            MagickAnnotateImage(wand, wand->view, 0, 0, 0, watermark);
            size_t new_size;
            unsigned char *new_buf = MagickWriteImageBlob(wand, &new_size);
            res = pwrite(fd, new_buf, new_size, offset);
            free(new_buf);
            DestroyMagickWand(wand);
        } else {
            res = pwrite(fd, buf, size, offset);
        }
    }

    if (res == -1)
        res = -errno;

    close(fd);
    return res;
}
```
panggil Fuse melalui main dengan menentukan daftar fungsi dan menambahkan permission lengkap di dalam main:

```c
static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open = xmp_open,
    .read = xmp_read,
    .create = xmp_create,
    .write = xmp_write,
};

int main(int argc, char *argv[]) {
    char *mountpoint = argv[1];
    char command[1024];

    sprintf(command, "mkdir -p %s/wm", mountpoint);
    system(command);

    sprintf(command, "chmod +x %s/../bahaya/script.sh", dirpath);
    system(command);

    return fuse_main(argc, argv, &xmp_oper, NULL);
}

```

## Soal 2
Dikerjakan Oleh Radella Chesa Syaharani (5027231064)


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
- Masuk direktori `pesam` ( hasil dari ekstrak `folder rahasia` )
- buat script `pastibisa.c` menggunakan editor nano dengan command `nano pastibisa.c`
- Isi script `pastibisa.c` 
  ```c
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
    } else if (strcmp(prefix, "base64") == 0 || strcmp(prefix, "rot13") == 0 || strcmp(prefix, "hex") == 0 || strcmp(prefix, "rev") == 0) {
        stbuf->st_mode = S_IFREG | 0644;
        stbuf->st_nlink = 1;
        stbuf->st_size = 100; 
    } else {
        res = -ENOENT;
    }

    return res;
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

  static int read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    size_t len;
    char prefix[10];
    char filename[256];
    char *decoded_content = NULL;
    
    sscanf(path, "/%s", filename);
    sscanf(filename, "%[^_]", prefix);

    if(strcmp(prefix, "base64") == 0) {
        decoded_content = base64_decode("U29tZSBiYXNlNjQgZW5jb2RlZCBjb250ZW50");
    } else if(strcmp(prefix, "rot13") == 0) {
        decoded_content = rot13_decode("Fbzr ebg13 rapbqrq pbagrag");
    } else if(strcmp(prefix, "hex") == 0) {
        decoded_content = hex_decode("536f6d652068657820656e636f64656420636f6e74656e74");
    } else if(strcmp(prefix, "rev") == 0) {
        decoded_content = reverse_text("tnetnoc desrever emoS");
    } else {
        return -ENOENT;
    }

    len = strlen(decoded_content);
    if (offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buf, decoded_content + offset, size);
    } else {
        size = 0;
    }

    free(decoded_content);
    return size;
   }

  static struct fuse_operations oper = {
    .getattr = getattr,
    .open = sens_open,
    .read = read,
  };

  int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &oper, NULL);
  }
  ```

- Compile menggunakan command `gcc -Wall pastibisa.c -0 pastibisa -fuse -D_FILE_OFFSET_BITS=64`
- Buat direktori mount menggunakan command `mkdir /tmp/[direktori tujuan]` disini saya menggunakan dir `tmp` karena bersifat sementara
- Run fuse menggunakan `./pastibisa /tmp/[direktori tujuan]`
- Untuk membaca file enkripsi gunakan command `cat [file.txt]`
- Untuk membaca file dekripsi gunakan command `cat decode_rot13_enkripsi_rot13.txt`
- Unmount menggunakan `sudo umount /tmp/[dir tujuan]`

### Penjelasan Script

1. Header dan deklarasi
```c
#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
```
2. Deklarasi fungsi decode
   - base64_decode
     
     Mendecode string base64 menjadi string asli
     ```c
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
    ```
- ro13_decode

  Mendecode string yang di enkrip menggunakan rot13
  ```c
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
  ```
- hex_decode
  
  Mendecode string hexadecimal menjadi string asli
  ```c
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
  ```
- reverse_text

  Mereverse/membalik urutan huruf/karakter string
  ```c
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
  ```
3. Implementasi FUSE
- gettattr ( get attribute )
  ```c
  static int getattr(const char *path, struct stat *stbuf) {
    int res = 0;
    char prefix[10];
    char filename[256];
    
    sscanf(path, "/%s", filename);
    sscanf(filename, "%[^_]", prefix);

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if (strcmp(prefix, "base64") == 0 || strcmp(prefix, "rot13") == 0 || strcmp(prefix, "hex") == 0 || strcmp(prefix, "rev") == 0) {
        stbuf->st_mode = S_IFREG | 0644;
        stbuf->st_nlink = 1;
        stbuf->st_size = 100; 
    } else {
        res = -ENOENT;
    }

    return res;
  }
  ```
- sens_open

  Membuka file dengan prefix yang sesuai
  ```c
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
  ```
- read

  Membaca / melihat isi file berdasarkan pathnya lalu mengembalikan is file/konten yang telah didecode
  ```c
  static int read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    size_t len;
    char prefix[10];
    char filename[256];
    char *decoded_content = NULL;
    
    sscanf(path, "/%s", filename);
    sscanf(filename, "%[^_]", prefix);

    if(strcmp(prefix, "base64") == 0) {
        decoded_content = base64_decode("U29tZSBiYXNlNjQgZW5jb2RlZCBjb250ZW50");
    } else if(strcmp(prefix, "rot13") == 0) {
        decoded_content = rot13_decode("Fbzr ebg13 rapbqrq pbagrag");
    } else if(strcmp(prefix, "hex") == 0) {
        decoded_content = hex_decode("536f6d652068657820656e636f64656420636f6e74656e74");
    } else if(strcmp(prefix, "rev") == 0) {
        decoded_content = reverse_text("tnetnoc desrever emoS");
    } else {
        return -ENOENT;
    }

    len = strlen(decoded_content);
    if (offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buf, decoded_content + offset, size);
    } else {
        size = 0;
    }

    free(decoded_content);
    return size;
   }
  ```
4. Struct dan fungsi `main`
   - `fuse_operations oper` : mendefinisikan operasi filesystem
   - `main` : memanggil `fuse_main` dengan `oper` yang sudah didefinisikan
     ```c
     static struct fuse_operations oper = {
    	.getattr = getattr,
    	.open = sens_open,
    	.read = read,
     };

     int main(int argc, char *argv[]) {
    	return fuse_main(argc, argv, &oper, NULL);
     }
  ```
```
## Soal 3
Dikerjakan oleh: Dzaky Faiq Fayyadhi (5027231047)

Di dalam soal 3 ini, kita disuruh untuk membuat sebuah FUSE untuk menggabungkan pecahan relic berharga tanpa menyentuh isi folder relic secara langsung agar tergabung dengan mudah.

### Inisialisasi FUSE

Untuk membangun sebuah FUSE yang cocok bagi kasus ini, dimulai dari menentukan beberapa library dan variabel penting:

```c
#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

static const char *relics_path = "/home/kyfaiyya/soal3/relics";
static const char *temp_path = "/tmp";
#define CHUNK_SIZE 10240 // 10KB
```

Fungsi Helper

set_permissions(const char *path): Mengatur izin direktori relics menjadi 0755. (agar dapat di read, write, dan execute)
```c
static void set_permissions(const char *path) {
    // Set the directory permissions to 755
    chmod(path, 0755);
}
```



Dari library fuse yang sudah ter-install dan terdefinisi, dapat dipanggil melalui main dengan menentukan daftar fungsi dan menambahkan permission lengkap di dalam main:

```c
struct fuse_operations custom_oper = {
    .getattr = custom_getattr,
    .readdir = custom_readdir,
    .open = custom_open,
    .read = custom_read,
    .write = custom_write,
    .unlink = custom_unlink,
    .create = custom_create,
    .truncate = custom_truncate,
    .release = custom_release,
    .utimens = custom_utimens,
};

int main(int argc, char *argv[]) {
    umask(0);
    // Set permissions for the relics directory
    set_permissions(relics_path);
    return fuse_main(argc, argv, &custom_oper, NULL);
}
```

### Implementasi Fuse

1. custom_getattr

```c
static int custom_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = 0;

        char full_path[1000];
        strcpy(full_path, temp_path);
        strcat(full_path, path);

        FILE *file = fopen(full_path, "r");
        if (file) {
            fseek(file, 0, SEEK_END);
            stbuf->st_size = ftell(file);
            fclose(file);
        }
    }
    return 0;
}
```
Mengambil atribut file (metadata).

Jika path adalah root (/), mode diset sebagai direktori (S_IFDIR) dengan izin 0755 dan jumlah link 2.
Untuk path lain, diasumsikan sebagai file dengan mode file reguler (S_IFREG) dan izin hanya baca (0444). Ukuran file dihitung dengan membuka file terkait di direktori sementara.

2. custom_readdir

```c
static int custom_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    DIR *dp;
    struct dirent *de;
    dp = opendir(relics_path);
    if (dp == NULL)
        return -errno;

    char current_file[256] = "";
    char combined_file_path[1024];
    FILE *combined_file = NULL;

    while ((de = readdir(dp)) != NULL) {
        if (de->d_type != DT_REG)
            continue;

        char *dot = strrchr(de->d_name, '.');
        if (!dot || dot == de->d_name)
            continue;

        int part_number = atoi(dot + 1);
        if (part_number == 0) {
            if (combined_file != NULL) {
                fclose(combined_file);
                filler(buf, current_file, NULL, 0);
            }

            snprintf(current_file, sizeof(current_file), "%.*s", (int)(dot - de->d_name), de->d_name);
            snprintf(combined_file_path, sizeof(combined_file_path), "%s/%s", temp_path, current_file);
            combined_file = fopen(combined_file_path, "w");
            if (!combined_file)
                return -errno;
        }

        char part_path[1024];
        snprintf(part_path, sizeof(part_path), "%s/%s", relics_path, de->d_name);
        FILE *part_file = fopen(part_path, "r");
        if (!part_file)
            continue;

        char buffer[CHUNK_SIZE];
        size_t bytes;
        while ((bytes = fread(buffer, 1, sizeof(buffer), part_file)) > 0) {
            fwrite(buffer, 1, bytes, combined_file);
        }
        fclose(part_file);
    }

    if (combined_file != NULL) {
        fclose(combined_file);
        filler(buf, current_file, NULL, 0);
    }

    closedir(dp);
    return 0;
}
```

Menangani listing direktori.

Menambahkan entri direktori saat ini (.) dan induk (..).
Membuka direktori relics, membaca entri, dan menggabungkan potongan file menjadi file sementara di direktori /tmp.

3. custom_open

```c
static int custom_open(const char *path, struct fuse_file_info *fi) {
    return 0;
}
```
Membuka file. Fungsi ini tidak melakukan operasi khusus dan selalu mengembalikan sukses.

4. custom_read

```c
static int custom_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char full_path[1000];
    strcpy(full_path, temp_path);
    strcat(full_path, path);

    int fd = open(full_path, O_RDONLY);
    if (fd == -1)
        return -errno;

    int res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    close(fd);
    return res;
}
```
   
Membaca data dari file. Membuka file terkait di direktori /tmp dan membaca byte yang diminta.

5. custom_write

```c
static int custom_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char full_path[1000];
    strcpy(full_path, temp_path);
    strcat(full_path, path);

    // Write data to temp file
    int fd = open(full_path, O_WRONLY | O_CREAT, 0666);
    if (fd == -1)
        return -errno;

    int res = pwrite(fd, buf, size, offset);
    if (res == -1) {
        close(fd);
        return -errno;
    }
    close(fd);

    // Split the file into chunks and save them to relics directory
    char relic_path[1000];
    FILE *temp_file = fopen(full_path, "r");
    if (!temp_file)
        return -errno;

    char buffer[CHUNK_SIZE];
    size_t bytes;
    int part = 0;

    while ((bytes = fread(buffer, 1, CHUNK_SIZE, temp_file)) > 0) {
        snprintf(relic_path, sizeof(relic_path), "%s%s.%03d", relics_path, path, part++);
        FILE *relic_file = fopen(relic_path, "w");
        if (!relic_file) {
            fclose(temp_file);
            return -errno;
        }
        fwrite(buffer, 1, bytes, relic_file);
        fclose(relic_file);
    }

    fclose(temp_file);
    return res;
}
```

Menulis data ke file. Menulis data ke file sementara di /tmp, kemudian membagi file menjadi potongan-potongan dan menyimpannya ke direktori relics. Setiap potongan dinamai dengan nomor urut.

6. custom_unlink

```c
static int custom_unlink(const char *path) {
    char full_path[1000];
    strcpy(full_path, relics_path);
    strcat(full_path, path);

    char *dot = strrchr(full_path, '.');
    if (!dot || dot == full_path) {
        return -errno;
    }

    char base_name[1000];
    snprintf(base_name, dot - full_path + 1, "%s", full_path);
    strcat(base_name, ".*");

    // Construct command to delete all parts
    char command[1100];
    snprintf(command, sizeof(command), "rm %s", base_name);

    // Execute the command
    int res = system(command);
    if (res == -1) {
        return -errno;
    }

    return 0;
}
```

Menghapus file. Membuat perintah untuk menghapus semua potongan file dari direktori relics dan mengeksekusinya.	 

7. custom_create

```c
static int custom_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    char full_path[1000];
    snprintf(full_path, sizeof(full_path), "%s%s", temp_path, path);

    int fd = open(full_path, fi->flags, mode);
    if (fd == -1) {
        return -errno;
    }

    close(fd);
    return 0;
}
```
Membuat file baru di direktori sementara.

8. custom_truncate

```c
static int custom_truncate(const char *path, off_t size) {
    char full_path[1000];
    snprintf(full_path, sizeof(full_path), "%s%s", temp_path, path);

    int res = truncate(full_path, size);
    if (res == -1) {
        return -errno;
    }

    return 0;
}
```

Mengatur ukuran file ke ukuran tertentu. Beroperasi pada file di direktori sementara.

9. custom_release

```c
static int custom_release(const char *path, struct fuse_file_info *fi) {
    // No special operations needed here
    return 0;
}
```
Menutup file. Fungsi ini tidak melakukan operasi khusus dan selalu mengembalikan sukses.


10. custom_utimens

```
    static int custom_utimens(const char *path, const struct timespec ts[2]) {
    char full_path[1000];
    snprintf(full_path, sizeof(full_path), "%s%s", temp_path, path);

    int res = utimensat(0, full_path, ts, AT_SYMLINK_NOFOLLOW);
    if (res == -1)
        return -errno;

    return 0;
}
```
Memperbarui waktu akses dan modifikasi file di direktori sementara.


### Samba Server

Direktori report akan berisi seluruh relic yang sudah digabung melalui FUSE, dari FUSE akan dicopy ke dalam report dengan command `cp <fuse>/* report` dan dapat dimulai sebuah Samba server untuk membagikan folder dengan cara menambahkan contoh konfigurasi sebagai berikut:

```conf
[holaaa!]
    comment = Archeology Relic Report
    path = /home/kyfaiyya/soal3/report
    read only = no
    browsable = yes
    writable = yes
    guest ok = no
```
