#include <devices.h>
#include <dirent.h>
#include <common.h>
#define EOF -1
#define MAX_CPU 8

extern spinlock_t trap_lock;
device_t *sda;
uint32_t current_dir[MAX_CPU];
uint32_t mode[MAX_CPU];
uint32_t *fat;
uint32_t clus;
uint32_t size[10000000 + 5] = {0};
struct fd_ fd[1024];

uint32_t GetClusLoc(uint32_t clus) {
	if (clus == 0) return 0;
	return 0x200000 + (clus - 1) * 512 * 8;	
}

void add_name(struct file *tep, const char *name) {
	tep -> size = strlen(name) + 1;
	sda -> ops -> write(sda, tep -> bias, tep, sizeof(struct file));
	char *p = pmm -> alloc(128);
	strcpy(p, name);
	p[strlen(name)] = EOF;
	uint32_t now = GetClusLoc(tep -> NxtClus);
	sda -> ops -> write(sda, now, p, strlen(name) + 1); 
	pmm -> free(p);
}

uint32_t TurnClus(uint32_t now) {
	return (now - 0x200000) / 512 / 8 + 1;	
}

int inode = 100;

struct file* create_file(uint32_t now, char *name, int type) {
	struct file *file = pmm -> alloc(sizeof(struct file));
	void *tep = pmm -> alloc(4096);
	while(1) {
		sda -> ops -> read(sda, now, tep, 4096);
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
				if (type != 0) {
					struct file* spj = pmm -> alloc(sizeof(struct file));
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
			nxt = nxt + 1;
		}
		if (flag == 1)break;
		if (GetClusLoc(fat[TurnClus(now)]) == 0) fat[TurnClus(now)] = ++clus;	
		now = GetClusLoc(fat[TurnClus(now)]);
	} 
	pmm -> free(tep);
	return file;
}

uint32_t ProcLoc;
uint32_t ZeroLoc, NullLoc, RandLoc;

static void vfs_init()  {
	sda = dev -> lookup("sda");
	fat = (uint32_t *)pmm -> alloc(0x100000);
	sda -> ops -> read(sda, 0x100000, fat, 0x100000);
	for (int i = 0; i < MAX_CPU; i++)
		current_dir[i] = 0x200000, mode[i] = 1;
	assert(fat != NULL);
	memset(fd, 0, sizeof(fd));
	fd[0].used = fd[1].used = fd[2].used = 1;
	for (int i = 3; i < 1024; i++)
		fd[i].used = 0;
	clus = fat[0];
	fat[0] = 0;
	struct file* tep = create_file(0x200000, "proc", 1);
    ProcLoc = GetClusLoc(tep -> NxtClus);
	tep = create_file(ProcLoc, "cpuiofo", 0);
	tep = create_file(ProcLoc, "memiofo", 0);

	tep = create_file(0x200000, "dev", 1);
	uint32_t nxt = GetClusLoc(tep -> NxtClus);
	tep = create_file(nxt, "zero", 0);
	ZeroLoc = tep -> bias;
	tep = create_file(nxt, "null", 0);
	NullLoc = tep -> bias;
	tep = create_file(nxt, "random", 0);
	RandLoc = tep -> bias;
	pmm -> free(tep);
}



