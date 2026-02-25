#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char** argv){
        int ret;
        int fd;
        int16_t raw_temp;
        float temperature;

        fd = open("/dev/ds18b20", O_RDWR);

        if(fd < 0){
                ret = -1;
                printf("open /dev/ds18b20 error\n");
                goto err_open;
        }

        ret = read(fd, &raw_temp, sizeof(int16_t));
        if(ret < 0){
                ret = -1;
                printf("read /dev/ds18b20 error\n");
                goto err_read;
        }
        
        temperature = raw_temp / 16.0f;
        
        printf("temperature: %f ℃ \n", temperature);
        

        close(fd);
        return 0;
 err_read:
        close(fd);
 err_open:
        return ret;
}
