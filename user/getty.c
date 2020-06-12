#include "kernel/types.h"
#include "user.h"
#include "kernel/fcntl.h"

char *argv[] = { "sh", 0 };

void
writeMessage(char *path) {
	int fd;
	char buf[128];

	fd = open(path, O_RDONLY);
	if(fd < 0) exit();
	read(fd, buf, sizeof(buf));
	printf("%s\n", buf);
	close(fd);
}

int
getinput(char *buf, int nbuf)
{
	memset(buf, 0, nbuf);
	gets(buf, nbuf);
	if(buf[0] == 0) // EOF
		return -1;
	return 0;
}

int
getty(int *uid, int *ngroups, int **gids) {
    static char buf[100];

	int n;

    printf("Enter your username: ");
    if(getinput(buf, sizeof(buf)) < 0) exit();
	n = strlen(buf);
	char username[n];
    safestrcpy(username, buf, n);

    printf("Enter your password: ");
    if(getinput(buf, sizeof(buf)) < 0) exit();
	n = strlen(buf);
	char password[n];
    safestrcpy(password, buf, n);

    int success = authuid(username, password, uid);

	if(authuid < 0) {
		return success;
	}
	getgids(username, ngroups, gids);
	return success;
}

int
main(void) {
    int uid;
	int fd;

	// Ensure that three file descriptors are open.
	while((fd = open("/dev/console", O_RDWR)) >= 0){
		if(fd >= 3){
			close(fd);
			break;
		}
	}
	
	if(getuid() != 0) {
		printf("Improper way to enter getty. Close the shell and getty will reopen automatically for you.\n");
		exit();
	}

	clrscr();
	writeMessage("/etc/issue");
	int ngroups = 0;
	int *gids = 0;
	while(getty(&uid, &ngroups, &gids)) {
		printf("Invalid user or missing home directory. Please try again\n");
	}
	writeMessage("/etc/motd");
	if(setgrps(ngroups, gids)) {
		printf("Major permission issue\n");
	}
	if(setuid(uid)) {
		printf("Major permission issue\n");
	}
	exec("/bin/sh", argv);
	printf("getty: exec sh failed\n");
	exit();
}