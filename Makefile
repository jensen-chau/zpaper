TARGET = zpaper
CC = gcc
BUILD_DIR = build
PROTOCOL_DIR = protocol
SRC_DIR = src

# 源文件和对象文件
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(addprefix $(BUILD_DIR)/, $(SRC:$(SRC_DIR)/%.c=%.o))

# Wayland 协议处理
WAYLAND_PROTOCOLS = $(wildcard $(PROTOCOL_DIR)/*.xml)
PROTOCOL_C = $(addprefix $(BUILD_DIR)/, $(WAYLAND_PROTOCOLS:$(PROTOCOL_DIR)/%.xml=%-protocol.c))
PROTOCOL_H = $(addprefix $(BUILD_DIR)/, $(WAYLAND_PROTOCOLS:$(PROTOCOL_DIR)/%.xml=%-protocol.h))

# 头文件路径
INCLUDE = -I $(BUILD_DIR) -I include

# 库
LIBS = -lwayland-client

# 编译标志
FLAGS = -Wall -Werror -Wextra -Wno-unused-parameter -Wno-unused-function
CFLAGS = -g -O0 $(INCLUDE) $(FLAGS) -MMD -MP

# 默认目标
.PHONY: all clean install uninstall

all: $(TARGET) | $(BUILD_DIR)

# 创建构建目录
$(BUILD_DIR):
	@mkdir -p $@

# 处理 Wayland 协议 XML 文件
$(BUILD_DIR)/%-protocol.c: $(PROTOCOL_DIR)/%.xml | $(BUILD_DIR)
	@wayland-scanner private-code < $< > $@

$(BUILD_DIR)/%-protocol.h: $(PROTOCOL_DIR)/%.xml | $(BUILD_DIR)
	@wayland-scanner client-header < $< > $@

# 编译对象文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(PROTOCOL_H) | $(BUILD_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@

# 编译协议对象文件
$(BUILD_DIR)/%-protocol.o: $(BUILD_DIR)/%-protocol.c $(BUILD_DIR)/%-protocol.h | $(BUILD_DIR)
	@$(CC) $(CFLAGS) -c $< -o $@

# 链接最终目标程序
$(TARGET): $(OBJ) $(PROTOCOL_C:.c=.o)
	@$(CC) $^ $(LIBS) -o $@
	@echo "构建成功：$(TARGET)"

# 清理构建产物
clean:
	@rm -rf $(BUILD_DIR)
	@rm -f $(TARGET)
	@echo "清理完成"

# 包含依赖文件
-include $(OBJ:.o=.d)

# 安装到系统目录
install: $(TARGET)
	@install -Dm755 $(TARGET) $(DESTDIR)/usr/local/bin/$(TARGET)
	@echo "已安装到 $(DESTDIR)/usr/local/bin/$(TARGET)"

# 从系统目录卸载
uninstall:
	@rm -f $(DESTDIR)/usr/local/bin/$(TARGET)
	@echo "已卸载 $(DESTDIR)/usr/local/bin/$(TARGET)"
