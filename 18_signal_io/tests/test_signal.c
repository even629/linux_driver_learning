#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <signal.h>

static int fd;
static char buf[32];
static char prefix[32];

static volatile sig_atomic_t data_ready = 0;

void handle_sigio(int sig)
{
        data_ready = 1;
}

int main(int argc, char **argv)
{
        pid_t pid;
        int ret;

        pid = fork();
        if (pid < 0) {
                perror("[fork error]");
                exit(EXIT_FAILURE);
        } else if (pid == 0) { // child write
                int i;
                sprintf(prefix, "[child pid:%d]", getpid());
                fd = open("/dev/signal_io_test0", O_RDWR);
                if (fd < 0) {
                        perror(prefix);
                        exit(EXIT_FAILURE);
                }

                for (i = 0; i < INT_MAX; i++) {
                        sprintf(buf, "Hello num %d", i);
                        ret = write(fd, buf, strlen(buf) + 1);
                        if (ret < 0) {
                                perror(prefix);
                                close(fd);
                                exit(EXIT_FAILURE);
                        }
                        printf("%s: write: %s\n", prefix, buf);
                        sleep(1);
                }
                close(fd);

        } else { // parent read by signal
                int flags;
                sprintf(prefix, "[parent pid:%d]", getpid());
                fd = open("/dev/signal_io_test0", O_RDWR);
                if (fd < 0) {
                        perror(prefix);
                        exit(EXIT_FAILURE);
                }
                sprintf(prefix, "[parent pid:%d]", getpid());

                // 1. 注册SIGIO信号的信号处理函数

                // signal(SIGIO, handle_sigio);
                struct sigaction act;
                act.sa_handler = handle_sigio;
                sigemptyset(&act.sa_mask); //当这个信号处理函数正在执行时，哪些信号要被“暂时屏蔽”
                act.sa_flags = 0; // 不启用任何特殊行为
                sigaction(SIGIO, &act, NULL);

                // 2. 设置能接收这个信号的进程
                fcntl(fd, F_SETOWN, getpid());
                // 3. 开启信号驱动io
                flags = fcntl(fd, F_GETFL);
                fcntl(fd, F_SETFL, flags | O_ASYNC);
                for (;;) {
                        pause(); // wait for signal
                        if (data_ready) {
                                data_ready = 0;
                                ret = read(fd, buf, sizeof(buf));
                                printf("%s: read: %s\n", prefix, buf);
                        }
                }
                close(fd);
        }

        return 0;
}
