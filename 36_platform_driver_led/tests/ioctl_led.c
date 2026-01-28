#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define LED_OPEN _IO('A', 0)
#define LED_CLOSE _IO('A', 1)
#define LED_STATUS _IOR('A', 2, int)

int main(int argc, char **argv)
{
        int fd, ret;
        uint32_t val = 0;
        fd = open("/dev/led0", O_RDWR);
        if (fd < 0)
                goto fail;

        if (argc == 1) { // get status
                ret = ioctl(fd, LED_STATUS, &val);
                if (ret < 0)
                        goto fail;
                printf("read val: %u success\n", val);
        } else if (argc >= 2) { // open or close
                val = atoi(argv[1]);
                if (val < 0) {
                        printf("unsupported\n");
                        exit(EXIT_FAILURE);
                } else if (val == 0) {
                        ret = ioctl(fd, LED_CLOSE);
                        if (ret < 0)
                                goto fail;
                } else {
                        ret = ioctl(fd, LED_OPEN);
                        if (ret < 0)
                                goto fail;
                }

        } else {
                perror("argc error\n");
                exit(EXIT_FAILURE);
        }

        return 0;
fail:
        perror("[test_led]: ");
        exit(EXIT_FAILURE);
}
