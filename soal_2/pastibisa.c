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
 
