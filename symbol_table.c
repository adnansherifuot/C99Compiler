#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"

#define MAX_SCOPE_DEPTH 100

// A stack of symbol tables for scope management
Symbol* scope_stack[MAX_SCOPE_DEPTH];
int current_scope_level = -1; // -1 means no scope is active

void init_symbol_table() {
    current_scope_level = -1;
    enter_scope(); // Enter the global scope (level 0)
}

void insert_symbol(const char *name, Type *type, int kind) {
    //printf("DEBUG:Semantic Check: Inserting symbol '%s' of kind %d into scope level %d\n", name, kind, current_scope_level);
    if (current_scope_level < 0) return; // Should not happen

    // Check for re-declaration
    for (Symbol *s = scope_stack[current_scope_level]; s != NULL; s = s->next) {
        if (strcmp(s->name, name) == 0) {
            // In a real compiler, you'd use yyerror here with line numbers
            fprintf(stderr, "Semantic Error: Redeclaration of identifier '%s'.\n", name);
            return;
        }
    }

    Symbol *new_symbol = (Symbol*) malloc(sizeof(Symbol));
    if (!new_symbol) {
        fprintf(stderr, "Fatal: Out of memory for new symbol\n");
        exit(1);
    }
    new_symbol->name = strdup(name);
    new_symbol->kind = kind;
    new_symbol->type = type; // Assign the pointer to the complex type
    new_symbol->next = scope_stack[current_scope_level];
    scope_stack[current_scope_level] = new_symbol;
}

Symbol* lookup_symbol(const char *name) {
    // Search from the current scope down to the global scope
    //printf("DEBUG:Semantic Check: Looking up symbol '%s' in scope level %d\n", name, current_scope_level);
    for (int i = current_scope_level; i >= 0; i--) {
        for (Symbol *s = scope_stack[i]; s != NULL; s = s->next) {
            if (strcmp(s->name, name) == 0) {
                //printf("DEBUG:Semantic Check: Found symbol '%s' in scope level %d\n", name, i);
                return s; // Found it!
            }
        }
    }
    return NULL; // Not found in any scope
}

void enter_scope() {
    if (current_scope_level < MAX_SCOPE_DEPTH - 1) {
        current_scope_level++;
        scope_stack[current_scope_level] = NULL;
    } else {
        fprintf(stderr, "Fatal: Maximum scope depth exceeded.\n");
        exit(1);
    }
}

void leave_scope() {
     if (current_scope_level > 0) { // Don't leave the global scope
         // Free all symbols in the current scope
         Symbol *current = scope_stack[current_scope_level];
         while (current) {
             Symbol *temp = current;
             current = current->next;
             free(temp->name);
             // Types are owned by the symbol table and will be freed in cleanup_symbol_table.
             free(temp);
         }
         scope_stack[current_scope_level] = NULL;
         current_scope_level--;
    }
}

void add_parameters_to_scope(Symbol* params) {
    Symbol* current = params;
    while (current) {
        // We insert the symbol into the current scope.
        // The function type and the scope will now both point to this parameter info.
        //printf("DEBUG:Semantic Check: Inserting parameter '%s' into scope\n", current->name);
        insert_symbol(current->name, current->type, current->kind);
        current = current->next;
    }
}

void cleanup_symbol_table() {
    for (int i = 0; i <= current_scope_level; i++) {
        Symbol *current = scope_stack[i];
        while (current) {
            Symbol *temp = current;
            current = current->next;
            
            // Free the type only if it's a variable or function.
            // Typedefs often point to types owned by other symbols (like structs),
            // so we avoid double-freeing. A more robust system might use reference counting.
            if (temp->kind == SYM_VARIABLE || temp->kind == SYM_FUNCTION) {
                free_type(temp->type);
            }
            free(temp->name);
            free(temp);
        }
        scope_stack[i] = NULL;
    }
    current_scope_level = -1;
}

void print_struct_members(Type* struct_type) {
    if (!struct_type || (struct_type->kind != TYPE_STRUCT && struct_type->kind != TYPE_UNION)) {
        printf("Not a struct or union type.\n");
        return;
    }

    StructMember* member = struct_type->data.record_info.members;
    printf("\n\t\t\tMembers of %s:\n", struct_type->data.record_info.name ? struct_type->data.record_info.name : "(anonymous)");
    while (member) {
        printf("\t\t\t  Name: %s, Type: ", member->name);
        print_type(member->type);
        printf(", Offset: %d bytes\n", member->offset);
        member = member->next;
    }
}
void print_type(Type *type) {
    if (!type) {
        printf("(unknown type)");
        return;
    }
    switch (type->kind) {
        case TYPE_BASE:
            printf("%s", type->data.base_name);
            break;
        case TYPE_POINTER:
            print_type(type->data.points_to);
            printf("*");
            break;
        case TYPE_FUNCTION:
            printf("function(");
            Symbol *param = type->data.function_info.params;
            if (param == NULL) {
                printf("void");
            } else {
                while (param) {
                    print_type(param->type);
                    if (param->name && strlen(param->name) > 0) {
                        printf(" %s", param->name);
                    }
                    param = param->next;
                    if (param) {
                        printf(", ");
                    }
                }
            }
            printf(") returning ");
            print_type(type->data.function_info.return_type);
            break;
        case TYPE_ARRAY:
            print_type(type->data.array_info.element_type);
            printf("[%d]", type->data.array_info.size);
            break;
        case TYPE_STRUCT:
            printf("struct");
            if (type->data.record_info.name) {
                printf(" %s", type->data.record_info.name);
            }else {
                printf(" (anonymous)");
            }
            printf(" with size [%d] bytes",type->size);
            print_struct_members(type);
            break;
        case TYPE_UNION:
            printf("union");
            if (type->data.record_info.name) {
                printf(" %s", type->data.record_info.name);
            }
            printf(" with size [%d] bytes",type->size);
            print_struct_members(type);
            break;
        default:
            printf("(complex type)");
    }
}
void print_symbol_table() {
    printf("\n--- Symbol Table Contents ---\n");
    for (int i = 0; i <= current_scope_level; i++) {
        printf("--- Scope Level %d ---\n", i);
        if (scope_stack[i] == NULL) {
            printf("  (empty)\n");
        } else {
            Symbol *s = scope_stack[i];
            while (s != NULL) {
                printf("  Name: %-15s, Kind: %s", s->name,
                       (s->kind == SYM_VARIABLE) ? "VARIABLE" : 
                       (s->kind == SYM_FUNCTION) ? "FUNCTION" : 
                       (s->kind == SYM_TYPENAME) ? "TYPENAME" : "UNKNOWN");
                printf(", Type: ");
                print_type(s->type);
                printf("\n");
                s = s->next;
            }
        }
    }
    printf("-----------------------------\n");
}

