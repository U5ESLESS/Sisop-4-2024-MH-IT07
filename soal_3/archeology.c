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

static void set_permissions(const char *path) {
    // Set the directory permissions to 755
    chmod(path, 0755);
}

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

static int custom_open(const char *path, struct fuse_file_info *fi) {
    return 0;
}

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

static int custom_truncate(const char *path, off_t size) {
    char full_path[1000];
    snprintf(full_path, sizeof(full_path), "%s%s", temp_path, path);

    int res = truncate(full_path, size);
    if (res == -1) {
        return -errno;
    }

    return 0;
}

static int custom_release(const char *path, struct fuse_file_info *fi) {
    // No special operations needed here
    return 0;
}

static int custom_utimens(const char *path, const struct timespec ts[2]) {
    char full_path[1000];
    snprintf(full_path, sizeof(full_path), "%s%s", temp_path, path);

    int res = utimensat(0, full_path, ts, AT_SYMLINK_NOFOLLOW);
    if (res == -1)
        return -errno;

    return 0;
}

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

