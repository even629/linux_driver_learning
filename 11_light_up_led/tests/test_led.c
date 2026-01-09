#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

int main(int argc, char **argv)
{
        int fd, err;
        uint32_t val = 0;
        fd = open("/dev/test_led0", O_RDWR);
        if (fd < 0)
                goto fail;

        if (argc == 1) { // read
                err = read(fd, &val, 4);
                if (err < 0)
                        goto fail;
                printf("read:%u\n", val);
        } else if (argc >= 2) { // write
                val = atoi(argv[1]);
                if (val < 0) {
                        perror("unsupported\n");
                        exit(EXIT_FAILURE);
                }
                err = write(fd, &val, 4);
                if (err < 0)
                        goto fail;
                printf("write val: %u success\n", val);

        } else {
                perror("argc error\n");
                exit(EXIT_FAILURE);
        }

        return 0;
fail:
        fprintf(stderr, "Error:%s[errno:%d]", strerror(errno), errno);
        exit(EXIT_FAILURE);
}
