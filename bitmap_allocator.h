// -*- mode:C++;c-basic-offset:4;indent-tabs-mode:nil -*-
#if !defined _BITMAP_ALLOCATOR_H
#define _BITMAP_ALLOCATOR_H 1

#include <assert.h>
#include <stdlib.h> // malloc(3)
#include <string.h> // memset(3)
// #include <stdint.h>

#if __cplusplus >= 201103L
#include <type_traits>
#endif
#include <new>      // operator/placement new

#include <ext/concurrence.h>
#include <bits/move.h>

#if !defined NDEBUG
#include <stdio.h>
#define assert_lt(X,Y) if (!((X)<(Y))) { fprintf(stderr, "%d < %d FAILED\n", (X), (Y)); assert((X)<(Y)); }
#define assert_gt(X,Y) if (!((X)>(Y))) { fprintf(stderr, "%d > %d FAILED\n", (X), (Y)); assert((X)>(Y)); }
#define assert_ge(X,Y) if (!((X)>=(Y))) { fprintf(stderr, "%d >= %d FAILED\n", (X), (Y)); assert((X)>=(Y)); }
#define assert_le(X,Y) if (!((X)<=(Y))) { fprintf(stderr, "%d <= %d FAILED\n", (X), (Y)); assert((X)<=(Y)); }
#define assert_eq(X,Y) if (!((X)==(Y))) { fprintf(stderr, "%d == %d FAILED\n", (X), (Y)); assert((X)==(Y)); }
#define assert_ne(X,Y) if (!((X)!=(Y))) { fprintf(stderr, "%d != %d FAILED\n", (X), (Y)); assert((X)!=(Y)); }

#else
#define assert_lt(X,Y)
#define assert_le(X,Y)
#define assert_gt(X,Y)
#define assert_ge(X,Y)
#define assert_eq(X,Y)
#endif

// #define DPRINTF(ARGS...) fprintf(stderr, ARGS);
#define DPRINTF(ARGS...)


namespace __gnu_cxx {
    using std::size_t;
    using std::ptrdiff_t;

    namespace __detail {
        inline bool get_bit_at(size_t *ptr, size_t index) {
            const size_t big_offset = index / (8 * sizeof(size_t));
            const size_t lil_offset = index % (8 * sizeof(size_t));
	    // DPRINTF("index: %u, big_offset: %u, lil_offset: %u\n",
	    // index, big_offset, lil_offset);
            const size_t *mem = ptr + big_offset;
            const size_t one = 1L;
            return *mem & (one << lil_offset);
        }

