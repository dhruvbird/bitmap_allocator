#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include <vector>
#include <memory>
#include <list>
#define NDEBUG

#include "bitmap_allocator.h"

using namespace std;
using namespace __gnu_cxx;

std::vector<int*> vpi;
std::vector<int> numbers;

void test_list(int n) {
    // typedef bitmap_allocator<int> alloc_t;
    typedef std::allocator<int> alloc_t;
    typedef std::list<int, alloc_t> list_t;
    list_t il;

    for (int i = 0; i < n; ++i) {
        il.push_back(numbers[i]);
    }
    fprintf(stderr, "Allocated %d objects\n", n);
    il.sort();
}

void test_alloc(int n) {
    // bitmap_allocator<int> ia;
    std::allocator<int> ia;

    for (int i = 0; i < n-1; ++i) {
        vpi.push_back(ia.allocate(1));
    }
    fprintf(stderr, "Allocated %d objects\n", n-1);
    vpi.push_back(ia.allocate(1));

    for (int i = 0; i < n; ++i) {
        ia.deallocate(vpi[i], 1);
    }
    vpi.clear();
}

int
main() {
    vpi.reserve(1<<23);
    numbers.reserve(1<<23);

    for (int i = 0; i < (1<<23); ++i) {
        numbers.push_back(rand());
    }

    for (int i = 0; i < 24; ++i) {
        // test_alloc(1<<i);
        test_list(1<<i);
    }
    // sleep(50);
}
