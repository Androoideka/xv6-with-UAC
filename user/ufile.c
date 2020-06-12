#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"
#include "kernel/fs.h"

#define BUFSIZE 128

char*
fmtname(char *path, char padding)
{
	static char buf[DIRSIZ+1];
	char *p;

	// Find first character after last slash.
	for(p=path+strlen(path); p >= path && *p != '/'; p--)
		;
	p++;

	// Return blank-padded name.
	if(strlen(p) >= DIRSIZ)
		return p;
	memmove(buf, p, strlen(p));
	memset(buf+strlen(p), padding, DIRSIZ-strlen(p));
	return buf;
}

void
mover(const char *oldpath, const char *newpath) {
    char buf[512], newbuf[512], *p, *q;
	int fd;
	struct dirent de;
	struct stat st;

	if((fd = open(oldpath, O_RDONLY)) < 0){
		fprintf(2, "move: cannot open %s %d\n", oldpath, fd);
		return;
	}

	if(fstat(fd, &st) < 0){
		fprintf(2, "move: cannot stat %s\n", oldpath);
		close(fd);
		return;
    }

	switch(st.type){
	case T_FILE:
        link(oldpath, newpath);
        unlink(oldpath);
		break;

	case T_DIR:
		if(strlen(newpath) + 1 + DIRSIZ + 1 > sizeof buf){
			printf("move: path too long\n");
			break;
		}
        mkdir(newpath);
        strcpy(buf, oldpath);
		strcpy(newbuf, newpath);
		p = buf+strlen(buf);
        q = newbuf + strlen(newbuf);
		*p++ = '/';
        *q++ = '/';
        int i = 0;
		while(read(fd, &de, sizeof(de)) == sizeof(de)){
			if(de.inum == 0)
				continue;
            if(i++ < 2) {
                continue;
            }
			memmove(p, de.name, DIRSIZ);
            memmove(q, de.name, DIRSIZ);
			p[DIRSIZ] = 0;
            q[DIRSIZ] = 0;
			if(stat(buf, &st) < 0){
				printf("move: cannot stat %s\n", buf);
				continue;
			}
            char *name = fmtname(buf, 0);
            int namelen = strlen(name);
            safestrcpy(p, name, namelen + 1);
            safestrcpy(q, name, namelen + 1);
            mover(buf, newbuf);
		}
		break;
	}
	close(fd);
}

void
permsapplyr(const char *path, int uid, int gid) {
    char buf[512], *p;
	int fd;
	struct dirent de;
	struct stat st;

	if((fd = open(path, O_RDONLY)) < 0){
		fprintf(2, "permset: cannot open %s %d\n", path, fd);
		return;
	}

	if(fstat(fd, &st) < 0){
		fprintf(2, "permset: cannot stat %s\n", path);
		close(fd);
		return;
    }

	switch(st.type){
	case T_FILE:
        chown(path, uid, gid);
		break;

	case T_DIR:
		if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
			printf("permset: path too long\n");
			break;
		}
        chown(path, uid, gid);
        strcpy(buf, path);
		p = buf+strlen(buf);
		*p++ = '/';
        int i = 0;
		while(read(fd, &de, sizeof(de)) == sizeof(de)){
			if(de.inum == 0)
				continue;
            if(i++ < 2) {
                continue;
            }
			memmove(p, de.name, DIRSIZ);
			p[DIRSIZ] = 0;
			if(stat(buf, &st) < 0){
				printf("permset: cannot stat %s\n", buf);
				continue;
			}
            char *name = fmtname(buf, 0);
            int namelen = strlen(name);
            safestrcpy(p, name, namelen + 1);
            permsapplyr(buf, uid, gid);
		}
		break;
	}
	close(fd);
}

int
getline(int fd, char **line, int *linesize, int *offset) {
    int lineptr = 0;
    char buf[BUFSIZE];
    int i = 0, n = 0;

    lseek(fd, *offset, SEEK_SET);

    while(buf[i] != '\n') {
        if(i >= n) {
            n = read(fd, buf, sizeof(buf));
            if(n <= 0) {
                if(lineptr == 0) return -1;
                (*line)[lineptr++] = 0;
                *offset += lineptr;
                return 0;
            }
            if(n < sizeof(buf)) {
                buf[n] = 0;
            }
            i = 0;
            continue;
        }
        if(lineptr + 2 >= (*linesize)) {
            *line = realloc(*line, *linesize, *linesize);
            *linesize *= 2;
        }
        (*line)[lineptr++] = buf[i++];
    }
    (*line)[lineptr++] = 0;
    *offset += lineptr;
    i++;
    return 0;
}

int
addline(int fd, const char *line, int linesize, const char *file) {
    lseek(fd, 0, SEEK_END);
    write(fd, "\n", 1);
    write(fd, line, linesize);

    return 0;
}

int
changeline(int fd, char *line, int linesize, const char *file, int *offset, int oldlength) {
    char buf[BUFSIZE];

    int filelength = strlen(file);
    char tempfile[filelength + 4];
    strncpy(tempfile, file, filelength);
    tempfile[filelength] = 't';
    tempfile[filelength + 1] = 'm';
    tempfile[filelength + 2] = 'p';
    tempfile[filelength + 3] = 0;

    int fdtemp = open(tempfile, O_CREATE | O_RDWR);
    int newsize = (*offset) - oldlength - 1;

    lseek(fd, *offset, SEEK_SET);
    int n = read(fd, buf, BUFSIZE);
    write(fdtemp, line, linesize);
    if(n > 0) {
        write(fdtemp, "\n", 1);
    }
    while(n > 0) {
        write(fdtemp, buf, n);
        n = read(fd, buf, BUFSIZE);
    }

    *offset = newsize + linesize + 1;

    lseek(fdtemp, 0, SEEK_SET);
    lseek(fd, newsize, SEEK_SET);

    while((n = read(fdtemp, buf, BUFSIZE)) > 0) {
        write(fd, buf, n);
        newsize += n;
    }
    
    lseek(fd, newsize, SEEK_SET);
    trim(fd);

    close(fdtemp);

    unlink(tempfile);
    return 0;
}

// Returns index, or NOTFOUND if not found
int
findstrinsetdivsep(const char *str, const char *set, char sep) {
    char *cmpstr = 0;
    int length = strlen(set);
    int i = 0;
    while(i < length) {
        int n = strchr(set + i, sep) - (set + i) + 1;
        n = n < 0 ? strlen(set + i) + 1 : n;
        cmpstr = malloc(n * sizeof(*cmpstr));
        safestrcpy(cmpstr, set + i, n);
        if(strcmp(cmpstr, str) == 0) {
			free(cmpstr);
            return i;
        }
        i += n;
		free(cmpstr);
    }
    return NOTFOUND;
}