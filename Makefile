#os detection
ifeq ($(OS),Windows_NT)     # is Windows_NT on XP, 2000, 7, Vista, 10...
    SYS_OS := Windows
else
    SYS_OS := $(shell uname)  # same as "uname -s"
endif
SYS_OS := $(patsubst CYGWIN%,Windows,$(SYS_OS))
SYS_OS := $(patsubst MSYS%,Windows,$(SYS_OS))
SYS_OS := $(patsubst MINGW%,Windows,$(SYS_OS))

$(info ~~~~~ Makefile configured for $(SYS_OS))
$(info -----)

#build default
ifeq ($(SYS_OS),Windows)
.DEFAULT_GOAL := win
CLEAN_DIR := ./wobj
#build with clang
CC := clang
CXX := clang++
endif
ifeq ($(SYS_OS),Linux)
.DEFAULT_GOAL := linux
CLEAN_DIR := ./lobj
#build with gcc
CC := gcc
CXX := g++
endif
ifeq ($(SYS_OS),Darwin)
.DEFAULT_GOAL := mac
CLEAN_DIR := ./mobj
#build with clang
CC := clang
CXX := clang++
endif

#universal cfg
SRC_DIRS := ./src
SRCS := $(shell find $(SRC_DIRS) -name *.cpp -or -name *.c -or -name *.s)
WTXTS := $(shell find $(SRC_DIRS) -name *.wren)
LTXTS := $(shell find $(SRC_DIRS) -name *.lua)
DEPS := $(OBJS:.o=.d)
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS)) -I./include
MKDIR_P ?= mkdir -p

WREN_TXTS := $(WTXTS:%=$(SRC_DIRS)/%.txt)
LUA_TXTS := $(LTXTS:%=$(SRC_DIRS)/%.txt)

#windows cfg
WBUILD_DIR := ./wobj
WOBJS := $(SRCS:%=$(WBUILD_DIR)/%.o)
WCPPFLAGS ?= $(INC_FLAGS) -std=c11 -Wall -m64 -O2
WLDFLAGS = ./lib/libwren.ucrt.a -lluajit-5.1 -lm -ldl

#linux cfg
LBUILD_DIR := ./lobj
LOBJS := $(SRCS:%=$(LBUILD_DIR)/%.o)
LCPPFLAGS ?= $(INC_FLAGS) -fPIC -std=gnu11 -Wall -m64 -O2
LLDFLAGS = ./lib/libwren.a -lluajit-5.1 -lpthread -lm -ldl

#macos cfg
MBUILD_DIR := ./mobj
MOBJS := $(SRCS:%=$(MBUILD_DIR)/%.o)
MCPPFLAGS ?= $(INC_FLAGS) -fPIC -std=gnu11 -Wall -m64 -O2
MLDFLAGS = ./lib/libwren.m.a -lpthread -lm -ldl

# text source
$(SRC_DIRS)/%.wren.txt: %.wren
	f2t $<

$(SRC_DIRS)/%.lua.txt: %.lua
	f2t $<

# assembly
./wobj/%.s.o: %.s
	$(MKDIR_P) $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

./lobj/%.s.o: %.s
	$(MKDIR_P) $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# c source, windows
./wobj/%.c.o: %.c %.h
	$(MKDIR_P) $(dir $@)
	$(CC) $(WCPPFLAGS) $(CFLAGS) -c $< -o $@

# c++ source, windows
./wobj/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) $(WCPPFLAGS) $(CXXFLAGS) -c $< -o $@

# c source, linux
./lobj/%.c.o: %.c %.h
	$(MKDIR_P) $(dir $@)
	$(CC) $(LCPPFLAGS) $(CFLAGS) -c $< -o $@

# c++ source, linux
./lobj/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) $(LCPPFLAGS) $(CXXFLAGS) -c $< -o $@

# c source, macos
./mobj/%.c.o: %.c %.h
	$(MKDIR_P) $(dir $@)
	$(CC) $(MCPPFLAGS) $(CFLAGS) -c $< -o $@

# c++ source, macos
./mobj/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) $(MCPPFLAGS) $(CXXFLAGS) -c $< -o $@

.PHONY: clean

texts: $(WREN_TXTS) $(LUA_TXTS)
	
linux: $(LOBJS)
	$(eval LDFLAGS=$(LLDFLAGS))
	$(info -----)
	$(CC) $(LOBJS) -shared -o $(LBUILD_DIR)/carrica.l.so $(LDFLAGS)

mac: $(MOBJS)
	$(eval LDFLAGS=$(MLDFLAGS))
	$(info -----)
	$(CC) $(MOBJS) -shared -o $(MBUILD_DIR)/carrica.m.so $(LDFLAGS)

win: $(WOBJS)
	$(eval LDFLAGS=$(WLDFLAGS))
	$(info -----)
	$(CC) $(WOBJS) -shared -o $(WBUILD_DIR)/carrica.dll $(LDFLAGS)
	strip $(WBUILD_DIR)/carrica.dll

clean:
	$(RM) -r $(CLEAN_DIR)

-include $(DEPS)

