#include "type_mapper.h"

#include <sstream>

TypeMapper::TypeMapper(BuiltinTypes &builtin_types) : builtin_types_(builtin_types) {}

std::string TypeMapper::map(const Type *rust_type) {
    if (!rust_type) {
        return "void";
    }

    // 1. 检查缓存
    auto it = type_cache_.find(rust_type);
    if (it != type_cache_.end()) {
        return it->second;
    }

    // 2. 根据类型种类分发
    std::string ir_type;

    switch (rust_type->kind) {
    case TypeKind::I32:
    case TypeKind::U32:
    case TypeKind::ISIZE:
    case TypeKind::USIZE:
    case TypeKind::BOOL:
    case TypeKind::CHAR:
    case TypeKind::ANY_INTEGER:
        ir_type = map_primitive(static_cast<const PrimitiveType *>(rust_type));
        break;

    case TypeKind::STR:
    case TypeKind::STRING:
    case TypeKind::RSTRING:
    case TypeKind::CSTRING:
    case TypeKind::RCSTRING:
        // TODO: 字符串类型暂时映射为i8*，后续可能需要实现为胖指针 { i8*, i64 }
        ir_type = "i8*";
        break;

    case TypeKind::ARRAY:
        ir_type = map_array(static_cast<const ArrayType *>(rust_type));
        break;

    case TypeKind::STRUCT:
        ir_type = map_struct(static_cast<const StructType *>(rust_type));
        break;

    case TypeKind::FUNCTION:
        ir_type = map_function(static_cast<const FunctionType *>(rust_type));
        break;

    case TypeKind::REFERENCE:
        ir_type = map_reference(static_cast<const ReferenceType *>(rust_type));
        break;

    case TypeKind::RAW_POINTER:
        ir_type = map_raw_pointer(static_cast<const RawPointerType *>(rust_type));
        break;

    case TypeKind::ENUM:
        ir_type = map_enum(static_cast<const EnumType *>(rust_type));
        break;

    case TypeKind::UNIT:
        ir_type = "void";
        break;

    case TypeKind::NEVER:
        ir_type = "void";
        break;

    default:
        // 未知类型，返回默认值
        ir_type = "i32";
        break;
    }

    // 3. 加入缓存
    type_cache_[rust_type] = ir_type;

    return ir_type;
}

std::string TypeMapper::map_primitive(const PrimitiveType *type) {
    switch (type->kind) {
    case TypeKind::I32:
    case TypeKind::U32:
    case TypeKind::ISIZE:
    case TypeKind::USIZE:
        // 32位平台：所有整数类型都是i32
        return "i32";

    case TypeKind::BOOL:
        return "i1";

    case TypeKind::CHAR:
        // Unicode标量值，使用32位
        return "i32";

    case TypeKind::ANY_INTEGER:
        // 任意整数类型默认为i32
        return "i32";

    default:
        return "i32";
    }
}

std::string TypeMapper::map_array(const ArrayType *type) {
    // [T; N] -> [N x T_ir]
    std::string elem_type = map(type->element_type.get());
    return "[" + std::to_string(type->size) + " x " + elem_type + "]";
}

std::string TypeMapper::map_struct(const StructType *type) {
    // struct Point -> %Point
    return "%" + type->name;
}

std::string TypeMapper::map_function(const FunctionType *type) {
    // fn(T1, T2) -> R  =>  R (T1, T2)*
    std::string ret_type = map(type->return_type.get());

    std::stringstream ss;
    ss << ret_type << " (";

    for (size_t i = 0; i < type->param_types.size(); ++i) {
        ss << map(type->param_types[i].get());
        if (i + 1 < type->param_types.size()) {
            ss << ", ";
        }
    }

    ss << ")*"; // 函数指针类型
    return ss.str();
}

std::string TypeMapper::map_reference(const ReferenceType *type) {
    // &T 和 &mut T 都映射为 T*
    std::string pointee_type = map(type->referenced_type.get());
    return pointee_type + "*";
}

std::string TypeMapper::map_raw_pointer(const RawPointerType *type) {
    // *const T 和 *mut T 都映射为 T*
    std::string pointee_type = map(type->pointee_type.get());
    return pointee_type + "*";
}

std::string TypeMapper::map_enum(const EnumType *type) {
    // TODO: 第一阶段简化处理，所有枚举映射为i32
    // 后续需要实现完整的tagged union支持
    //   简单枚举: 直接用i32表示判别式
    //   复杂枚举: 需要生成 { i32, [largest_variant_size x i8] } 结构
    return "i32";
}

std::string TypeMapper::get_zero_value(const Type *type) {
    if (!type) {
        return "void";
    }

    switch (type->kind) {
    case TypeKind::I32:
    case TypeKind::U32:
    case TypeKind::ISIZE:
    case TypeKind::USIZE:
    case TypeKind::CHAR:
    case TypeKind::ANY_INTEGER:
        return "0";

    case TypeKind::BOOL:
        return "false";

    case TypeKind::REFERENCE:
    case TypeKind::RAW_POINTER:
    case TypeKind::STR:
    case TypeKind::STRING:
    case TypeKind::RSTRING:
    case TypeKind::CSTRING:
    case TypeKind::RCSTRING:
        return "null";

    case TypeKind::ARRAY:
        return "zeroinitializer";

    case TypeKind::STRUCT:
        return "zeroinitializer";

    case TypeKind::ENUM:
        // TODO: 枚举的零值需要根据具体定义确定
        return "0";

    default:
        return "0";
    }
}

std::string TypeMapper::declare_struct_type(const StructType *type) {
    // 检查是否已声明
    if (declared_structs_[type->name]) {
        return "";
    }

    declared_structs_[type->name] = true;

    // 注意：此函数目前未使用
    // 实际的结构体声明通过 IRGenerator::visit_struct_decl 完成
    // 该函数使用 StructDecl 的 fields 来保证字段顺序

    // 生成: %Point = type { i32, i32 }
    std::stringstream ss;
    ss << "%" << type->name << " = type { ";

    // 使用 fields map（顺序可能不确定）
    size_t count = 0;
    for (const auto &[field_name, field_type] : type->fields) {
        if (count > 0) {
            ss << ", ";
        }
        ss << map(field_type.get());
        count++;
    }

    ss << " }\n";
    return ss.str();
}
