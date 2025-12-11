# Compile the Computational Topology project.

CONF_URL	:= https://github.com/Nell-Mills/Config.git
CONF_DIR	:= Include/NM-Config
CONF_FILES	:= Config.h LICENSE
CONF_CLONE	:= git clone $(CONF_URL) $(CONF_DIR)/Temp && \
		$(foreach file, $(CONF_FILES), cp $(CONF_DIR)/Temp/$(file) $(CONF_DIR) &&) \
		rm -rf $(CONF_DIR)/Temp

VKA_URL		:= https://github.com/Nell-Mills/Vulkan-Abstraction.git
VKA_DIR		:= Include/Vulkan-Abstraction
VKA_FILES	:= Vulkan-Abstraction.h Vulkan-Abstraction.c LICENSE
VKA_CLONE	:= git clone $(VKA_URL) $(VKA_DIR)/Temp && \
		$(foreach file, $(VKA_FILES), cp $(VKA_DIR)/Temp/$(file) $(VKA_DIR) &&) \
		mkdir -p Include/Volk && cp $(VKA_DIR)/Temp/Include/Volk/* \
		Include/Volk && rm -rf $(VKA_DIR)/Temp

DEPS_CLONE := $(CONF_CLONE) && $(VKA_CLONE) &&

DEPS_PROJECT :=\
$(wildcard Source/*.c)

DEPS_INCLUDE :=\
Include/Volk/*.c\
Include/Vulkan-Abstraction/*.c\
Include/TinyOBJLoaderC/tinyobj_loader_c.c

CC	:= gcc -std=c99 -Wall -Wextra -Wno-unused-parameter
OUT	:= -o Computational-Topology
MAIN	:= Computational-Topology.c
DEPS	:= $(DEPS_PROJECT) $(DEPS_INCLUDE)
CFLAGS	:= -I Include -fopenmp
LFLAGS	:= -L Libs -Wl,-rpath '$$ORIGIN/Libs' -lSDL3
DEFINES	:=
DEBUG	:= -D CT_DEBUG -D VKA_DEBUG -g -O0

release: $(MAIN)
	$(DEPS_CLONE) $(CC) $(MAIN) $(DEPS) $(DEFINES) $(CFLAGS) $(OUT) $(LFLAGS)

debug: $(MAIN)
	$(DEPS_CLONE) $(CC) $(MAIN) $(DEPS) $(DEFINES) $(DEBUG) $(CFLAGS) $(OUT) $(LFLAGS)
