This library is intended to use in "mock" testing of library that directly manipulates flash memory, but without using actual flash.

It utilizes a memory mapped file to store the flash data itself, maps the flash into whatever address space you specify, similar to on embedded devices.

There are two pages mapped: RW, and RO.   RO is the one generally used by your library, and the Read-Write page should be managed through the library functions similar to flash writes on many embedded applications.  The code faults if a write is attempted to the read-only page.
