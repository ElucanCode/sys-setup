# Usage
There are two ways to use `sys-setup`:

## shebang
Make sure `sys-setup.c` is executable and simply run:
```sh
$ ./sys-setup.c
```

## manual
```sh
$ gcc -Wall -rdynamic sys-setup.c -o sys-setup -ldl
$ ./sys-setup
```

# How it works
It lists all sub directories, checks if they contain a `install.c` file. Every
such file is compiled to a `.so` file, which is then `dlopen`-ed. The only
compile-time dependencies are `libc` and `libdl`. There are some optional
dependencies which are checked and loaded at runtime.
