#include <common.h>
#include <devices.h>
#define MAX_CPU 8

spinlock_t vfs_lock;

struct current_node{
	uint32_t inode;
	uint32_t clus;
};

struct current_node NowNode[MAX_CPU];

static void vfs_init() {
	kmt -> spin_init(&vfs_lock, "vfs_lock");
	for (int i = 0; i < MAX_CPU; i++) {
		NowNode[i].clus  = 1;
		NowNode[i].inode = 0;	
	}
	char pos[4096];
	printf("666\n");
	device_t *sda = dev -> lookup("sda");
	while(1);
	if (sda == NULL) {printf("3232\n");return;}
//	sda -> ops -> read(sda, 0x200000, pos, 1);
	for (int i = 0; i < 4096; i++)printf("%c", pos[i]);
}
/*
struct current_node find_dir (struct current_node now, const char *path, int p, int len) {
	while (p < len && path[p] == '/') p++;
	if (p == len) return now;
	char name[64];
	int tot = 0;
	for (int i = p; i < len && path[i] != '/'; i++)
		name[tot++] = path[i], name[tot] = 0;
	return now;	
}
*/

static int vfs_chdir(const char *path) {
//	kmt -> spin_lock(&vfs_lock);
//	int id = cpu_current();
/*	struct current_node now;
	if (path[0]== '/') now.inode = 0, now.clus = 1;
	else now.inode = NowNode[id].inode, now.clus = NowNode[id].clus;
*/
	return 0;	
//	struct current_node tep = find_dir(now, path, 0, strlen(path));
//	kmt -> spin_unlock(&vfs_lock);
}

MODULE_DEF(vfs) = {
	.init  = vfs_init,	
	.chdir = vfs_chdir,	
};
