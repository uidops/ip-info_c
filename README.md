<div align="center"><img src="https://github.com/siruidops/ip-info_c/raw/main/.tmp/text.gif"/>

![License](https://img.shields.io/badge/license-BSD-blue) ![State](https://img.shields.io/badge/state-developing-cyan) ![Language](https://img.shields.io/badge/language-C-purple)
</div>

# Get information about an IP
Based on <a href="http://ip-api.com">ip-api.com</a> <br />
Tested on Gentoo (X86-64) + LLVM/CLANG 13.0.0 + GNU Make 4.3

Dependencies for build:
```
make (gmake or bmake)
clang/llvm (or gcc/binutils)
json-c
```

Build and install (clang/llvm):
``` bash
$ make
$ make install
```

Build and install (gcc/binutils):
``` bash
$ make CC=gcc STRIP=strip CFLAGS="-O2"
$ make install
```

Clean:
``` bash
$ make clean
```

Uninstall:
``` bash
$ make uninstall
```

Usage:
``` bash
$ ip-info {IP or host}
$ ip-info # For get your ip detail (Without any argument).
```
