#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantics.h"

/* --- Semantic Analysis --- */

/**
 * @brief Extracts the base type name from a declaration_specifiers AST node.
 * This is a simplified implementation and assumes the first type specifier is the primary one.
 * e.g., "unsigned int" will return "int". A more robust implementation would combine them.
 */
Type* get_base_type_from_specifiers(ASTNode* specifiers_node) {
    if (!specifiers_node) {
        // printf("DEBUG: No specifiers node provided, defaulting to int type.\n");
        return create_base_type("int"); // Default to int
    }

    // Traverse the list of specifiers to find a type specifier
    ASTNode* current = specifiers_node;
    while(current) {
        ASTNode* specifier_to_check = current;
 
        // If we're looking at a list node, we need to inspect its first child, which holds the actual specifier.
        if (strcmp(current->node_type, "DeclarationSpecifiers") == 0 ||
            strcmp(current->node_type, "SpecifierQualifierList") == 0) {
            if (current->num_children > 0) {
                specifier_to_check = current->children[0];
            }
        }

        // Now, check the actual specifier node (either the list item or the node itself)
        if (strcmp(specifier_to_check->node_type, "TypeSpecifier") == 0) {
            //printf("DEBUG: Found TypeSpecifier for type extraction.\n");
            //printf("DEBUG: Specifier value: %s\n", specifier_to_check->value);
            return create_base_type(specifier_to_check->value);
        } else if (strcmp(specifier_to_check->node_type, "StructSpecifier") == 0) {
            // printf("DEBUG: Found StructSpecifier for type extraction.\n");
            return specifier_to_check->type; // Return the pre-built struct type
        } else if (strcmp(specifier_to_check->node_type, "TypeName") == 0) {
            // printf("DEBUG: Found TypeName for type extraction.\n");
            Symbol* sym = lookup_symbol(specifier_to_check->value);
            if (sym && sym->kind == SYM_TYPENAME) {
                return sym->type; // Return the type associated with the typedef name
            }
            yyerror("Unknown type name");
            // printf("DEBUG: Unknown type name '%s' encountered.\n", specifier_to_check->value);
            return create_base_type("int"); // Error recovery
        }
        if (current->num_children > 1) {
            current = current->children[1];
        } else {
            break;
        }
    }
    return create_base_type("int"); // Default if no type found
}

/**
 * @brief Finds the ParameterList AST node within a function declarator.
 */
ASTNode* get_function_parameters_node(ASTNode* declarator_node) {
    if (!declarator_node) return NULL;

    if (strcmp(declarator_node->node_type, "FunctionDeclarator") == 0) {
        if (declarator_node->num_children > 1) {
            // The parameter list is the second child
            return declarator_node->children[1];
        }
    }
    return NULL;
}

/**
 * @brief Builds a linked list of Symbol structs from a ParameterList AST node.
 */
Symbol* build_parameter_list_from_ast(ASTNode* param_list_node) {
    if (!param_list_node || strcmp(param_list_node->node_type, "EmptyParameterList") == 0) {
        return NULL;
    }

    // Handle the base case: a single parameter declaration.
    if (strcmp(param_list_node->node_type, "ParameterDeclaration") == 0) {
        //printf("DEBUG: Processing ParameterDeclaration\n");
        ASTNode* specifiers = param_list_node->children[0];
        Type* param_type = get_base_type_from_specifiers(specifiers);

        // Handle case like 'void' which has no declarator.
        if (param_list_node->num_children < 2) {
            //printf("DEBUG: Void Parameter");
            free_type(param_type);
            param_type = NULL;
            return NULL;
        }

        ASTNode* declarator = param_list_node->children[1];
        char* param_name = get_declarator_name(declarator);

        Symbol* new_param = (Symbol*) malloc(sizeof(Symbol));
        Type* full_param_type = build_declarator_type(param_type, declarator);
        new_param->name = param_name ? param_name : strdup("");
        new_param->type = full_param_type;
        new_param->kind = SYM_VARIABLE;
        new_param->next = NULL; // This is the end of a chain.
        return new_param;
    }

    // Handle the recursive case: a list of parameters.
    if (strcmp(param_list_node->node_type, "ParameterList") == 0) {
        // Recursively build the list from the left side (the previous parameters).
        Symbol* head = build_parameter_list_from_ast(param_list_node->children[0]);

        // Process the right side (the new parameter).
        Symbol* new_param = build_parameter_list_from_ast(param_list_node->children[1]);

        // Find the end of the existing list and append the new parameter.
        if (head) {
            Symbol* current = head;
            while (current->next) {
                current = current->next;
            }
            current->next = new_param;
            return head;
        }
        return new_param;
    }

    return NULL; // Should not be reached with correct AST structure.
}

