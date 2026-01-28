#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
        int fd, ret;
        uint32_t val = 0;
        fd = open("/dev/led0", O_RDWR);
        if (fd < 0)
                goto fail;

        if (argc == 1) { // read
                ret = read(fd, &val, 4);
                if (ret < 0)
                        goto fail;
                printf("read:0x%08x\n", val);
        } else if (argc >= 2) { // write
                val = atoi(argv[1]);
                if (val < 0) {
                        perror("unsupported\n");
                        exit(EXIT_FAILURE);
                }
                ret = write(fd, &val, 4);
                if (ret < 0)
                        goto fail;
                printf("write val: %u success\n", val);

        } else {
                perror("argc error\n");
                exit(EXIT_FAILURE);
        }

        return 0;
fail:
        perror("[test_led]: ");
        exit(EXIT_FAILURE);
}
