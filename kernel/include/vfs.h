#include <stdint.h>
//#include <user.h>

struct file {
	char name[32];
	uint32_t type, len, inode, NxtClus, count, size, bias, flag;
}__attribute__((packed));


struct fd_ {
	int used;
	int flag;
	int bias;
	struct file* file;
};
