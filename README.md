# Catbios

A "BIOS" for `qemu-system-x86_64` 440FX and Q35 machines. This really just inits the BIOS video mode it's much less of a actual BIOS as much as it is a fun project for me.


# If you want to test it:
Install Clang and ld.ldd and call `make`

Then call `qemu-system-x86_64 -bios rom.bin` or feel free to try it on other VMs tho no promises it works or works as intended

# Does it work with GCC?
Ehhhh probably untested.