/**
 * @brief Checks if a declarator AST node represents a function.
 */
int is_function_declarator(ASTNode* declarator_node) {
    if (!declarator_node) return 0;

    // It's a function if the node type is "FunctionDeclarator"
    if (strcmp(declarator_node->node_type, "FunctionDeclarator") == 0) {
        return 1;
    }
    // Could be nested inside pointers, e.g. int (*f)();
    return 0; // Simplified for now
}

/**
 * @brief Extracts the function name from a declarator AST node.
 */
char* get_declarator_name(ASTNode* declarator_node) {
    if (!declarator_node) return NULL;

    // Case 1: It's a simple identifier (e.g., "int x;")
    if (strcmp(declarator_node->node_type, "Identifier") == 0) {
        return strdup(declarator_node->value);
    }

    // Case 2: It's a function declarator (e.g., "int main(void)")
    if (strcmp(declarator_node->node_type, "FunctionDeclarator") == 0 && declarator_node->num_children > 0) {
        ASTNode* direct_declarator = declarator_node->children[0];
        // Recursively find the name inside nested declarators
        return get_declarator_name(direct_declarator);
    }
    // Case 3: It's an array declarator (e.g., "int x[10]")
    if (strcmp(declarator_node->node_type, "ArrayDeclarator") == 0 && declarator_node->num_children > 0) {
        // The name is inside the direct_declarator part
        return get_declarator_name(declarator_node->children[0]);
    }
    // Case 4: It's a pointer declarator (e.g., "int *p")
    if (strcmp(declarator_node->node_type, "PointerDeclarator") == 0 && declarator_node->num_children > 0) {
        // The name is inside the direct_declarator, which is the second child
        return get_declarator_name(declarator_node->children[1]);
    }
    // Case 5: It's an init declarator list
    if(strcmp(declarator_node->node_type, "InitDeclaratorList") == 0 && declarator_node->num_children > 0) {
        // The name is in the first child
        return get_declarator_name(declarator_node->children[0]);
    }
    // Case 6: It's an init declarator
    if(strcmp(declarator_node->node_type, "InitDeclarator") == 0 && declarator_node->num_children > 0) {
        // The name is in the first child
        return get_declarator_name(declarator_node->children[0]);
    }
    return NULL;
}

/**
 * @brief Evaluates a constant expression AST node and returns its integer value.
 * This is a simplified version for array dimensions.
 */
int evaluate_constant_expression(ASTNode* expr_node) {
    if (!expr_node) return 0;
    if (strcmp(expr_node->node_type, "IntConstant") == 0) {
        return atoi(expr_node->value);
    }
    // Add more cases here for complex constant expressions if needed
    fprintf(stderr, "Warning: Unsupported constant expression for array size. Only integer literals are supported.\n");
    return 0;
}

/**
 * @brief Recursively builds a complex type from a base type and a declarator AST node.
 * This handles pointers, arrays, and functions.
 */
Type* build_declarator_type(Type* base_type, ASTNode* declarator_node) {
    if (!declarator_node) return base_type;
    //printf("Building type for declarator node type: %s\n", declarator_node->node_type);
    if (strcmp(declarator_node->node_type, "PointerDeclarator") == 0) {
        // This is a pointer. The base_type is what the pointer *points to*.
        // We need to find the innermost declarator to apply the base type,
        // then wrap it with pointer types on the way out. The base_type is "consumed".
        Type* current_type = build_declarator_type(base_type, declarator_node->children[1]);
        
        // Now, wrap with pointer types for each '*' in the 'pointer' part of the AST.
        ASTNode* pointer_part = declarator_node->children[0];
        while (pointer_part) {
            current_type = create_pointer_type(current_type);
            pointer_part = (pointer_part->num_children > 0) ? pointer_part->children[0] : NULL;
        }
        return current_type;
    }
    if (strcmp(declarator_node->node_type, "ArrayDeclarator") == 0) {
        // It's an array. Recursively build the type of the element.
        // The base_type is "consumed" by the recursive call.
        Type* element_type = build_declarator_type(base_type, declarator_node->children[0]);
        int size = 0;
        if (declarator_node->num_children > 1) {
            // Evaluate the expression to get the array size.
            size = evaluate_constant_expression(declarator_node->children[1]);
        }
        return create_array_type(element_type, size);
    }
    if (strcmp(declarator_node->node_type, "Identifier") == 0) {
        // Base case: we've reached the identifier. The type is just the base type.
        //printf("Reached identifier declarator type building.\n Base type kind: %d Base type name: %s \n", base_type->kind,base_type->data.base_name);
        return base_type;
    }
    // Base case: we've reached the identifier. The type is just the base type.
    //printf("Reached base case for declarator type building.\n Base type kind: %d Base type name: %s \n", base_type->kind,base_type->data.base_name);
    return base_type;
}

