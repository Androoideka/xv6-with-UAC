#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"

int
main(int argc, char *argv[])
{
    if(argc != 2) {
        exit();
    }

    int uid = atoi(argv[1]);

    char *user = malloc(128 * sizeof(char));
    int code = getusername(uid, &user);
    if(code == 0) {
        printf("%s\n", user);
    } else if (code == -1) {
        printf("User not found.\n");
    } else if(code == -1000) {
        printf("Insufficient permissions to read file.\n");
    }
    free(user);

	exit();
}