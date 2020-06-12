#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"

//printf("%s %s %d %d %s %s\n", usr->username, usr->password, usr->uid, usr->gid, usr->fullname, usr->homedir);

#define BUFSIZE 128
#define passwdfile "/etc/passwd"

static int availableuid = 1002;

struct user {
    char *username;
    char *password;
    int uid;
    int gid;
    char *fullname;
    char *homedir;
};

static int
getuserlength(struct user usr) {
    int n = 0;
    n += strlen(usr.username) + 2;
    if(usr.password != 0) {
        n += strlen(usr.password);
    }
    n += digitcount(usr.uid) + 1;
    n += digitcount(usr.gid) + 1;
    n += strlen(usr.fullname) + 1;
    n += strlen(usr.homedir);
    return n;
}

static struct user*
userfromattr(char *username, char *password, int uid, 
    int gid, char *fullname, char *homedir, struct user *usr) {
    usr = malloc(sizeof(*usr));

    usr->username = 0;
    if(username != 0) {
        int n = strlen(username) + 1;
        usr->username = malloc(n * sizeof(*username));
        safestrcpy(usr->username, username, n);
    }
    usr->password = 0;
    if(password != 0) {
        int n = strlen(password) + 1;
        usr->password = malloc(n * sizeof(*password));
        safestrcpy(usr->password, password, n);
    }
    usr->uid = uid;
    usr->gid = gid;
    usr->fullname = 0;
    if(fullname != 0) {
        int n = strlen(fullname) + 1;
        usr->fullname = malloc(n * sizeof(*fullname));
        safestrcpy(usr->fullname, fullname, n);
    }
    usr->homedir = 0;
    if(homedir != 0) {
        int n = strlen(homedir) + 1;
        usr->homedir = malloc(n * sizeof(*homedir));
        safestrcpy(usr->homedir, homedir, n);
    }

    if(usr->homedir != 0) {
        mkdir(usr->homedir);
        chown(usr->homedir, usr->uid, usr->uid);
    }

    return usr;
}

static struct user*
user(const char *line, struct user *usr) {
    usr = malloc(sizeof(*usr));
    memset(usr, 0, sizeof(*usr));
    for(int userDescSegment = 0, i = 0; userDescSegment < 6; userDescSegment++) {
        int segmentLength = strchr(line + i, ':') - (line + i) + 1;
        segmentLength = segmentLength < 0 ? strlen(line + i) + 1 : segmentLength;
        if(userDescSegment == 0) {
            usr->username = malloc(segmentLength * sizeof(char));
            safestrcpy(usr->username, line + i, segmentLength);
        }
        if(userDescSegment == 1 && segmentLength > 1) {
            usr->password = malloc(segmentLength * sizeof(char));
            safestrcpy(usr->password, line + i, segmentLength);
        }
        if(userDescSegment == 2) {
            char uidstring[segmentLength];
            safestrcpy(uidstring, line + i, segmentLength);
            usr->uid = atoi(uidstring);
        }
        if(userDescSegment == 3) {
            char gidstring[segmentLength];
            safestrcpy(gidstring, line + i, segmentLength);
            usr->gid = atoi(gidstring);
        }
        if(userDescSegment == 4) {
            usr->fullname = malloc(segmentLength * sizeof(char));
            safestrcpy(usr->fullname, line + i, segmentLength);
        }
        if(userDescSegment == 5) {
            usr->homedir = malloc(segmentLength * sizeof(char));
            safestrcpy(usr->homedir, line + i, segmentLength);
        }
        i += segmentLength;
    }
    return usr;
}

static int
writeuser(int fd, struct user *usr, int *offset, int oldlength) {
    int i = 0;
    int n = 0;

    int linesize = getuserlength(*usr);
    char line[linesize];

    n = strlen(usr->username);
    i += n;
    strncpy(line, usr->username, n);
    line[i++] = ':';

    if(usr->password != 0) {
        n = strlen(usr->password);
        strncpy(line + i, usr->password, n);
        i += n;
    }
    line[i++] = ':';

    int temp = usr->uid;
    n = digitcount(temp);
    char uidstring[n--];
    do {
        uidstring[n--] = '0' + temp % 10;
        temp /= 10;
    } while(temp > 0);
    strncpy(line + i, uidstring, sizeof(uidstring));
    i += sizeof(uidstring);
    line[i++] = ':';

    temp = usr->gid;
    n = digitcount(temp);
    char gidstring[n--];
    do {
        gidstring[n--] = '0' + temp % 10;
        temp /= 10;
    } while(temp != 0);
    strncpy(line + i, gidstring, sizeof(gidstring));
    i += sizeof(gidstring);
    line[i++] = ':';

    n = strlen(usr->fullname);
    strncpy(line + i, usr->fullname, n);
    i += n;
    line[i++] = ':';

    n = strlen(usr->homedir);
    strncpy(line + i, usr->homedir, n);

    if(oldlength <= 0) {
        return addline(fd, line, linesize, passwdfile);
    }
    return changeline(fd, line, linesize, passwdfile, offset, oldlength);
}

