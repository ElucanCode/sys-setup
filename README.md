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
`sys-setup` lists all sub directories, checks if they contain a `install.c` file. Every
such file is compiled to a `.so` file, which is then `dlopen`-ed. The only compile-time
dependencies are `libc` and `libdl`. There are some optional dependencies which are
checked and loaded at runtime.

## How do I create a new installer?
Extremely easy!
1. Create a new directory with the name of what it is you would like to install.
2. Create a `install.c` file inside this directory and copy any files you would
   like to install using this installer here.
3. The `install.c` file has to implement all functions marked with `INSTALLER_NECESSARY`
   inside `installer.h` and may implement every `INSTALLER_OPTIONAL` function.
4. Write all your installation code inside the `run_install` function.
5. Done! `sys-setup` will automagically pick your installer up.
