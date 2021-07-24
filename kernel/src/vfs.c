#include <devices.h>
#include <dirent.h>
#include <vfs.h>
#define MAX_CPU 8

spinlock_t vfs_lock;
device_t *sda;
uint32_t current_dir[MAX_CPU];
uint32_t mode[MAX_CPU];
uint32_t *fat;
struct fd_ fd[1024];

uint32_t GetClusLoc(uint32_t clus) {
	if (clus == 0) return 0;
	return 0x200000 + (clus - 1) * 512 * 8;	
}

uint32_t TurnClus(uint32_t now) {
	return (now - 0x200000) / 512 / 8 + 1;	
}

static void vfs_init() {
	kmt -> spin_init(&vfs_lock, "vfs_lock");
	sda = dev -> lookup("sda");
	fat = (uint32_t *)pmm -> alloc(0x100000);
	sda -> ops -> read(sda, 0x100000, fat, 0x100000);
	for (int i = 0; i < MAX_CPU; i++)
		current_dir[i] = 0x200000, mode[i] = 1;
	assert(fat != NULL);
	fd[0].used = fd[1].used = fd[2].used = 1;
}

uint32_t solve_path(uint32_t now, const char *path, int *status, struct file *file) {
	if (path[0] == 0) return now;
	char *name = pmm -> alloc(256); ///////////////////////////////
	int i = 0;
	while (path[0] == '/') path++;
	while (path[i] != 0 && path[i] != '/') 
		name[i] = path[i], name[i + 1] = 0, i++;
	path = path + i;
	assert(strcmp("proc", name) && strcmp("dev", name)); ///////////////////////
	void *tep = pmm -> alloc(4096); ///////////////////////////	
	while (1) {
		if (now == 0) break;
		sda -> ops -> read(sda, now, tep, 4096);
		struct file *nxt = tep;
		for (int i = 0; i < 64; i++) {
			if (strcmp(name, nxt -> name) == 0) {
				assert(nxt -> flag == 0); ///////////////////////////
				if (nxt -> type == DT_DIR) {
					memcpy(file, nxt, sizeof(struct file));
					return solve_path(GetClusLoc(nxt -> NxtClus), path, status, file);
				}
				else {
					if (path[0] != 0) return -1;
					memcpy(file, nxt, sizeof(struct file));
					return 0;
				}					
			}
			nxt = nxt + 1;
		}
		now = GetClusLoc(fat[TurnClus(now)]);
	}
	return -1;
}

static int vfs_chdir(const char *path) {
	kmt -> spin_lock(&vfs_lock);
	int id = cpu_current();
	uint32_t now = (path[0] == '/') ? 0x200000 : current_dir[id];
	int status = (now == 0x200000) ? 1 : mode[id];
	
	assert(mode[id] == 1); ///////////////////////////////////////////
	struct file tep;
	uint32_t nxt = solve_path(now, path + (path[0] == '/'), &status, &tep);
	int result = 0;
	if (nxt == -1) result = -1;
	else current_dir[id] = nxt;
	printf("%s %x\n", path, nxt);
	kmt -> spin_unlock(&vfs_lock);
	return result;
}

static int vfs_open(const char *path, int flags) {
	kmt -> spin_lock(&vfs_lock);
	int id = cpu_current();
	uint32_t now = (path[0] == '/') ? 0x200000 : current_dir[id];
	int status = (now == 0x200000) ? 1 : mode[id];
	
	assert(mode[id] == 1); ///////////////////////////////////////////

	struct file* tep = pmm -> alloc(sizeof(struct file));
	uint32_t nxt = solve_path(now, path + (path[0] == '/'), &status, tep);
	int result = -1;
	if (nxt == -1) result = -1;
	else {
		for (int i = 0; i < 1024; i++) 
			if (fd[i].used == 0) {
				fd[i].used = 1; 
				fd[i].flag = flags;	
				fd[i].file = tep;
				fd[i].bias = 0;
				printf("%s\n", tep -> name);
				result = i;
				break;
			}
	}
	kmt -> spin_unlock(&vfs_lock);	
	return result;
}

MODULE_DEF(vfs) = {
	.init  = vfs_init,	
	.chdir = vfs_chdir,
	.open  = vfs_open,
};
