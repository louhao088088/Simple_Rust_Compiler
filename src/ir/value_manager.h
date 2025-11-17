#pragma once
#include <string>
#include <unordered_map>
#include <vector>

/**
 * VariableInfo - 变量信息
 *
 * 存储变量在IR中的表示和元数据
 * 注意：使用纯字符串表示，不依赖LLVM C++ API
 */
struct VariableInfo {
    std::string alloca_name; // IR中的变量名（如 "%x.addr", "%0", "@global_var"）
    std::string type_str;    // IR类型字符串（如 "i32", "i32*", "[10 x i32]"）
    bool is_mutable;         // 是否可变（Rust语义）
    bool is_parameter;       // 是否是函数参数
    bool is_global;          // 是否是全局变量

    VariableInfo() : is_mutable(false), is_parameter(false), is_global(false) {}

    VariableInfo(const std::string &alloca, const std::string &type, bool mut, bool param = false,
                 bool global = false)
        : alloca_name(alloca), type_str(type), is_mutable(mut), is_parameter(param),
          is_global(global) {}
};

/**
 * ValueManager - 变量和值管理器
 *
 * 核心职责:
 * 1. 管理变量作用域栈
 * 2. 变量名到IR变量的映射（纯字符串）
 * 3. 支持变量遮蔽(shadowing)
 * 4. 检测重复定义
 * 5. 处理局部变量、函数参数和全局变量
 *
 * 设计原则：
 * - 不使用LLVM C++ API
 * - 所有IR表示都是字符串
 * - 与IREmitter和TypeMapper协作
 */
class ValueManager {
  public:
    ValueManager();

    // ========== 作用域管理 ==========

    /**
     * 进入新作用域
     * 例如：函数体、代码块、循环体
     */
    void enter_scope();

    /**
     * 退出当前作用域
     * 注意：不能退出全局作用域
     */
    void exit_scope();

    /**
     * 获取当前作用域深度
     * @return 0表示全局作用域，1表示第一层嵌套，以此类推
     */
    size_t scope_depth() const;

    // ========== 变量操作 ==========

    /**
     * 在当前作用域定义局部变量
     * @param name 源代码中的变量名
     * @param alloca_name IR中的alloca变量名（如 "%0"）
     * @param type_str IR类型字符串（如 "i32"）
     * @param is_mutable 是否可变
     */
    void define_variable(const std::string &name, const std::string &alloca_name,
                         const std::string &type_str, bool is_mutable);

    /**
     * 定义函数参数
     * 函数参数在IR中是函数参数（如 %a），不是alloca结果
     * @param name 参数名
     * @param param_name IR中的参数名（如 "%a"）
     * @param type_str IR类型字符串
     * @param is_mutable 是否可变（Rust中参数默认不可变）
     */
    void define_parameter(const std::string &name, const std::string &param_name,
                          const std::string &type_str, bool is_mutable = false);

    /**
     * 定义全局变量
     * 全局变量在IR中使用@前缀（如 @global_var）
     * @param name 变量名
     * @param global_name IR中的全局变量名（如 "@global_var"）
     * @param type_str IR类型字符串
     * @param is_mutable 是否可变（Rust中需要用static mut）
     */
    void define_global(const std::string &name, const std::string &global_name,
                       const std::string &type_str, bool is_mutable = false);

    /**
     * 查找变量（从当前作用域向外层查找）
     * @param name 源代码中的变量名
     * @return 变量信息指针，未找到返回nullptr
     */
    VariableInfo *lookup_variable(const std::string &name);

    /**
     * 查找变量（const版本）
     */
    const VariableInfo *lookup_variable(const std::string &name) const;

    /**
     * 检查当前作用域是否已定义该变量
     * 用于检测重复定义（不包括shadowing）
     * @param name 源代码中的变量名
     * @return true表示当前作用域已定义
     */
    bool is_defined_in_current_scope(const std::string &name) const;

    /**
     * 检查变量是否存在（在任何作用域）
     * @param name 源代码中的变量名
     * @return true表示变量存在
     */
    bool variable_exists(const std::string &name) const;

    /**
     * 查找当前作用域的变量（不向外层查找）
     * @param name 源代码中的变量名
     * @return 变量信息指针，未找到返回nullptr
     */
    VariableInfo *lookup_variable_in_current_scope(const std::string &name);

    // ========== 调试和辅助 ==========

    /**
     * 获取当前作用域的所有变量名（用于调试）
     */
    std::vector<std::string> get_current_scope_variables() const;

    /**
     * 清空所有作用域（用于测试）
     */
    void clear();

  private:
    /**
     * Scope - 单个作用域
     * 存储该作用域内定义的所有变量
     */
    struct Scope {
        std::unordered_map<std::string, VariableInfo> variables;
    };

    std::vector<Scope> scope_stack_; // 作用域栈
};
