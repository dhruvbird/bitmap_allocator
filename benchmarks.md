| Test - Alloc     | Running Time(sec) | Memory Used% |
|:----------------:|------------------:|-------------:|
| New Bitmap Alloc | 0.823             | 2.1%         |
| Old Bitmap Alloc | 0.562             | 2.1%         |
| std::allocator   | 1.239             | 6.3%         |

| Test - List      | Running Time(sec) | Memory Used% |
|:----------------:|------------------:|-------------:|
| New Bitmap Alloc | 9.881             | 1.9%         |
| Old Bitmap Alloc | 9.945             | 1.1%         |
| std::allocator   | 13.748            | 5.2%         |

Note: The new bitmap allocator was tested with assertions
enabled. Assertions only add a 2% overhead, so 9.881 sec just becomes
9.679 above. We don't bother diabling them for these benchmarks.

```
$ uname -a
Linux ubuntu 2.6.38-16-generic-pae #67-Ubuntu SMP Thu Sep 6 18:18:02 UTC 2012 i686 i686 i386 GNU/Linux
$ g++ --version
g++ (Ubuntu 4.6.0-3~ppa1) 4.6.1 20110409 (prerelease)
```
