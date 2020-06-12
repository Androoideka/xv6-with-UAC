#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"

void
printmode(int mada) {
    char map[3] = { 'r', 'w', 'x'};
    char mode[10];
    for(int i = 0; i < 10; i++) {
        mode[i] = '-';
    }
    for(int i = 1; i < 10; i++) {
		int shift = 1 << (9 - i);
		if(mada & shift) {
			mode[i] = map[(i - 1) % 3];
		}
    }
    mode[10] = 0;
    printf("%s", mode);
}

void
chmod1(char *file, int mode) {
    if(chmod(file, mode) < 0) {
        printf("Couldn't change permissions of %s\n", file);
    }
}

int
preparemode(char *mode_fmt) {
    int mode = 1;
    if(mode_fmt[2] == 's') {
        mode = mode << 9;
    } else {
        if(mode_fmt[2] == 'w') {
            mode = mode << 1;
        } else if(mode_fmt[2] == 'r') {
            mode = mode << 2;
        } else if(mode_fmt[2] != 'x') {
            printf("Invalid format.\n");
            exit();
        }
        if(mode_fmt[0] == 'u') {
            mode = mode << 6;
        } else if(mode_fmt[0] == 'g') {
            mode = mode << 3;
        } else if(mode_fmt[0] == 'a') {
            mode = mode | (mode << 3) | (mode << 6);
        } else if(mode_fmt[0] != 'o') {
            printf("Invalid format.\n");
            exit();
        }
    }
    if(mode_fmt[1] == '-') {
        mode = ~mode;
    } else if(mode_fmt[1] != '+') {
        printf("Invalid format.\n");
        exit();
    }
    return mode;
}

void
chmod2(char *file, int addedMode, int removeflag) {
    int mode = 0;
    struct stat st;
    if(stat(file, &st) < 0) {
        printf("Cannot stat %s\n", file);
        return;
    }
    for(int j = 1; j < 10; j++) {
        if(st.mode[j] != '-') {
            mode = mode | (1 << (9-j));
        }
    }
    /*printmode(mode);
    printf(" ");
    printmode(addedMode);
    printf("\n");*/
    if(removeflag) {
        mode = mode & addedMode;
    } else {
        mode = mode | addedMode;
    }
    /*printmode(mode);
    printf("\n");*/
    if(chmod(file, mode) < 0) {
        printf("Couldn't change permissions of %s\n", file);
    }
}

int
main(int argc, char *argv[])
{
    if(argc < 3) {
        printf("usage: chmod mode/mode_fmt file...");
        exit();
    }
    int mode;
    int flag;
    if(argv[1][0] >= '0' && argv[1][0] <= '9') {
        mode = atoi(argv[1]);
        flag = 0;
    } else {
        mode = preparemode(argv[1]);
        flag = 1;
    }

    for(int i = 2; i < argc; i++) {
        int fd = open(argv[i], O_RDONLY);
        if(fd < 0) {
            printf("File %s is invalid or unreadable.\n", argv[i]);
            exit();
        }
    }

    for(int i = 2; i < argc; i++) {
        if(flag) {
            chmod2(argv[i], mode, argv[1][1] == '-');
        } else {
            chmod1(argv[i], mode);
        }
    }

	exit();
}