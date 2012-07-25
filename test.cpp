// -*- mode:c++; c-basic-offset:4 -*-
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <vector>
#include <memory>
#include <list>

#if defined BALLOC
#include "bitmap_allocator.h"
#elif defined EXT_BALLOC
#include <ext/bitmap_allocator.h>
#endif

using namespace std;
using namespace __gnu_cxx;

std::vector<int*> vpi;
std::vector<int> numbers;

void test_list(int n) {
#if defined BALLOC || defined EXT_BALLOC
    typedef bitmap_allocator<int> alloc_t;
#else
    typedef std::allocator<int> alloc_t;
#endif
    typedef std::list<int, alloc_t> list_t;
    list_t il;

    for (int i = 0; i < n; ++i) {
        il.push_back(numbers[i]);
    }
    fprintf(stderr, "Allocated %d objects\n", n);
    il.sort();
}

void test_alloc(int n) {
#if defined BALLOC || defined EXT_BALLOC
    bitmap_allocator<int> ia;
#else
    std::allocator<int> ia;
#endif

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
main(int argc, char *argv[]) {
    vpi.reserve(1<<23);
    numbers.reserve(1<<23);

    for (int i = 0; i < (1<<23); ++i) {
        numbers.push_back(rand());
    }

    for (int i = 0; i < 24; ++i) {
#if defined TEST_ALLOC
	test_alloc(1<<i);
#else
	test_list(1<<i);
#endif
    }
    std::string command = "ps aux | grep ";
    command += argv[0];
    int ret = system(command.c_str());
    (void)ret;
}
