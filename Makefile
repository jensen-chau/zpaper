TARGETS = zpaper zpaperd
CC = gcc
BUILD_DIR = build
PROTOCOL_DIR = protocol

WAYLAND_PROTOCOLS = $(wildcard $(PROTOCOL_DIR)/*.xml)
PROTOCOL_C = $(addprefix $(BUILD_DIR)/, $(WAYLAND_PROTOCOLS:$(PROTOCOL_DIR)/%.xml=%-protocol.c))
PROTOCOL_H = $(addprefix $(BUILD_DIR)/, $(WAYLAND_PROTOCOLS:$(PROTOCOL_DIR)/%.xml=%-protocol.h))

INCLUDE = -I $(BUILD_DIR) -I include

LIBS = -lwayland-client -lm

FLAGS = -Wall -Werror -Wextra -Wno-unused-parameter -Wno-unused-function -Wno-misleading-indentation -Wno-implicit-fallthrough
CFLAGS = -g -O0 $(INCLUDE) $(FLAGS) -MMD -MP

.PHONY: all clean install uninstall

.PRECIOUS: $(PROTOCOL_C) $(PROTOCOL_H)

all: $(TARGETS) | $(BUILD_DIR)

$(BUILD_DIR):
	@mkdir -p $@ $(BUILD_DIR)/src/client $(BUILD_DIR)/src/daemon $(BUILD_DIR)/src/common $(BUILD_DIR)/src/handlers

$(BUILD_DIR)/%-protocol.c: $(PROTOCOL_DIR)/%.xml | $(BUILD_DIR)
	wayland-scanner private-code < $< > $@

$(BUILD_DIR)/%-protocol.h: $(PROTOCOL_DIR)/%.xml | $(BUILD_DIR)
	wayland-scanner client-header < $< > $@

$(BUILD_DIR)/%-protocol.o: $(BUILD_DIR)/%-protocol.c $(BUILD_DIR)/%-protocol.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

COMMON_SRC = src/common/ipc.c src/common/config.c
HANDLERS_SRC = src/handlers/wallpaper.c src/handlers/image_handler.c
DAEMON_SRC = src/daemon/main.c src/daemon/daemon.c
CLIENT_SRC = src/client/main.c
WAYLAND_SRC = src/wayland_state.c src/wayland_dispatch.c src/wl_shm_pool.c

DAEMON_OBJ = $(COMMON_SRC) $(HANDLERS_SRC) $(WAYLAND_SRC) $(DAEMON_SRC)
DAEMON_OBJ := $(addprefix $(BUILD_DIR)/, $(DAEMON_SRC:src/%=%.o) $(COMMON_SRC:src/%=%.o) $(HANDLERS_SRC:src/%=%.o) $(WAYLAND_SRC:src/%=%.o))

CLIENT_OBJ = $(COMMON_SRC) $(CLIENT_SRC)
CLIENT_OBJ := $(addprefix $(BUILD_DIR)/, $(CLIENT_SRC:src/%=%.o) $(COMMON_SRC:src/%=%.o))

$(BUILD_DIR)/src/client/%.o: src/client/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/src/daemon/%.o: src/daemon/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/src/common/%.o: src/common/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/src/handlers/%.o: src/handlers/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/src/%.o: src/%.c $(PROTOCOL_H) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

zpaperd: $(BUILD_DIR)/src/daemon/main.o $(BUILD_DIR)/src/daemon/daemon.o $(BUILD_DIR)/src/common/ipc.o $(BUILD_DIR)/src/common/config.o $(BUILD_DIR)/src/handlers/wallpaper.o $(BUILD_DIR)/src/handlers/image_handler.o $(BUILD_DIR)/src/wayland_state.o $(BUILD_DIR)/src/wayland_dispatch.o $(BUILD_DIR)/src/wl_shm_pool.o $(PROTOCOL_C:.c=.o)
	$(CC) $^ $(LIBS) -o $@
	@echo "Built: zpaperd"

zpaper: $(BUILD_DIR)/src/client/main.o $(BUILD_DIR)/src/common/ipc.o $(PROTOCOL_C:.c=.o)
	$(CC) $^ $(LIBS) -o $@
	@echo "Built: zpaper"

clean:
	@rm -f $(TARGETS)
	@rm -rf $(BUILD_DIR)
	@echo "Clean complete"

-include $(OBJ:.o=.d) $(DAEMON_OBJ:.o=.d) $(CLIENT_OBJ:.o=.d)

install: $(TARGETS)
	@install -Dm755 zpaper $(DESTDIR)/usr/local/bin/zpaper
	@install -Dm755 zpaperd $(DESTDIR)/usr/local/bin/zpaperd
	@echo "Installed to $(DESTDIR)/usr/local/bin/"

uninstall:
	@rm -f $(DESTDIR)/usr/local/bin/zpaper
	@rm -f $(DESTDIR)/usr/local/bin/zpaperd
	@echo "Uninstalled"
