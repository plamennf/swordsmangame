#pragma once

#include "hash_table.h"

enum Variable_Type {
    VARIABLE_TYPE_INT,
    VARIABLE_TYPE_FLOAT,
    VARIABLE_TYPE_DOUBLE,
    VARIABLE_TYPE_BOOL,
};

struct Variable_Binding {
    Variable_Type type;
    void *value_pointer;
};

struct Variable_Service {
    u64 modtime = 0;
    String_Hash_Table <Variable_Binding> value_lookup;
    
    void attach(char *name, int *value);
    void attach(char *name, float *value);
    void attach(char *name, double *value);
    void attach(char *name, bool *value);
};

bool load_vars_file(Variable_Service *service, char *filepath);