uint32_t solve_path(uint32_t now, const char *path, int *status, struct file *file, int create) {
	if (path[0] == 0) return now;
	char *name = pmm -> alloc(256); ///////////////////////////////
	int i = 0;
	while (path[0] == '/') path++;
	while (path[i] != 0 && path[i] != '/') 
		name[i] = path[i], name[i + 1] = 0, i++;
	path = path + i;
	void *tep = pmm -> alloc(4096); ///////////////////////////	
	assert(strcmp(name, "proc") && strcmp(name, "dev"));
	uint32_t lst = 0;
	while (1) {
		if (now == 0) break;
		lst = now;
		sda -> ops -> read(sda, now, tep, 4096);
		struct file *nxt = tep;
		for (int i = 0; i < 64; i++) {
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
	assert(0);
	kmt -> spin_lock(&trap_lock);
	int id = cpu_current();
	uint32_t now = (path[0] == '/') ? 0x200000 : current_dir[id];
	int status = (now == 0x200000) ? 1 : mode[id];
	
	assert(mode[id] == 1); ///////////////////////////////////////////
	struct file tep;
	uint32_t nxt = solve_path(now, path + (path[0] == '/'), &status, &tep, 0);
	int result = 0;
	if (nxt == -1 || nxt == 1) result = -1;
	else current_dir[id] = nxt;
//	printf("%s %x\n", path, nxt);
	kmt -> spin_unlock(&trap_lock);
	return result;
}
static int T = 0;

static int vfs_open(const char *path, int flags) {
	kmt -> spin_lock(&trap_lock);
	T++;
	if (T == 4)assert(0);
	int id = cpu_current();
	uint32_t now = (path[0] == '/') ? 0x200000 : current_dir[id];
	assert(current_dir[id] == 0x200000);
	int status = (now == 0x200000) ? 1 : mode[id];
	
	assert(mode[id] == 1); ///////////////////////////////////////////

	struct file* tep = pmm -> alloc(sizeof(struct file));
	status = flags;
	uint32_t nxt = solve_path(now, path + (path[0] == '/'), &status, tep, (flags & O_CREAT) != 0);
	int result = -1;
	if (nxt == -1) result = -1;
	else {
		assert(0);
		if (tep -> type == DT_DIR && flags != O_RDONLY)assert(0);
	//	if (nxt == 0) printf("CREATE!!\n");
		if (nxt == 0x200000) {
			tep -> NxtClus = 1, strcpy(tep -> name, "/"), tep -> type = DT_DIR;	
		}
		for (int i = 0; i < 1024; i++) 
			if (fd[i].used == 0) {
				fd[i].used = 1; 
				fd[i].flag = flags;	
				fd[i].file = tep;
				fd[i].bias = 0;
			//	printf("%s %x\n", tep -> name, tep -> bias);
				result = i;
				break;
			}
	}
    assert(result == -1);	
	kmt -> spin_unlock(&trap_lock);	
	return result;
}

static int vfs_close(int num) {
	assert(0);
	kmt -> spin_lock(&trap_lock);
	int result = -1;
	if (num < 0 || num >= 1024) result = -1;
	else {
		if (fd[num].used == 0 || fd[num].file == NULL) result = -1;
		else fd[num].used = 0, result = 0, pmm -> free(fd[num].file);
	}
	kmt -> spin_unlock(&trap_lock);
	return result;
} 

static int vfs_mkdir(const char *pathname) {
	assert(0);
	kmt -> spin_lock(&trap_lock);
	int id = cpu_current();
	uint32_t now = (pathname[0] == '/') ? 0x200000 : current_dir[id];
	int status = (now == 0x200000) ? 1 : mode[id];

	assert(mode[id] == 1);
	
	struct file *tep = pmm -> alloc(sizeof(struct file));
	uint32_t nxt = solve_path(now, pathname + (pathname[0] == '/'), &status, tep, 2);	
	int result = -1;
	if (nxt != 0) result = -1;
	else result = 0;
	kmt -> spin_unlock(&trap_lock);
	return result;
}

static int count_file(uint32_t now, int flag, struct ufs_dirent* obj, int st, int nd) {
	int count = 0, tot = 0;
	now = GetClusLoc(now);
	void *tep = pmm -> alloc(4096);
	while(1) {
		if (now == 0) break;
		sda -> ops -> read(sda, now, tep, 4096);
		struct file *nxt = tep;
		for (int i = 0; i < 64; i++) {
			if (nxt -> name[0] != 0) {
				if (count >= st && count <= nd && flag) {
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
	assert(0);
	kmt -> spin_lock(&trap_lock);
	int result = 0;
	if (fd[fd_num].used == 0) result = -1;
	else {
		result = 0;
		buf -> id = fd[fd_num].file -> inode;
		if (fd[fd_num].file -> type == DT_DIR) {
			buf -> type = T_DIR;
			buf -> size = sizeof(struct ufs_dirent) * count_file(fd[fd_num].file -> NxtClus, 0, NULL, 0, 0);	
		}
		else {
			buf -> type = T_FILE;
			buf -> size = (size[fd[fd_num].file -> inode] == 0) ? fd[fd_num].file -> size : size[fd[fd_num].file -> inode];
		}
	//	printf("%d %d %d\n", buf -> id, buf -> type, buf -> size);
	}
	kmt -> spin_unlock(&trap_lock);
	return result;
}

static int vfs_link(const char *oldpath, const char *newpath) {
	assert(0);
   	kmt -> spin_lock(&trap_lock);
	int id = cpu_current(), result = -1;

	uint32_t now = (oldpath[0] == '/') ? 0x200000 : current_dir[id];
	int status = (now == 0x200000) ? 1 : mode[id];

	struct file* old = pmm -> alloc(sizeof(struct file));
	uint32_t nxt = solve_path(now, oldpath + (oldpath[0] == '/'), &status, old, 0);
	
	if (nxt != 1) result = -1;
	else {
		now = (newpath[0] == '/') ? 0x200000 : current_dir[id];
		struct file* new = pmm -> alloc(sizeof(struct file));		
		uint32_t nxt = solve_path(now, newpath + (newpath[0] == '/'), &status, new, 1);
		if (nxt != 0) result = -1;
		else {
			result = 0;	
			clus = clus - 1;
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
	assert(0);
	kmt -> spin_lock(&trap_lock);
	int id = cpu_current();
	uint32_t now = (path[0] == '/') ? 0x200000 : current_dir[id];
	int status = (now == 0x200000) ? 1 : mode[id];
	
	assert(mode[id] == 1); ///////////////////////////////////////////

	struct file* tep = pmm -> alloc(sizeof(struct file));
	uint32_t nxt = solve_path(now, path + (path[0] == '/'), &status, tep, 0);
	int result = -1;
	if (nxt != 1) result = -1;
	else {
		result = 0;
		int bias = tep -> bias;
		memset(tep, 0, sizeof(struct file));
		sda -> ops -> write(sda, bias, tep, sizeof(struct file));	
	}
	pmm -> free(tep);
	kmt -> spin_unlock(&trap_lock);	
	return result;
}

static int vfs_read(int fd_num, void *buf, int count) {
	assert(0);
	kmt -> spin_lock(&trap_lock);
	char *obj = (char *)buf;
	int result = 0;
//	printf ("%x %x\n", RandLoc, fd[fd_num].file -> bias);
	if (fd[fd_num].used == 0 || fd[fd_num].file == NULL) result = -1;
	else {
		if (fd[fd_num].file -> bias == ZeroLoc) {
		    for (int i = 0; i < count; i++) obj[i] = 0;
			result = count;
		//	printf("read zero\n");
		}
		else if (fd[fd_num].file -> bias == NullLoc) {
			for (int i = 0; i < count; i++) obj[i] = EOF;
			result = count;
		//	printf("read null\n");
		}
		else if (fd[fd_num].file -> bias == RandLoc) {
			for (int i = 0; i < count; i++) obj[i] = rand() % 256;
			result = count;
		//	printf("read rand\n");
			
		}
		else if (fd[fd_num].file -> type != DT_DIR) {
			result = 0;
			int sz    = fd[fd_num].file -> size;
			int now   = GetClusLoc(fd[fd_num].file -> NxtClus);
			int bias  = fd[fd_num].bias;
			int loc   = 0; 
			int p     = 0;
			char *tep = pmm -> alloc(4096);

			while (1) {
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
				}
				now = GetClusLoc(fat[TurnClus(now)]);
				if (now == 0 || count == 0 || loc == sz) break;
			}
			pmm -> free(tep);
			fd[fd_num].file -> bias = loc;
			result = p;
		}
		else {
			result = 0;		
			assert(count % sizeof(struct ufs_dirent) == 0);
			count = count / sizeof(struct ufs_dirent);
			int sz = count_file(fd[fd_num].file -> NxtClus, 1, buf, fd[fd_num].bias / sizeof(struct ufs_dirent), fd[fd_num].bias / sizeof(struct ufs_dirent) + count - 1);
			fd[fd_num].bias += sizeof(struct ufs_dirent) * sz;
			result = sizeof(struct ufs_dirent) * sz; 
		}
	}
	kmt -> spin_unlock(&trap_lock);
	return result;
}

static int vfs_write(int fd_num, void *buf, int count) {
	assert(0);
	kmt -> spin_lock(&trap_lock);
	char *obj = (char *)buf;
	int result = 0;
	if (fd[fd_num].used == 0 || fd[fd_num].file == NULL) result = -1;
	else if ((fd[fd_num].flag & O_WRONLY) == 0) result = -1;
	else {
		if (fd[fd_num].file -> bias == ZeroLoc) {
		//	printf("write zero\n");
		    result = -1;	
		}
		else if (fd[fd_num].file -> bias == NullLoc) {
		//	printf("write null\n");
			result = count;
		}
		else if (fd[fd_num].file -> bias == RandLoc) {
	//		printf("write null\n");
			result = -1;
		}
		else if (fd[fd_num].file -> type != DT_DIR) {
			result = 0;
			int sz    = fd[fd_num].file -> size;
			int now   = GetClusLoc(fd[fd_num].file -> NxtClus);
			int bias  = fd[fd_num].bias;
			int loc   = 0; 
			int p     = 0;

			while (1) {
			//	printf("%x\n", now);
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
				}
				if (count == 0) break;
				if (GetClusLoc(fat[TurnClus(now)]) == 0) fat[TurnClus(now)] = ++clus;	
				now = GetClusLoc(fat[TurnClus(now)]);
			}
			fd[fd_num].bias = loc;
			if (loc > sz) size[fd[fd_num].file -> inode] = loc;
			result = p;
		}
		else result = -1;
	}
	kmt -> spin_unlock(&trap_lock);
	return result;
}

static int vfs_lseek(int fd_num, int offset, int whence) {
	assert(0);
	kmt -> spin_lock(&trap_lock);
	int result = -1;
	if (fd_num < 0 || fd_num >= 1024 || fd[fd_num].file == NULL) result = -1;
	else {
		if (whence == SEEK_CUR) fd[fd_num].bias += offset;
		else if (whence == SEEK_SET) fd[fd_num].bias = offset;	
		else fd[fd_num].bias = (size[fd[fd_num].file -> inode] == 0) ? fd[fd_num].file -> size - offset : size[fd[fd_num].file -> inode] - offset;
		result = fd[fd_num].bias;
	}
	kmt -> spin_unlock(&trap_lock);
	return result;
}

static int vfs_dup(int fd_num) {
	assert(0);
	kmt -> spin_lock(&trap_lock);
	int newfd = -1;
	for (int i = 0; i < 1024; i++) 
		if (fd[i].used == 0) {
			newfd = i; break;	
		}
	if (newfd != -1) memcpy(&fd[newfd], &fd[fd_num], sizeof(struct fd_));
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
