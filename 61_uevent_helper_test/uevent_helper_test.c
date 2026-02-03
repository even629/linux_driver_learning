#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
        int fd = open("/dev/ttyFIQ0", O_WRONLY);
        dup2(fd, STDOUT_FILENO);
        printf("SUBSYSTEM is %s\n", getenv("SUBSYSTEM"));
        return 0;
}
