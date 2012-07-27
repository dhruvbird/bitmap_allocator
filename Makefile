CXXFLAGS := $(FLAGS) -Wall -O2 -I . -I /usr/include/i386-linux-gnu/ -std=c++0x test.cpp 
ifndef CXX
	CXX := g++
endif

all: ba_alloc ba_list eba_alloc eba_list sa_alloc sa_list

ba_alloc: bitmap_allocator.h test.cpp
	$(CXX) $(CXXFLAGS) -DBALLOC -DTEST_ALLOC -o ba_alloc

ba_list: bitmap_allocator.h test.cpp
	$(CXX) $(CXXFLAGS) -DBALLOC -DTEST_LIST -o ba_list

eba_alloc: bitmap_allocator.h test.cpp
	$(CXX) $(CXXFLAGS) -DEXT_BALLOC -DTEST_ALLOC -o eba_alloc

eba_list: bitmap_allocator.h test.cpp
	$(CXX) $(CXXFLAGS) -DEXT_BALLOC -DTEST_LIST -o eba_list

sa_alloc: bitmap_allocator.h test.cpp
	$(CXX) $(CXXFLAGS) -DSALLOC -DTEST_ALLOC -o sa_alloc

sa_list: bitmap_allocator.h test.cpp
	$(CXX) $(CXXFLAGS) -DSALLOC -DTEST_LIST -o sa_list

test: all
	time ./ba_alloc
	time ./ba_list
	time ./eba_alloc
	time ./eba_list
	time ./sa_alloc
	time ./sa_list

clean:
	rm -f ./ba_alloc ./ba_list ./eba_alloc ./eba_list ./sa_alloc ./sa_list
