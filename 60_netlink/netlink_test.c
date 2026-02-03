#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/netlink.h>
#include <unistd.h>

#define UEVENT_BUF_SIZE 8192

int main(int argc, char *argv[])
{
        int ret;
        int len = 0, i = 0;
        int socket_fd;
        char buf[UEVENT_BUF_SIZE] = { 0 };        
        struct sockaddr_nl nl = {
                .nl_family = AF_NETLINK,
                .nl_pid = 0, // 由内核分配 Port ID（用户空间通常设为 0）
                .nl_groups = 1, // 只接收属于属于基本组的内核事件
        };

        socket_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_KOBJECT_UEVENT);
        if (socket_fd < 0) {
                perror("socket");
                exit(EXIT_FAILURE);
        }
        ret = bind(socket_fd, (const struct sockaddr *)&nl, sizeof(struct sockaddr_nl));
        if (ret < 0) {
                perror("bind");
                close(socket_fd);
                exit(EXIT_FAILURE);
        }

        printf("Listening for uevents...\n");

        while (1) {
                memset(buf, 0, sizeof(buf));
                len = recv(socket_fd, buf, sizeof(buf), 0);
                if (len < 0) {
                        perror("recv");
                        break;
                }

                for (i = 0; i < len && i < sizeof(buf) - 1; i++) {
                        if (*(buf + i) == '\0') {
                                buf[i] = '\n';
                        }
                }
                buf[len] = '\0'; // 确保字符终止

                printf("\n--- Uevent Received ---\n%s", buf);
        }

        close(socket_fd);
        return 0;
}
