# 编译器设置
CXX      := g++
CXXFLAGS := -std=c++20 -Wall -O2 -g
LDFLAGS  := 

# 如果需要开启 Sanitize (内存泄露检测等)，取消下面两行的注释
# CXXFLAGS += -fsanitize=address,leak,undefined
# LDFLAGS  += -fsanitize=address,leak,undefined

# 目标可执行文件名
TARGET := code

# 构建目录 (中间文件存放位置)
BUILD_DIR := build

# 源文件列表 (与你的 CMakeLists 保持一致)
SRCS := src/pre_processor/pre_processor.cpp \
        src/lexer/lexer.cpp \
        src/ast/ast.cpp \
        src/parser/parser.cpp \
        src/semantic/semantic.cpp \
        src/semantic/name_resolution.cpp \
        src/semantic/type_check.cpp \
        src/semantic/type_resolve.cpp \
        src/semantic/const_evaluate.cpp \
        src/ir/ir_generator_main.cpp \
        src/ir/ir_generator_statements.cpp \
        src/ir/ir_generator_expressions.cpp \
        src/ir/ir_generator_control_flow.cpp \
        src/ir/ir_generator_complex_exprs.cpp \
        src/ir/ir_generator_builtins.cpp \
        src/ir/ir_generator_helpers.cpp \
        src/ir/ir_emitter.cpp \
        src/ir/type_mapper.cpp \
        src/ir/value_manager.cpp \
        src/tool/number.cpp \
        src/error/error.cpp \
        src/main.cpp

# 自动生成对象文件路径 (例如: src/main.cpp -> build/src/main.o)
OBJS := $(SRCS:%.cpp=$(BUILD_DIR)/%.o)
# 自动生成依赖文件路径 (.d 文件)
DEPS := $(OBJS:%.o=%.d)

# 默认目标
all: $(TARGET)

# 链接步骤
$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	@$(CXX) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build successful!"

# 编译步骤 (通用规则)
# -MMD -MP 用于自动生成头文件依赖关系
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# 清理规则
clean:
	@echo "Cleaning up..."
	@rm -rf $(BUILD_DIR) $(TARGET)

# --- 新增部分 ---
# 定义 build 目标，让它直接去执行 all 目标
build: all
# ----------------

# 包含自动生成的依赖文件
-include $(DEPS)

# 修改 .PHONY，把 build 也加进去，防止目录下有名为 build 的文件导致冲突
.PHONY: all clean build