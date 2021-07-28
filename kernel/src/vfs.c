#include <devices.h>
#include <dirent.h>
#include <common.h>
//#define CheckTask
#define EOF -1
#define MAX_CPU 128
extern spinlock_t trap_lock;
extern struct task *current[MAX_CPU];
device_t *sda;
uint32_t mode[MAX_CPU];
uint32_t *fat;
uint32_t clus;
struct fd_ fd[1024];
int real_bias[1024];
static int tot = 0;

struct SzList {
	uint32_t inode;
    int size;
	struct SzList *nxt;
};
struct SzList *SzHead;

uint32_t GetClusLoc(uint32_t clus) {
	if (clus == 0) return 0;
	return FILE_START + (clus - 1) * 512 * 8;	
}

void add_name(struct file *tep, const char *name) {
	assert(tep -> flag == 0xffffffff);
	tep -> size = strlen(name) + 1;
	sda -> ops -> write(sda, tep -> bias, tep, sizeof(struct file));
	char *p = pmm -> alloc(128);
	assert(p != NULL);
	strcpy(p, name);
	p[strlen(name)] = EOF;
	uint32_t now = GetClusLoc(tep -> NxtClus);
	sda -> ops -> write(sda, now, p, strlen(name) + 1); 
	pmm -> free(p);
}

uint32_t TurnClus(uint32_t now) {
	return (now - FILE_START) / 512 / 8 + 1;	
}

static int inode = 100;

struct file* create_file(uint32_t now, char *name, int type) {
	struct file *file = pmm -> alloc(sizeof(struct file));
	void *tep = pmm -> alloc(4096);
	assert(tep != NULL && file != NULL);

	while(1) {
		sda -> ops -> read(sda, now, tep, 512);
		struct file *nxt = tep;
		int flag = 0;
		for (int i = 0; i < 64; i++) {
			if (nxt -> name[0] == 0) {
				strcpy(file -> name, name);
				if (type == 0) file -> type = 8;
				else file -> type = DT_DIR;
				file -> size    = 0; 	
				file -> inode   = ++inode;	
				file -> NxtClus = ++clus;
				file -> bias    = now + 64 * i;
				file -> flag    = 0xffffffff;
				flag = 1;
				sda -> ops -> write(sda, now + i * 64, file, sizeof(struct file));
                // make dir for . and ..
				if (type != 0) {
					struct file* spj = pmm -> alloc(sizeof(struct file));
					assert(spj != NULL);
					strcpy(spj -> name, ".");
					spj -> type    = DT_DIR;
					spj -> NxtClus = clus;
					spj -> size    = 0;
					spj -> inode   = ++inode;
					spj -> bias    = GetClusLoc(clus);
					spj -> flag    =  0xffffffff;
					sda -> ops -> write(sda, GetClusLoc(clus), spj, sizeof(struct file));

					strcpy(spj -> name, "..");
					spj -> NxtClus = TurnClus(now);
					spj -> inode   = ++inode;
					spj -> type    = DT_DIR;
					spj -> bias    = GetClusLoc(clus) + 64;
					sda -> ops -> write(sda, GetClusLoc(clus) + 64, spj, sizeof(struct file));
					
					pmm -> free(spj);
				}
				break;
			}
			assert(nxt -> flag == 0xffffffff);
			nxt = nxt + 1;
		}
		if (flag == 1)break;
		if (fat[TurnClus(now)] == 0) fat[TurnClus(now)] = ++clus;	
		now = GetClusLoc(fat[TurnClus(now)]);
	} 
	pmm -> free(tep);
	return file;
}

uint32_t ProcLoc;
uint32_t ZeroLoc, NullLoc, RandLoc;

