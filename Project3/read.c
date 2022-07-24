#include <stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/stat.h>


int main() 
{
	int f;
	char s[20];
	f = open("/dev/pipe_test", O_RDONLY, S_IREAD);
	
	if(f != -1)
	{
		read(f, s, 13);
		printf("%s\n", s);
		return 0;
	}
	else {
		printf("file open failed...\n");
		return -1;
	}
}
