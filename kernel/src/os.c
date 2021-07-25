#include <common.h>
#include <devices.h>

#define MAX_CPU 128

spinlock_t trap_lock;
extern uint32_t current_dir[8];

void func(void *args) {
	int ti = 0;
	while(1) {
		int i = ienabled();
		iset(false);
		printf("Hello from CPU#%d for %d times with arg %s!\n", cpu_current(), ti++, args);	  
		if (i)iset(true);
	}
}


int Lists_sum = 0;

sem_t empty, fill;

void producer() {
	while(1){kmt->sem_wait(&empty); putch('('); kmt->sem_signal(&fill);}
}

void comsumer() {
	while(1){kmt->sem_wait(&fill); putch(')');  kmt->sem_signal(&empty);}
}
int T = 0;
/*
static void tty_reader(void *arg) {
	  device_t *tty = dev->lookup(arg);
	  char cmd[128], resp[128], ps[16];
	  sprintf(ps, "(%s) $ ", arg);
	   while (1) {
		    tty->ops->write(tty, 0, ps, strlen(ps));
			int nread = tty->ops->read(tty, 0, cmd, sizeof(cmd) - 1);
		    cmd[nread - 1] = '\0';
			if (cmd[0] == '0')printf("%d\n", vfs -> open(cmd + 2, O_WRONLY | O_CREAT));
			else if (cmd[0] == '1')printf("%d\n", vfs -> chdir(cmd + 2));
			else if (cmd[0] == '2')printf("%d\n", vfs -> close(atoi(cmd + 2)));
			else if (cmd[0] == '3')printf("%d\n", vfs -> mkdir(cmd + 2));
			else if (cmd[0] == '4')printf("%d\n", vfs -> fstat(atoi(cmd + 2), pmm -> alloc(32)));
			else if (cmd[0] == '5') {
				cmd[3] = 0;
				printf("%d\n", vfs -> link(cmd + 4, cmd + 2));
			}
			else if (cmd[0] == '6')printf("%d\n", vfs -> unlink(cmd + 2));
			else if (cmd[0] == '7') {
				cmd[3] = 0;
				struct ufs_dirent *now = pmm -> alloc(4096);
				int sz = vfs -> read(atoi(cmd + 2), now, 2);
				for (int off = 0; off + sizeof(struct ufs_dirent) <= sz; off += sizeof(struct ufs_dirent)) {
					printf("%d %s\n", now -> inode, now -> name);
					now = now + 1;	
				}
				pmm -> free(now);
			}
			else if (cmd[0] == '8') {
				cmd[3] = 0;	
				char *now = pmm -> alloc(4096 * 2);
				now[0] = 's', now[1] = 't'; now[4096] = 'p';
				vfs -> write(atoi(cmd + 2), now, 4096 * 2);
				pmm -> free(now);
			}
		    tty->ops->write(tty, 0, resp, strlen(resp));
	  }
}
*/
static void traverse(const char *root) {
  int sz = 4096;
  #define MAX_PATH_LEN 4096
  char *buf = pmm->alloc(sz); // asserts success
  struct ufs_stat s;

  int fd = vfs->open(strcmp(root, "") == 0 ? "/" : root, O_RDONLY), nread;
  if (fd < 0) goto release;

  vfs->fstat(fd, &s);
  if (s.type == T_DIR) {
    while ( (nread = vfs->read(fd, buf, sz)) > 0) {
      for (int offset = 0;
           offset +  sizeof(struct ufs_dirent) <= nread;
           offset += sizeof(struct ufs_dirent)) {
        struct ufs_dirent *d = (struct ufs_dirent *)(buf + offset);
        if (d->name[0] != '.') { // 小彩蛋：你这下知道为什么
                                 // Linux 以 “.” 开头的文件是隐藏文件了吧
          char *fname = pmm->alloc(MAX_PATH_LEN); // assert success
          sprintf(fname, "%s/%s", root, d->name);
          traverse(fname);
          pmm->free(fname);
        }
      }
    }
  }

release:
  if (fd >= 0) vfs->close(fd);
  pmm->free(buf);
}

