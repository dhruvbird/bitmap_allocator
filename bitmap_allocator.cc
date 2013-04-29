// -*- mode:C++;c-basic-offset:4;indent-tabs-mode:nil -*-
// Bitmap Allocator. Out of line function definitions.

#include <ext/bitmap_allocator.h>

namespace __gnu_cxx _GLIBCXX_VISIBILITY(default)
{
  namespace __detail
  {
  _GLIBCXX_BEGIN_NAMESPACE_VERSION
    template struct memory_chunk<sizeof(char)>;
    template struct memory_chunk<sizeof(wchar_t)>;

    template struct alloc_impl<sizeof(char)>;
    template struct alloc_impl<sizeof(wchar_t)>;
  _GLIBCXX_END_NAMESPACE_VERSION
  }

_GLIBCXX_BEGIN_NAMESPACE_VERSION

  // Instantiations.
  template class bitmap_allocator<char>;
  template class bitmap_allocator<wchar_t>;

_GLIBCXX_END_NAMESPACE_VERSION
} // namespace
