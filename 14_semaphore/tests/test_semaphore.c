#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv)
{
        pid_t pid;
        int fd;

        pid = fork();
        if (pid < 0) {
                perror("fork error");
                exit(EXIT_FAILURE);
        } else if (pid == 0) { // child
                fd = open("/dev/semaphore_test0", O_RDWR);
                if (fd < 0) {
                        perror("child open /dev/semaphore_test0 error");
                        exit(EXIT_FAILURE);
                }
                printf("[child: pid<%d>] open /dev/semaphore_test0 success\n", getpid());
                sleep(2);
                close(fd);
        } else { // parent
                fd = open("/dev/semaphore_test0", O_RDWR);
                if (fd < 0) {
                        perror("parent open /dev/semaphore_test0 error");
                        exit(EXIT_FAILURE);
                }
                printf("[parent: pid<%d>] open /dev/semaphore_test0 success\n", getpid());
                sleep(2);
                close(fd);
        }

        return 0;
}
