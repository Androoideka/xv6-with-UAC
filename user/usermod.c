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
    if(argc < 3) {
        printf("usage: usermod [-l login] [-u UID] [-c name] [-d dir] [-m] [-G grupa...] [-a] login\n");
        exit();
    }
    char *username;
    char *newusername = 0;
    char *homedir = 0;
    char *fullname = 0;
    int uid = 0;

    int mflag = 0;
    int groupindex = -1, aflag = 0;

    int i, n;
    for(i = 1; i < argc - 1; i++) {
        if(strcmp(argv[i], "-l") == 0) {
            i++;
            n = strlen(argv[i]) + 1;
            newusername = malloc(n * sizeof(*newusername));
            safestrcpy(newusername, argv[i], n);
        } else if(strcmp(argv[i], "-c") == 0) {
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
                free(newusername);
                free(homedir);
                free(fullname);
                exit();
            }
        } else if(strcmp(argv[i], "-g") == 0) {
            groupindex = ++i;
        } else if(strcmp(argv[i], "-m") == 0) {
            mflag = 1;
        } else if(strcmp(argv[i], "-a") == 0) {
            aflag = 1;
        }
    }
    n = strlen(argv[i]) + 1;
    username = malloc(n * sizeof(*username));
    safestrcpy(username, argv[i], n);

    int code = changeuser(username, newusername, uid, fullname, homedir, mflag);
    if(code == RESERVED) {
        printf("The provided UID and/or username are not available.\n");
        exit();
    } else if(code == NOPERMS) {
        printf("Insufficient permissions to modify user.\n");
        exit();
    } else if(code < 0) {
        printf("Unknown error\n");
        exit();
    } else {
        printf("User %s modified successfully.\n", username);
        if(groupindex != -1) {
            if(newusername != 0) {
                changegroups(newusername, argv[groupindex], aflag);
            }
            changegroups(username, argv[groupindex], aflag);
        }
    }

	exit();
}