#define NOTFOUND -1
#define RESERVED -2
#define NOPERMS  -1000
#define AUTHFAIL -200
#define EXISTS    1;
#define UNKNOWN  -666;

struct stat;
struct rtcdate;

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int getuid(void);
int geteuid(void);
int setuid(int);
int chmod(const char*, int);
int chown(const char*, int, int);
void clrscr(void);
void hideinp(void);
void showinp(void);
int lseek(int, int, int);
void trim(int);
int setgrps(int, int*);

// ulib.c
int stat(const char*, struct stat*);
// string and memory methods
char* strcpy(char*, const char*);
char* strncpy(char*, const char*, int);
char* safestrcpy(char*, const char*, int);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int findstrinsetdivsep(const char*, const char*, char);
int strcmp(const char*, const char*);
void fprintf(int, const char*, ...);
void printf(const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
// umalloc.c
void* malloc(uint);
void* realloc(void*, uint, uint);
void free(void*);
// number methods
int atoi(const char*);
int digitcount(int);
// find authentication object
int getusername(int, char**);
int getgroupname(int, char**);
int getuidforname(const char*, int*);
int getgidforname(const char*, int*);
int authuid(const char*, const char*, int*);
int getgids(const char*, int *, int **);
// change authentication object
int changepass(const char*, const char*, const char*);
int changeuser(const char*, const char*, int, const char*, const char*, int);
int changegroups(const char*, const char*, int);
int updateusername(const char*, const char*);
int adduser(char*, int, char*, char*);
int addgroup(char*, int);
// file methods
int getline(int, char**, int*, int*);
char *fmtname(char *, char);
int changeline(int, char*, int, const char*, int*, int);
int addline(int, const char*, int, const char*);
void mover(const char*, const char*);
void permsapplyr(const char*, int, int);
