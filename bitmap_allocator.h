// -*- mode:C++;c-basic-offset:4 -*-
#if !defined BITMAP_ALLOCATOR_H
#define BITMAP_ALLOCATOR_H

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <new>

#if !defined NDEBUG
#define assert_lt(X,Y) if (!((X)<(Y))) { fprintf(stderr, "%d < %d FAILED\n", (X), (Y)); assert((X)<(Y)); }
#define assert_gt(X,Y) if (!((X)>(Y))) { fprintf(stderr, "%d > %d FAILED\n", (X), (Y)); assert((X)>(Y)); }
#define assert_le(X,Y) if (!((X)<=(Y))) { fprintf(stderr, "%d <= %d FAILED\n", (X), (Y)); assert((X)<=(Y)); }
#define assert_eq(X,Y) if (!((X)==(Y))) { fprintf(stderr, "%d == %d FAILED\n", (X), (Y)); assert((X)==(Y)); }
#define assert_ne(X,Y) if (!((X)!=(Y))) { fprintf(stderr, "%d != %d FAILED\n", (X), (Y)); assert((X)!=(Y)); }

#else
#define assert_lt(X,Y)
#define assert_gt(X,Y)
#define assert_eq(X,Y)
#endif

// #define DPRINTF(ARGS...) fprintf(stderr, ARGS);
#define DPRINTF(ARGS...)


namespace __gnu_cxx {
    namespace {
        bool get_bit_at(size_t *ptr, size_t index) {
            const size_t big_offset = index / (8 * sizeof(size_t));
            const size_t lil_offset = index % (8 * sizeof(size_t));
	    // DPRINTF("index: %u, big_offset: %u, lil_offset: %u\n",
	    // index, big_offset, lil_offset);
            const size_t *mem = ptr + big_offset;
            const size_t one = 1L;
            return *mem & (one << lil_offset);
        }

        void set_bit_at(size_t *ptr, size_t index, bool value) {
	    DPRINTF("set_bit_at(%u) = %d\n", index, value);
            const size_t big_offset = index / (8 * sizeof(size_t));
            const size_t lil_offset = index % (8 * sizeof(size_t));
            size_t *mem = ptr + big_offset;
            const size_t one = 1L;
            if (value) {
                *mem |= (one << lil_offset);
            } else {
                *mem &= (~(one << lil_offset));
            }
        }

	enum { TOP_MAGIC = 482781718, BOT_MAGIC = 213827793 };

	/* A memory_chunk<SIZE> is a chunk of memory for objects of
	 * size 'SIZE'. The number of such objects that can be stored
	 * in this memory_chunk<SIZE> is specified in the constructor.
	 *
	 */
	template <size_t SIZE>
        struct memory_chunk {
            // _M_size stores the number of memory objects of size
            // SIZE that this memory_chunk stores
            size_t _M_size;
            size_t _M_top_magic;
            size_t _M_idx;
            size_t _M_offset;
            size_t _M_range;

	    // For the Segment Tree, a reset(0) bit means that the
	    // block is free whereas a set(1) bit means that the block
	    // is allocated.
            size_t _M_seg_tree[0];

