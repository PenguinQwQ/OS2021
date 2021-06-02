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
	uint16_t BPB_RsvdSecCnt;
	uint8_t BPB_NumFATs;
	uint8_t BPB_RootEntCnt[2];
	uint8_t BPB_TotSec16[2];
	uint8_t BPB_Media;
	uint8_t BPB_FATSz16[2];
	uint8_t BPB_SecPerTr[2];
	uint8_t BPB_NumHead[2];
	uint8_t BPB_HiddSec[4];
	uint8_t BPB_TotSec32[4];
	uint32_t BPB_FATSz32;
	uint8_t BPB_ExtFlags[2];
	uint8_t BPB_FSVer[2];
	uint32_t BPB_RootClus;
	uint8_t BPB_FSInfo[2];
	uint8_t BPB_BkBootSec[2];
	uint8_t BPB_Reserved[12];
	uint8_t BS_DrvNum;
	uint8_t BS_Reserved1;
	uint8_t BS_BootSig;
	uint8_t BS_VolID[4];
	uint8_t BS_VolLab[11];
	uint8_t BS_FilSysType[8];
	uint8_t BS_empty[420];
	uint16_t Signature_word;	
}__attribute__((packed));

int main(int argc, char *argv[]) {
	int fd = open("fs.img", 0);
	assert(fd > 0);
	struct fat_header *disk;
	disk = mmap(NULL, 128 * 1024 *1024, PROT_READ, MAP_PRIVATE, fd, 0);

	assert(disk != NULL);
	assert(sizeof(struct fat_header) == 512);
	assert(disk -> Signature_word == 0xaa55);
	
	uint32_t fat1, fat2, root_dir;
	fat1 = (uint32_t)disk -> BPB_RsvdSecCnt * 512;
	fat2 = fat1 + disk -> BPB_FATSz32 * 512;
	root_dir = fat1 + disk -> NumFATs * disk -> BPB_FATSz32 * 512 + \
	           disk -> BPB_RootClus * 512;

	printf("%x %x %x %x\n", fat1, fat2, root_dir, disk -> BPB_RootClus);

	return 0;
}
