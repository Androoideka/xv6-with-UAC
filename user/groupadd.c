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
        printf("usage: groupadd [-g gid] naziv_grupe\n");
        exit();
    }
    char *groupname;
    int gid = 0;

    int i = 1, n;
    if(strcmp(argv[i], "-g") == 0) {
        i++;
        gid = atoi(argv[i]);
        i++;
    }
    n = strlen(argv[i]) + 1;
    groupname = malloc(n * sizeof(*groupname));
    safestrcpy(groupname, argv[i], n);

    int code = addgroup(groupname, gid);
    if(code == 0) {
        printf("Group %s added successfully.\n", groupname);
    } else if(code == RESERVED) {
        printf("The provided GID and/or groupname are not available.\n");
    } else if(code == NOPERMS) {
        printf("Insufficient permissions to add a new group.\n");
    }

	exit();
}