        inline void set_bit_at(size_t *ptr, size_t index, bool value) {
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

        /** @class  memory_chunk bitmap_allocator.h bitmap_allocator.h
         *
         *  @brief A memory_chunk<SIZE> is a chunk of memory for
	 *  objects of size 'SIZE'. The number of such objects that
	 *  can be stored in this memory_chunk<SIZE> is specified by
	 *  calling the function set_size(), which actually initializes this structure.
         *
         *  @detail Calling other members before calling set_size() is
         *  an error, and might not be caught since this class is
         *  placed into-preallocated memory rather than being
         *  allocated using new.
         */
	template <size_t SIZE>
        struct memory_chunk {
            size_t _M_top_magic;
            // _M_size stores the number of memory objects of size
            // SIZE that this memory_chunk stores.
            size_t _M_size;
            // The index of the cached pointer into the segment tree
            // array (representation). The permissible range for
            // _M_idx is [0 .. _M_size*2 - 2]. When ALL the memory
            // spaces ara available OR when NO memory spaces are
            // available, _M_idx holds the value 0.
            size_t _M_idx;
            // The # of free (available) memory spaces. _M_free ==
            // _M_size to start with.
            size_t _M_free;
            // The offset into the last level of the segment tree
            // tracking the beginning of the first leaf node under the
            // subtree rooted at the node at segment tree index
            // _M_idx.
            size_t _M_offset;
            // The # of leaf nodes in the subtree rooted at the
            // subtree rooted at segment tree index _M_idx. _M_range
            // starts off with the same value as _M_size. If we are at
            // a leaf node, _M_range is 1. If _M_range is 0, we are
            // below a leaf (i.e. no possible node matches).
            size_t _M_range;

	    // For the Segment Tree, a reset(0) bit means that the
	    // block is free whereas a set(1) bit means that the block
	    // is allocated.
            size_t _M_seg_tree[0];

	    /** @brief Allocates a single block of size SIZE. Returns
	     *  NULL if no free block exists in the current
	     *  memory_chunk.
	     *
             *  @param hint Allocate memory near the pointer with
             *  address 'hint'. This is currently unused.
             *
             *  @return NULL if no memory could be allocated or a
             *  pointer pointing to the allocated block of memory of
             *  size 'SIZE'.
             *
             */
            char *allocate_block(const char *hint __attribute__((unused)) = NULL) {
		// DPRINTF("memory_chunk<%u>::allocate_block()\n", SIZE);
		DPRINTF("_M_size: %u\n", _M_size);

                if (_M_idx == 0) {
                    assert_eq(_M_offset, 0);
                    assert_eq(_M_range, _M_size);
                }

                size_t *pseg = reinterpret_cast<size_t*>(seg_tree());
                assert_lt(_M_idx, _M_size);
                assert_ge(_M_idx, 0);
		DPRINTF("get_bit_at(%u) = %d\n", _M_idx, get_bit_at(pseg, _M_idx));

                if (!has_free()) {
                    // There is nothing here.
                    return NULL;
                } else {
                    // We MUST have the root node as unset to indicate
                    // an available memory space.
                    //
                    // Remember that _M_idx is always a valid
		    // index in the range [0 .. _M_size*2 - 2] and
		    // never forrays into an invalid range.
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

                        // This loop updates the bit in the segment
                        // tree at index 'parent'. Hence, stopping
                        // when _M_idx is 0 is correct.
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
                        --_M_free;
                        assert_le(_M_free, _M_size);
                        assert_ge(_M_free, 0);
                        return mem() + SIZE * offset;
                    }
                }
		assert(false);
                return NULL;
            }


	    /** @brief Deallocates an object allocated via this
	     *  memory_chunk<SIZE>.
	     *
             *  @param ptr The pointer pointing to a single object
             *  worth of memory to be deallocated.
             *
             */
            void deallocate_block(char *ptr) {
		assert(this->has_this_block(ptr));
                assert_eq((ptr - mem()) % SIZE, 0);

                size_t idx = (ptr - mem()) / SIZE;
                size_t *pseg = reinterpret_cast<size_t*>(seg_tree());

		assert_lt(idx, _M_size);
                idx += (_M_size - 1);
                set_bit_at(pseg, idx, 0);
                ++_M_free;

                assert_le(_M_free, _M_size);
                assert_ge(_M_free, 0);

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

	    /** @brief Check if 'ptr' is owned by this memory owner.
             *
             *  @param ptr The pointer to check
             *
             *  @return true if the memory block 'ptr' was allocated
	     *  using this memory_chunk<SIZE>.
	     *
	     */
	    bool has_this_block(char *ptr) {
		return ptr >= this->mem() && ptr < this->mem() + this->size() * SIZE;
	    }

            /** @brief Returns the size (capacity) of the
             *  allocator. i.e. how many object it can potentially
             *  hold.
             */
            size_t size() const {
                return _M_size;
            }

            /** @brief Returns true if all memory spaced owned by this
             *  allocator are available for use, and false otherwise.
             */
            bool empty() const {
                return _M_free == _M_size;
            }

            /** @brief Returns true if some (at least one) memory
             *  spaced owned by this allocator is available for use,
             *  and false otherwise.
             */
            bool has_free() const {
                return _M_free > 0;
            }

            memory_chunk<SIZE>& set_size(size_t size) {
                _M_idx = 0;
                _M_offset = 0;
                _M_size = size;
                _M_range = size;
                _M_free = _M_size;
                return *this;
            }

            void invalidate() {
                assert(this->empty());
                _M_size = 0;
                _M_free = 0;
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
                const size_t *bot_magic_ptr = reinterpret_cast<const size_t*>(&(this->mem()[SIZE * size()]));
                return *bot_magic_ptr;
            }

            char* seg_tree() {
                return reinterpret_cast<char*>(&_M_seg_tree);
            }

            const char* seg_tree() const {
                return reinterpret_cast<const char*>(&_M_seg_tree);
            }

            char* mem() {
                return this->seg_tree() + this->size() * 2 / 8;
            }

            const char* mem() const {
                return this->seg_tree() + this->size() * 2 / 8;
            }
	};

        /// Scoped lock idiom.
        // Acquire the mutex here with a constructor call, then release with
        // the destructor call in accordance with RAII style.
        class __recursive_mutex_scoped_lock
        {
        public:
            typedef __recursive_mutex __mutex_type;

        private:
            __mutex_type& _M_device;

            __recursive_mutex_scoped_lock(const __scoped_lock&);
            __recursive_mutex_scoped_lock& operator=(const __recursive_mutex_scoped_lock&);

        public:
            explicit __recursive_mutex_scoped_lock(__recursive_mutex& __name) : _M_device(__name)
            { _M_device.lock(); }

            ~__recursive_mutex_scoped_lock() throw()
            { _M_device.unlock(); }
        };

        /** @class  alloc_impl bitmap_allocator.h bitmap_allocator.h
         *
         *  @brief The bitmap allocator implementation class.
         *
         *  @detail Implements the allocation and deallocation routines.
         *
         */
	template <size_t SIZE>
        struct alloc_impl {
            // The # of valid chunks in _M_chunks.
            //
            // _M_chunks[_M_num_chunks - 1 .. _M_num_chunks)
            // is valid.
            size_t _M_num_chunks;
            // For ~64TiB worth of memory on a 32-bit machine
            // (impossible in practice) since we start with 16 objects
            // in the first chunk.
            memory_chunk<SIZE>* _M_chunks[42];
#if defined __GTHREADS
            // We use a recursive mutex since allocate(n) calls itself
            // at times. We could make allocate(n) iterative, but this
            // is cleaner.
            __recursive_mutex _M_mutex;
#endif

            alloc_impl()
                : _M_num_chunks(0) {
            }

            /* Expected complexity: O(1) per call. Degenerates to
             * O(log n) if we allocate the last block in a segment
             * tree, and then deallocate it immediately, and keep
             * doing this in a cycle. However, this is improbable.
             */
            char* allocate(size_t n) {
		DPRINTF("alloc_impl<%u>::allocate(%u)\n", SIZE, n);
                if (n != 1) {
                    return reinterpret_cast<char*>(operator new(SIZE * n));
                }

                // TODO: Verify top & bottom Magic values.

#if defined __GTHREADS
                // Take scoped lock.
                __recursive_mutex_scoped_lock __impl_lock(_M_mutex);
#endif

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
                // MUST be a power of 2.
                const size_t objects_requested = one << _M_num_chunks;
                // Rounds off objects_requested to a multiple of the #
                // of bits in a size_t type.
                const size_t objects_returned = objects_requested * (8 * sizeof(size_t) / 2);
                // The # of bytes the segment tree representation will
                // occupy.
		const size_t seg_tree_bytes = (objects_returned * 2) / 8;
                const size_t mem_size = sizeof(memory_chunk<SIZE>) /* For the header */ +
		    seg_tree_bytes /* For the Segment Tree */ +
                    objects_returned * SIZE /* For the actual objects */ +
                    sizeof(size_t) /* For the trailing magic */;

		DPRINTF("objects_requested:    %10u\n"
                        "objects_returned:     %10u\n"
			"seg_tree_bytes:       %10u\n"
			"user_memory_size:     %10u (%u * %u)\n"
			"sizeof(memory_chunk): %10u\n"
			"mem_size:             %10u\n",
                        objects_requested, objects_returned,
                        seg_tree_bytes,
                        objects_returned * SIZE, objects_returned, SIZE,
                        sizeof(memory_chunk<SIZE>),
                        mem_size);

                memory_chunk<SIZE> *pchunk = reinterpret_cast<memory_chunk<SIZE>*>(malloc(mem_size));
                if (!pchunk) {
                    return NULL;
                }
                (*pchunk).set_size(objects_returned)
                    .set_top_magic(TOP_MAGIC)
                    .set_bot_magic(BOT_MAGIC);

		// Clear out the segment tree
		memset(pchunk->seg_tree(), 0, seg_tree_bytes * 2);

                _M_chunks[_M_num_chunks] = pchunk;
                std::swap(_M_chunks[0], _M_chunks[_M_num_chunks]);
                ++_M_num_chunks;
                return allocate(n);
            }

            /* Expected complexity: O(1) per call
             */
            void deallocate(char *ptr, size_t n) {
                if (ptr == NULL) {
                    return;
                }

                if (n != 1) {
                    operator delete(ptr);
                    return;
                }

                // TODO: Verify top & bottom Magic values.

#if defined __GTHREADS
                // Take scoped lock.
                __recursive_mutex_scoped_lock __impl_lock(_M_mutex);
#endif

                for (size_t i = 0; i < _M_num_chunks; ++i) {
                    if (_M_chunks[i]->has_this_block(ptr)) {
                        _M_chunks[i]->deallocate_block(ptr);

                        // _M_chunks[1] should contain the previous
			// value of _M_chunks[0] so that we can
			// quickly [in O(1)] determine if there is
			// another free memory chunk available or not.
                        if (i != 0) {
                            // We need to swap. Check if we can move 0 -> 1.
                            if (_M_num_chunks > 1) {
                                std::swap(_M_chunks[0], _M_chunks[1]);
                            }

                            if (i > 1) {
                                // Move to front only if i > 1 since
                                // if i == 1, we have already
                                // performed the swap above.
                                std::swap(_M_chunks[0], _M_chunks[i]);
                            }
                        }

                        // Remove empty chunks, but only if there is
                        // more than one empty chunk remaining.
                        //
                        if (_M_num_chunks > 1 && _M_chunks[0]->empty() &&
                            _M_chunks[1]->has_free()) {
                            std::swap(_M_chunks[1], _M_chunks[0]);
                            std::swap(_M_chunks[1], _M_chunks[_M_num_chunks - 1]);

                            // fprintf(stderr, "Freeing chunk with size: %d\n", _M_chunks[_M_num_chunks - 1]->size());
                            // Free _M_chunks[_M_num_chunks - 1]
                            _M_chunks[_M_num_chunks - 1]->invalidate();
                            ::free(reinterpret_cast<void*>(_M_chunks[_M_num_chunks - 1]));
                            --_M_num_chunks;
                        }
                        return;
                    }
                }

                // You tried to deallocate an object that wasn't
                // allocated via this allocator.
                assert(false);
            }

        };
    } // namespace __detail

_GLIBCXX_BEGIN_NAMESPACE_VERSION

