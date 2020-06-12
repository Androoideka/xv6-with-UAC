#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"

int 
extractgid(char *info) {
    int gid;
    int code;
    if(info[0] >= '0' && info[0] <= '9') {
        gid=atoi(info);

        char *group = malloc(128 * sizeof(char));
        code = getgroupname(gid, &group);
        free(group);
    } else {
        code = getgidforname(info, &gid);
    }
    if(code != 0) {
        printf("No such group found.\n");
        exit();
    }

    return gid;
}

int
main(int argc, char *argv[])
{
    if(getuid() != 0) {
        printf("This program is only available while logged into Superuser.");
        exit();
    }
    if(argc < 3) {
        printf("chgrp group files...\n");
        exit();
    }
    int gid = extractgid(argv[1]);

    for(int i = 2; i < argc; i++) {
        int fd = open(argv[i], O_RDONLY);
        if(fd < 0) {
            printf("File %s is invalid or unreadable.\n", argv[i]);
            exit();
        }
    }

    for(int i = 2; i < argc; i++) {
        struct stat st;
        if(stat(argv[i], &st) < 0) {
            printf("Couldn't stat file %s.\n", argv[i]);
            continue;
        }
        if(chown(argv[i], st.uid, gid) < 0) {
            printf("Couldn't change permissions of file %s.\n", argv[i]);
        }
    }

	exit();
}