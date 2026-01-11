#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv)
{
        pid_t pid;
        int fd;
        char prefix[16];        

        pid = fork();
        if (pid < 0) {
                perror("fork error");
                exit(EXIT_FAILURE);
        } else if (pid == 0) {
                snprintf(prefix, sizeof(prefix), "[pid: %d]", getpid());

                fd = open("/dev/mutex_test0", O_RDWR);
                if (fd < 0) {
                        perror(prefix);
                        exit(EXIT_FAILURE);
                }
                printf("%s child open success\n", prefix);
                sleep(2);
                close(fd);

        } else {
                snprintf(prefix, sizeof(prefix), "[pid: %d]", getpid());

                fd = open("/dev/mutex_test0", O_RDWR);
                if (fd < 0) {
                        perror(prefix);
                        exit(EXIT_FAILURE);
                }
                printf("%s parent open success\n", prefix);
                sleep(2);
                close(fd);
        }

        return 0;
}
