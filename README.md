# malloc-tools
NPM module that exports glibc malloc's stats and trim functions
Functions exported:
 - malloc_info()
 - mallinfo2() (falls back to mallinfo() if glibc is older than 2.33)
 - malloc_stats()
 - malloc_trim()
For usage example, see test.js
