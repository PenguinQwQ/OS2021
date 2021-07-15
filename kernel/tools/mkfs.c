#include <user.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#define MAX_LENGTH 32

struct file{
	char name[32];
	uint32_t type;
	uint32_t len;
	uint32_t inode;
	uint32_t NxtClus;
	uint32_t count;
	uint32_t size;
	uint8_t others[8];
}__attribute__((packed));



uint32_t FirstLoc, clus;
uint32_t *fat;
static uint8_t *disk;

uint32_t GetClusLoc(uint32_t clus) {
	assert(clus >= 1);
	return FirstLoc + (clus - 1) * 512 * 8;
}

uint32_t TurnClus(uint32_t now) {
	return (now - FirstLoc) / 512 / 8 + 1;
}

uint32_t GetNext(uint32_t now, uint32_t sz) {
	uint32_t nxt = now + sz;
	if ((nxt  & 4095) == 0) {
		fat[TurnClus(now)] = clus + 1;
		clus += 1;
		return GetClusLoc(clus);
	}
	else return nxt;
}

void solve(DIR *dir, char *s) {
	struct dirent *ptr;
	struct file *now = (struct file *)(disk + GetClusLoc(clus) - MAX_LENGTH * 2);
	int CurrentClus = clus;
	while ((ptr = readdir(dir)) != NULL) {
		uint16_t len = (uint16_t)strlen(ptr -> d_name);
		int bias = 0, flag = 0;
		while (len) {
			now = (struct file *)(disk + GetNext((uintptr_t)now - (uintptr_t)disk, MAX_LENGTH * 2));
			if (flag == 0) now -> others[0] = 1, flag = 1;
			now -> len = len;
			now -> size = ptr -> d_reclen;
			now -> type = ptr -> d_type;
			now -> count = 1;
			now -> inode = ptr -> d_ino;
			int less = (len >= MAX_LENGTH ? MAX_LENGTH : len);
			for (int i = 0; i < less; i++) now -> name[i] = ptr -> d_name[i + bias];
			len -= less;
			bias += MAX_LENGTH;
		}
//		if (strcmp(ptr -> d_name, ".git") == 0)continue;
		char *p = malloc(1024);
		strcpy(p, s);
		strcat(p, "/");
		strcat(p, ptr -> d_name);
		if (ptr -> d_type == DT_DIR) {
			DIR *ChDir = opendir(p);
			assert(ChDir != NULL);
			if (strcmp(ptr -> d_name, ".") == 0) {
				now -> NxtClus = CurrentClus;
				free(p);
				continue;
			}
			else if (strcmp(ptr -> d_name, "..") == 0) {
				now -> NxtClus = CurrentClus - 1;
				free(p);
				continue;
			}
			now -> NxtClus = clus + 1;
			clus = clus + 1;
			solve(ChDir, p);
		}
		else {
			FILE *fp = fopen(p, "r");
			assert(fp != NULL);

			int sz = 4096, flag = 0;
			while (sz == 4096) {
				if (flag == 0) now -> NxtClus = clus + 1;
				else fat[clus] = clus + 1;
				clus = clus + 1;
				flag = 1;
				uint8_t *tep = disk + GetClusLoc(clus);
				sz = fread(tep, 1, 4096, fp);	
				printf("%s %s %d %d\n", p, now -> name, sz, now -> NxtClus);
			}
			fclose(fp);
		}
		free(p);
	}
	return;
}

int main(int argc, char *argv[]) {
  int fd, size = atoi(argv[1]) << 20;

  assert((fd = open(argv[2], O_RDWR)) > 0);
  assert((ftruncate(fd, size)) == 0);
  assert((disk = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) != (void *)-1);

  uint8_t *p = disk;

  p += 0x100000;
  fat = (uint32_t *)p;
  p += 0x100000;
  FirstLoc = 0x200000;

  DIR *dir = opendir(argv[3]);
  assert(dir != NULL);

  clus = 1;
  solve(dir, argv[3]);

  // TODO: mkfs

  munmap(disk, size);
  close(fd);
}
