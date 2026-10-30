#include <shared.h>

uint32_t Schema_t::GetOffset(const char* module_name, const char* class_name, const char* field_name) {
    uint32_t res = 0;

    CSchemaTypeScope* type_scope = SchemaSystem->FindTypeScopeForModule(module_name);
    if (!type_scope)
        return res;

    CSchemaDeclaredClass* declared_class = nullptr;
    if (!this->FindDeclaredClass(type_scope, class_name, &declared_class) || !declared_class)
        return res;

    CSchemaClassInfo* schema_class = declared_class->class_info;
    if (!schema_class || !schema_class->fields)
        return res;

    for (uint16_t i = 0; i < schema_class->num_fields; ++i) {
        CSchemaClassField& field = schema_class->fields[i];
        if (field.name && std::strcmp(field.name, field_name) == 0) {
            res = field.offset;
            break;
        }
    }

    return res;
}

bool Schema_t::FindDeclaredClass(CSchemaTypeScope* type_scope, const char* class_name, CSchemaDeclaredClass** out_class) {
    if (!type_scope || !class_name || !out_class)
        return false;

    for (uint16_t i = 0; i < type_scope->num_declared_classes; ++i) {
        CSchemaDeclaredClass* declared_class = type_scope->declared_classes[i].declared_class;
        if (declared_class && declared_class->name && strcmp(declared_class->name, class_name) == 0) {
            *out_class = declared_class;
            return true;
        }
    }

    return false;
}