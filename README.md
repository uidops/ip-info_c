<div align="center"><img src="https://github.com/siruidops/ip-info_c/raw/main/.tmp/text.gif"/>

![License](https://img.shields.io/badge/license-BSD-blue) ![State](https://img.shields.io/badge/state-developing-cyan) ![Language](https://img.shields.io/badge/language-C-purple)
</div>

# Get information about an IP
This program looks up geographic and ASN information for an IP address using local MaxMind GeoLite2 databases (City + ASN). When run without an argument the program attempts to determine your public IP via ipify.org.

Dependencies and requirements
- A C compiler and a strip tool (examples: `clang`, `gcc`, `llvm-strip`).
- `json-c` development library (used for parsing ipify JSON).
- `libmaxminddb` development library (required at build and runtime).
- `pkg-config` (recommended to discover compiler/linker flags).
- The GeoLite2 dataset files:
  - `dataset/GeoLite2-City.mmdb`
  - `dataset/GeoLite2-ASN.mmdb`

Important: MaxMind's GeoLite2 databases require accepting their license. Obtain the `.mmdb` files from MaxMind (you may need a license key) and place them in the repository `dataset/` directory or allow `make install` to install the provided files to the system share directory.

Platform examples — install build dependencies
- macOS (Homebrew):
  - brew install pkg-config json-c libmaxminddb
- Debian/Ubuntu:
  - sudo apt update
  - sudo apt install build-essential pkg-config libjson-c-dev libmaxminddb-dev

Build
```bash
# default build (Makefile is configured to compile with -DHAVE_MAXMINDDB and link libmaxminddb)
make
```

Custom build flags (example using clang and LTO)
```bash
make CC=clang STRIP=llvm-strip CFLAGS="-O3 -flto=thin -march=native -fuse-ld=lld"
```

Install
```bash
sudo make install
```

What install does
- Installs the binary to `/usr/local/bin/ip-info` by default.
- Installs dataset files to `/usr/local/share/ip-info/` (copies `dataset/GeoLite2-*.mmdb`).
  - The installed binary expects the datasets to be available in the `dataset/` folder at build time or under `/usr/local/share/ip-info/` at runtime.

Uninstall
```bash
sudo make uninstall
```

Clean build artifacts
```bash
make clean
```

Test
```bash
make test
# or run against a specific IP:
./ip-info 8.8.8.8
```

Notes about datasets
- This project requires local GeoLite2 `.mmdb` files (City + ASN). If you don't have them in `dataset/`, the install target will copy whatever `.mmdb` files are present in `dataset/` into `/usr/local/share/ip-info/`.
- If you need an automated helper to download GeoLite2 (requires a MaxMind account and license key) we can add a `make download-datasets` target that accepts a license key and downloads the archives for you. Ask if you'd like that.

Usage
```bash
# Lookup example IP
$ ip-info 8.8.8.8

# Lookup your own public IP (no args)
$ ip-info
```
