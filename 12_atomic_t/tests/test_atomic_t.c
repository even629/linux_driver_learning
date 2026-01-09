#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
        pid_t pid;
        int fd;

        pid = fork();

        if (pid < 0) {
                goto fail;
        } else if (pid == 0) { // child process
                fd = open("/dev/atomic_t_test0", O_RDWR);
                if (fd < 0)
                        goto fail;

                printf("child open device success\n");
                // do something
                sleep(3);
                close(fd);

        } else { // parent process
                fd = open("/dev/atomic_t_test0", O_RDWR);
                if(fd < 0){
                        goto fail;
                }
                printf("parent open device success\n");
                // do something
                sleep(3);
                close(fd);
        }

        return 0;
fail:
        fprintf(stderr, "Error:%s[errno:%d]\n", strerror(errno), errno);
        exit(EXIT_FAILURE);
}
