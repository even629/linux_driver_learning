#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <poll.h>

void gpio_export(int gpio_num);

void gpio_unexport(int gpio_num);

void gpio_ctrl(char *gpio_path, char *attr, char *value);

int gpio_interrupt(char *gpio_path, char **attrs, int nr_attrs);

int main(int argc, char **argv)
{
        int gpio_num;
        char *endptr;
        char gpio_path[64];
        char *int_attr[] = { "value" };

        if(argc < 2){
                printf("usage: gpio_test [gpio_num]\n");
                exit(EXIT_FAILURE);
        }

        gpio_num = strtol(argv[1], &endptr, 10);
        if (*endptr != '\0') {
                perror("gpio num error");
                exit(EXIT_FAILURE);
        }

        sprintf(gpio_path, "/sys/class/gpio/gpio%d", gpio_num);

        if (access(gpio_path, F_OK) != 0)
                gpio_export(gpio_num);

        gpio_ctrl(gpio_path, "direction", "out");        
        gpio_ctrl(gpio_path, "value", "1");

        // gpio_ctrl(gpio_path, "direction", "in");
        gpio_interrupt(gpio_path, int_attr,
                       sizeof(int_attr) / sizeof(int_attr[0])); // 监听GPIO引脚的中断事件

        gpio_unexport(gpio_num);

        return 0;
}

void gpio_export(int gpio_num)
{
        int fd;
        ssize_t cnt;
        char buf[32];

        sprintf(buf, "%d", gpio_num);
        fd = open("/sys/class/gpio/export", O_WRONLY);
        if (fd < 0) {
                perror("open error");
                exit(EXIT_FAILURE);
        }

        cnt = write(fd, buf, strnlen(buf, 32));
        if (cnt <= 0) {
                printf("export gpio%d error\n", gpio_num);
                close(fd);
                exit(EXIT_FAILURE);
        }

        close(fd);
}

void gpio_unexport(int gpio_num)
{
        int fd;
        ssize_t cnt;
        char buf[32];

        sprintf(buf, "%d", gpio_num);
        fd = open("/sys/class/gpio/unexport", O_WRONLY);
        if (fd < 0) {
                perror("open error");
                exit(EXIT_FAILURE);
        }

        cnt = write(fd, buf, strnlen(buf, 32));
        if (cnt < 0) {
                printf("export gpio%d error\n", gpio_num);
                close(fd);
                exit(EXIT_FAILURE);
        }

        close(fd);
}

void gpio_ctrl(char *gpio_path, char *attr, char *value)
{
        int fd;
        char attr_path[128];
        ssize_t cnt;
        sprintf(attr_path, "%s/%s", gpio_path, attr);
        fd = open(attr_path, O_WRONLY);
        if (fd < 0) {
                perror("open error");
                exit(EXIT_FAILURE);
        }
        cnt = write(fd, value, strlen(value));
        if (cnt < 0) {
                printf("ctrl %s error\n", attr_path);
                close(fd);
                exit(EXIT_FAILURE);
        }
        close(fd);
}

int gpio_interrupt(char *gpio_path, char **attrs, int nr_attrs)
{
        int fd, i, opened;
        int ret;
        int cnt;
        char file_path[128];
        struct pollfd *fds;
        char buf[64] = { 0 };

        fds = (struct pollfd *)malloc(sizeof(struct pollfd) * nr_attrs);
        memset((void *)fds, 0, sizeof(struct pollfd) * nr_attrs);

        for (i = 0; i < nr_attrs; i++) {
                sprintf(file_path, "%s/%s", gpio_path, attrs[i]);
                fd = open(file_path, O_RDONLY);
                if (fd < 0) {
                        printf("open %s error, stop trying to open\n", file_path);
                        break;
                }

                read(fd, buf, sizeof(buf)); // 清除首次中断触发

                fds[i].fd = fd;
                fds[i].events = POLLPRI; //GPIO sysfs 中断通过 “紧急数据” 通知，对应 POLLPRI。
        }
        opened = i;

        for (;;) {
                ret = poll(fds, opened, -1);
                if (ret <= 0) {
                        perror("poll error");
                        goto clean;
                }

                // 检查是哪个触发了事件
                for (i = 0; i < opened; i++) {
                        if (fds[i].revents & (POLLPRI)) {
                                lseek(fds[i].fd, 0,
                                      SEEK_SET); //sysfs GPIO 的 value 文件是一次性可读的
                                cnt = read(fds[i].fd, buf, sizeof(buf) - 1); // 读取当前触发的值
                                if (cnt > 0 && cnt < sizeof(buf))
                                        buf[cnt] = '\0';
                                printf("value is %s\n", buf);
                        }
                }
        }

clean:
        for (i = 0; i < opened; i++)
                close(fds[i].fd);

        free(fds);

        return ret;
}