static void vfs_init()  {
	// init for sda
	sda = dev -> lookup("sda");
	// init for fat
	fat = (uint32_t *)pmm -> alloc(0x10000);
	assert(fat != NULL);
	sda -> ops -> read(sda, FAT_START, fat, 0x10000);
	clus = fat[0];
	assert(clus != 0);
	fat[0] = 0;


	// init for fd
	memset(fd, 0, sizeof(fd));
	fd[0].used = fd[1].used = fd[2].used = 1;
	for (int i = 3; i < 1024; i++)
		fd[i].used = 0;
	for (int i = 0; i < MAX_CPU; i++)
		mode[i] = 1;

	// init for dev and proc
	struct file* tep = create_file(FILE_START, "proc", 1);
	assert(tep != NULL && tep -> flag == 0xffffffff);
   
    ProcLoc = GetClusLoc(tep -> NxtClus);
	tep = create_file(ProcLoc, "cpuinfo", 0);
	tep = create_file(ProcLoc, "meminfo", 0);
	

	tep = create_file(FILE_START, "dev", 1);
	assert(tep != NULL && tep -> flag == 0xffffffff);
	uint32_t nxt = GetClusLoc(tep -> NxtClus);
	tep = create_file(nxt, "zero", 0);
	ZeroLoc = tep -> bias;
	tep = create_file(nxt, "null", 0);
	NullLoc = tep -> bias;
	tep = create_file(nxt, "random", 0);
	RandLoc = tep -> bias;
	pmm -> free(tep);

	ZeroLoc = ZeroLoc ? ZeroLoc : 0xffffffff;
	RandLoc = RandLoc ? RandLoc : 0xffffffff;
	NullLoc = NullLoc ? NullLoc : 0xffffffff;
}



uint32_t solve_path(uint32_t now, const char *path, int *status, struct file *file, int create) {
	while (path[0] == '/') path++;
	if (path[0] == 0) return now;

	char *name = pmm -> alloc(256); 
	assert(name != NULL);
	int i = 0;
	while (path[i] != 0 && path[i] != '/') 
		name[i] = path[i], name[i + 1] = 0, i++;
	path = path + i;

	void *tep = pmm -> alloc(0x100000);
	assert(tep != NULL);
	uint32_t lst = 0;
	while (1) {
		if (now == 0) break; // current dir
		lst = now;
		sda -> ops -> read(sda, now, tep, 4096);
		struct file *nxt = (struct file *)tep;
		assert(nxt != NULL);
		assert(nxt -> flag = 0xffffffff);
		for (int i = 0; i < 64; i++) {
			if (nxt -> name[0] != 0)assert(nxt -> flag == 0xffffffff);
			if (strcmp(name, nxt -> name) == 0) {
				if (nxt -> type == DT_DIR) {
					memcpy(file, nxt, sizeof(struct file));
					pmm -> free(tep), pmm -> free(name);
					return solve_path(GetClusLoc(nxt -> NxtClus), path, status, file, create);
				}
				else {
					if (path[0] != 0) {
						pmm -> free(tep), pmm -> free(name);
						return -1;
					}
					memcpy(file, nxt, sizeof(struct file));
					pmm -> free(tep), pmm -> free(name);
					return 1;
				}					
			}
			nxt = nxt + 1;
		}
		now = GetClusLoc(fat[TurnClus(now)]);
		assert(now == 0);
	}

	if (path[0] == 0 && create == 1) {
		assert(lst != 0);
		struct file *nxt = create_file(lst, name, 0); 
		memcpy(file, nxt, sizeof(struct file));
		pmm -> free(nxt), pmm -> free(tep), pmm -> free(name);
		return 0;
	}
	if (path[0] == 0 && create == 2) {
		assert(lst != 0);
		struct file *nxt = create_file(lst, name, 1);
		memcpy(file, nxt, sizeof(struct file));
		pmm -> free(nxt), pmm -> free(tep), pmm -> free(name);
		return 0;
	}
	pmm -> free(tep), pmm -> free(name);
	return -1;
}

static int vfs_chdir(const char *path) {
	kmt -> spin_lock(&trap_lock);
	int id = cpu_current();
	uint32_t now = (path[0] == '/') ? FILE_START : current[id] -> inode;
	int status = O_RDONLY;
	struct file* tep = pmm -> alloc(sizeof(struct file));
	assert(tep != NULL);
	uint32_t nxt = solve_path(now, path + (path[0] == '/'), &status, tep, 0);

	int result = -1;
	if (nxt == -1 || nxt == 0 || nxt == 1) result = -1;
	else current[id] -> inode = nxt, result = 0;
    #ifdef CheckTask
	printf("chdir result:%d current dir:%s location:%x\n", result, path, nxt);
	#endif
	pmm -> free(tep);
	kmt -> spin_unlock(&trap_lock);
	return result;
}

