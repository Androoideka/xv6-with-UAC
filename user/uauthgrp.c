#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"

//printf("%s %d %s\n", grp->groupname, grp->gid, grp->users);

#define BUFSIZE 128
#define groupfile "/etc/group"

static int availablegid = 1002;

struct group {
    char *groupname;
    int gid;
    char *users;
};

static int
getgrouplength(struct group grp) {
    int n = 0;
    n += strlen(grp.groupname) + 1;
    n += digitcount(grp.gid) + 1;
    if(grp.users != 0) {
        n += strlen(grp.users);
    }
    return n;
}

static struct group*
groupfromattr(char *groupname, int gid, char *users, struct group *grp) {
    grp = malloc(sizeof(*grp));

    grp->groupname = 0;
    if(groupname != 0) {
        int n = strlen(groupname) + 1;
        grp->groupname = malloc(n * sizeof(*groupname));
        safestrcpy(grp->groupname, groupname, n);
    }
    grp->gid = gid;
    grp->users = 0;
    if(users != 0) {
        int n = strlen(users) + 1;
        grp->users = malloc(n * sizeof(*users));
        safestrcpy(grp->users, users, n);
    }

    return grp;
}

static struct group*
group(const char *line, struct group *grp) {
    grp = malloc(sizeof(*grp));
    memset(grp, 0, sizeof(*grp));
    for(int groupDescSegment = 0, i = 0; groupDescSegment < 3; groupDescSegment++) {
        int segmentLength = strchr(line + i, ':') - (line + i) + 1;
        segmentLength = segmentLength < 0 ? strlen(line + i) + 1 : segmentLength;
        if(groupDescSegment == 0) {
            grp->groupname = malloc(segmentLength * sizeof(char));
            safestrcpy(grp->groupname, line + i, segmentLength);
        }
        if(groupDescSegment == 1) {
            char gidstring[segmentLength];
            safestrcpy(gidstring, line + i, segmentLength);
            grp->gid = atoi(gidstring);
        }
        if(groupDescSegment == 2 && segmentLength > 1) {
            grp->users = malloc(segmentLength * sizeof(char));
            safestrcpy(grp->users, line + i, segmentLength);
        }
        i += segmentLength;
    }
    return grp;
}

static int
writegroup(int fd, struct group *grp, int *offset, int oldlength) {
    int i = 0;
    int n = 0;

    int linesize = getgrouplength(*grp);
    char line[linesize];

    n = strlen(grp->groupname);
    i += n;
    strncpy(line, grp->groupname, n);
    line[i++] = ':';

    int temp = grp->gid;
    n = digitcount(temp);
    char gidstring[n--];
    do {
        gidstring[n--] = '0' + temp % 10;
        temp /= 10;
    } while(temp != 0);
    strncpy(line + i, gidstring, sizeof(gidstring));
    i += sizeof(gidstring);
    line[i++] = ':';

    if(grp->users != 0) {
        n = strlen(grp->users);
        strncpy(line + i, grp->users, n);
    }

    if(oldlength <= 0) {
        return addline(fd, line, linesize, groupfile);
    }
    return changeline(fd, line, linesize, groupfile, offset, oldlength);
}

static struct group*
freegroup(struct group *grp) {
    if(grp != 0) {
        free(grp->groupname);
        grp->groupname = 0;
        free(grp->users);
        grp->users = 0;
        free(grp);
        grp = 0;
    }
    return grp;
}

static int
findgroupbyname(int fd, const char *groupname, struct group **grp, int *offset) {
    int linesize = BUFSIZE;
    char *line = malloc(linesize * sizeof(*line));

    do {
        if(getline(fd, &line, &linesize, offset) != 0) {
            free(line);
            return NOTFOUND;
        }
        *grp = freegroup(*grp);
        *grp = group(line, *grp);
    } while(strcmp((*grp)->groupname, groupname) != 0);

    free(line);
    return 0;
}

static int
findgroupbygid(int fd, int gid, struct group **grp, int *offset) {
    int linesize = BUFSIZE;
    char *line = malloc(linesize * sizeof(*line));

    do {
        if(getline(fd, &line, &linesize, offset) != 0) {
            free(line);
            return NOTFOUND;
        }
        *grp = freegroup(*grp);
        *grp = group(line, *grp);
    } while((*grp)->gid != gid);

    free(line);
    return 0;
}

static int
addusertogroup(const char *username, struct group *grp) {
    int userlength = strlen(username) + 1;
    if(grp->users == 0) {
        grp->users = malloc(userlength * sizeof(char));
        safestrcpy(grp->users, username, userlength);
        return 0;
    }
    int code = findstrinsetdivsep(username, grp->users, ',');
    if(code > 0) {
        return EXISTS;
    }
    if(code < 0 && code != NOTFOUND) {
        return code;
    }
    int n = strlen(grp->users);
    char *newUsers = malloc((n + userlength + 1) * sizeof(*newUsers)); // 1 more for the ,
    strncpy(newUsers, grp->users, n);
    newUsers[n] = ',';
    safestrcpy(newUsers + n + 1, username, userlength);
    free(grp->users);
    grp->users = newUsers;

    return 0;
}

static int
removeuserfromgroup(const char *username, struct group *grp) {
    int code = findstrinsetdivsep(username, grp->users, ',');
    if(code < 0) {
        return code;
    }
    int n = strchr(grp->users + code, ',') - (grp->users + code);
    int length = strlen(grp->users);
    if(n < 0) {
        n = length - code;
        if(n >= length) {
            free(grp->users);
            grp->users = 0;
            return 0;
        }
    }
    if(code > 0) {
        n++;
        code--;
    }
    char *newUsers = malloc((length - n + 1) * sizeof(*newUsers));
    strncpy(newUsers, grp->users, code);
    int end = code + n + 1;
    safestrcpy(newUsers + code, grp->users + end, length - end + 1);
    free(grp->users);
    grp->users = newUsers;

    return 0;
}