static struct user*
freeuser(struct user *usr) {
    if(usr != 0) {
        free(usr->username);
        usr->username = 0;
        free(usr->password);
        usr->password = 0;
        free(usr->fullname);
        usr->fullname = 0;
        free(usr->homedir);
        usr->homedir = 0;
        free(usr);
        usr = 0;
    }
    return usr;
}

static int
finduserbyuid(int fd, int uid, struct user **usr, int *offset) {
    int linesize = BUFSIZE;
    char *line = malloc(linesize * sizeof(*line));

    do {
        if(getline(fd, &line, &linesize, offset) != 0) {
            free(line);
            return NOTFOUND;
        }
        *usr = freeuser(*usr);
        *usr = user(line, *usr);
    } while((*usr)->uid != uid);

    free(line);
    return 0;
}

static int
finduserbyname(int fd, const char *username, struct user **usr, int *offset) {
    int linesize = BUFSIZE;
    char *line = malloc(linesize * sizeof(*line));

    do {
        if(getline(fd, &line, &linesize, offset) != 0) {
            free(line);
            return NOTFOUND;
        }
        *usr = freeuser(*usr);
        *usr = user(line, *usr);
    } while(strcmp((*usr)->username, username) != 0);

    free(line);
    return 0;
}

int
authuid(const char *username, const char *pwd, int *uid) {
    struct user *usr = 0;

    int fd = open(passwdfile, O_RDONLY);
    if(fd < 0) {
        return NOPERMS; // User does not have privileges
    }
    int offset = 0;

    int code = finduserbyname(fd, username, &usr, &offset);
    close(fd);
    
    if(code < 0) {
        usr = freeuser(usr);
        return code; // User not found or insufficient permissions
    }
    if(usr->password != 0 && strcmp(usr->password, pwd) != 0) {
        usr = freeuser(usr);
        return AUTHFAIL; // Authentication failed
    }
    *uid = usr->uid;
    code = chdir(usr->homedir);
    usr = freeuser(usr);
    return code;
}

int
getuidforname(const char* username, int *uid) {
    struct user *usr = 0;

    int fd = open(passwdfile, O_RDONLY);
    if(fd < 0) {
        return NOPERMS; // User does not have privileges
    }
    int offset = 0;

    int code = finduserbyname(fd, username, &usr, &offset);
    close(fd);
    *uid = usr->uid;
    usr = freeuser(usr);
    return code;
}

int
getusername(int uid, char **username) {
    struct user *usr = 0;

    int fd = open(passwdfile, O_RDONLY);
    if(fd < 0) {
        return NOPERMS; // User does not have privileges
    }
    int offset = 0;

    int code = finduserbyuid(fd, uid, &usr, &offset);
    close(fd);
    if(code < 0) {
        usr = freeuser(usr);
        return code; // User not found or insufficient permissions
    }
    int length = strlen(usr->username) + 1;
    if(length > BUFSIZE) {
        *username = realloc(*username, length, length - BUFSIZE);
    }
    safestrcpy(*username, usr->username, length);
    usr = freeuser(usr);
    return 0;
}

