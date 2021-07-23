#include <kernel.h>
#include <klib.h>

int main() {
  ioe_init();
  cte_init(os->trap);
  iset(false);
  os->init();
  mpe_init(os->run);
  return 1;
}
