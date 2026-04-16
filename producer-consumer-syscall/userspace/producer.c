#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
//#include "P.h"

int main()
{
    long int s = syscall(449);
    printf("%d\n",s);
    //reading random number from the dev/random

    //mknod("fifo",0666|S_IFIFO,0);

    int  fd = open("/dev/urandom", O_WRONLY);
    char buf[8];

    printf("random number written to enqueued to queue is:\n");
    // for(int i = 0; i < 8;i++)
    // {
    //     printf("%c",buf[i]);
    // }

    srand(100);   
    
    //printf("number to be queued is:\n");
    int buf1[19];
    long long int num;
    for(int i = 0;i<19;i++)
        {buf1[i] = rand()%9 + 1;
            
            num = num*10 + buf1[i];
        }
        
        printf("%lld\n",num);

    int mkret = mknod("fifo",0667|S_IFIFO,0);
    int fw = open("fifo",O_WRONLY);

 

    
   write(fw,buf1,sizeof(buf1));
   close(fd);

    return 0;


    size_t result= read(fd, (void*)write,8);
    long int v = syscall(450,write);

    close(fw);

}
