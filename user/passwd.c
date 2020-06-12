#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

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
main(int argc, char *argv[])
{
    char buf[100];
    int n = 0;

    hideinp();

    printf("Enter your old password: ");
    if(getinput(buf, sizeof(buf)) < 0) exit();
    n = strlen(buf);
    char oldpwd[n];
    safestrcpy(oldpwd, buf, n);

    printf("\nEnter your new password: ");
    if(getinput(buf, sizeof(buf)) < 0) exit();
    n = strlen(buf);
    char newpwd[n];
    safestrcpy(newpwd, buf, n);

    printf("\nEnter your new password once more: ");
    if(getinput(buf, sizeof(buf)) < 0) exit();
    n = strlen(buf);
    char newpwdagain[n];
    safestrcpy(newpwdagain, buf, n);

    showinp();

    printf("\n");

    if(strcmp(newpwd, newpwdagain) != 0) {
        printf("Your new password didn't match.\n");
        exit();
    }
    char *user = malloc(128 * sizeof(char));
    if (argc == 2) {
        safestrcpy(user, argv[1], strlen(argv[1]) + 1);
    } else {
        if(getusername(getuid(), &user) != 0) {
            printf("User not found.\n");
            exit();
        }
    }
    int code = changepass(user, oldpwd, newpwd);
    if(code == NOTFOUND) {
        printf("User not found.\n");
    } else if(code == AUTHFAIL) {
        printf("Password change failed. Authentication error\n");
    } else if (code < 0) {
        printf("Unknown error\n");
    } else {
        printf("Password changed successfully.\n");
    }
    exit();
}