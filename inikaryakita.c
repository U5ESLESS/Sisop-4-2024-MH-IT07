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

static const char *dirpath = "/path/to/gallery";
static const char *watermark = "inikaryakita.id";

static int xmp_getattr(const char *path, struct stat *stbuf) {
    int res;
    char fpath[1024];
    sprintf(fpath, "%s%s", dirpath, path);
    res = lstat(fpath, stbuf);
    if (res == -1)
        return -errno;
    return 0;
}

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
