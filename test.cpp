#include <iostream>
#include <stdio.h>
#include "bitmap_allocator.h"

using namespace std;
using namespace __gnu_cxx;

int
main() {
  bitmap_allocator<int> ia;
  for (int i = 0; i < 240; ++i) {
    ia.allocate(1);
  }
  fprintf(stderr, "Allocated 240 objects\n");
  ia.allocate(1);
}