static int vfs_open(const char *path, int flags) {
	kmt -> spin_lock(&trap_lock);
	int id = cpu_current();
	uint32_t now = (path[0] == '/') ? FILE_START : current[id] -> inode;
	int status = 1;
	struct file* tep = pmm -> alloc(sizeof(struct file));
	assert(tep != NULL);
	status = flags;
	uint32_t nxt = solve_path(now, path + (path[0] == '/'), &status, tep, (flags & O_CREAT) != 0);
	int result = -1;
	if (nxt == -1 || (nxt != 0 && nxt != 1 && flags != O_RDONLY)) {
		result = -1;
		pmm -> free(tep);
	}
	else {
		#ifdef CheckTask	
		if (nxt == 0) printf("CREATE!!\n");
		#endif
		if (nxt == FILE_START) {
			tep -> NxtClus = 1, strcpy(tep -> name, "/"), tep -> type = DT_DIR;	tep -> flag = 0xffffffff;
		}
		for (int i = 0; i < 1024; i++) 
			if (fd[i].used == 0) {
				fd[i].used = 1; 
				fd[i].flag = flags;
				fd[i].file = tep;
				fd[i].real_bias = NULL;
				fd[i].bias = 0;
				result = i;
				break;
			}
		#ifdef CheckTask
		if (nxt != 0)printf("open result:%d name:%s location:%x\n", result, tep -> name, tep -> bias);
		else printf("open result:%d name:%s  location:%x CREATE!!\n", result, tep -> name, tep -> bias);
		#endif
	}
	kmt -> spin_unlock(&trap_lock);	
	return result;
}

static int vfs_close(int num) {
	kmt -> spin_lock(&trap_lock);
	int result = -1;
	if (num < 0 || num >= 1024) result = -1;
	else {
		if (fd[num].used == 0 || fd[num].file == NULL) result = -1;
		else fd[num].used = 0, result = 0, pmm -> free(fd[num].file), fd[num].real_bias = NULL;
	}
	#ifdef CheckTask
	printf("close result:%d fd:%d\n", result, num);
	#endif
	kmt -> spin_unlock(&trap_lock);
	return result;
} 

static int vfs_mkdir(const char *pathname) {
	kmt -> spin_lock(&trap_lock);
	int id = cpu_current();
	uint32_t now = (pathname[0] == '/') ? FILE_START : current[id] -> inode;
	int status = O_CREAT;
	struct file *tep = pmm -> alloc(sizeof(struct file));
	assert(tep != NULL);
	uint32_t nxt = solve_path(now, pathname + (pathname[0] == '/'), &status, tep, 2);	
	
	int result = -1;
	if (nxt != 0) result = -1;
	else result = 0;
	#ifdef CheckTask
	printf("mkdir result:%d path:%s\n", result, pathname);
	#endif
	kmt -> spin_unlock(&trap_lock);
	return result;
}

static int count_file(uint32_t now, int flag, struct ufs_dirent* obj, int st, int nd) {
	int count = 0, tot = 0;
	void *tep = pmm -> alloc(4096);
	assert(tep != NULL);
	while(1) {
		if (now == 0) break;
		sda -> ops -> read(sda, now, tep, 4096);
		struct file *nxt = tep;
		for (int i = 0; i < 64; i++) {
			if (nxt -> name[0] != 0) {
				if (count >= st && count <= nd && flag) { // remaining delete problem
					struct ufs_dirent p;
					strcpy(p.name, nxt -> name), p.inode = nxt -> inode;
					memcpy(obj, &p, sizeof(struct ufs_dirent));
					obj = obj + 1;	
					tot++;
				} 
				count = count + 1; 
			}
			nxt = nxt + 1;
		}
		now = GetClusLoc(fat[TurnClus(now)]);
	} 
	pmm -> free(tep);
	if (flag) return tot;
	return count;
} 

