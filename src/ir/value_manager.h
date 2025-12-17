#pragma once
#include <string>
#include <unordered_map>
#include <vector>

/**
 * VariableInfo - Variable information
 *
 * Stores variable representation and metadata in IR
 */
struct VariableInfo {
    std::string alloca_name;
    std::string type_str;
    bool is_mutable;
    bool is_parameter;
    bool is_global;

    VariableInfo() : is_mutable(false), is_parameter(false), is_global(false) {}

    VariableInfo(const std::string &alloca, const std::string &type, bool mut, bool param = false,
                 bool global = false)
        : alloca_name(alloca), type_str(type), is_mutable(mut), is_parameter(param),
          is_global(global) {}
};

/**
 * ValueManager - Variable and value manager
 *
 * Core responsibilities:
 * 1. Manage variable scope stack
 * 2. Variable name to IR variable mapping (pure strings)
 * 3. Support variable shadowing
 * 4. Detect duplicate definitions
 * 5. Handle local variables, function parameters, and global variables
 *
 * Design principles:
 * - Do not use LLVM C++ API
 * - All IR representations are strings
 * - Cooperate with IREmitter and TypeMapper
 */
class ValueManager {
  public:
    ValueManager();

    /**
     * Enter new scope
     * E.g., function body, code block, loop body
     */
    void enter_scope();

    /**
     * Exit current scope
     * Note: Cannot exit global scope
     */
    void exit_scope();

    /**
     * Get current scope depth
     * @return 0 for global scope, 1 for first nested level, and so on
     */
    size_t scope_depth() const;

    /**
     * Define local variable in current scope
     * @param name Variable name in source code
     * @param alloca_name Alloca variable name in IR (e.g., "%0")
     * @param type_str IR type string (e.g., "i32")
     * @param is_mutable Whether mutable
     */
    void define_variable(const std::string &name, const std::string &alloca_name,
                         const std::string &type_str, bool is_mutable);

    /**
     * Define function parameter
     * Function parameters in IR are function arguments (e.g., %a), not alloca results
     * @param name Parameter name
     * @param param_name Parameter name in IR (e.g., "%a")
     * @param type_str IR type string
     * @param is_mutable Whether mutable (parameters are immutable by default in Rust)
     */
    void define_parameter(const std::string &name, const std::string &param_name,
                          const std::string &type_str, bool is_mutable = false);

    /**
     * Define global variable
     * Global variables use @ prefix in IR (e.g., @global_var)
     * @param name Variable name
     * @param global_name Global variable name in IR (e.g., "@global_var")
     * @param type_str IR type string
     * @param is_mutable Whether mutable (requires static mut in Rust)
     */
    void define_global(const std::string &name, const std::string &global_name,
                       const std::string &type_str, bool is_mutable = false);

    /**
     * Lookup variable (searches from current scope outward)
     * @param name Variable name in source code
     * @return Pointer to variable info, nullptr if not found
     */
    VariableInfo *lookup_variable(const std::string &name);

    /**
     * Lookup variable (const version)
     */
    const VariableInfo *lookup_variable(const std::string &name) const;

    /**
     * Check if variable is defined in current scope
     * Used to detect duplicate definitions (not including shadowing)
     * @param name Variable name in source code
     * @return true if already defined in current scope
     */
    bool is_defined_in_current_scope(const std::string &name) const;

    /**
     * Check if variable exists (in any scope)
     * @param name Variable name in source code
     * @return true if variable exists
     */
    bool variable_exists(const std::string &name) const;

    /**
     * Lookup variable in current scope only (does not search outer scopes)
     * @param name Variable name in source code
     * @return Pointer to variable info, nullptr if not found
     */
    VariableInfo *lookup_variable_in_current_scope(const std::string &name);

    /**
     * Get all variable names in current scope (for debugging)
     */
    std::vector<std::string> get_current_scope_variables() const;

    /**
     * Clear all scopes (for testing)
     */
    void clear();

  private:
    /**
     * Scope - Single scope
     * Stores all variables defined in this scope
     */
    struct Scope {
        std::unordered_map<std::string, VariableInfo> variables;
    };

    std::vector<Scope> scope_stack_;
};
