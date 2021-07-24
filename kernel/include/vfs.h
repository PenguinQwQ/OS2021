#include <stdint.h>
#include <common.h>

struct file {
	char name[32];
	uint32_t type, len, inode, NxtClus, count, size, bias, flag;
}__attribute__((packed));

