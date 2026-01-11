#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char **argv)
{
        pid_t pid;
        int fd = -1;

        pid = fork();
        if (pid < 0) {
                perror("fork error");
                fprintf(stderr, "Error: %s[errno: %d]\n", strerror(errno), errno);
                exit(EXIT_FAILURE);
        } else if (pid == 0) { // child process
                printf("[child: <pid: %d>] try to open /dev/spinlock_test0\n",getpid());
                fd = open("/dev/spinlock_test0", O_RDWR);
                if (fd < 0) {
                        perror("[child] open error");
                        fprintf(stderr, "[child] Error: %s[errno: %d]\n", strerror(errno), errno);
                        exit(EXIT_FAILURE);
                }
                printf("[child: <pid: %d>] open /dev/spinlock_test0 success\n", getpid());
                sleep(2);
                close(fd);

        } else { // parent process
                printf("[parent: <pid: %d>] try to open /dev/spinlock_test0\n", getpid());
                fd = open("/dev/spinlock_test0", O_RDWR);
                if (fd < 0) {
                        perror("[parent] open error");
                        fprintf(stderr, "[parent] Error: %s[errno: %d]\n", strerror(errno), errno);
                        exit(EXIT_FAILURE);
                }
                sleep(2);
                printf("[parent: <pid: %d>] open /dev/spinlock_test0 success\n", getpid());

                close(fd);
        }

        return 0;
}
