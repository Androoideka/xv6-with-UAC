#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"
#include "kernel/fs.h"

void
ls(char *path, int flag)
{
	char buf[512], *p;
	int fd;
	struct dirent de;
	struct stat st;

	if((fd = open(path, O_RDONLY)) < 0){
		fprintf(2, "ls: cannot open %s\n", path);
		return;
	}

	if(fstat(fd, &st) < 0){
		fprintf(2, "ls: cannot stat %s\n", path);
		close(fd);
		return;
	}

	switch(st.type){
	case T_FILE:
		printf("%s %d %d %d\n", fmtname(path, ' '), st.type, st.ino, st.size);
		break;

	case T_DIR:
		if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
			printf("ls: path too long\n");
			break;
		}
		strcpy(buf, path);
		p = buf+strlen(buf);
		*p++ = '/';
		while(read(fd, &de, sizeof(de)) == sizeof(de)){
			if(de.inum == 0)
				continue;
			memmove(p, de.name, DIRSIZ);
			p[DIRSIZ] = 0;
			if(stat(buf, &st) < 0){
				printf("ls: cannot stat %s\n", buf);
				continue;
			}
			if(flag) {
				char *user = malloc(128 * sizeof(char));
    			int code = getusername(st.uid, &user);
				if(code != 0) {
					safestrcpy(user, "NULL", 5);
				}
				char *group = malloc(128 * sizeof(char));
    			code = getgroupname(st.gid, &group);
				if(code != 0) {
					safestrcpy(group, "NULL", 5);
				}
				printf("%s %d %d %d %s %s %s\n", fmtname(buf, ' '), st.type, st.ino, st.size, st.mode, user, group);
				free(user);
				free(group);
			} else {
				printf("%s %d %d %d\n", fmtname(buf, ' '), st.type, st.ino, st.size);
			}
		}
		break;
	}
	close(fd);
}

int
main(int argc, char *argv[])
{
	int i;
	int startArg = 0;

	if(argc > 1 && strcmp(argv[startArg+1], "-l") == 0) {
		startArg++;
	}
	if(argc < (startArg + 2)) {
		ls(".", startArg);
		exit();
	}
	for(i=(startArg + 1); i<argc; i++)
		ls(argv[i], startArg);
	exit();
}