int are_types_compatible(Type* type1, Type* type2) {
    if (!type1 || !type2) return 0; // Incompatible if either is NULL

    if (type1->kind != type2->kind) {
        // Allow assignment of int to double/float
        if (type1->kind == TYPE_BASE && type2->kind == TYPE_BASE) {
            if ((strcmp(type1->data.base_name, "double") == 0 || strcmp(type1->data.base_name, "float") == 0) &&
                strcmp(type2->data.base_name, "int") == 0) {
                return 1;
            }
        }
        return 0;
    }

    switch (type1->kind) {
        case TYPE_BASE:
            return strcmp(type1->data.base_name, type2->data.base_name) == 0;
        case TYPE_POINTER:
            return are_types_compatible(type1->data.points_to, type2->data.points_to);
        // More complex cases for arrays, structs etc. would go here
        default:
            return 0;
    }
}

void check_semantics(ASTNode *node) {
    if (!node) return;

       if (strcmp(node->node_type, "DefaultStatement") == 0) {
        // Nothing to check pre-order, the statement is handled by the loop
    }
    // Post-order traversal: process children first.
    //for (int i = 0; i < node->num_children; i++) {
    //     check_semantics(node->children[i]);
    //}
    // Special handling for function definitions to manage scope.
    // This must be done *after* the children have been visited in a generic sense,
    // but we will re-visit the body in the correct scope.
    if (strcmp(node->node_type, "FunctionDefinition") == 0) {
        //printf("DEBUG:Semantic Check: Entering function definition\n");
         // The function signature has been checked. Now handle the body.
         enter_scope();

         // Find the parameters in the declarator and add them to the new scope.
         ASTNode* declarator_node = node->children[1];
         ASTNode* params_ast_node = get_function_parameters_node(declarator_node);
         Symbol* params_list = build_parameter_list_from_ast(params_ast_node);
         if (params_list) {
            //printf("DEBUG:Semantic Check: Adding parameters to scope\n");
             add_parameters_to_scope(params_list);
         }

         // Now, check the function body (child 2) within the new scope.
         //printf("Semantic Check: Checking function body\n");
         check_semantics(node->children[2]);

         leave_scope();
         return; // Stop further processing for this node.
     }
    // Pre-order actions for nodes that manage context, like Switch
    if (strcmp(node->node_type, "SwitchStatement") == 0) {
        // Check the controlling expression type
        check_semantics(node->children[0]); // Check the expression first
        Type* expr_type = node->children[0]->type;
        if (!expr_type || expr_type->kind != TYPE_BASE || strcmp(expr_type->data.base_name, "int") != 0) {
            fprintf(stderr, "Semantic Error: switch quantity not an integer.\n");
        }
    }

    if (strcmp(node->node_type, "CaseStatement") == 0) {
        // Check that the case expression is a constant integer
        check_semantics(node->children[0]); // Check the expression
        ASTNode* case_expr = node->children[0];
        if (strcmp(case_expr->node_type, "IntConstant") != 0) {
            fprintf(stderr, "Semantic Error: case label does not reduce to an integer constant.\n");
        }
        // The statement part of the case is handled by the loop below
    }
    // Post-order actions
    
    if (strcmp(node->node_type, "Identifier") == 0) {
        Symbol *sym = lookup_symbol(node->value);
        if (!sym) {
            fprintf(stderr, "Semantic Error: Identifier '%s' is not declared.\n", node->value);
        } else {
            node->type = sym->type; // Assign the type from the symbol table to the AST node
        }
    } else if (strcmp(node->node_type, "IntConstant") == 0) {
        node->type = create_base_type("int");
    } else if (strcmp(node->node_type, "FloatConstant") == 0) {
        node->type = create_base_type("double");
    } else if (strcmp(node->node_type, "CharConstant") == 0) {
        node->type = create_base_type("char");
    } else if (strcmp(node->node_type, "MemberAccess") == 0) { // For s.m
        // By this point, the child has been checked due to post-order traversal.
        ASTNode* struct_node = node->children[0];
        check_semantics(struct_node);
        char* member_name = node->value;

        if (!struct_node->type) {
            // This can happen if the struct variable itself was not declared.
            // The error for the undeclared identifier would have already been reported.
            return;
        }

        if (struct_node->type->kind != TYPE_STRUCT && struct_node->type->kind != TYPE_UNION) {
            fprintf(stderr, "Semantic Error: Request for member '%s' in something that is not a struct or union.\n", member_name);
            return;
        }

        // Find the member in the struct's member list
        StructMember* member = get_struct_member(struct_node->type, member_name);
        if (member) {
            // The type of the whole expression (e.g., v.x1) is the type of the member.
            node->type = member->type;
        } else {
            fprintf(stderr, "Semantic Error: No member named '%s' in '%s %s'.\n",
                    member_name, struct_node->type->kind == TYPE_STRUCT ? "struct" : "union", struct_node->type->data.record_info.name);
        }
    } else if (strcmp(node->node_type, "Assignment") == 0) {
        ASTNode *lhs = node->children[0];
        ASTNode *rhs = node->children[1];
        //printf("DEBUG: Semantic Check: Checking assignment lhs\n");
        check_semantics(lhs);
        //printf("DEBUG: Semantic Check: Checking assignment rhs\n");
        check_semantics(rhs);
        //printf("DEBUG: Semantic Check: Checking assignment type\n");
        //printf("DEBUG: %s %s\n", lhs->node_type, rhs->node_type);
        if (lhs->type && rhs->type) {
            if (!are_types_compatible(lhs->type, rhs->type)) {
                // Safely get the name of the LHS. It might not be an identifier (e.g., *p).
                char* lhs_name = get_declarator_name(lhs);
                fprintf(stderr, "Semantic Error: Type mismatch in assignment to '%s'.\n", 
                        lhs_name ? lhs_name : "expression");
                if (lhs_name) free(lhs_name);
            }
        }
        node->type = lhs->type; // The type of an assignment is the type of the left-hand side
    } else if (strcmp(node->node_type, "BinaryOp") == 0) {
        ASTNode *left = node->children[0];
        ASTNode *right = node->children[1];
        if (left->type && right->type) {
            if (!are_types_compatible(left->type, right->type)) {
                 fprintf(stderr, "Semantic Error: Type mismatch in binary operation '%s'.\n", node->value);
            }
            // Relational operators result in an int
            if (strcmp(node->value, "==") == 0 || strcmp(node->value, "!=") == 0 ||
                strcmp(node->value, "<") == 0 || strcmp(node->value, ">") == 0 ||
                strcmp(node->value, "<=") == 0 || strcmp(node->value, ">=") == 0) {
                node->type = create_base_type("int");
            } else {
                node->type = left->type; // Result type is the same as operands for now
            }
        }
    } else if (strcmp(node->node_type, "FunctionCall") == 0) {
        ASTNode* func_ident = node->children[0];
        Symbol* func_sym = lookup_symbol(func_ident->value);
        printf("Semantic Check: Analyzing call to function '%s'\n", func_ident->value);

        if (!func_sym || func_sym->kind != SYM_FUNCTION) {
            fprintf(stderr, "Semantic Error: Calling '%s' which is not a function.\n", func_ident->value);
            return;
        }

        node->type = func_sym->type->data.function_info.return_type;

        // Check argument count and types
        printf("Semantic Check: Checking argument count and types\n");
        Symbol* expected_param = func_sym->type->data.function_info.params;
        //printf("DEBUG: Obtained expected parameters from Symbol Table \n");
        //printf("Number of children: %d\n", node->num_children );
        ASTNode* arg_list = (node->num_children > 1) ? node->children[1] : NULL;
        //printf("DEBUG: Obtained argument list from AST \n");

        int arg_count = 0;
        if((arg_list!= NULL) && (strcmp(arg_list->node_type, "ArgumentList") == 0)){
            // This is a simplified traversal for an ArgumentList
            while(arg_list) {
                printf("Semantic Check: Checking argument %d\n", arg_count);
                ASTNode* current_arg= arg_list->children[arg_count];
                printf("Semantic Check: Checking argument type for argument %s\n",current_arg->value);
                if (!expected_param) {
                    fprintf(stderr, "Semantic Error: Too many arguments to function '%s'.\n", func_ident->value);
                    break;
                }
                if (!are_types_compatible(expected_param->type, current_arg->type)) {
                    fprintf(stderr, "Semantic Check: Expected type for argument %d is '%s'.\n", arg_count, expected_param->type->data.base_name);
                    fprintf(stderr, "Semantic Error: Type mismatch for argument %d in call to '%s'.\n", 
                            arg_count, func_ident->value ? func_ident->value : "function");
                }
                printf("Semantic Check: Argument %d type matches expected type.\n", arg_count);
                expected_param = expected_param->next;
                if (arg_list->num_children > arg_count+1) {
                    printf("Semantic Check: Moving to next argument\n");
                    arg_count++;
                    //arg_list = arg_list->children[1];
                } else {
                    printf("Semantic Check: No more arguments\n");
                    break;
                }
            }
        }else{
            if (arg_list != NULL) {
                //printf("DEBUG: arg_list is not NULL\n");
                if (!expected_param) {
                        fprintf(stderr, "Semantic Error: Too many arguments to function '%s'.\n", func_ident->value);
                }
                if (!are_types_compatible(expected_param->type, arg_list->type)) {
                        //fprintf(stderr, "Semantic Check: Expected type for argument %d is '%s'.\n", arg_count, expected_param->type->data.base_name);
                        fprintf(stderr, "Semantic Error: Type mismatch for argument %d in call to '%s'.\n", 
                                arg_count, func_ident->value ? func_ident->value : "function");
                }
                expected_param = expected_param->next;
            }
        }
        if (expected_param != NULL) {
            fprintf(stderr, "Semantic Error: Too few arguments to function '%s'.\n", func_ident->value);
        }
    } else if (strcmp(node->node_type, "ArrayAccess") == 0) {
        ASTNode* array_node = node->children[0];
        ASTNode* index_node = node->children[1];
        check_semantics(array_node);
        check_semantics(index_node);
        if (array_node->type && array_node->type->kind == TYPE_ARRAY) {
            // The type of the result of an array access is the element type.
            node->type = array_node->type->data.array_info.element_type;

            // Check for out-of-bounds access if the index is a constant.
            if (strcmp(index_node->node_type, "IntConstant") == 0) {
                int index_val = atoi(index_node->value);
                int array_size = array_node->type->data.array_info.size;
                if (index_val < 0 || index_val >= array_size) {
                    char* array_name = get_declarator_name(array_node);
                    fprintf(stderr, "Semantic Error: Array index %d is out of bounds for array '%s' of size %d.\n", 
                            index_val, array_name ? array_name : "array", array_size);
                    if (array_name) free(array_name);
                }
            }
        } else {
            fprintf(stderr, "Semantic Error: Attempting to index a non-array type.\n");
        }
    } else if (strcmp(node->node_type, "PointerMemberAccess") == 0) { // For p->m
        ASTNode* ptr_node = node->children[0];
        check_semantics(ptr_node);
        char* member_name = node->value;

        if (!ptr_node->type) {
            // Error for undeclared identifier would have already been reported.
            return;
        }

        // 1. Check if the left side is a pointer.
        if (ptr_node->type->kind != TYPE_POINTER) {
            fprintf(stderr, "Semantic Error: Arrow operator -> applied to non-pointer type.\n");
            return;
        }

        Type* struct_type = ptr_node->type->data.points_to;
        // 2. Check if it points to a struct or union.
        if (struct_type->kind != TYPE_STRUCT && struct_type->kind != TYPE_UNION) {
            fprintf(stderr, "Semantic Error: Arrow operator -> applied to pointer to non-struct/union type.\n");
            return;
        }

        // 3. Find the member in the struct's member list.
        StructMember* member = get_struct_member(struct_type, member_name);
        if (member) {
            // 4. The type of the whole expression is the type of the member.
            node->type = member->type;
        } else {
            fprintf(stderr, "Semantic Error: No member named '%s' in '%s %s'.\n",
                    member_name, struct_type->kind == TYPE_STRUCT ? "struct" : "union", struct_type->data.record_info.name);
        }
    } else if (strcmp(node->node_type, "SwitchStatement") == 0) {
        // Post-order check for the switch body is implicitly handled by the traversal loop.
        // Duplicate case checks would require passing state down, which is more complex.
        // For now, we rely on the IR generation phase to handle the logic.
    } else {
        //printf("DEBUG: Processing Node: %s, %s, %d\n", node->node_type, node->value ? node->value : "no value" , node->num_children);
        for(int i = 0; i < node->num_children; i++) {
            check_semantics(node->children[i]);
        }
    }

}