	    /* Allocates a single block of size SIZE. Returns
	     * NULL if no free block exists in the current
	     * memory_chunk.
	     *
	     */
            char *allocate_block(/* hint */) {
		// DPRINTF("memory_chunk<%u>::allocate_block()\n", SIZE);
		DPRINTF("_M_size: %u\n", _M_size);

                if (_M_idx == 0) {
                    assert_eq(_M_offset, 0);
                    assert_eq(_M_range, _M_size);
                }

                size_t *pseg = reinterpret_cast<size_t*>(seg_tree());
                assert_lt(_M_idx, _M_size);
		DPRINTF("get_bit_at(%u) = %d\n", _M_idx, get_bit_at(pseg, _M_idx));

                if (_M_idx == 0 && get_bit_at(pseg, _M_idx) != 0) {
                    // There is nothing here.
                    return NULL;
                } else {
		    // Even if _M_idx == 0 and the bit at that
		    // position is == 0 (i.e. the previous condition
		    // failing), the assertion below _must_ succeed.
                    assert(get_bit_at(pseg, _M_idx) == 0);
                }

                while (_M_idx < _M_size * 2 - 1) {
                    DPRINTF("foo1\n");

                    // We have something free under this node.
		    size_t left  = _M_idx * 2 + 1;
                    size_t right = _M_idx * 2 + 2;
                    _M_range /= 2;
		    DPRINTF("pseg: %u, idx: %u, left: %u, right: %u\n", *pseg, _M_idx, left, right);

                    if (left < _M_size * 2 - 1 && get_bit_at(pseg, left) == 0) {
                        _M_idx = left;
                    } else if (right < _M_size * 2 - 1 && get_bit_at(pseg, right) == 0) {
                        _M_idx = right;
                        _M_offset += _M_range;
                    } else {
                        // Must be the end of the array.

			// This should never happen since the last
			// allocation for memory should set all the
			// bit all the way up to the root.
			assert_gt(_M_idx, _M_size - 2);
                        assert_eq(_M_range, 0);
                        _M_range = 1;

                        // Set the bit at position 'idx'.
                        set_bit_at(pseg, _M_idx, 1);
                        const size_t offset = _M_offset;

                        while (_M_idx != 0) {
                            DPRINTF("foo2\n");

                            const size_t parent = (_M_idx - 1) / 2;
                            left  = parent * 2 + 1;
                            right = parent * 2 + 2;

                            if (_M_idx == left) {
                                // FIXME: Update _M_offset & _M_range
                                _M_idx = parent;
                                _M_range *= 2;

                                if (get_bit_at(pseg, right) == 0) {
                                    // Stop setting bits.
                                    break;
                                }

                            } else {
                                // FIXME: Update _M_offset & _M_range
                                _M_idx = parent;
                                _M_offset -= _M_range;
                                _M_range *= 2;

                                if (get_bit_at(pseg, left) == 0) {
                                    // Stop setting bits.
                                    break;
                                }
                            }
                            // Set bit at 'parent' to indicate that
                            // everything under the tree rooted at
                            // 'parent' is completely allocated, with
                            // no free blocks.
                            set_bit_at(pseg, parent, 1);
                        }
                        return mem() + SIZE * offset;
                    }
                }
		assert(false);
                return NULL;
            }

	    /* Deallocates an object allocated via this
	     * memory_chunk<SIZE>.
	     *
	     */
            void deallocate_block(char *ptr) {
		assert(this->has_this_block(ptr));
                assert((ptr - mem()) % SIZE == 0);

                size_t idx = (ptr - mem()) / SIZE;
                size_t *pseg = reinterpret_cast<size_t*>(seg_tree());

		assert_lt(idx, _M_size);
                idx += (_M_size - 1);
                set_bit_at(pseg, idx, 0);

                // Since we freed a child, all parent nodes above this
                // node on the path from the root to this node should
                // be reset to 0 to indicate that there is a free node
                // below this node.
                while (idx != 0) {
                    DPRINTF("foo3\n");

                    const size_t parent = (idx-1) / 2;
		    // We prematurely stop if any of the bits we want
		    // to reset is already reset(0). This is because
		    // all bits from the root to this node will
		    // already be reset, and hence we need not bother
		    // reseting them again.
                    if (get_bit_at(pseg, parent) == 0) {
                        break;
                    }
                    set_bit_at(pseg, parent, 0);
                    idx = parent;
                }
            }

	    /* Returns true if the memory blocks 'ptr' was allocated
	     * using this memory_chunk<SIZE>.
	     *
	     */
	    bool has_this_block(char *ptr) {
		return ptr >= this->mem() && ptr < this->mem() + this->size() * SIZE;
	    }

            size_t size() const {
                return _M_size;
            }

            memory_chunk<SIZE>& set_size(size_t size) {
                _M_idx = 0;
                _M_offset = 0;
                _M_size = size;
                _M_range = size;
                return *this;
            }

            memory_chunk<SIZE>& set_top_magic(size_t magic) {
                _M_top_magic = magic;
                return *this;
            }

            memory_chunk<SIZE>& set_bot_magic(size_t magic) {
                size_t *bot_magic_ptr = reinterpret_cast<size_t*>(&(this->mem()[SIZE * size()]));
                *bot_magic_ptr = magic;
                return *this;
            }

            size_t top_magic() const {
                return _M_top_magic;
            }

            size_t bot_magic() const {
                const size_t *bot_magic_ptr = reinterpret_cast<size_t*>(&(this->mem()[SIZE * size()]));
                return *bot_magic_ptr;
            }

            char* seg_tree() {
                return reinterpret_cast<char*>(&_M_seg_tree);
            }

            char* mem() {
                return this->seg_tree() + this->size() * 2 / 8;
            }
	};

	template <size_t SIZE>
        struct alloc_impl {
            size_t _M_num_chunks;
            memory_chunk<SIZE>* _M_chunks[42]; // For 4TiB worth of memory

            alloc_impl()
                : _M_num_chunks(0) {
            }

