#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
    int dev = open("/dev/mydevice", O_RDONLY);
    if(dev == -1){
        printf("Openning was not possible\n");
        return -1;
    }

    printf("Openning was sucessuful\n");
    close(dev);
    return 0;
}