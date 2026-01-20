#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <assert.h>

#define TEST_CLEAR _IO('L', 0)

int main(int argc, char **argv)
{
        int fd;
        int ret;
        char buf[64] = { 0 };
        fd = open("/dev/ioctl_test0", O_RDWR);
        if (fd < 0)
                exit(EXIT_FAILURE);
        snprintf(buf, sizeof(buf), "Hello World");
        ret = write(fd, buf, strlen(buf) + 1);
        if (ret < 0)
                exit(EXIT_FAILURE);

        ret = ioctl(fd, TEST_CLEAR);
        if (ret < 0)
                exit(EXIT_FAILURE);

        ret = read(fd, buf, sizeof(buf));
        if(ret < 0)
                exit(EXIT_FAILURE);

        assert(strlen(buf) == 0);

        return 0;
}
