#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"

char buf[1] = { 0 };

int
main(int argc, char *argv[])
{
	int fd, i;

	if(argc <= 1){
		exit();
	}

	for(i = 1; i < argc; i++){
		if((fd = open(argv[i], O_WRONLY)) < 0){
			printf("writejunk: cannot open %s\n", argv[i]);
			exit();
		}
        write(fd, buf, 1);
		close(fd);
	}
	exit();
}