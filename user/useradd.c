#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
    /*if(getuid() != 0) {
        printf("This program is only available while logged into Superuser.\n");
        exit();
    }*/
    if(argc % 2 != 0) {
        printf("usage: useradd [-d dir] [-u uid] [-c name] login\n");
        exit();
    }
    char *username;
    char *homedir = 0;
    char *fullname = 0;
    int uid = 0;

    int i, n;
    for(i = 1; i < argc - 1; i++) {
        if(strcmp(argv[i], "-c") == 0) {
            i++;
            n = strlen(argv[i]) + 1;
            fullname = malloc(n * sizeof(*fullname));
            safestrcpy(fullname, argv[i], n);
        } else if(strcmp(argv[i], "-d") == 0) {
            i++;
            n = strlen(argv[i]) + 1;
            homedir = malloc(n * sizeof(*homedir));
            safestrcpy(homedir, argv[i], n);
        } else if(strcmp(argv[i], "-u") == 0) {
            i++;
            uid = atoi(argv[i]);
            if(uid == 0) {
                printf("UID 0 is reserved for the Superuser.\n");
                free(homedir);
                free(fullname);
                exit();
            }
        }
    }
    n = strlen(argv[i]) + 1;
    username = malloc(n * sizeof(*username));
    safestrcpy(username, argv[i], n);
    if(fullname == 0) {
        fullname = malloc(n * sizeof(*fullname));
        safestrcpy(fullname, username, n);
    }
    if(homedir == 0) {
        // Append /home/ (6 characters)
        homedir = malloc((n + 6) * sizeof(*homedir));
        strncpy(homedir, "/home/", 6);
        safestrcpy(homedir + 6, username, n);
    }

    int code = adduser(username, uid, fullname, homedir);
    if(code == RESERVED) {
        printf("The provided UID and/or username are not available.\n");
    } else if(code == NOPERMS) {
        printf("Insufficient permissions to add a new user.\n");
    } else if(code < 0) {
        printf("Unknown error\n");
    } else {
        printf("User %s added successfully.\n", username);
    }

	exit();
}