static int vfs_fstat(int fd_num, struct ufs_stat *buf) {
	kmt -> spin_lock(&trap_lock);
	int result = -1;
	if (fd_num < 0 || fd_num >= 1024 || fd[fd_num].used == 0 || fd[fd_num].file == NULL) result = -1;
	else {
		result = 0;
		buf -> id = fd[fd_num].file -> inode;
		if (fd[fd_num].file -> type == DT_DIR) {
			buf -> type = T_DIR;
			buf -> size = sizeof(struct ufs_dirent) * count_file(GetClusLoc(fd[fd_num].file -> NxtClus), 0, NULL, 0, 0);	
		}
		else {
			buf -> type = T_FILE;
			struct SzList* NowSz = SzHead;
			int sz = fd[fd_num].file -> size;
			while (NowSz != NULL) {
				if (NowSz -> inode == fd[fd_num].file -> inode) {
					sz = NowSz -> size;
					break;
				}
				NowSz = NowSz -> nxt;	
			}
			buf -> size = sz;
		}
		#ifdef CheckTask
		printf("stat id:%d type:%d size:%d\n", buf -> id, buf -> type, buf -> size);
		#endif
	}
	kmt -> spin_unlock(&trap_lock);
	return result;
}

static int vfs_link(const char *oldpath, const char *newpath) {
   	kmt -> spin_lock(&trap_lock);
	int id = cpu_current(), result = -1;

	uint32_t now = (oldpath[0] == '/') ? FILE_START : current[id] -> inode;
	int status = O_RDONLY;
	struct file* old = pmm -> alloc(sizeof(struct file));
	assert(old != NULL);
	uint32_t nxt = solve_path(now, oldpath + (oldpath[0] == '/'), &status, old, 0);
	
	if (nxt != 1) result = -1;
	else {
		now = (newpath[0] == '/') ? FILE_START : current[id] -> inode;
		struct file* new = pmm -> alloc(sizeof(struct file));
		status = O_CREAT;		
		assert(new != NULL);
		uint32_t nxt = solve_path(now, newpath + (newpath[0] == '/'), &status, new, 1);
		if (nxt != 0) {
			result = -1;
			pmm -> free(new);
		}
		else {
			result = 0;	
			clus = clus - 1;
			#ifdef CheckTask
			printf("link success form location %x to location %x\n", old -> bias, new -> bias);
			#endif
			old -> bias = new -> bias;
			strcpy(old -> name, new -> name);
			sda -> ops -> write(sda, new -> bias, old, sizeof(struct file));
		}	
		pmm -> free(new);
	}
	pmm -> free(old);
	kmt -> spin_unlock(&trap_lock);
	return result;
}

static int vfs_unlink(const char* path) {
	kmt -> spin_lock(&trap_lock);
	int id = cpu_current();
	uint32_t now = (path[0] == '/') ? FILE_START : current[id] -> inode;
	int status = O_RDONLY;
	struct file* tep = pmm -> alloc(sizeof(struct file));
	assert(tep != NULL);
	uint32_t nxt = solve_path(now, path + (path[0] == '/'), &status, tep, 0);
	int result = -1;
	if (nxt != 1) result = -1;
	else {
		result = 0;
		int bias = tep -> bias;
		memset(tep, 0, sizeof(struct file));
		sda -> ops -> write(sda, bias, tep, sizeof(struct file));	
		#ifdef CheckTask
		printf("unlink success from location %x\n", bias);
		#endif
	}
	pmm -> free(tep);
	kmt -> spin_unlock(&trap_lock);	
	return result;
}