int get_type_size(Type* type) {
    if (!type) return 0;
    if (type->size > 0) return type->size; // Return cached size if available

    switch (type->kind) {
        case TYPE_BASE:
            if (strcmp(type->data.base_name, "char") == 0) return 1;
            if (strcmp(type->data.base_name, "short") == 0) return 2;
            if (strcmp(type->data.base_name, "int") == 0) return 4;
            if (strcmp(type->data.base_name, "long") == 0) return 8;
            if (strcmp(type->data.base_name, "float") == 0) return 4;
            if (strcmp(type->data.base_name, "double") == 0) return 8;
            return 4; // Default
        case TYPE_POINTER:
            return 8; // Assuming a 64-bit architecture
        case TYPE_STRUCT:
        case TYPE_UNION:
            // The size is calculated and set in calculate_struct_layout during parsing.
            // If type->size is 0 or less, it means the struct/union is only declared (e.g., "struct T;")
            // but not yet defined, which is a forward declaration.
            return type->size;
        case TYPE_ARRAY:
             return type->data.array_info.size * get_type_size(type->data.array_info.element_type);
        default:
            return 0;
    }
}

/* --- Type Management Functions --- */

Type* create_base_type(const char *base_name) {
    Type *new_type = (Type*) malloc(sizeof(Type));
    new_type->kind = TYPE_BASE;
    new_type->data.base_name = strdup(base_name);
    return new_type;
}

Type* create_array_type(Type *element_type, int size) {
    Type *new_type = (Type*) malloc(sizeof(Type));
    new_type->kind = TYPE_ARRAY;
    new_type->data.array_info.element_type = element_type;
    new_type->data.array_info.size = size;
    return new_type;
}

Type* create_aggregate_type(int kind, const char *name) {
    Type *new_type = (Type*) malloc(sizeof(Type));
    new_type->kind = kind; // TYPE_STRUCT or TYPE_UNION
    new_type->data.record_info.name = name ? strdup(name) : NULL;
    new_type->data.record_info.members = NULL;
    new_type->size = 0; // Initialize size to 0, indicating an incomplete type.
    return new_type;
}

Type* create_pointer_type(Type *points_to) {
    Type *new_type = (Type*) malloc(sizeof(Type));
    new_type->kind = TYPE_POINTER;
    new_type->data.points_to = points_to;
    return new_type;
}

Type* create_function_type(Type *return_type, Symbol *params) {
    Type *new_type = (Type*) malloc(sizeof(Type));
    new_type->kind = TYPE_FUNCTION;
    new_type->data.function_info.return_type = return_type;
    new_type->data.function_info.params = params;
    return new_type;
}

void free_type(Type *type) {
    if (!type) return;

    switch (type->kind) {
        case TYPE_BASE:
            free(type->data.base_name);
            break;
        case TYPE_ARRAY:
            // Recursively free the element type
            free_type(type->data.array_info.element_type);
            break;
        case TYPE_POINTER:
            // Recursively free the pointed-to type
            free_type(type->data.points_to);
            break;
        case TYPE_FUNCTION:
            free_type(type->data.function_info.return_type);
            Symbol *param = type->data.function_info.params;
            while (param) {
                Symbol *temp = param;
                param = param->next;
                free(temp->name);
                free_type(temp->type);
                free(temp);
            }
            break;
        case TYPE_STRUCT:
        case TYPE_UNION:
            if (type->data.record_info.name) free(type->data.record_info.name);
            StructMember *member = type->data.record_info.members;
            while (member) {
                StructMember *temp = member;
                member = member->next;
                free(temp->name);
                free_type(temp->type);
                free(temp);
            }
            break;
    }
    free(type);
}

/**
 * @brief Finds a member by name within a struct or union type.
 * @param struct_type The struct or union type to search in.
 * @param member_name The name of the member to find.
 * @return A pointer to the StructMember if found, otherwise NULL.
 */
StructMember* get_struct_member(Type* struct_type, const char* member_name) {
    if (!struct_type || (struct_type->kind != TYPE_STRUCT && struct_type->kind != TYPE_UNION)) {
        return NULL;
    }

    StructMember* current_member = struct_type->data.record_info.members;
    while (current_member) {
        if (strcmp(current_member->name, member_name) == 0) {
            return current_member;
        }
        current_member = current_member->next;
    }

    return NULL; // Member not found
}