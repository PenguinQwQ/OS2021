#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h>

struct fat_header{
	uint8_t BS_jmpBoot[3];
	uint8_t BS_OEMName[8];
	uint8_t BPB_BytsPerSer[2];
	uint8_t BPB_SecPerClus;
	uint8_t BPB_RsvdSecCnt[2];
	uint8_t BPB_NumFATs;
	uint8_t BPB_RootEntCnt[2];
	uint8_t BPB_TotSec16[2];
	uint8_t BPB_Media;
	uint8_t BPB_FATSz16[2];
	uint8_t BPB_SecPerTr[2];
	uint8_t BPB_NumHead[2];
	uint8_t BPB_HiddSec[4];
	uint8_t BPB_TotSec32[4];
	uint8_t BS_DrvNum;
	uint8_t BS_Reserved1;
	uint8_t BS_BootSig;
	uint8_t BS_VolID[4];
	uint8_t BS_VolLab[11];
	uint8_t BS_FilSysType[8];
	uint8_t BS_empty[448];
	uint8_t Signature_word[2];	
}__attribute__((packed));

int main(int argc, char *argv[]) {
	int fd = open("fs.img", 0);
	assert(fd > 0);
	struct fat_hender *disk;
	disk = mmap(NULL, 128 * 1024 *1024, PROT_READ, MAP_PRIVATE, fd, 0);

	assert(disk != NULL);
	assert(sizeof(struct fat_header) == 512);
	assert(disk -> Signature_word == 0x55aa);
	return 0;
}
