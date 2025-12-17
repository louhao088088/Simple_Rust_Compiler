#pragma once
#include "../semantic/semantic.h"

#include <string>
#include <unordered_map>

/**
 * TypeMapper - Mapper from Rust types to LLVM IR type strings
 *
 * Core responsibilities:
 * 1. Convert Rust types to LLVM IR type string representation
 * 2. Cache converted types to improve performance
 * 3. Generate struct type definitions
 * 4. Provide zero value representation for types
 */
class TypeMapper {
  public:
    /**
     * Constructor
     * @param builtin_types Reference to built-in types
     */
    explicit TypeMapper(BuiltinTypes &builtin_types);

    /**
     * Main conversion interface: Map Rust type to LLVM IR type string
     * @param rust_type Rust type pointer
     * @return LLVM IR type string (e.g., "i32", "[10 x i32]", "%Point")
     */
    std::string map(const Type *rust_type);

    /**
     * Map primitive type
     * @return IR type string (e.g., "i32", "i1", "i64")
     */
    std::string map_primitive(const PrimitiveType *type);

    /**
     * Map array type
     * @return IR array type (e.g., "[10 x i32]")
     */
    std::string map_array(const ArrayType *type);

    /**
     * Map struct type (returns type reference)
     * @return IR struct type reference (e.g., "%Point")
     */
    std::string map_struct(const StructType *type);

    /**
     * Map function type
     * @return IR function pointer type (e.g., "i32 (i32, i32)*")
     */
    std::string map_function(const FunctionType *type);

    /**
     * Map reference type
     * @return IR pointer type (e.g., "i32*")
     */
    std::string map_reference(const ReferenceType *type);

    /**
     * Map raw pointer type
     * @return IR pointer type (e.g., "i32*")
     */
    std::string map_raw_pointer(const RawPointerType *type);

    /**
     * Map enum type
     * @return IR type (simplified to i32 in first phase)
     */
    std::string map_enum(const EnumType *type);

    /**
     * Get zero value string representation for type
     * @return Zero value string (e.g., "0", "false", "null", "zeroinitializer")
     */
    std::string get_zero_value(const Type *type);

    /**
     * Generate struct type definition string
     * @return Struct definition (e.g., "%Point = type { i32, i32 }\n")
     *         Returns empty string if already declared
     */
    std::string declare_struct_type(const StructType *type);

  private:
    BuiltinTypes &builtin_types_;

    std::unordered_map<const Type *, std::string> type_cache_;

    std::unordered_map<std::string, bool> declared_structs_;
};
