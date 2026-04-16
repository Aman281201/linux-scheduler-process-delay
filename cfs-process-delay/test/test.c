

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>
#include <unistd.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h> 
#include <errno.h> 
//#include <linux/pid.h>

void func(){
	long sum = 0;
	for(int i=0;i<500000000;i++){
		sum+=i;
	}
	return;
}

extern int errno;



int main()
{


	// struct pid *pid_struct = NULL;
	// struct task_struct *process;
	



	int errnum;
	printf("Without Delay- \n");
	//double start_wo = omp_get_wtime();
	for(int i=1;i<=10;i++){
		int cpid = fork();
		if(cpid==0){
			syscall(448,getpid(),0);
			double start_wo = omp_get_wtime();
			func();
			double time_wo = omp_get_wtime() - start_wo;
			printf("Process finished in %f seconds \n",time_wo);
			exit(0);
		}
	}
	for(int i=1;i<=10;i++){
		wait(NULL);
	}

	printf("With Delay of 5 ms and above- \n");
	//double start_w = omp_get_wtime();
	for(int i=1;i<=10;i++){
		int cpid = fork();
		if(cpid==0){
			//double start_w = omp_get_wtime();
			long errval =0;
			// pid_struct = find_get_pid(getpid());

			// process = pid_task(pid_struct, PIDTYPE_PID);
			// process->se.vruntime += 50000000*i;


			errval = syscall(448,getpid(),1000);
			double start_w = omp_get_wtime();
			if(errval!=0){
				errnum = errno;
				fprintf(stderr,"Value of errno: %d\n", errno);
				perror("ERROR");
				fprintf(stderr,"Error encountered: %s\n", strerror(errnum));
			}
			func();
			double time_w = omp_get_wtime() - start_w;
			printf("Process with RT finished in %f seconds \n",time_w);
			exit(0);
		}
	}
	for(int i=1;i<=10;i++){
		wait(NULL);
	}

	int errcheck;
	printf("ERROR CHECK (Y=1 or 2/N=3) ");
	scanf("%d",&errcheck);
	if(errcheck==1){
		long errval=0;
		//errval = syscall(441,-50,-50);
		if(errval!=0){
			errnum = errno;
			fprintf(stderr,"Value of errno: %d\n", errno);
			perror("ERROR");
			fprintf(stderr,"Error encountered: %s\n", strerror(errnum));
		}
		
	}
	else if(errcheck==2){
		long errval = 0;
		//errval = syscall(441,20000,50);
		if(errval!=0){
			errnum = errno;
			fprintf(stderr,"Value of errno: %d\n", errno);
			perror("ERROR");
			fprintf(stderr,"Error encountered: %s\n", strerror(errnum));
		}
	}

	return 0;
}
