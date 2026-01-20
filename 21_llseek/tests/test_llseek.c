#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define DEV_PATH "/dev/llseek_test0"
#define BUF_SIZE 64

static void dump_buf(const char *tag, const char *buf, ssize_t len)
{
        printf("%s (%zd bytes): \"", tag, len);
        for (ssize_t i = 0; i < len; i++) {
                if (buf[i] >= 32 && buf[i] <= 126)
                        putchar(buf[i]);
                else
                        printf("\\x%02x", (unsigned char)buf[i]);
        }
        printf("\"\n");
}

int main(void)
{
        int fd;
        char buf[BUF_SIZE];
        ssize_t ret;
        off_t off;

        printf("open %s\n", DEV_PATH);
        fd = open(DEV_PATH, O_RDWR);
        if (fd < 0) {
                perror("open");
                return 1;
        }

        /* ================= write ================= */
        const char *msg = "Hello llseek test!";
        printf("\n[TEST] write \"%s\"\n", msg);

        ret = write(fd, msg, strlen(msg));
        if (ret < 0) {
                perror("write");
                goto out;
        }
        printf("write ret = %zd\n", ret);

        /* ================= SEEK_SET ================= */
        printf("\n[TEST] lseek SEEK_SET 0\n");
        off = lseek(fd, 0, SEEK_SET);
        if (off < 0) {
                perror("lseek SEEK_SET");
                goto out;
        }
        printf("current offset = %ld\n", off);

        memset(buf, 0, sizeof(buf));
        ret = read(fd, buf, sizeof(buf));
        if (ret < 0) {
                perror("read");
                goto out;
        }
        dump_buf("read", buf, ret);

        /* ================= SEEK_CUR ================= */
        printf("\n[TEST] lseek SEEK_CUR -6\n");
        off = lseek(fd, -6, SEEK_CUR);
        if (off < 0) {
                perror("lseek SEEK_CUR");
        } else {
                printf("current offset = %ld\n", off);
        }

        memset(buf, 0, sizeof(buf));
        ret = read(fd, buf, 6);
        dump_buf("read", buf, ret);

        /* ================= SEEK_END ================= */
        printf("\n[TEST] lseek SEEK_END -5\n");
        off = lseek(fd, -5, SEEK_END);
        if (off < 0) {
                perror("lseek SEEK_END");
        } else {
                printf("current offset = %ld\n", off);
        }

        memset(buf, 0, sizeof(buf));
        ret = read(fd, buf, 5);
        dump_buf("read", buf, ret);

        /* ================= EOF ================= */
        printf("\n[TEST] read until EOF\n");
        off = lseek(fd, 0, SEEK_SET);
        printf("seek to %ld\n", off);

        while (1) {
                ret = read(fd, buf, 8);
                if (ret == 0) {
                        printf("EOF reached\n");
                        break;
                }
                if (ret < 0) {
                        perror("read");
                        break;
                }
                dump_buf("chunk", buf, ret);
        }

        /* ================= invalid lseek ================= */
        printf("\n[TEST] invalid lseek (beyond end)\n");
        off = lseek(fd, 100, SEEK_SET);
        if (off < 0)
                printf("expected error: %s\n", strerror(errno));
        else
                printf("unexpected success, off=%ld\n", off);

out:
        close(fd);
        return 0;
}
