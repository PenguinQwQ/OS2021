#include <kernel.h>
#include <klib.h>

int main() {
  ioe_init();
  cte_init(os->trap);
  os->init();
  printf("666\n");
  mpe_init(os->run);
  return 1;
}
