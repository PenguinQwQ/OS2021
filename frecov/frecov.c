#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h>
#define MAX_NUM 262144 * 2

struct fat_header{
	uint8_t BS_jmpBoot[3];
	uint8_t BS_OEMName[8];
	uint16_t BPB_BytsPerSer;
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
	uint32_t BPB_TotSec32;
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

struct short_file{
	uint8_t DIR_Name[11];
	uint8_t DIR_others_1[9];
	uint16_t FstClusHl;
	uint8_t ti[4];
	uint16_t FstClusLO;
	uint8_t DIR_FileSize[4];
}__attribute__((packed));

struct long_file{
	uint8_t LDIR_Ord;
	char LDIR_Name1[10];
	uint8_t LDIR_Attr;
	uint8_t LDIR_Type;
	uint8_t LDIR_CHksum;
	char LDIR_Name2[12];
	uint8_t LDIR_FstClusOI[2];
	char LDIR_Name3[4];
}__attribute__((packed));

uint32_t fat1, fat2, FirstData, RootDir, TotClus;
struct fat_header *disk;
uint8_t *p;

uint32_t divided[4][MAX_NUM];
uint32_t tot[4];

uint32_t cal_Clus(int num) {
	return FirstData + (num - 2) * disk -> BPB_SecPerClus  * disk -> BPB_BytsPerSer;
}

void divide() {
	uint32_t loc = FirstData, bj = 0;
	for (int i = 0; i < TotClus; i++, loc += disk -> BPB_SecPerClus * 512) {
		printf("%d\n", i);
	}

}

int main(int argc, char *argv[]) {
	int fd = open("fs.img", 0);
	assert(fd > 0);
	disk = mmap(NULL, 128 * 1024 *1024, PROT_READ, MAP_PRIVATE, fd, 0);

	assert(disk != NULL);
	assert(sizeof(struct fat_header) == 512);
	assert(disk -> Signature_word == 0xaa55);
	assert(sizeof(struct short_file) == 32);
	assert(sizeof(struct long_file)  == 32);
	
	fat1 = (uint32_t)disk -> BPB_RsvdSecCnt * disk -> BPB_BytsPerSer;
	fat2 = fat1 + disk -> BPB_FATSz32 * disk -> BPB_BytsPerSer;
	FirstData = (disk -> BPB_RsvdSecCnt + (disk -> BPB_NumFATs * disk -> BPB_FATSz32))\
	            * disk -> BPB_BytsPerSer;
	RootDir = cal_Clus(disk -> BPB_RootClus);
	TotClus = (disk -> BPB_TotSec32 - (disk -> BPB_RsvdSecCnt + disk -> BPB_NumFATs *\
			   						  disk -> BPB_FATSz32)) / disk -> BPB_SecPerClus;
	p = (uint8_t *)disk;

	divide();

	return 0;
}