void test() {
	vfs->mkdir("/etw1ld63cgzmq");
vfs->chdir("etw1ld63cgzmq");
vfs->mkdir("/etw1ld63cgzmq/d9afza5vpuemop");
vfs->chdir("d9afza5vpuemop");
{
  int fd = vfs->open("/etw1ld63cgzmq/d9afza5vpuemop/b3.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/etw1ld63cgzmq/d9afza5vpuemop/a-3.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/etw1ld63cgzmq/d9afza5vpuemop/a2.txt", O_CREAT | O_RDWR);
  if (fd > 0) vfs->close(fd);
}
vfs->chdir("..");
vfs->mkdir("/etw1ld63cgzmq/jcf8z7r7pn0r2");
vfs->chdir("jcf8z7r7pn0r2");
{
  int fd = vfs->open("/etw1ld63cgzmq/jcf8z7r7pn0r2/b1.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/etw1ld63cgzmq/jcf8z7r7pn0r2/a3.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/etw1ld63cgzmq/jcf8z7r7pn0r2/a2.txt", O_CREAT | O_RDWR);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/etw1ld63cgzmq/jcf8z7r7pn0r2/a-1.txt", O_CREAT | O_RDWR);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/etw1ld63cgzmq/jcf8z7r7pn0r2/a-3.txt", O_CREAT | O_RDWR);
  if (fd > 0) vfs->close(fd);
}
vfs->chdir("..");
vfs->mkdir("1dgjoc8ebhmhzf");
vfs->chdir("/etw1ld63cgzmq/1dgjoc8ebhmhzf");
{
  int fd = vfs->open("/etw1ld63cgzmq/1dgjoc8ebhmhzf/a3.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/etw1ld63cgzmq/1dgjoc8ebhmhzf/a-3.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/etw1ld63cgzmq/1dgjoc8ebhmhzf/a1.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
vfs->chdir("..");
{
  int fd = vfs->open("/etw1ld63cgzmq/a-1.txt", O_CREAT | O_RDWR);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/etw1ld63cgzmq/a-3.txt", O_CREAT | O_RDWR);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/etw1ld63cgzmq/a1.txt", O_CREAT | O_RDWR);
  if (fd > 0) vfs->close(fd);
}
vfs->chdir("..");
vfs->mkdir("h51ctvjkyfefmod");
vfs->chdir("/h51ctvjkyfefmod");
vfs->mkdir("vc8rip4wswit");
vfs->chdir("/h51ctvjkyfefmod/vc8rip4wswit");
{
  int fd = vfs->open("/h51ctvjkyfefmod/vc8rip4wswit/c-1.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/h51ctvjkyfefmod/vc8rip4wswit/c3.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/h51ctvjkyfefmod/vc8rip4wswit/b3.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/h51ctvjkyfefmod/vc8rip4wswit/c-2.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
vfs->chdir("..");
vfs->mkdir("/h51ctvjkyfefmod/2759ac5ut3d0m");
vfs->chdir("2759ac5ut3d0m");
{
  int fd = vfs->open("/h51ctvjkyfefmod/2759ac5ut3d0m/c-3.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/h51ctvjkyfefmod/2759ac5ut3d0m/a2.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/h51ctvjkyfefmod/2759ac5ut3d0m/b3.txt", O_CREAT | O_RDWR);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/h51ctvjkyfefmod/2759ac5ut3d0m/b-2.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
vfs->chdir("..");
vfs->mkdir("/h51ctvjkyfefmod/mtrlg4zfbr2h");
vfs->chdir("mtrlg4zfbr2h");
{
  int fd = vfs->open("/h51ctvjkyfefmod/mtrlg4zfbr2h/b1.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/h51ctvjkyfefmod/mtrlg4zfbr2h/a-1.txt", O_CREAT | O_RDWR);
  if (fd > 0) vfs->close(fd);
}
vfs->chdir("..");
vfs->mkdir("/h51ctvjkyfefmod/xc531x8lnysaij");
vfs->chdir("xc531x8lnysaij");
{
  int fd = vfs->open("/h51ctvjkyfefmod/xc531x8lnysaij/a1.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/h51ctvjkyfefmod/xc531x8lnysaij/c3.txt", O_CREAT | O_RDWR);
  if (fd > 0) vfs->close(fd);
}
vfs->chdir("..");
vfs->mkdir("/h51ctvjkyfefmod/4pdtl7etzvt0");
vfs->chdir("4pdtl7etzvt0");
{
  int fd = vfs->open("/h51ctvjkyfefmod/4pdtl7etzvt0/a3.txt", O_CREAT | O_RDWR);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/h51ctvjkyfefmod/4pdtl7etzvt0/a-3.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/h51ctvjkyfefmod/4pdtl7etzvt0/c-2.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
vfs->chdir("..");
{
  int fd = vfs->open("/h51ctvjkyfefmod/b1.txt", O_CREAT | O_RDWR);
  if (fd > 0) vfs->close(fd);
}
vfs->chdir("..");
vfs->mkdir("/hsiysh7mcz2xm3");
vfs->chdir("hsiysh7mcz2xm3");
vfs->mkdir("cnjgm3yx8jg5j");
vfs->chdir("/hsiysh7mcz2xm3/cnjgm3yx8jg5j");
{
  int fd = vfs->open("/hsiysh7mcz2xm3/cnjgm3yx8jg5j/b-2.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/hsiysh7mcz2xm3/cnjgm3yx8jg5j/a-3.txt", O_CREAT | O_RDWR);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/hsiysh7mcz2xm3/cnjgm3yx8jg5j/b2.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
vfs->chdir("..");
vfs->mkdir("/hsiysh7mcz2xm3/ef7ceoicta2vkj3");
vfs->chdir("ef7ceoicta2vkj3");
{
  int fd = vfs->open("/hsiysh7mcz2xm3/ef7ceoicta2vkj3/b2.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/hsiysh7mcz2xm3/ef7ceoicta2vkj3/c-1.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/hsiysh7mcz2xm3/ef7ceoicta2vkj3/a-2.txt", O_CREAT | O_RDWR);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/hsiysh7mcz2xm3/ef7ceoicta2vkj3/c3.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
vfs->chdir("..");
vfs->mkdir("dkipsvdc40j5fjw");
vfs->chdir("/hsiysh7mcz2xm3/dkipsvdc40j5fjw");
{
  int fd = vfs->open("/hsiysh7mcz2xm3/dkipsvdc40j5fjw/a-3.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/hsiysh7mcz2xm3/dkipsvdc40j5fjw/c2.txt", O_CREAT | O_RDWR);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/hsiysh7mcz2xm3/dkipsvdc40j5fjw/c-3.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/hsiysh7mcz2xm3/dkipsvdc40j5fjw/b2.txt", O_CREAT | O_RDWR);
  if (fd > 0) vfs->close(fd);
}
vfs->chdir("..");
{
  int fd = vfs->open("/hsiysh7mcz2xm3/b-1.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/hsiysh7mcz2xm3/a-3.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/hsiysh7mcz2xm3/c1.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/hsiysh7mcz2xm3/a3.txt", O_CREAT | O_RDWR);
  if (fd > 0) vfs->close(fd);
}
vfs->chdir("..");
vfs->mkdir("/jw3f4nsa23antht8");
vfs->chdir("jw3f4nsa23antht8");
vfs->mkdir("6706thj11fg0");
vfs->chdir("/jw3f4nsa23antht8/6706thj11fg0");
{
  int fd = vfs->open("/jw3f4nsa23antht8/6706thj11fg0/a-1.txt", O_CREAT | O_RDWR);
  if (fd > 0) vfs->close(fd);
}
vfs->chdir("..");
vfs->mkdir("b5uqfwehwbwwlao");
vfs->chdir("/jw3f4nsa23antht8/b5uqfwehwbwwlao");
{
  int fd = vfs->open("/jw3f4nsa23antht8/b5uqfwehwbwwlao/a2.txt", O_CREAT | O_RDWR);
  if (fd > 0) vfs->close(fd);
}
vfs->chdir("..");
vfs->mkdir("/jw3f4nsa23antht8/asxbojl3h4wq");
vfs->chdir("asxbojl3h4wq");
{
  int fd = vfs->open("/jw3f4nsa23antht8/asxbojl3h4wq/a-3.txt", O_CREAT | O_RDWR);
  if (fd > 0) vfs->close(fd);
}
vfs->chdir("..");
vfs->mkdir("ulfg8tkyjiou6ppl");
vfs->chdir("/jw3f4nsa23antht8/ulfg8tkyjiou6ppl");
{
  int fd = vfs->open("/jw3f4nsa23antht8/ulfg8tkyjiou6ppl/b2.txt", O_CREAT | O_RDWR);
  if (fd > 0) vfs->close(fd);
}
vfs->chdir("..");
vfs->mkdir("/jw3f4nsa23antht8/fp2x7dy0a0u2nx");
vfs->chdir("fp2x7dy0a0u2nx");
{
  int fd = vfs->open("/jw3f4nsa23antht8/fp2x7dy0a0u2nx/c-2.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/jw3f4nsa23antht8/fp2x7dy0a0u2nx/b3.txt", O_CREAT | O_RDWR);
  if (fd > 0) vfs->close(fd);
}
vfs->chdir("..");
{
  int fd = vfs->open("/jw3f4nsa23antht8/a1.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/jw3f4nsa23antht8/b1.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/jw3f4nsa23antht8/a2.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
vfs->chdir("..");
{
  int fd = vfs->open("/b-3.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/b3.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/c-1.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
{
  int fd = vfs->open("/c2.txt", O_CREAT | O_WRONLY);
  if (fd > 0) vfs->close(fd);
}
traverse("");
}

static void os_init() {
  T++;
  Lists_sum = 0;
  pmm->init();
  kmt->init();
  dev -> init();
  vfs->init();
  kmt->spin_init(&trap_lock, "os_trap"); 
  kmt->create(pmm -> alloc(sizeof(task_t)), "tty_reader", test, "tty1");
 // kmt->create(pmm -> alloc(sizeof(task_t)), "tty_reader", tty_reader, "tty1");
 // kmt->create(pmm -> alloc(sizeof(task_t)), "tty_reader", tty_reader, "tty2");
  
/*  kmt -> sem_init(&empty, "empty", 10);
  kmt -> sem_init(&fill,  "fill" , 0);
 for (int i = 0; i < 4; i++) 
	  kmt->create(pmm->alloc(sizeof(task_t)), "producer", producer, NULL);
	
 for (int i = 0; i < 5; i++) 
	  kmt->create(pmm->alloc(sizeof(task_t)), "consumer", comsumer, NULL);
  */ 
}

static void os_run() {
  iset(true);
  while(1);
}

extern task_t *task_head;
extern task_t *current[MAX_CPU];
task_t origin[MAX_CPU];
#define N 65536
task_t *valid[N], *lst[N];
int tot = 0;

static Context* os_trap(Event ev, Context *context) {
	assert(ienabled() == false);
	assert(ev.event != EVENT_ERROR);
	kmt -> spin_lock(&trap_lock);
	int id = cpu_current();
	if (current[id] != NULL) {
		current[id] -> ctx = context;
		current[id] -> inode = current_dir[id];
		assert(current[id] -> on == true);
	}
	else {
		current[id] = &origin[id];
		origin[id].ctx = context;
		current[id] -> status = RUNNING;
		current[id] -> on = true;
	}
	if (lst[id] != NULL) lst[id] -> sleep_flag = false;
	lst[id] = current[id];
	lst[id] -> sleep_flag = true;
	panic_on(current[id] == NULL, "null current");
	panic_on(current[id] -> on == false, "may be crazy");

	for (int i = 0; i < Lists_sum; i++) 
		if (ev.event == Lists[i].event || Lists[i].event == EVENT_NULL){
			Lists[i].func(ev, context);
	}
	if (current[id] -> status != BLOCKED) current[id] -> status = SUITABLE;
	current[id] -> on = false;
	
	task_t *now = task_head;
	tot = 0;
	while (now != NULL)	{
		if (now ->sleep_flag == true) assert(now -> on == false);
		if (now -> status == SUITABLE && now -> on == false) {
			if (now -> sleep_flag == true && now != current[id]) {
				now = now -> next;
				continue;	
			}
			valid[tot++] = now;
		}
		now = now -> next;
	}

	if (tot == 0) {
		current[id] = &origin[cpu_current()];
		current[id] -> status = RUNNING;
		current[id] -> on = true;
		current_dir[id] = current[id] -> inode;
		kmt -> spin_unlock(&trap_lock);
		return current[id] -> ctx;
	}

	int nxt = rand() % tot;
	current[id] = valid[nxt];
	assert(current[id] != NULL);
	current[id] -> status = RUNNING;
	current[id] -> sleep_flag = false;
	current[id] -> on = true;
	assert(current[id] -> status == RUNNING);
	current_dir[id] = current[id] -> inode;
	kmt -> spin_unlock(&trap_lock);
	return current[id] -> ctx;	
}

static void os_on_irq(int seq, int event, handler_t handler) {
	assert(Lists_sum < 65536);
	Lists[Lists_sum].func  = handler;
	Lists[Lists_sum].seq   = seq;
	Lists[Lists_sum].event = event;
	Lists_sum              = Lists_sum + 1;
	// bubble sort
	for (int j = 0; j < Lists_sum - 1; j++)
			for (int i = 0; i < Lists_sum - 1 - j; i++)
				if (Lists[i].seq > Lists[i + 1].seq) {
					int tep = Lists[i].seq;
					Lists[i].seq = Lists[i + 1].seq;
					Lists[i + 1].seq	= tep;

					tep = Lists[i].event;
					Lists[i].event = Lists[i + 1].event;
					Lists[i + 1].event = tep;

					handler_t _tep = Lists[i].func;
					Lists[i].func = Lists[i + 1].func;
					Lists[i + 1].func = _tep;
				}
}

MODULE_DEF(os) = {
  .init    = os_init,
  .run     = os_run,
  .trap    = os_trap,
  .on_irq  = os_on_irq,
};