// Helper function to calculate and store the layout of a struct/union.
void calculate_struct_layout(Type* struct_type, ASTNode* decl_list_node) {
    if (!struct_type || !decl_list_node) return;

    int current_offset = 0;
    int max_member_size = 0;
    StructMember* head = NULL;
    StructMember* tail = NULL;

    // Traverse the list of struct_declaration nodes
    while (decl_list_node) {
        ASTNode* struct_decl;
        // The AST for a list is (list, item). The base case is just (item).
        if (strcmp(decl_list_node->node_type, "StructDeclarationList") == 0) {
             struct_decl = (decl_list_node->num_children > 1) ? decl_list_node->children[1] : decl_list_node->children[0];
        } else { // Base case where the node is already a StructDeclaration
             struct_decl = decl_list_node;
        }

        Type* base_member_type = get_base_type_from_specifiers(struct_decl->children[0]);

        // This list contains one or more declarators for the same base type
        ASTNode* member_declarator_list = struct_decl->children[1];
        while (member_declarator_list) {
            ASTNode* declarator = member_declarator_list->children[0];
            // printf("DEBUG: Processing Member Declarator Node: %s, %s, %d\n", declarator->node_type, declarator->value ? declarator->value : "no value" , declarator->num_children);   
            char* member_name = get_declarator_name(declarator);
            Type* member_type = build_declarator_type(base_member_type, declarator);
            // printf("DEBUG: Found Member: %s, Type: %s\n", member_name, member_type->data.base_name);
            StructMember* new_member = (StructMember*)malloc(sizeof(StructMember));
            new_member->name = member_name;
            new_member->type = member_type;
            new_member->offset = current_offset;
            new_member->next = NULL;

            if (!head) head = tail = new_member;
            else { 
                tail->next = new_member; 
                tail = new_member;
            }

            // For a struct, advance the offset. For a union, it stays 0.
            if (struct_type->kind == TYPE_STRUCT) {
                current_offset += get_type_size(member_type);
            } else if (struct_type->kind == TYPE_UNION) {
                int member_size = get_type_size(member_type);
                if (member_size > max_member_size) {
                    max_member_size = member_size;
                }
            }
            member_declarator_list = (member_declarator_list->num_children > 1) ? member_declarator_list->children[1] : NULL;
        }
        // Move to the next item in the list.
        if (strcmp(decl_list_node->node_type, "StructDeclarationList") == 0 && decl_list_node->num_children > 1) {
            decl_list_node = decl_list_node->children[0];
        } else {
            decl_list_node = NULL; // End of the list
        }
    }
    struct_type->data.record_info.members = head;
    // For a struct, the size is the final offset. For a union, it's the max member size.
    if (struct_type->kind == TYPE_STRUCT) {
        struct_type->size = current_offset;
    } else { // TYPE_UNION
        struct_type->size = max_member_size;
    }
}

// Improved function to get a member's offset from a struct/union type.
int get_member_offset(Type* struct_type, const char* member_name) {
    //printf("DEBUG: Getting member offset for member '%s'\n", member_name);
    //printf("DEBUG: Struct type kind: %d\n", struct_type ? struct_type->kind : -1);
    if (!struct_type) return -1;

    // For pointers to structs, we need to look at what it points to.
    if (struct_type->kind == TYPE_POINTER) {
        struct_type = struct_type->data.points_to;
        if (!struct_type) return -1;
    }

    // Ensure we are dealing with a struct or union.
    if (struct_type->kind != TYPE_STRUCT && struct_type->kind != TYPE_UNION) {
        return -1;
    }

    StructMember* member = struct_type->data.record_info.members;
    while (member) {
        if (strcmp(member->name, member_name) == 0) {
            return member->offset;
        }
        member = member->next;
    }

    return -1; // Member not found
}