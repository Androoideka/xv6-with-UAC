#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Device

struct stat {
	short type;    // Type of file
	int dev;       // File system's disk device
	uint ino;      // Inode number
	short nlink;   // Number of links to file
	uint size;     // Size of file in bytes
	int uid;       // File's user owner
	int gid;       // File's group owner
	char mode[10]; // File permissions
};
