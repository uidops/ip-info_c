<div align="center"><img src="https://github.com/siruidops/ip-info_c/raw/main/.tmp/text.gif"/>

![License](https://img.shields.io/badge/license-BSD-blue) ![Status](https://img.shields.io/badge/state-success-cyan) ![Language](https://img.shields.io/badge/language-C-purple)
</div>

# Get information about IP in C 
tested on Gentoo + CLANG 12.0.0 + GNU Make 4.3

Dependencies:
```
make
clang/llvm (or gcc)
```

Build and install (clang/llvm):
``` bash
$ make
$ (sudo/doas) make install
```

Build and install (gcc):
``` bash
$ make CC=gcc STRIP=strip
$ (sudo/doas) make install
```

Clean:
``` bash
$ make clean
```

Uninstall:
``` bash
$ sudo make uninstall
```

Usage:
``` bash
$ ip-info {IP or host}
$ ip-info # For get your ip detail (Without any argument).
```
