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

void get_full_path(char fpath[1024], const char *path)
{
    sprintf(fpath, "%s%s", source_dir, path);
}

int is_tujuan(const char *path)
{
    return strcmp(path, "/tujuan.txt") == 0;
}

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

static int xmp_getattr(const char *path, struct stat *stbuf)
{
    int res;
    char fpath[1024];

    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {

        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;

        return 0;
    }

    if (is_tujuan(path)) {

        char content[4096];
        generate_tujuan(content);

        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen(content);

        return 0;
    }

    get_full_path(fpath, path);

    res = lstat(fpath, stbuf);

    if (res == -1)
        return -errno;

    return 0;
}

static int xmp_readdir(const char *path,
                       void *buf,
                       fuse_fill_dir_t filler,
                       off_t offset,
                       struct fuse_file_info *fi)
{
    (void) offset;
    (void) fi;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    DIR *dp;
    struct dirent *de;

    dp = opendir(source_dir);

    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {

        struct stat st;

        memset(&st, 0, sizeof(st));

        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;

        filler(buf, de->d_name, &st, 0);
    }

    closedir(dp);

    filler(buf, "tujuan.txt", NULL, 0);

    return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
    char fpath[1024];

    if (is_tujuan(path))
        return 0;

    get_full_path(fpath, path);

    int fd = open(fpath, O_RDONLY);

    if (fd == -1)
        return -errno;

    close(fd);

    return 0;
}

static int xmp_read(const char *path,
                    char *buf,
                    size_t size,
                    off_t offset,
                    struct fuse_file_info *fi)
{
    (void) fi;

    if (is_tujuan(path)) {

        char content[4096];
        generate_tujuan(content);

        size_t len = strlen(content);

        if (offset < len) {

            if (offset + size > len)
                size = len - offset;

            memcpy(buf, content + offset, size);

        } else {
            size = 0;
        }

        return size;
    }

    char fpath[1024];

    get_full_path(fpath, path);

    int fd = open(fpath, O_RDONLY);

    if (fd == -1)
        return -errno;

    int res = pread(fd, buf, size, offset);

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
};

int main(int argc, char *argv[])
{
    if (argc < 3) {

        fprintf(stderr,
                "Usage: %s <source_dir> <mount_point>\n",
                argv[0]);

        exit(EXIT_FAILURE);
    }

    realpath(argv[1], source_dir);

    argv[1] = argv[2];
    argc--;

    return fuse_main(argc, argv, &xmp_oper, NULL);
}
