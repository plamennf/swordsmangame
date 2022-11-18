#include "pch.h"
#include "variable_service.h"
#include "text_file_handler.h"
#include "array.h"

void Variable_Service::attach(char *name, int *value) {
    Variable_Binding binding;
    binding.type = VARIABLE_TYPE_INT;
    binding.value_pointer = (void *)value;
    value_lookup.add(name, binding);
}

void Variable_Service::attach(char *name, float *value) {
    Variable_Binding binding;
    binding.type = VARIABLE_TYPE_FLOAT;
    binding.value_pointer = (void *)value;
    value_lookup.add(name, binding);
}

void Variable_Service::attach(char *name, double *value) {
    Variable_Binding binding;
    binding.type = VARIABLE_TYPE_DOUBLE;
    binding.value_pointer = (void *)value;
    value_lookup.add(name, binding);
}

void Variable_Service::attach(char *name, bool *value) {
    Variable_Binding binding;
    binding.type = VARIABLE_TYPE_BOOL;
    binding.value_pointer = (void *)value;
    value_lookup.add(name, binding);
}

bool load_vars_file(Variable_Service *service, char *filepath) {
    Text_File_Handler handler;
    handler.start_file(filepath, filepath, "load_vars_file");
    if (handler.failed) return false;

    while (true) {
        char *line = handler.consume_next_line();
        if (!line) break;

        char *at = line;
        
        Array <char> variable_name;
        variable_name.use_temporary_storage = true;
        while (at[0] != ' ') {
            if (!*at) return false;
            variable_name.add(*at);
            at++;
        }
        variable_name.add(0);

        at = eat_spaces(at);

        Array <char> variable_value;
        variable_value.use_temporary_storage = true;
        while (at[0]) {
            variable_value.add(*at);
            at++;
        }
        variable_value.add(0);

        Variable_Binding *binding_pointer = service->value_lookup.find(variable_name.data);
        if (!binding_pointer) continue;

        if (binding_pointer->type == VARIABLE_TYPE_INT) {
            int *ptr = (int *)binding_pointer->value_pointer;
            *ptr = atoi(variable_value.data);
        } else if (binding_pointer->type == VARIABLE_TYPE_FLOAT) {
            float *ptr = (float *)binding_pointer->value_pointer;
            *ptr = (float)atof(variable_value.data);
        } else if (binding_pointer->type == VARIABLE_TYPE_DOUBLE) {
            double *ptr = (double *)binding_pointer->value_pointer;
            *ptr = atof(variable_value.data);
        } else if (binding_pointer->type == VARIABLE_TYPE_BOOL) {
            bool *ptr = (bool *)binding_pointer->value_pointer;
            
            variable_value.data = lowercase(variable_value.data);
            if (strings_match(variable_value.data, "true") || strings_match(variable_value.data, "1")) {
                *ptr = true;
            } else if (strings_match(variable_value.data, "false") || strings_match(variable_value.data, "0")) {
                *ptr = false;
            }
        }
    }

    return true;
}
