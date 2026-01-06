#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

int main(int argc, char **argv)
{
        int fd;
        ssize_t size;
        char buf[32] = { 0 };
        char data[32] = "Hello Linux Kernel";

        fd = open("/dev/cpy2usr_test_dev", O_RDWR);
        if (fd < 0) {
                fprintf(stderr, "Error: %s[errno: %d]", strerror(errno), errno);
                perror("open /dev/cpy2usr_test_dev failed");
                exit(EXIT_FAILURE);
        }

        size = read(fd, buf, sizeof(buf));
        if (size == 0) {
                printf("nothing read\n");
        } else {
                printf("read from kbuf: %s", buf);
        }


        size = write(fd, data, sizeof(data));
        if(size ==0){
                printf("nothing write\n");
        }else{
                printf("write to kbuf\n");
        }

        close(fd);
        return 0;
        
}
