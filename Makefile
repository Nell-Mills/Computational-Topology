# Compile the Computational Topology project.

CONF_URL	:= https://github.com/Nell-Mills/Config.git
CONF_DIR	:= Include/NM-Config
CONF_FILES	:= Config.h LICENSE
CONF_CLONE	:= git clone $(CONF_URL) $(CONF_DIR)/Temp && \
		$(foreach file, $(CONF_FILES), cp $(CONF_DIR)/Temp/$(file) $(CONF_DIR) &&)\
		rm -rf $(CONF_DIR)/Temp

DEPS_CLONE := $(CONF_CLONE) &&

DEPS_PROJECT :=\
$(wildcard Source/*.c)

DEPS_THIRD_PARTY :=\
Include/TinyOBJLoaderC/tinyobj_loader_c.c

CC	:= gcc -std=c99 -Wall -Wextra -Wno-unused-parameter
OUT	:= -o Computational-Topology
MAIN	:= Computational-Topology.c
DEPS	:= $(DEPS_PROJECT) $(DEPS_THIRD_PARTY)
CFLAGS	:= -I Include -fopenmp
LFLAGS	:= -lSDL2 -lrt
DEFINES	:= -D _XOPEN_SOURCE=600
DEBUG	:= -D CT_DEBUG -g -O0

release: $(MAIN)
	$(DEPS_CLONE) $(CC) $(MAIN) $(DEPS) $(DEFINES) $(CFLAGS) $(OUT) $(LFLAGS)

debug: $(MAIN)
	$(DEPS_CLONE) $(CC) $(MAIN) $(DEPS) $(DEFINES) $(DEBUG) $(CFLAGS) $(OUT) $(LFLAGS)
