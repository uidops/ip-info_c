#  (https://github.com/siruidops/ip-info_c).
#  Copyright (c) 2021 uidops.
# 
#  This program is free software: you can redistribute it and/or modify  
#  it under the terms of the GNU General Public License as published by  
#  the Free Software Foundation, version 3.
#
#  This program is distributed in the hope that it will be useful, but 
#  WITHOUT ANY WARRANTY; without even the implied warranty of 
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
#  General Public License for more details.
#
#  You should have received a copy of the GNU General Public License 
#  along with this program. If not, see <http://www.gnu.org/licenses/>.

.POSIX:

SRC = ip-info.c
PREFIX = /usr/local
CC = gcc
CFLAGS =
TARGET = ip-info

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

install: $(TARGET)
	mkdir -p $(PREFIX)/bin
	cp -f $(TARGET) $(PREFIX)/bin
	chmod 755 $(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)

clean:
	rm $(TARGET)