int
changepass(const char *username, const char *oldpwd, const char *newpwd) {
    struct user *usr = 0;

    int fd = open(passwdfile, O_RDWR);
    if(fd < 0) {
        return NOPERMS; // User does not have privileges
    }
    int offset = 0;

    if(finduserbyname(fd, username, &usr, &offset) < 0) {
        close(fd);
        usr = freeuser(usr);
        return NOTFOUND; // User not found
    }
    int uid = getuid();
    if(uid != 0 && (usr->uid != uid || (usr->password != 0 && strcmp(usr->password, oldpwd) != 0))) {
        close(fd);
        usr = freeuser(usr);
        return NOPERMS; // User does not have the necessary privileges to change password of requested user
    }
    // Allow superuser to modify password of any user in any case. 
    // If not the superuser, the current user can modify only their own password.
    // The app allows them to modify the password if they got their current password right 
    // or if they do not have a password set (password field is an empty string)

    int oldlength = getuserlength(*usr);

    int n = strlen(newpwd) + 1;
    free(usr->password);
    usr->password = malloc(n * sizeof(*usr->password));
    safestrcpy(usr->password, newpwd, n);

    int code = writeuser(fd, usr, &offset, oldlength);
    close(fd);
    usr = freeuser(usr);
    return code;
}

int
adduser(char* username, int uid, char *fullname, char* homedir) {
    struct user *usr = 0;

    int fd = open(passwdfile, O_RDWR);
    if(fd < 0) {
        return NOPERMS; // User does not have privileges
    }
    int offset = 0;

    if(finduserbyname(fd, username, &usr, &offset) == 0) {
        close(fd);
        usr = freeuser(usr);
        return RESERVED; // Username already reserved for another user
    }
    lseek(fd, 0, SEEK_SET);
    offset = 0;

    if(uid == 0){
        while(finduserbyuid(fd, availableuid, &usr, &offset) == 0) {
            availableuid++;
            lseek(fd, 0, SEEK_SET);
            offset = 0;
        }
        uid = availableuid++;
    } else if(finduserbyuid(fd, uid, &usr, &offset) == 0) {
        close(fd);
        usr = freeuser(usr);
        return RESERVED; // UID already reserved for another user
    }
    usr = freeuser(usr);

    usr = userfromattr(username, 0, uid, uid, fullname, homedir, usr);
    int code = addgroup(username, uid);
    if(code < 0) {
        return RESERVED;
    }
    code = changegroups(username, username, 0);
    if(code < 0) {
        return UNKNOWN;
    }

    code = writeuser(fd, usr, &offset, 0);
    close(fd);
    usr = freeuser(usr);
    return code;
}

int
changeuser(const char *username, const char *newusername, int uid, const char *fullname, const char *homedir, int m) {
    struct user *usr = 0;

    int fd = open(passwdfile, O_RDWR);
    if(fd < 0) {
        return NOPERMS; // User does not have privileges
    }
    int offset = 0;

    if(newusername && finduserbyname(fd, newusername, &usr, &offset) == 0) {
        close(fd);
        usr = freeuser(usr);
        return RESERVED; // Username already reserved for another user
    }
    lseek(fd, 0, SEEK_SET);
    offset = 0;
    if(uid && finduserbyuid(fd, uid, &usr, &offset) == 0) {
        close(fd);
        usr = freeuser(usr);
        return RESERVED; // UID already reserved for another user
    }
    lseek(fd, 0, SEEK_SET);
    offset = 0;
    usr = freeuser(usr);

    int code = finduserbyname(fd, username, &usr, &offset);
    if(code < 0) {
        close(fd);
        usr = freeuser(usr);
        return code;
    }

    int oldlength = getuserlength(*usr);

    if(newusername != 0) {
        int n = strlen(newusername) + 1;
        usr->username = malloc(n * sizeof(char));
        safestrcpy(usr->username, newusername, n);
    }
    if(uid != 0) {
        usr->uid = uid;
        permsapplyr(usr->homedir, usr->uid, usr->uid);
    }
    if(fullname != 0) {
        int n = strlen(fullname) + 1;
        usr->fullname = malloc(n * sizeof(char));
        safestrcpy(usr->fullname, fullname, n);
    }
    if(homedir != 0) {
        if(m != 0) {
            mover(usr->homedir, homedir);
        } else {
            mkdir(homedir);
        }
        permsapplyr(homedir, usr->uid, usr->uid);
        int n = strlen(homedir) + 1;
        usr->homedir = malloc(n * sizeof(char));
        safestrcpy(usr->homedir, homedir, n);
    }

    lseek(fd, 0, SEEK_SET);
    code = writeuser(fd, usr, &offset, oldlength);
    close(fd);
    if(code >= 0 && newusername != 0) {
        updateusername(username, newusername);
    }
    usr = freeuser(usr);
    return code;
}
