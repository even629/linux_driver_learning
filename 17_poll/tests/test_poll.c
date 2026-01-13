#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <poll.h>

int main(int argc, char **argv)
{
        pid_t pid;
        int fd, ret;
        char prefix[32];
        int ops_nr = 10;

        pid = fork();

        if (pid < 0) {
                perror("[fork error]");
                exit(EXIT_FAILURE);
        } else if (pid == 0) { //child
                char buf[32];
                int i;
                sprintf(prefix, "[child, pid:%d]:", getpid());

                fd = open("/dev/poll_test0", O_RDWR);
                if (fd < 0) {
                        fprintf(stderr, "%s Error: %s(errno:%d)\n", prefix, strerror(errno), errno);
                        exit(EXIT_FAILURE);
                }
                // child write for about 10 secs
                for (i = 0; i < ops_nr; i++) {
                        sprintf(buf, "%d", i);
                        ret = write(fd, buf, strlen(buf) + 1);
                        if (ret < 0) {
                                close(fd);
                                fprintf(stderr, "%s Error: %s(errno:%d)\n", prefix, strerror(errno),
                                        errno);
                                exit(EXIT_FAILURE);
                        }
                        sleep(1);
                }
                close(fd);

        } else { // parent
                char buf[32];
                struct pollfd poll_fds[1];

                sprintf(prefix, "[parent, pid:%d]:", getpid());

                fd = open("/dev/poll_test0", O_RDWR);
                if (fd < 0) {
                        fprintf(stderr, "%s Error: %s(errno:%d)\n", prefix, strerror(errno), errno);
                        exit(EXIT_FAILURE);
                }
                poll_fds[0].fd = fd;
                poll_fds[0].events = POLLIN;

                for (;;) {
                        ret = poll(poll_fds, sizeof(poll_fds) / sizeof(struct pollfd), 3000);
                        if (ret == 0) {
                                printf("timeout\n");
                        } else if (ret < 0) {
                                close(fd);
                                fprintf(stderr, "%s Error: %s(errno:%d)\n", prefix, strerror(errno),
                                        errno);
                                exit(EXIT_FAILURE);
                        } else {
                                if (poll_fds[0].revents & POLLIN) {
                                        ret = read(fd, buf, sizeof(buf));
                                        if (ret < 0) {
                                                close(fd);
                                                fprintf(stderr, "%s Error: %s(errno:%d)\n", prefix,
                                                        strerror(errno), errno);
                                                exit(EXIT_FAILURE);
                                        }
                                        printf("%s read: %s, read ret: %d\n", prefix, buf, ret);
                                        ops_nr--;
                                        if (ops_nr == 0) {
                                                break;
                                        }
                                } else {
                                        printf("%s poll_fds[0].revents is %d\n", prefix,
                                               poll_fds[0].revents);
                                }
                        }
                }

                close(fd);
        }

        return 0;
}
