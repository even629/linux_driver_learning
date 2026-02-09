#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

int main(int argc, char **argv)
{
        int fd, ret = 0;

        struct input_event evt;

        fd = open("/dev/input/event3", O_RDONLY);
        if (fd < 0) {
                perror("open /dev/input/event3 error");
                exit(EXIT_FAILURE);
        }

        while (1) {
                ret = read(fd, &evt, sizeof(struct input_event));
                if (ret < 0)
                        goto err;

                switch (evt.type) {
                case EV_KEY:
                        switch (evt.code) {
                        case KEY_1:
                                switch (evt.value) {
                                case 0:
                                        printf("value is 0\n");
                                        break;
                                case 1:
                                        printf("value is 1\n");
                                        break;
                                case 2:
                                        printf("value is 2\n");
                                        break;
                                default:
                                        printf("no support, event.value is %d\n", evt.value);
                                        break;
                                }

                                break;
                        default:
                                printf("no support, event.code is %d\n", evt.code);
                                break;
                        }

                        break;
                case EV_SYN:
                        printf("SYN\n");
                        break;
                default:
                        
                        printf("no support, event.type is %d\n", evt.type);
                        break;
                }
        }

        return 0;
err:
        close(fd);

        return ret;
}
