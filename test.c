#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>

int main()
{
	void * mtx=NULL;
	int i=0,n=100;
	time_t now;
	struct tm * timeinfo;
	//first mutex init
	if(syscall(331,&mtx,3) ==0)
	{
		printf("eroare");
		return -1;
	}
	
	for(i=0;i<n;i++)
	{
		int pid = fork();
		if(pid==0)
		{
			//mutex lock
			if(syscall(331,&mtx,1)==0)
			{
				printf("eroare");
				return -1;
			}
			//simulate work
			
			time(&now);
			timeinfo = localtime(&now);
			printf("%02d:%02d:%d -  Critical section %d\n",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,getpid());
			sleep(1);
			
			//mutex unlock
			if(syscall(331,&mtx,2)==0)
			{
				printf("eroare");
				return -1;
			}
			return 0;
		}
	}
	
	for(i=0;i<n;i++)
	{
		wait(NULL);
	}

	return 0;
}
