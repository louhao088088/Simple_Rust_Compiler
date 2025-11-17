#include "value_manager.h"

ValueManager::ValueManager() {
    // 创建全局作用域
    enter_scope();
}

void ValueManager::enter_scope() { scope_stack_.emplace_back(); }

void ValueManager::exit_scope() {
    // 保留全局作用域，至少保持一个作用域
    if (scope_stack_.size() > 1) {
        scope_stack_.pop_back();
    }
    // TODO: 是否需要在退出作用域时做清理工作？
    // 例如：记录日志、验证所有变量都被使用等
}

size_t ValueManager::scope_depth() const {
    // 全局作用域深度为0
    if (scope_stack_.empty()) {
        return 0;
    }
    return scope_stack_.size() - 1;
}

void ValueManager::define_variable(const std::string &name, const std::string &alloca_name,
                                   const std::string &type_str, bool is_mutable) {
    if (scope_stack_.empty()) {
        // 不应该发生，构造函数已经创建了全局作用域
        return;
    }

    VariableInfo info(alloca_name, type_str, is_mutable, false, false);
    scope_stack_.back().variables[name] = info;
}

void ValueManager::define_parameter(const std::string &name, const std::string &param_name,
                                    const std::string &type_str, bool is_mutable) {
    if (scope_stack_.empty()) {
        return;
    }

    // TODO: 函数参数的处理策略
    // 选项1: 直接使用参数名（如 %a），在使用时不需要load
    // 选项2: 在函数entry创建alloca，store参数值，后续统一处理
    // 当前采用选项1，标记is_parameter=true以便区分处理

    VariableInfo info(param_name, type_str, is_mutable, true, false);
    scope_stack_.back().variables[name] = info;
}

void ValueManager::define_global(const std::string &name, const std::string &global_name,
                                 const std::string &type_str, bool is_mutable) {
    if (scope_stack_.empty()) {
        return;
    }

    // 全局变量定义在全局作用域（索引0）
    // 注意：global_name应该以@开头（如 "@global_var"）
    VariableInfo info(global_name, type_str, is_mutable, false, true);
    scope_stack_[0].variables[name] = info;
}

VariableInfo *ValueManager::lookup_variable(const std::string &name) {
    // 从最内层向外层查找
    for (auto it = scope_stack_.rbegin(); it != scope_stack_.rend(); ++it) {
        auto var_it = it->variables.find(name);
        if (var_it != it->variables.end()) {
            return &var_it->second;
        }
    }
    return nullptr; // 未找到
}

const VariableInfo *ValueManager::lookup_variable(const std::string &name) const {
    for (auto it = scope_stack_.rbegin(); it != scope_stack_.rend(); ++it) {
        auto var_it = it->variables.find(name);
        if (var_it != it->variables.end()) {
            return &var_it->second;
        }
    }
    return nullptr;
}

bool ValueManager::is_defined_in_current_scope(const std::string &name) const {
    if (scope_stack_.empty()) {
        return false;
    }

    return scope_stack_.back().variables.find(name) != scope_stack_.back().variables.end();
}

VariableInfo *ValueManager::lookup_variable_in_current_scope(const std::string &name) {
    if (scope_stack_.empty()) {
        return nullptr;
    }

    auto it = scope_stack_.back().variables.find(name);
    if (it != scope_stack_.back().variables.end()) {
        return &it->second;
    }
    return nullptr;
}

bool ValueManager::variable_exists(const std::string &name) const {
    return lookup_variable(name) != nullptr;
}

std::vector<std::string> ValueManager::get_current_scope_variables() const {
    std::vector<std::string> result;
    if (!scope_stack_.empty()) {
        for (const auto &[name, _] : scope_stack_.back().variables) {
            result.push_back(name);
        }
    }
    return result;
}

void ValueManager::clear() {
    scope_stack_.clear();
    enter_scope(); // 重新创建全局作用域
}
