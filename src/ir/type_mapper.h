#pragma once
#include "../semantic/semantic.h"

#include <string>
#include <unordered_map>

/**
 * TypeMapper - Rust类型到LLVM IR类型字符串的映射器
 *
 * 核心职责:
 * 1. 将Rust类型转换为LLVM IR类型字符串表示
 * 2. 缓存已转换的类型以提高性能
 * 3. 生成结构体类型定义
 * 4. 提供类型的零值表示
 */
class TypeMapper {
  public:
    /**
     * 构造函数
     * @param builtin_types 内置类型引用
     */
    explicit TypeMapper(BuiltinTypes &builtin_types);

    /**
     * 主转换接口：将Rust类型映射为LLVM IR类型字符串
     * @param rust_type Rust类型指针
     * @return LLVM IR类型字符串（如"i32", "[10 x i32]", "%Point"）
     */
    std::string map(const Type *rust_type);

    /**
     * 映射基础类型
     * @return IR类型字符串（如"i32", "i1", "i64"）
     */
    std::string map_primitive(const PrimitiveType *type);

    /**
     * 映射数组类型
     * @return IR数组类型（如"[10 x i32]"）
     */
    std::string map_array(const ArrayType *type);

    /**
     * 映射结构体类型（返回类型引用）
     * @return IR结构体类型引用（如"%Point"）
     */
    std::string map_struct(const StructType *type);

    /**
     * 映射函数类型
     * @return IR函数指针类型（如"i32 (i32, i32)*"）
     */
    std::string map_function(const FunctionType *type);

    /**
     * 映射引用类型
     * @return IR指针类型（如"i32*"）
     */
    std::string map_reference(const ReferenceType *type);

    /**
     * 映射原始指针类型
     * @return IR指针类型（如"i32*"）
     */
    std::string map_raw_pointer(const RawPointerType *type);

    /**
     * 映射枚举类型
     * @return IR类型（第一阶段简化为i32）
     */
    std::string map_enum(const EnumType *type);

    /**
     * 获取类型的零值字符串表示
     * @return 零值字符串（如"0", "false", "null", "zeroinitializer"）
     */
    std::string get_zero_value(const Type *type);

    /**
     * 生成结构体类型定义字符串
     * @return 结构体定义（如"%Point = type { i32, i32 }\n"）
     *         如果已声明则返回空字符串
     */
    std::string declare_struct_type(const StructType *type);

  private:
    BuiltinTypes &builtin_types_;

    std::unordered_map<const Type *, std::string> type_cache_;

    std::unordered_map<std::string, bool> declared_structs_;
};
