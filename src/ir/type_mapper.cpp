#include "type_mapper.h"

#include <sstream>

TypeMapper::TypeMapper(BuiltinTypes &builtin_types) : builtin_types_(builtin_types) {}

std::string TypeMapper::map(const Type *rust_type) {
    if (!rust_type) {
        return "void";
    }

    auto it = type_cache_.find(rust_type);
    if (it != type_cache_.end()) {
        return it->second;
    }

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
        ir_type = "i32";
        break;
    }

    type_cache_[rust_type] = ir_type;

    return ir_type;
}

std::string TypeMapper::map_primitive(const PrimitiveType *type) {
    switch (type->kind) {
    case TypeKind::I32:
    case TypeKind::U32:
    case TypeKind::ISIZE:
    case TypeKind::USIZE:
        return "i32";

    case TypeKind::BOOL:
        return "i1";

    case TypeKind::CHAR:
        return "i32";

    case TypeKind::ANY_INTEGER:
        return "i32";

    default:
        return "i32";
    }
}

std::string TypeMapper::map_array(const ArrayType *type) {
    std::string elem_type = map(type->element_type.get());
    return "[" + std::to_string(type->size) + " x " + elem_type + "]";
}

std::string TypeMapper::map_struct(const StructType *type) {
    return "%" + type->name;
}

std::string TypeMapper::map_function(const FunctionType *type) {
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
    std::string pointee_type = map(type->referenced_type.get());
    return pointee_type + "*";
}

std::string TypeMapper::map_raw_pointer(const RawPointerType *type) {
    std::string pointee_type = map(type->pointee_type.get());
    return pointee_type + "*";
}

std::string TypeMapper::map_enum(const EnumType *type) {
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
        return "0";

    default:
        return "0";
    }
}

std::string TypeMapper::declare_struct_type(const StructType *type) {
    if (declared_structs_[type->name]) {
        return "";
    }

    declared_structs_[type->name] = true;


    std::stringstream ss;
    ss << "%" << type->name << " = type { ";

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
