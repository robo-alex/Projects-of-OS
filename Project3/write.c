#include <stdio.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>

int main()
{
	int f;
	char s[] = "Hello World!";
	f = open("/dev/pipe_test", O_WRONLY, S_IWRITE);
	
	if(f != -1)
	{
		write(f, s, 13);
		return 0;
	}
	else {
		printf("file open failed...\n");
		return -1;
	}
}
