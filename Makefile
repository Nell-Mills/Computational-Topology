# Compile the Computational Topology project.

CONF_URL	:= https://github.com/Nell-Mills/Config.git
CONF_DIR	:= Include/NM-Config
CONF_FILES	:= Config.h LICENSE
CONF_CLONE	:= git clone $(CONF_URL) $(CONF_DIR)/Temp && \
		$(foreach file, $(CONF_FILES), cp $(CONF_DIR)/Temp/$(file) $(CONF_DIR) &&)\
		rm -rf $(CONF_DIR)/Temp

MP_URL		:= https://github.com/Nell-Mills/Mesh-Processing.git
MP_DIR		:= Include/Mesh-Processing
MP_FILES	:= $(wildcard Source/*.c) $(wildcard Source/*.h) LICENSE
MP_CLONE	:= git clone $(MP_URL) $(MP_DIR)/Temp && \
		$(foreach file, $(MP_FILES), cp $(MP_DIR)/Temp/$(file) $(MP_DIR) &&)\
		rm -rf $(MP_DIR)/Temp

DEPS_CLONE := $(CONF_CLONE) && $(MP_CLONE) &&

DEPS_PROJECT :=\
$(wildcard Source/*.c)

DEPS_THIRD_PARTY :=\
$(wildcard Include/Mesh-Processing/*.c)

CC	:= gcc -std=c99 -Wall -Wextra -Wno-unused-parameter
OUT	:= -o Computational-Topology
MAIN	:= Computational-Topology.c
DEPS	:= $(DEPS_PROJECT) $(DEPS_THIRD_PARTY)
CFLAGS	:= -I Include
LFLAGS	:=
DEFINES	:=
DEBUG	:= -D CT_DEBUG -D MP_DEBUG -g -O0

release: $(MAIN)
	$(DEPS_CLONE) $(CC) $(MAIN) $(DEPS) $(DEFINES) $(CFLAGS) $(OUT) $(LFLAGS)

debug: $(MAIN)
	$(DEPS_CLONE) $(CC) $(MAIN) $(DEPS) $(DEFINES) $(DEBUG) $(CFLAGS) $(OUT) $(LFLAGS)
