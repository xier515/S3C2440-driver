#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#define INPUT_REPLAY 0

void print_usage(char *file)
{
	printf("Usage:\n");
	printf("%s write <file>\n", file);
	printf("%s replay\n", file);
}

int main(int argc, char ** argv)
{
	int fd;
	int fd_data;
	int buf[100];
	int len;
	if(argc != 2 && argc != 3 )
	{
		print_usage(argv[0]);
		return -1;
	}
	fd = open("/dev/input_emu", O_RDWR);
	if(fd < 0)
	{
		printf("can't open /dev/input_emu\n");
		return -1;
	}
	if(strcmp(argv[1], "replay") == 0)
	{
		ioctl(fd, INPUT_REPLAY);
		
	}
	else if(strcmp(argv[1], "write") == 0)
	{
		if(argc != 3)
		{
			print_usage(argv[0]);
			return -1;
		}
		fd_data = open(argv[2], O_RDONLY);
		if(fd_data < 0)
		{
			printf("can't open %s\n", argv[2]);
			return -1;
		}
		while(1)
		{
			len = read(fd_data, buf, 100);
			if(len == 0)
			{
				printf("write sucesse.\n");
				break;
			}
			else
			{
				write(fd, buf, len);
			}
		}
	}
	return 0;
}