static int vfs_read(int fd_num, void *buf, int count) {
	kmt -> spin_lock(&trap_lock);
	char *obj = (char *)buf;
	int result = 0;
	if (fd_num < 0 || fd_num >= 1024 || fd[fd_num].used == 0 || fd[fd_num].file == NULL) result = -1;
	else {
		if (fd[fd_num].file -> bias == ZeroLoc) {
		    for (int i = 0; i < count; i++) obj[i] = 0;
			result = count;
			#ifdef CheckTask
			printf("read zero\n");
			#endif
		}
		else if (fd[fd_num].file -> bias == NullLoc) {
			for (int i = 0; i < count; i++) obj[i] = EOF;
			result = count;
			#ifdef CheckTask
			printf("read null\n");
			#endif
		}
		else if (fd[fd_num].file -> bias == RandLoc) {
			for (int i = 0; i < count; i++) obj[i] = rand() % 256;
			result = count;
			#ifdef CheckTask
			printf("read rand\n");
			#endif	
		}
		else if (fd[fd_num].file -> type != DT_DIR) {
			result = 0;
			int sz    = fd[fd_num].file -> size;
			int now   = GetClusLoc(fd[fd_num].file -> NxtClus);
			int bias  = fd[fd_num].bias;
			int loc   = 0; 
			int p     = 0;

			if (fd[fd_num].real_bias != NULL) bias = *fd[fd_num].real_bias;
			struct SzList* NowSz = SzHead;
			while (NowSz != NULL) {
				if (NowSz -> inode == fd[fd_num].file -> inode) {
					sz = NowSz -> size;
					break;
				}
				NowSz = NowSz -> nxt;	
			}

			char *tep = pmm -> alloc(4096);
			assert(tep != NULL);

			while (1) {
				if (count == 0) break;
				if (bias >= 4096) bias -= 4096, loc += 4096;
				else  {
					loc += bias;
					sda -> ops -> read(sda, now, tep, 4096);
					for (int i = bias; i < 4096; i++) {
						if (loc == sz || count == 0) break;
						obj[p++] = tep[i]; 			
						count--;
						loc++;
					}
					bias = 0;
				}
				now = GetClusLoc(fat[TurnClus(now)]);
				if (now == 0 || count == 0 || loc == sz) break;
			}
			pmm -> free(tep);
			fd[fd_num].bias = loc;
			if (fd[fd_num].real_bias != NULL) *fd[fd_num].real_bias = loc;
			result = p;
		}
		else {
			result = 0;		
			assert(count % sizeof(struct ufs_dirent) == 0);
			count = count / sizeof(struct ufs_dirent);
			if (fd[fd_num].real_bias != NULL)
					fd[fd_num].bias = *fd[fd_num].real_bias;
			int sz = count_file(GetClusLoc(fd[fd_num].file -> NxtClus), 1, buf, fd[fd_num].bias / sizeof(struct ufs_dirent), fd[fd_num].bias / sizeof(struct ufs_dirent) + count - 1);
			fd[fd_num].bias += sizeof(struct ufs_dirent) * sz;
			result = sizeof(struct ufs_dirent) * sz; 
		}
	}
	kmt -> spin_unlock(&trap_lock);
	return result;
}

static int vfs_write(int fd_num, void *buf, int count) {
	kmt -> spin_lock(&trap_lock);
	char *obj = (char *)buf;
	int result = 0;
	if (fd_num < 0 || fd_num >= 1024 || fd[fd_num].used == 0 || fd[fd_num].file == NULL) {
		result = -1;
		assert(0);
	}
	else if (fd[fd_num].flag ==  O_WRONLY || fd[fd_num].flag == O_CREAT) {
		result = -1;
		assert(0);
	}
	else {
		if (fd[fd_num].file -> bias == ZeroLoc) {
			#ifdef CheckTask
			printf("write zero\n");
			#endif
		    result = -1;	
		}
		else if (fd[fd_num].file -> bias == NullLoc) {
		    #ifdef CheckTask
			printf("write null\n");
			#endif
			result = count;
		}
		else if (fd[fd_num].file -> bias == RandLoc) {
			#ifdef CheckTask
			printf("write rand\n");
			#endif
			result = -1;
		}
		else if (fd[fd_num].file -> type != DT_DIR) {
			result = 0;
			int sz    = fd[fd_num].file -> size;
			int now   = GetClusLoc(fd[fd_num].file -> NxtClus);
			int bias  = fd[fd_num].bias;
			int loc   = 0; 
			int p     = 0;
			if (fd[fd_num].real_bias != NULL) {
				bias = *fd[fd_num].real_bias;
			}
			struct SzList* NowSz = SzHead;
			while (NowSz != NULL) {
				if (NowSz -> inode == fd[fd_num].file -> inode) {
					sz = NowSz -> size;
					break;
				}
				NowSz = NowSz -> nxt;	
			}

			while (1) {
				if (bias >= 4096) bias -= 4096, loc += 4096;
				else  {
					loc += bias;
					for (int i = bias; i < 4096; i++) {
						if (count == 0) break;
						sda -> ops -> write(sda, now + i, obj + p, 1);
						p++;
						count--;
						loc++;
					}
					bias = 0;
				}
				if (count == 0) break;
				if (fat[TurnClus(now)] == 0) fat[TurnClus(now)] = ++clus;	
				now = GetClusLoc(fat[TurnClus(now)]);
			}
			fd[fd_num].bias = loc;
			if (fd[fd_num].real_bias != NULL) *fd[fd_num].real_bias = loc;
			if (loc > sz) {
				struct SzList *NowSz = SzHead, *lst;
				int finish;
				if (NowSz == NULL) {
					SzHead = pmm -> alloc(sizeof(struct SzList));
					assert(SzHead != NULL);
					SzHead -> inode = fd[fd_num].file -> inode;	
					SzHead -> size = loc;
					finish = 1;
				}
				else {
					while (NowSz != NULL) {
						lst = NowSz;
						if (NowSz -> inode == fd[fd_num].file -> inode) {
							NowSz -> size = loc;
							finish = 1;
							break;
						}
						NowSz = NowSz -> nxt;
					}
					if (finish == 0) {
						NowSz = pmm -> alloc(sizeof(struct SzList));
						assert(NowSz != NULL);
						NowSz -> inode = fd[fd_num].file -> inode;	
						NowSz -> size = loc;
						lst -> nxt = NowSz; 	
					}	
				}
			}
			result = p;
		}
		else assert(0);
	}
	kmt -> spin_unlock(&trap_lock);
	return result;
}