int
getgroupname(int gid, char **groupname) {
    struct group *grp = 0;

    int fd = open(groupfile, O_RDONLY);
    if(fd < 0) {
        return NOPERMS; // User does not have privileges
    }
    int offset = 0;

    int code = findgroupbygid(fd, gid, &grp, &offset);
    close(fd);
    if(code != 0) {
        grp = freegroup(grp);
        return code; // Group not found or insufficient permissions
    }
    int length = strlen(grp->groupname) + 1;
    if(length > BUFSIZE) {
        *groupname = realloc(*groupname, length, length - BUFSIZE);
    }
    safestrcpy(*groupname, grp->groupname, length);
    grp = freegroup(grp);
    return 0;
}

int
getgidforname(const char* groupname, int *gid) {
    struct group *grp = 0;

    int fd = open(groupfile, O_RDONLY);
    if(fd < 0) {
        return NOPERMS; // User does not have privileges
    }
    int offset = 0;

    int code = findgroupbyname(fd, groupname, &grp, &offset);
    close(fd);
    *gid = grp->gid;
    grp = freegroup(grp);
    return code;
}

int
updateusername(const char *username, const char *newUsername) {
    struct group *grp = 0;

    int fd = open(groupfile, O_RDWR);
    if(fd < 0) {
        return NOPERMS;
    }
    int offset = 0;

    int linesize = BUFSIZE;
    char *line = malloc(linesize * sizeof(*line));

    while(getline(fd, &line, &linesize, &offset) == 0) {
        grp = group(line, grp);
        if(grp->users == 0) continue;
        int oldlength = getgrouplength(*grp);
        int code = removeuserfromgroup(username, grp);
        if(code >= 0) {
            addusertogroup(newUsername, grp);
        }
        writegroup(fd, grp, &offset, oldlength);
        grp = freegroup(grp);
    }
    free(line);
    close(fd);

    return 0;
}

int
addgroup(char* group, int gid) {
    struct group *grp = 0;

    int fd = open(groupfile, O_RDWR);
    if(fd < 0) {
        return NOPERMS; // User does not have privileges
    }
    int offset = 0;

    if(findgroupbyname(fd, group, &grp, &offset) == 0) {
        close(fd);
        grp = freegroup(grp);
        return RESERVED; // Groupname already reserved for another user
    }
    lseek(fd, 0, SEEK_SET);
    offset = 0;
    if(gid == 0){
        while(findgroupbygid(fd, availablegid, &grp, &offset) == 0) {
            availablegid++;
            lseek(fd, 0, SEEK_SET);
            offset = 0;
        }
        gid = availablegid++;
    } else if(findgroupbygid(fd, gid, &grp, &offset) == 0) {
        close(fd);
        grp = freegroup(grp);
        return RESERVED; // GID already reserved for another user
    }
    grp = freegroup(grp);

    grp = groupfromattr(group, gid, 0, grp);

    int code = writegroup(fd, grp, &offset, 0);
    close(fd);
    grp = freegroup(grp);
    return code;
}

int
changegroups(const char *username, const char *groups, int flag) {
    struct group *grp = 0;

    int fd = open(groupfile, O_RDWR);
    if(fd < 0) {
        return NOPERMS;
    }
    int offset = 0;

    int linesize = BUFSIZE;
    char *line = malloc(linesize * sizeof(*line));

    if(flag != 1) {
        while(getline(fd, &line, &linesize, &offset) != 0) {
            grp = freegroup(grp);
            grp = group(line, grp);
            if(findstrinsetdivsep(grp->groupname, groups, ',') == NOTFOUND) {
                removeuserfromgroup(username, grp);
            }
        }
    }

    int i = 0;
    int length = strlen(groups);
    while(i < length) {
        int n = strchr(groups + i, ',') - (groups + i) + 1;
        n = n < 0 ? strlen(groups + i) + 1 : n;
        char *group = malloc(n * sizeof(*group));
        safestrcpy(group, groups + i, n);
        i += n;

        lseek(fd, 0, SEEK_SET);
        offset = 0;
        int code = findgroupbyname(fd, group, &grp, &offset);
        free(group);
        if(code < 0) {
            grp = freegroup(grp);
            printf("Group %s not found.\n", group);
            continue;
        }
        int oldlength = getgrouplength(*grp);
        addusertogroup(username, grp);

        code = writegroup(fd, grp, &offset, oldlength);
        grp = freegroup(grp);
        if(code < 0) {
            printf("Fatal error.\n");
        }
    }

    close(fd);
    return 0;
}

int
getgids(const char *username, int *ngroups, int **gids) {
    struct group *grp = 0;

    int fd = open(groupfile, O_RDONLY);
    if(fd < 0) {
        return NOPERMS;
    }
    int offset = 0;

    int linesize = BUFSIZE;
    char *line = malloc(linesize * sizeof(*line));

    while(getline(fd, &line, &linesize, &offset) == 0) {
        grp = group(line, grp);
        if(grp->users == 0) continue;
        int code = findstrinsetdivsep(username, grp->users, ',');
        if(code >= 0) {
            *gids = realloc(*gids, *ngroups, 1);
            (*gids)[(*ngroups)++] = grp->gid;
        }
        grp = freegroup(grp);
    }
    free(line);
    close(fd);

    return 0;
}
