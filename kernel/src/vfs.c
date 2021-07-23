#include <common.h>
#include <devices.h>
#define MAX_CPU 8

spinlock_t vfs_lock;

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

struct current_node{
	uint32_t inode;
	uint32_t clus;
	uint32_t type;
	uint32_t count;
	uint32_t size;
	char name[32];
};

struct current_node NowNode[MAX_CPU];
device_t *sda;
uint32_t *fat;
bool used[1000];
struct current_node p_fd[1000];

uint32_t GetClusLoc(uint32_t clus) {
	assert(clus >= 1);
	return 0x200000 + (clus - 1) * 512 * 8;	
}
static void vfs_init() {
	kmt -> spin_init(&vfs_lock, "vfs_lock");
	for (int i = 0; i < MAX_CPU; i++) {
		NowNode[i].clus  = 1;
		NowNode[i].inode = 0;	
	}
	sda = dev -> lookup("sda");
	fat = (uint32_t *)pmm -> alloc(0x100000);
	sda -> ops -> read(sda, 0x100000, fat, 0x100000);
	assert(fat != NULL);
	used[0] = used[1] = used[2] = true;
	for (int i = 3; i < 1000; i++) used[i] = false;
}

struct current_node find_dir (struct current_node now, const char *path, int p, int len) {
	while (p < len && path[p] == '/') p++;
	if (p == len) return now;
	char name[64];
	int tot = 0;
	for (int i = p; i < len && path[i] != '/'; i++)
		name[tot++] = path[i], name[tot] = 0;
	
	while (1) {
		int loc = now.clus, flag = 0;
		if (loc == 0) break;
		void *tep = pmm -> alloc(4096);
		assert(tep != NULL);

		sda -> ops -> read(sda, GetClusLoc(loc), tep, 4096);
		struct file *fl = (struct file *)tep;
		for (int i = 0; i < 64; i++) {
			if (fl -> name[0] == 0) {
				flag = 1;
				break;	
			}
			else if (strcmp(fl -> name, name) == 0) {
				struct current_node nxt;
				nxt.inode = fl -> inode;
				nxt.clus = fl -> NxtClus;
				nxt.type = fl -> type;
				nxt.size = fl -> size;
				nxt.count = fl -> count;
				strcpy(nxt.name, fl -> name);
				pmm -> free(tep);
				return find_dir(nxt, path, p, len);
			}
			fl = fl + 1;
		}
		pmm -> free(tep);
		if (flag == 0) now.clus = fat[now.clus];
		else break;
	}
	now.clus = -1;
	return now;	
}


static int vfs_chdir(const char *path) {
	kmt -> spin_lock(&vfs_lock);
	int id = cpu_current();
	struct current_node now;
	if (path[0]== '/') now.inode = 0, now.clus = 1;
	else now.inode = NowNode[id].inode, now.clus = NowNode[id].clus;
	struct current_node tep = find_dir(now, path, 0, strlen(path));
	int flag = 0;
	if (tep.clus == -1) flag = -1;
	else NowNode[id].clus = tep.clus, NowNode[id].inode = tep.inode;
	kmt -> spin_unlock(&vfs_lock);
	return flag;
}

static void fd_copy(struct current_node* a, struct current_node* b) {
	memcpy(a, b, sizeof(struct current_node));	
}

static int vfs_open(const char *pathname, int flags) {
	kmt -> spin_lock(&vfs_lock);
	int id = cpu_current();
	struct current_node now;
	if (pathname[0]== '/') now.inode = 0, now.clus = 1;
	else now.inode = NowNode[id].inode, now.clus = NowNode[id].clus;
	struct current_node tep = find_dir(now, pathname, 0, strlen(pathname));
	int fd = -1;
	if (tep.clus != -1) {
		for (int i = 3; i < 1000; i++) 
			if (!used[i]) {
				fd = i; break;	
			}
		if (fd != -1) {
			used[fd] = true;
			fd_copy(&p_fd[fd], &tep);
		}
	}
	kmt -> spin_unlock(&vfs_lock);	
	return fd;
}

MODULE_DEF(vfs) = {
	.init  = vfs_init,	
	.chdir = vfs_chdir,	
	.open  = vfs_open,
};
