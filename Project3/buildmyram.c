
#include <minix/paths.h>

#include <sys/ioc_memory.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
	int fd;
	signed long size;
	char *d;

	if(argc < 2 || argc > 3) {
		fprintf(stderr, "usage: %s <size in MB> [device]\n",
			argv[0]);
		return 1;
	}

	d = argc == 2 ? _PATH_RAMDISK : argv[2];
	if((fd=open(d, O_RDONLY)) < 0) {
		perror(d);
		return 1;
	}

#define MFACTOR (1024*1024) 
	size = atol(argv[1])*MFACTOR;

	if(size < 0) {
		fprintf(stderr, "size should be non-negative.\n");
		return 1;
	}

	if(ioctl(fd, MIOCRAMSIZE, &size) < 0) {
		perror("MIOCRAMSIZE");
		return 1;
	}

	fprintf(stderr, "size on %s set to %ldMB\n", d, size/MFACTOR);

	return 0;
}

