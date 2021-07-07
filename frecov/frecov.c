#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h>
//#define check
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
	uint32_t DIR_FileSize;
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

struct bmp{
	uint16_t bfType;
	uint32_t bfSize;
	uint8_t bf_rev[4];
	uint32_t bf_off; 
	uint32_t bf_size_2;
	uint32_t width;
	uint32_t height;
}__attribute__((packed));

uint32_t fat1, fat2, FirstData, RootDir, TotClus;
struct fat_header *disk;
uint8_t *p;

uint32_t divided[4][MAX_NUM];
uint32_t tot[4];

uint32_t cal_Clus(int num) {
	return FirstData + (num - 2) * disk -> BPB_SecPerClus  * disk -> BPB_BytsPerSer;
}

int judge_empty(uint32_t loc) {
	int bj = 0;
	for (int i = 0; i < 32; i++)
		if (*(p + i) != 0) {
			bj = 1;
			break;
		}
	if (bj == 0) {
		divided[0][tot[0]++] = loc;
		return 0;	
	}
	return -1;
}

int judge_dir(uint32_t loc) {
	struct short_file *tep = (struct short_file *)(p + loc);
	int cnt = 0;
	for (int i = 1; i <= 20; i++) {
		if (tep -> DIR_Name[8] == 'B' && tep -> DIR_Name[9] == 'M' && \
			tep -> DIR_Name[10] == 'P')
				cnt++;
		tep = tep + 1;	
	}
	if (cnt >= 3) {
		divided[1][tot[1]++] = loc;
		return 1;	
	}
	return -1;
}

int judge_bmphead(uint32_t loc) {
	uint16_t *tep = (uint16_t *)(p + loc);
	if (*tep == 0x4d42) {
		divided[2][tot[2]++] = loc;
		return 2;	
	}	
	return -1;
}

void divide() {
	uint32_t loc = FirstData;
	for (int i = 0; i < TotClus; i++, loc += disk -> BPB_SecPerClus * 512) {

		int id = judge_empty(loc);
		if (id == -1) id = judge_dir(loc);
		if (id == -1) id = judge_bmphead(loc);
		if (id == -1) id = 3, divided[3][tot[3]++] = loc;
			
		#ifdef DEBUG
		printf("loc=0x%x id=%d\n", loc, id);
		#endif
	}

}

static char name[512];
static int n_now = 0;
 
void get_name(char c) {
	if ((uint8_t)c != 0x20) name[n_now++] = c;
	name[n_now] = '\0';
}

void SolveLongName(struct long_file * now) {
	while (now -> LDIR_Attr == 0x0f) {
		for (int i = 0; i < 10; i += 2)
			if (now -> LDIR_Name1[i] == '\0' && now -> LDIR_Name1[i + 1]=='\0') return;	
			else get_name(now -> LDIR_Name1[i]);
		for (int i = 0; i < 12; i += 2)
			if (now -> LDIR_Name2[i] == '\0' && now -> LDIR_Name2[i + 1]=='\0') return;	
			else get_name(now -> LDIR_Name2[i]);
		for (int i = 0; i < 4; i += 2)
			if (now -> LDIR_Name3[i] == '\0' && now -> LDIR_Name3[i + 1]=='\0') return;	
			else get_name(now -> LDIR_Name3[i]);
		now = now - 1;
	}
}
static uint8_t line[65536];
static int ti = 0;

int pd(uint8_t a, uint8_t b) {
	if (a > b) return a - b;
	else return b - a;
} 
int MAX_c, unique;

uint8_t* findClus(int loc, int sum, int id, int skip) {
	int minn = INT_MAX;
	uint8_t* ans = NULL;

	for (int i = id; i < tot[3]; i++) {
		uint8_t *start = (uint8_t *)(p + divided[3][i]);
/*		int bias = sum - loc - 1, bj = 0;
		for (int j = 0; j < skip; j++)
			if (bias - j >= 0 && *(start + bias - j) != 0) bj = 1;
		if (bj)continue;
*/		int val = 0;
		for (int j = loc; j < sum; j++) val += pd(*start, line[j]), start = start + 1;
		for (int j = 0; j < loc; j++)   val += pd(*start, line[j]), start = start + 1;
		if (val < minn) {
			minn = val, ans = (uint8_t *)(p + divided[3][i]); unique = i + 1;
			if (minn < MAX_c) MAX_c = minn;
			if (val <= 10000) { return ans;}
		}
	}
	if (minn < MAX_c) MAX_c = minn;

	for (int i = 0; i < id; i++) {
		uint8_t *start = (uint8_t *)(p + divided[3][i]);
/*		int bias = sum - loc - 1, bj = 0;
		for (int j = 0; j < skip; j++)
			if (bias - j >= 0 && *(start + bias - j) != 0) bj = 1;
		if (bj)continue;
*/		int val = 0;
		for (int j = loc; j < sum; j++) val += pd(*start, line[j]), start = start + 1;
		for (int j = 0; j < loc; j++)   val += pd(*start, line[j]), start = start + 1;
		if (val < minn) {
			minn = val, ans = (uint8_t *)(p + divided[3][i]); unique = i + 1;
		}
	}
	assert (ans != NULL);
	if (ans == NULL) return (uint8_t *) (p + divided[3][id]);
	return ans;
}
uint8_t ans_file[10000000];
int SumBmp = 0;

