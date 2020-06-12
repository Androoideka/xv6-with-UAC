#include "kernel/types.h"
#include "user.h"
#include "kernel/fcntl.h"

int
main(void) {
    printf("%d %d\n", getuid(), geteuid());
    exit();
}