#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"

int
extractuid(char *info, int *uid) {
    int length = strchr(info, ':') - info + 1;
    if (length < 2) return 1;

    int code;
    char str[length];
    safestrcpy(str, info, length);
    if(str[0] >= '0' && str[0] <= '9') {
        *uid=atoi(str);

        char *user = malloc(128 * sizeof(char));
        code = getusername(*uid, &user);
        free(user);
    } else {
        code = getuidforname(str, uid);
    }
    if(code < 0) {
        printf("No such user found.\n");
        exit();
    }

    return 0;
}

int
extractgid(char *info, int *gid) {
    int i = strchr(info, ':') - info + 1;
    int length = strlen(info) - i + 1;
    if (length < 2) return 1;

    int code;
    char str[length];
    safestrcpy(str, info + i, length);
    if(str[0] >= '0' && str[0] <= '9') {
        *gid=atoi(str);

        char *group = malloc(128 * sizeof(char));
        code = getgroupname(*gid, &group);
        free(group);
    } else {
        code = getgidforname(str, gid);
    }
    if(code < 0) {
        printf("No such group found.\n");
        exit();
    }

    return 0;
}

int
main(int argc, char *argv[])
{
    if(getuid() != 0) {
        printf("This program is only available while logged into Superuser.");
        exit();
    }
    if(argc < 3) {
        printf("usage: chown [owner][:[group]] file...\n");
        exit();
    }
    int uid;
    int gid;
    int flaguser = extractuid(argv[1], &uid);
    int flaggrp = extractgid(argv[1], &gid);

    for(int i = 2; i < argc; i++) {
        int fd = open(argv[i], O_RDONLY);
        if(fd < 0) {
            printf("File %s is invalid or unreadable.\n", argv[i]);
            exit();
        }
    }

    for(int i = 2; i < argc; i++) {
        if(flaguser || flaggrp) {
            struct stat st;
            if(stat(argv[i], &st) < 0) {
                printf("Couldn't stat file %s.\n", argv[i]);
                continue;
            }
            if(flaguser) {
                uid = st.uid;
            }
            if(flaggrp) {
                gid = st.gid;
            }
        }
        if(chown(argv[i], uid, gid) < 0) {
            printf("Couldn't change permissions of file %s.\n", argv[i]);
        } else {
            printf("Successfully changed permissions of file %s.\n", argv[i]);
        }
    }

	exit();
}