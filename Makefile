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
	/usr/bin/time -v ./ba_alloc
	/usr/bin/time -v ./ba_list
	/usr/bin/time -v ./eba_alloc
	/usr/bin/time -v ./eba_list
	/usr/bin/time -v ./sa_alloc
	/usr/bin/time -v ./sa_list

clean:
	rm -f ./ba_alloc ./ba_list ./eba_alloc ./eba_list ./sa_alloc ./sa_list