static int vfs_lseek(int fd_num, int offset, int whence) {
	kmt -> spin_lock(&trap_lock);
	int result = -1;
	if (fd_num < 0 || fd_num >= 1024 || fd[fd_num].file == NULL) result = -1;
	else {
		int bias = fd[fd_num].bias;
		if (fd[fd_num].real_bias != NULL) bias = *fd[fd_num].real_bias;
		if (whence == SEEK_CUR) bias += offset;
		else if (whence == SEEK_SET) bias = offset;	
		else {
			int sz = fd[fd_num].file -> size;
			struct SzList* NowSz = SzHead;
			while (NowSz != NULL) {
				if (NowSz -> inode == fd[fd_num].file -> inode) {
					sz = NowSz -> size;
					break;
				}
				NowSz = NowSz -> nxt;	
			}
			bias = sz + offset;
		}
		result = bias;
		fd[fd_num].bias = bias;
		if (fd[fd_num].real_bias != NULL) *fd[fd_num].real_bias = bias;
	}
	kmt -> spin_unlock(&trap_lock);
	return result;
}

static int vfs_dup(int fd_num) {
	kmt -> spin_lock(&trap_lock);
	int newfd = -1;
	if (fd_num < 0 || fd_num >= 1024 || fd[fd_num].used == 0) newfd = -1;
	else {
		for (int i = 0; i < 1024; i++) 
			if (fd[i].used == 0) {
				newfd = i; 
				fd[i].used = 1;
				break;	
			}
		if (newfd != -1) {
			fd[newfd].bias = fd[fd_num].bias;	
			fd[newfd].flag = fd[fd_num].flag;
			fd[newfd].file = pmm -> alloc(sizeof(struct file));
			assert(fd[newfd].file != NULL);
			assert(fd[fd_num].file != NULL);
			memcpy(fd[newfd].file, fd[fd_num].file, sizeof(struct file));	
		}
	}
	if (fd[fd_num].real_bias == NULL) {
		tot++;
		assert(tot < 1024);	
		real_bias[tot - 1] = fd[fd_num].bias;
		fd[fd_num].real_bias = &real_bias[tot - 1];
	}
	fd[newfd].real_bias = fd[fd_num].real_bias;
	fd[fd_num].bias = fd[newfd].bias = *fd[fd_num].real_bias;
	kmt -> spin_unlock(&trap_lock);
	return newfd;
}

MODULE_DEF(vfs) = {
	.init   = vfs_init,	
	.chdir  = vfs_chdir,
	.open   = vfs_open,
	.close  = vfs_close,
	.mkdir  = vfs_mkdir,
	.fstat  = vfs_fstat,
	.link   = vfs_link,
	.unlink = vfs_unlink,
	.read   = vfs_read,
	.write  = vfs_write,
	.lseek  = vfs_lseek,
	.dup    = vfs_dup,
};
