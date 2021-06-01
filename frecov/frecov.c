#include <stdio.h>
#include <string.h>

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
	uint8_t BS_Reserved;
	uint8_t BS_BootSig;
	uint8_t BS_VolID;
	uint8_t BS_VolLab[11];
	uint8_t BS_FilSysType[8];
	uint8_t BS_empty[448];
	uint8_t Signature_word[2];	
};

int main(int argc, char *argv[]) {
	panic_on(sizeof(struct fat_header) != 512, "bad header!");

}
