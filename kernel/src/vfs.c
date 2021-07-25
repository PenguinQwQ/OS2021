#include <devices.h>
#include <dirent.h>
#include <vfs.h>
#define MAX_CPU 8

extern spinlock_t trap_lock;
device_t *sda;
uint32_t current_dir[MAX_CPU];
uint32_t mode[MAX_CPU];
uint32_t *fat;
uint32_t clus;
struct fd_ fd[1024];

uint32_t GetClusLoc(uint32_t clus) {
	if (clus == 0) return 0;
	return 0x200000 + (clus - 1) * 512 * 8;	
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
					spj -> NxtClus = clus;
					spj -> size    = 0;
					spj -> inode   = ++inode;
					spj -> bias    = GetClusLoc(clus);
					spj -> flag    =  0xffffffff;
					sda -> ops -> write(sda, GetClusLoc(clus), spj, sizeof(struct file));

					strcpy(spj -> name, "..");
					spj -> NxtClus = TurnClus(now);
					spj -> inode   = ++inode;
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

static void vfs_init()  {
	sda = dev -> lookup("sda");
	fat = (uint32_t *)pmm -> alloc(0x100000);
	sda -> ops -> read(sda, 0x100000, fat, 0x100000);
	for (int i = 0; i < MAX_CPU; i++)
		current_dir[i] = 0x200000, mode[i] = 1;
	assert(fat != NULL);
	fd[0].used = fd[1].used = fd[2].used = 1;
	clus = fat[0];
}



uint32_t solve_path(uint32_t now, const char *path, int *status, struct file *file, int create) {
	if (path[0] == 0) return now;
	char *name = pmm -> alloc(256); ///////////////////////////////
	int i = 0;
	while (path[0] == '/') path++;
	while (path[i] != 0 && path[i] != '/') 
		name[i] = path[i], name[i + 1] = 0, i++;
	path = path + i;
	assert(strcmp("proc", name) && strcmp("dev", name)); ///////////////////////
	void *tep = pmm -> alloc(4096); ///////////////////////////	

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
					printf("%d\n", file -> size);
					return solve_path(GetClusLoc(nxt -> NxtClus), path, status, file, create);
				}
				else {
					if (path[0] != 0) {
						pmm -> free(tep), pmm -> free(name);
						return -1;
					}
					memcpy(file, nxt, sizeof(struct file));
					pmm -> free(tep), pmm -> free(name);
					return 0;
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
	kmt -> spin_lock(&trap_lock);
	int id = cpu_current();
	uint32_t now = (path[0] == '/') ? 0x200000 : current_dir[id];
	int status = (now == 0x200000) ? 1 : mode[id];
	
	assert(mode[id] == 1); ///////////////////////////////////////////
	struct file tep;
	uint32_t nxt = solve_path(now, path + (path[0] == '/'), &status, &tep, 0);
	int result = 0;
	if (nxt == -1) result = -1;
	else current_dir[id] = nxt;
	printf("%s %x\n", path, nxt);
	kmt -> spin_unlock(&trap_lock);
	return result;
}

static int vfs_open(const char *path, int flags) {
	kmt -> spin_lock(&trap_lock);
	int id = cpu_current();
	uint32_t now = (path[0] == '/') ? 0x200000 : current_dir[id];
	int status = (now == 0x200000) ? 1 : mode[id];
	
	assert(mode[id] == 1); ///////////////////////////////////////////

	struct file* tep = pmm -> alloc(sizeof(struct file));
	uint32_t nxt = solve_path(now, path + (path[0] == '/'), &status, tep, (flags & O_CREAT) != 0);
	int result = -1;
	if (nxt == -1) result = -1;
	else {
		for (int i = 0; i < 1024; i++) 
			if (fd[i].used == 0) {
				fd[i].used = 1; 
				fd[i].flag = flags;	
				fd[i].file = tep;
				fd[i].bias = 0;
				printf("%s %x\n", tep -> name, tep -> bias);
				result = i;
				break;
			}
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
		else fd[num].used = 0, result = 0, pmm -> free(fd[num].file);
	}
	kmt -> spin_unlock(&trap_lock);
	return result;
} 

static int vfs_mkdir(const char *pathname) {
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

static int count_file(uint32_t now, int flag) {
	int count = 0;
	now = GetClusLoc(now);
	void *tep = pmm -> alloc(4096);
	while(1) {
		if (now == 0) break;
		sda -> ops -> read(sda, now, tep, 4096);
		struct file *nxt = tep;
		for (int i = 0; i < 64; i++) {
			if (nxt -> name[0] != 0) count = count + 1; 
			nxt = nxt + 1;
		}
		now = GetClusLoc(fat[TurnClus(now)]);
	} 
	pmm -> free(tep);
	return count;
} 

static int vfs_fstat(int fd_num, struct ufs_stat *buf) {
	kmt -> spin_lock(&trap_lock);
	int result = 0;
	if (fd[fd_num].used == 0) result = -1;
	else {
		result = 0;
		buf -> id = fd[fd_num].file -> inode;
		if (fd[fd_num].file -> type == DT_DIR) {
			buf -> type = T_DIR;
			buf -> size = sizeof(struct ufs_dirent) * count_file(fd[fd_num].file -> NxtClus, 0);	
		}
		else {
			buf -> type = T_FILE;
			buf -> size = fd[fd_num].file -> size;
		}
		printf("%d %d %d\n", buf -> id, buf -> type, buf -> size);
	}
	kmt -> spin_unlock(&trap_lock);
	return result;
}

static int vfs_link(const char *oldpath, const char *newpath) {
   	kmt -> spin_lock(&trap_lock);
	int id = cpu_current(), result = -1;

	uint32_t now = (oldpath[0] == '/') ? 0x200000 : current_dir[id];
	int status = (now == 0x200000) ? 1 : mode[id];

	struct file* old = pmm -> alloc(sizeof(struct file));
	uint32_t nxt = solve_path(now, oldpath + (oldpath[0] == '/'), &status, old, 0);
			printf("%d %x\n", old -> size, old -> bias);
	
	if (nxt <= 0) result = -1;
	else {
		now = (newpath[0] == '/') ? 0x200000 : current_dir[id];
		struct file* new = pmm -> alloc(sizeof(struct file));		
		uint32_t nxt = solve_path(now, newpath + (newpath[0] == '/'), &status, new, 1);
		if (nxt != 0) result = -1;
		else {
			result = 0;	
			clus = clus - 1;
			printf("%x %d %x\n", new -> bias, old -> size, old -> bias);
			old -> bias = new -> bias;
			sda -> ops -> write(sda, new -> bias, old, sizeof(struct file));
		}	
		pmm -> free(new);
	}
	pmm -> free(old);
	kmt -> spin_unlock(&trap_lock);
	return result;
}

MODULE_DEF(vfs) = {
	.init   = vfs_init,	
	.chdir  = vfs_chdir,
	.open   = vfs_open,
	.close  = vfs_close,
	.mkdir  = vfs_mkdir,
	.fstat  = vfs_fstat,
	.link   = vfs_link,
};