    template <typename T>
    class bitmap_allocator {
        // Design choice: bitmap_allocator<int> and
        // bitmap_allocator<float> do NOT share the same memory space
        // even if sizeof(int) == sizeof(float). This is because they
        // are different types, which may have unrelated lifespans.
        static __detail::alloc_impl<sizeof(T)> impl;

    public:
        typedef size_t     size_type;
        typedef ptrdiff_t  difference_type;
        typedef T*         pointer;
        typedef const T*   const_pointer;
        typedef T&         reference;
        typedef const T&   const_reference;
        typedef T          value_type;
#if __cplusplus >= 201103L
      // _GLIBCXX_RESOLVE_LIB_DEFECTS
      // 2103. propagate_on_container_move_assignment
      typedef std::true_type propagate_on_container_move_assignment;
#endif

        template<typename T1>
        struct rebind
        { typedef bitmap_allocator<T1> other; };

#if __cplusplus >= 201103L
      template<typename _Up, typename... _Args>
        void
        construct(_Up* __p, _Args&&... __args)
	{ ::new((void *)__p) _Up(std::forward<_Args>(__args)...); }

      template<typename _Up>
        void 
        destroy(_Up* __p)
        { __p->~_Up(); }
#else
        void 
        construct(pointer __p, const_reference __val) 
        { ::new((void *)__p) T(__val); }

        void 
        destroy(pointer __p) { __p->~T(); }
#endif

	bitmap_allocator() throw() { }

        ~bitmap_allocator() throw() { }

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
    __detail::alloc_impl<sizeof(T)> bitmap_allocator<T>::impl;

    template<typename T1, typename T2>
    inline bool
    operator==(const bitmap_allocator<T1>&, const bitmap_allocator<T2>&) throw()
    { return true; }

    template<typename T1, typename T2>
    inline bool
    operator!=(const bitmap_allocator<T1>&, const bitmap_allocator<T2>&) throw()
    { return false; }

_GLIBCXX_END_NAMESPACE_VERSION
} // namespace __gnu_cxx

#endif // BITMAP_ALLOCATOR_H