int find_info(struct short_file * now) {
	uint32_t loc = now -> FstClusHl;
	loc = (loc << 16) | now -> FstClusLO;
	loc = cal_Clus(loc);

	struct bmp *tep = (struct bmp *)(p + loc);
	if (tep -> bfType != 0x4d42) return 0;

	static char file_name[1024];
	sprintf(file_name, "/tmp/%s", name);
	FILE *fd = fopen(file_name, "w");
	assert(fd != NULL);
	uint8_t *start = p + loc + tep -> bf_off;
	for (int i = 0; i < tep -> bf_off; i++)
		fwrite(p + loc + i, 1, 1, fd);

	int height = tep -> height, width = tep -> width, cnt = ((tep -> width * 24 + 31) >> 5) << 2;
	int skip = 4 - (((width * 24) >> 3) & 3), sum = cnt * height;
	assert(sum <= 10000000);
	#ifdef DEBUG
	printf("%x %d %d %d %d ", loc, skip, width, height, cnt * height);
	#endif

	int now_loc = 0, off = tep -> bf_off;
	MAX_c = INT_MAX;
	
	int id = -1;
	for (int i = 0; i < tot[3]; i++) {
		if (divided[3][i] == loc + disk -> BPB_BytsPerSer * disk -> BPB_SecPerClus) {
			id = i;
			break;	
		}
	}
	assert(id != -1);
	
	int ans_loc = 0;
	while (sum) {
		line[now_loc++] = *start;
		ans_file[ans_loc++] = *start;
		start = start + 1;
		off = off + 1;
		if (off == disk -> BPB_BytsPerSer * disk -> BPB_SecPerClus) {
			start = findClus(now_loc, cnt, id, skip), off = 0;
			id = unique;
		}
		if (now_loc == cnt) now_loc = 0; 
		sum--;
	}
	ans_file[ans_loc] = '\0';
	fwrite(ans_file, ans_loc, 1, fd);
	fclose(fd);	

	/// output ///

	sprintf(file_name, "sha1sum /tmp/%s", name);
	FILE *fp = popen(file_name, "r");
	static char buf[1024];
	fscanf(fp, "%s", buf);
	printf("%s %s\n", buf, name);
	fflush(stdout);
	SumBmp++;
	pclose(fp);
	return 1;	
}
void deal() {
	for (int i = 0; i < tot[1]; i++) {
	
		struct short_file *tep = (struct short_file *)(p + divided[1][i]), *lst;
		uint32_t loc = divided[1][i];

		while ((uintptr_t)tep < \
			   (uintptr_t) p + divided[1][i] + disk -> BPB_SecPerClus * 512) {
			loc += 32;
			if (tep -> DIR_Name[8] != 'B' || tep -> DIR_Name[9] != 'M' || \
				tep -> DIR_Name[10] != 'P') {
				tep = tep + 1;
				continue;
			}
			
			n_now = 0;
			lst = tep - 1;

			if (tep -> DIR_Name[6] != '~') {
				for (int j = 0; j < 8; j++)
					get_name(tep -> DIR_Name[j]);
				name[n_now++] = '.';
				for (int j = 8; j < 11; j++)
					get_name(tep -> DIR_Name[j]);
				if (lst -> DIR_others_1[0] == 0x0f) n_now = 0, name[n_now] = '\0';
			}

			if (n_now == 0) {
				SolveLongName((struct long_file *)lst);
				int success = find_info(tep);
				#ifdef DEBUG
				if (success) printf(" %d %x %s \n", tep -> DIR_FileSize, loc - 32, name);
				printf("%d\n", MAX_c);
				#endif
			}
			tep = tep + 1;
		}
	}
}

#ifdef check
struct chk{
	char sum[64];
	char name[64];
}e[1024], f[1024];

void checker() {
	FILE *fd = fopen("/tmp/ans.txt", "r");
	assert(fd != NULL);
	SumBmp = 97;
	for (int i = 0; i < SumBmp; i++) {
		fscanf(fd, "%s %s", e[i].sum, e[i].name);
	}
	fclose(fd);

	fd = fopen("/tmp/std.txt", "r");
	assert(fd != NULL);
	SumBmp = 97;
	for (int i = 0; i < SumBmp; i++) {
		fscanf(fd, "%s %s", f[i].sum, f[i].name);
	}
	fclose(fd);
	int cor_name = 0, cor_all = 0;
	for (int i = 0; i < SumBmp; i++) {
		int ok = 0;
		for (int j = 0; j < SumBmp; j++) {
			if (strcmp(e[i].name, f[j].name) == 0) {
				ok = 1;
				cor_name++;
				if (strcmp(e[i].sum, f[j].sum) == 0) {
					cor_all ++;	
					fprintf(stderr, "%d %s %s\n", i, e[i].sum, e[i].name);
					break;
				}
				else 
					fprintf(stderr, "%d %s\n", i, e[i].name);	
				break;
			}
		}
		if (!ok) fprintf(stderr, "%d Wa %s\n", i, e[i].name);
	}
	fprintf(stderr, "%lf %lf\n", (double)cor_name / SumBmp, (double)cor_all / SumBmp);
}
#endif

int main(int argc, char *argv[]) {
	int fd = open(argv[1], 0);
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
	
	#ifdef check
	freopen("/tmp/ans.txt", "w", stdout);
	#endif

	divide();
	deal();

	#ifdef check
	checker();
	#endif
	return 0;
}
