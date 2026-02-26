#
# BSD 2-Clause License
#
# Copyright (c) 2026, uidops
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

.POSIX:

SRC = ./src/ip-info.c
PREFIX = /usr/local
CC ?= cc
STRIP ?= llvm-strip
CFLAGS ?= -Wall -Wextra -O3 -flto=thin -pipe -fstack-protector-strong -fpie -DHAVE_MAXMINDDB
LIBS != pkg-config --cflags --libs json-c libmaxminddb
TARGET = ip-info
DATASET = dataset/GeoLite2-City.mmdb dataset/GeoLite2-ASN.mmdb

.PHONY: clean test all

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)
	$(STRIP) -s $(TARGET)

install: $(TARGET)
	mkdir -p $(PREFIX)/bin
	mkdir -p $(PREFIX)/share/ip-info
	cp -f $(TARGET) $(PREFIX)/bin
	cp -f $(DATASET) $(PREFIX)/share/ip-info/
	chmod +x $(PREFIX)/bin/$(TARGET)
	chmod 644 $(PREFIX)/share/ip-info/*.mmdb

uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)
	rm -rf $(PREFIX)/share/ip-info

clean:
	rm -f $(TARGET)

test: $(TARGET)
	./ip-info 8.8.8.8
