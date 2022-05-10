<div align="center"><img src="https://github.com/siruidops/ip-info_c/raw/main/.tmp/text.gif"/>

![License](https://img.shields.io/badge/license-BSD-blue) ![State](https://img.shields.io/badge/state-developing-cyan) ![Language](https://img.shields.io/badge/language-C-purple)
</div>

# Get information about an IP
API: <a href="http://ip-api.com">ip-api.com</a> <br />

Dependencies for build:
```
make
a c compiler and a strip tool, it depends on you (suggestion: clang/llvm, gcc or tcc)
json-c c library
```

Build and install:
``` bash
$ make
$ make install # run as root
```

Build and install with custom c compiler, strip and cflags (for example clang):
``` bash
$ make CC=clang STRIP=llvm-strip CFLAGS="-O3 -flto=thin -march=native -fuse-ld=lld"
$ make install # run as root
```

Clean:
``` bash
$ make clean
```

Uninstall:
``` bash
$ make uninstall
```

Test:
``` bash
$ make test
```

Usage:
``` bash
$ ip-info {IP or host}
$ ip-info # For get your ip detail (Without any argument).
```