            char* allocate(size_t n) {
		DPRINTF("alloc_impl<%u>::allocate(%u)\n", SIZE, n);
                if (n != 1) {
                    return reinterpret_cast<char*>(operator new(SIZE * n));
                }

                for (size_t i = 0; i < _M_num_chunks; ++i) {
		    DPRINTF("calling _M_chunks[%u]::allocate_block()\n", i);
                    char *mem = _M_chunks[i]->allocate_block();
                    if (mem) {
                        // Set this as the first block so that
                        // subsequent searches find it first.
                        if (i != 0) {
                            std::swap(_M_chunks[i], _M_chunks[0]);
                        }
                        return mem;
                    } else {
			DPRINTF("Block #%u does not have any free memory\n", i);
		    }
                }

                // No free block was found. Allocate a new chunk and
                // restart.
                const size_t one = 1L;
                const size_t num_blocks_in_chunk = (one << (_M_num_chunks)) * (8 * sizeof(size_t) / 2);
		const size_t seg_tree_bytes = (num_blocks_in_chunk * 2) / 8;
                const size_t mem_size = sizeof(memory_chunk<SIZE>) /* For the header */ +
		    seg_tree_bytes /* For the Segment Tree */ +
                    num_blocks_in_chunk * SIZE /* For the actual objects */ +
                    sizeof(size_t) /* For the trailing magic */;

		DPRINTF("num_blocks_in_chunk:  %10u\n"
			"seg_tree_bytes:       %10u\n"
			"mem_size:             %10u\n"
			"user_memory_size:     %10u\n"
			"sizeof(memory_chunk): %10u\n",
			num_blocks_in_chunk, seg_tree_bytes, mem_size,
			num_blocks_in_chunk * SIZE, sizeof(memory_chunk<SIZE>));

                memory_chunk<SIZE> *pchunk = reinterpret_cast<memory_chunk<SIZE>*>(malloc(mem_size));
                if (!pchunk) {
                    return NULL;
                }
                (*pchunk).set_size(num_blocks_in_chunk)
                    .set_top_magic(TOP_MAGIC)
                    .set_bot_magic(BOT_MAGIC);

		// Clear out the segment tree
		memset(pchunk->seg_tree(), 0, seg_tree_bytes * 2);

                _M_chunks[_M_num_chunks] = pchunk;
                std::swap(_M_chunks[0], _M_chunks[_M_num_chunks]);
                ++_M_num_chunks;
                return allocate(n);
            }

            void deallocate(char *ptr, size_t n) {
                if (n != 1) {
                    operator delete(ptr);
                    return;
                }

                for (size_t i = 0; i < _M_num_chunks; ++i) {
                    if (_M_chunks[i]->has_this_block(ptr)) {
                        _M_chunks[i]->deallocate_block(ptr);

			// Move to front.
                        if (i != 0) {
                            std::swap(_M_chunks[0], _M_chunks[i]);
                        }
			return;
                    }
                }

                // You tried to deallocate an object that wasn't
                // allocated via this allocator.
                assert(false);
            }

        };
    }

    template <typename T>
    class bitmap_allocator {
        static alloc_impl<sizeof(T)> impl;

    public:
        typedef size_t     size_type;
        typedef ptrdiff_t  difference_type;
        typedef T*         pointer;
        typedef const T*   const_pointer;
        typedef T&         reference;
        typedef const T&   const_reference;
        typedef T          value_type;

        template<typename T1>
        struct rebind
        { typedef bitmap_allocator<T1> other; };

        void 
        construct(pointer __p, const T& __val) 
        { ::new((void *)__p) T(__val); }

        void 
        destroy(pointer __p) { __p->~T(); }

	bitmap_allocator() throw() { }

	bitmap_allocator(const bitmap_allocator&) throw() { }

        template<typename T1>
        bitmap_allocator(const bitmap_allocator<T1>&) throw() { }

	pointer
	address(reference __x) const { return std::__addressof(__x); }
	
	const_pointer
	address(const_reference __x) const { return std::__addressof(__x); }

	size_type
	max_size() const throw() 
	{ return size_t(-1) / sizeof(T); }

	T* allocate(size_t nobjs, const void* = 0) {
	    T *ptr = reinterpret_cast<T*>(this->impl.allocate(nobjs));
	    if (!ptr) {
		std::__throw_bad_alloc();
	    }
	    return ptr;
	}

	void deallocate(T *ptr, size_t n) {
	    this->impl.deallocate(reinterpret_cast<char*>(ptr), n);
	}
    };

    template <typename T>
    alloc_impl<sizeof(T)> bitmap_allocator<T>::impl;

    template<typename T>
    inline bool
    operator==(const bitmap_allocator<T>&, const bitmap_allocator<T>&)
    { return true; }
  
    template<typename T>
    inline bool
    operator!=(const bitmap_allocator<T>&, const bitmap_allocator<T>&)
    { return false; }

}

#endif // BITMAP_ALLOCATOR_H
