// -*- mode:C++;c-basic-offset:4 -*-
#if !defined BITMAP_ALLOCATOR_H
#define BITMAP_ALLOCATOR_H

namespace __gnu_cxx {
    namespace {
        bool get_bit_at(size_t *ptr, size_t index) {
            const size_t big_offset = index / sizeof(size_t);
            const size_t lil_offset = index % size_t(size_t);
            const size_t *mem = ptr + big_offset;
            const size_t one = 1L;
            return *mem & (one << lil_offset);
        }

        void set_bit_at(size_t *ptr, size_t index, bool value) {
            const size_t big_offset = index / sizeof(size_t);
            const size_t lil_offset = index % size_t(size_t);
            size_t *mem = ptr + big_offset;
            const size_t one = 1L;
            if (value) {
                *mem |= (one << lil_offset);
            } else {
                *mem & (~(one << lil_offset));
            }
        }

        struct segment_tree {
        };

        template <size_t OBJECT_SIZE>
        struct memory_chunk {
            // _M_size stores the number of memory objects of size
            // OBJECT_SIZE that this memory_chunk stores
            size_t _M_size;
            size_t _M_top_magic;
            size_t _M_seg_tree[0];

            char *allocate_block(/* hint */) {
                size_t idx = 0;
                size_t offset = 0;
                size_t range = _M_size;
                size_t *pseg = reinterpret_cast<size_t*>(segment_tree());
                assert(idx < _M_size);
                if (get_bit_at(pseg, idx) != 0) {
                    // There is nothing here.
                    return NULL;
                }
                while (idx < _M_size * 2 - 1) {
                    // We have something free under this node.
                    const size_t left = idx * 2 + 1;
                    const size_t right = idx * 2 + 2;
                    range /= 2;
                    if (left < _M_size * 2 - 1 && get_bit_at(left) == 0) {
                        idx = left;
                    } else if (right < _M_size * 2 - 1 && get_bit_at(right) == 0) {
                        idx = right;
                        offset += range;
                    } else {
                        // Must be the end of the array.
                        // Set the bit at position 'idx'.
                        set_bit_at(pseg, idx, 1);
                        while (idx != 0) {
                            size_t parent = idx / 2;
                            left = parent * 2 + 1;
                            right = parent * 2 + 1;
                            if (get_bit_at(pseg, left) == 0 || get_bit_at(pseg, right) == 0) {
                                // Stop setting bits.
                                break;
                            }
                            // Set bit at 'parent' to indicate that
                            // everything under the tree rooted at
                            // 'parent' is completely allocated, with
                            // no free blocks.
                            set_bit_at(pseg, parent, 1);
                            idx /= 2;
                        }
                        return mem() + OBJECT_SIZE * offset;
                    }

                }
            }

            void deallocate_block(char *ptr) {
            }

            size_t size() const {
                return _M_size;
            }

            memory_chunk<OBJECT_SIZE>& set_size(size_t size) {
                _M_size = size;
                return *this;
            }

            memory_chunk<OBJECT_SIZE>& set_top_magic(size_t magic) {
                _M_top_magic = magic;
                return *this;
            }

            memory_chunk<OBJECT_SIZE>& set_bot_magic(size_t magic) {
                size_t *bot_magic_ptr = &(_M_mem[OBJECT_SIZE * size()]);
                *bot_magic_ptr = magic;
                return *this;
            }

            size_t top_magic() const {
                return _M_top_magic;
            }

            size_t bot_magic() const {
                const size_t *bot_magic_ptr = &(_M_mem[OBJECT_SIZE * size()]);
                return *bot_magic_ptr;
            }

            char* seg_tree() {
                return &_M_seg_tree;
            }

            char* mem() {
                return reinterpret_cast<char*>(&_M_seg_tree + _M_size * 2 / sizeof(size_t));
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
                if (n != 1) {
                    return reinterpret_cast<char*>(malloc(SIZE * n));
                }

                for (size_t i = 0; i < _M_num_chunks; ++i) {
                    char *mem = _M_chunks[i].allocate_block();
                    if (mem) {
                        // Set this as the first block so that
                        // subsequent searches find it first.
                        std::swap(_M_chunks[i], _M_chunks[0]);
                        return mem;
                    }
                }

                // No free block was found. Allocate a new chunk and
                // restart.
                const size_t one = 1L;
                const size_t num_blocks_in_chunk = (one << (_M_num_chunks + 1)) * sizeof(size_t) / 2;
                const size_t mem_size = sizeof(memory_chunk<SIZE>) /* For the header */ +
                    num_blocks_in_chunk * SIZE /* For the actual objects */ +
                    sizeof(size_t) /* For the trailing magic */;
                memory_chunk<SIZE> *pchunk = reinterpret_cast<memory_chunk<SIZE>*>(malloc(mem_size));
                if (!pchunk) {
                    return NULL;
                }
                (*pchunk).set_size(num_blocks_in_chunk)
                    .set_top_magic(TOP_MAGIC)
                    .set_bot_magic(BOT_MAGIC);

                _M_chunks[_M_num_chunks] = pchunk;
                std::swap(_M_chunks[0], _M_chunks[_M_num_chunks]);
                ++_M_num_chunks;
                return allocate(n);
            }

            void deallocate(char *ptr, size_t n) {
                if (n != 1) {
                    free(ptr);
                    return;
                }

                for (size_t i = 0; i < _M_num_chunks; ++i) {
                    const size_t mem_byte_length = _M_chunks[i].size() * SIZE;
                    if (ptr >= _M_chunks[i].mem() && ptr < _M_chunks[i].mem() + mem_byte_length) {
                        _M_chunks[i].deallocate_block(ptr);
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
    };

}

#endif // BITMAP_ALLOCATOR_H
