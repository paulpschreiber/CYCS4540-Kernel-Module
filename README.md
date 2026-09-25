# Bat-to-Cat Kernel Module Demo

This project implements a proof-of-concept Linux kernel module for Ubuntu 22.04. It installs a `kretprobe` on `vfs_read` and, when the file being read is named `text.txt`, it rewrites the returned buffer from `bat` to `cat` before returning the data to userspace.

This is intended for an educational lab or disposable VM only. It is not suitable for production use.

## Prerequisites

On Ubuntu 22.04, install the build dependencies:

```bash
sudo apt update
sudo apt install -y build-essential linux-headers-$(uname -r)
```

## Build

```bash
make
```

This should produce a module file such as `bat_to_cat.ko` if the kernel headers match the running kernel.

## Load the module

```bash
sudo insmod bat_to_cat.ko
```

## Test the behavior

Create a temporary file named `text.txt` containing the text `bat`:

```bash
printf 'bat bat\n' > /tmp/text.txt
cat /tmp/text.txt
```

With the module loaded, the read should present the rewritten text:

```text
cat cat
```

## Unload the module

```bash
sudo rmmod bat_to_cat
```

## Notes

- The behavior is intentionally restricted to files named `text.txt`.
- This does not attempt to sanitize or validate arbitrary kernel data paths.
- The module is a demonstration project and is not intended for shipping in a real system.
