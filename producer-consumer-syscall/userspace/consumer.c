#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
//#include "P.h"

int main()
{
    //reading from file
    printf("number dequeued from fifo is:\n");
    long int w = syscall(451);
    
    int fd = open("fifo",O_RDONLY);
    char buf[19];
    read(fd,buf,sizeof(char)*19);

    for(int i = 0; i < 19; i++)
    {
        printf("%d",buf[i]);
    }
    close(fd);
    return 0;
    printf("%d",w);
}