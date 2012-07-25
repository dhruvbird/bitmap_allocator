CXXFLAGS := $(FLAGS) -Wall -O2 -I . test.cpp

all: ba_alloc ba_list eba_alloc eba_list sa_alloc sa_list bitmap_allocator.h test.cpp

ba_alloc:
	g++ $(CXXFLAGS) -DBALLOC -DTEST_ALLOC -o ba_alloc

ba_list:
	g++ $(CXXFLAGS) -DBALLOC -DTEST_LIST -o ba_list

eba_alloc:
	g++ $(CXXFLAGS) -DEXT_BALLOC -DTEST_ALLOC -o eba_alloc

eba_list:
	g++ $(CXXFLAGS) -DEXT_BALLOC -DTEST_LIST -o eba_list

sa_alloc:
	g++ $(CXXFLAGS) -DSALLOC -DTEST_ALLOC -o sa_alloc

sa_list:
	g++ $(CXXFLAGS) -DSALLOC -DTEST_LIST -o sa_list

test: all
	./ba_alloc
	./ba_list
	./eba_alloc
	./eba_list
	./sa_alloc
	./sa_list

clean:
	rm -f ./ba_alloc ./ba_list ./eba_alloc ./eba_list ./sa_alloc ./sa_list
