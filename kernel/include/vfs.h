#include <stdint.h>
#include <common.h>
#include <user.h>

struct file {
	char name[32];
	uint32_t type, len, inode, NxtClus, count, size, bias, flag;
}__attribute__((packed));


struct fd_ {
	int used;
	int type;
	int flag;
	int loc;	
	struct file* file;
};
