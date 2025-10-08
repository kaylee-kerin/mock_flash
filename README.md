This library implements a mmap mock for working with nand flash.  Used for tests, examples, or other such needs.  No flash devices will be harmed by running it.

# Theory of Operation
We MMAP a single file descriptor in twice.  One as a read/write page, and once as a read-only page.

As long as we exclusively work with the read-only page, and use the flash write/erase functions as one would normally - we can emulate the write/erase behavior of a normal NAND flash.

# Understanding Flash
We start by thinking of how flash works for storing data.  0 or 1. 

_TODO: Flash is made up of pages_

Bits in a flash page
* Can be changed from 1->0 individually
* Can be changed from 0->1 all at once.

_TODO: Example normal use_

  _SHOW Random data page_
  _SHOW Erased Page_
  _SHOW Written Page_

...Nothing in these requirements says we can't write to the same page twice, if we only change some of it...

  _SHOW Show page with partial data, 0xFF for the rest_
  _SHOW Change a few more 1s to 0s_
  _SHOW Still a few more!_


It's a simple trick, one people have been taking advantage of for years, but we can use it to build some interesting functionality.





