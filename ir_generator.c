#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "ir_generator.h"
#include "symbol_table.h" // Needed for get_type_size, lookup_symbol, etc.

/* --- Global Variables for IR --- */
Instruction *main_ir_head = NULL;
Instruction *main_ir_tail = NULL;
Instruction *other_funcs_ir_head = NULL;
Instruction *other_funcs_ir_tail = NULL;
Instruction *global_declarations_head=NULL;
Instruction *global_declarations_tail=NULL;

int temp_counter = 0;
int label_counter = 0;

Operand current_break_label;    // To store the break label for loops
Operand current_continue_label; // To store the continue label for loops

int is_in_main_function = 0; // Flag to direct emit() to output in main list
int is_global_declaration=1; // Flag to direct emit() to output in global declarations list
/* --- Helper functions for IR generation --- */

Operand create_operand_none() {
    Operand op;
    op.type = OP_NONE;
    // No need to initialize val for OP_NONE
    return op;
}

Operand create_operand_int(int val) {
    Operand op;
    op.type = OP_INT_CONST;
    op.val.int_val = val;
    return op;
}

Operand create_operand_float(double val) {
    //printf("DEBUG: Float constant: %f\n", val);
    Operand op;
    op.type = OP_FLOAT_CONST;
    op.val.float_val = val;
    return op;
}

Operand create_operand_char(char val) {
    Operand op;
    op.type = OP_CHAR_CONST;
    op.val.char_val = val;
    return op;
}

Operand create_operand_string(const char *val) {
    Operand op;
    op.type = OP_STRING_LITERAL;
    op.val.str_val = strdup(val);
    return op;
}

Operand create_operand_identifier(const char *name) {
    Operand op;
    op.type = OP_IDENTIFIER;
    op.val.name = strdup(name);
    return op;
}

Operand create_operand_temp() {
    Operand op;
    op.type = OP_TEMPORARY;
    char temp_name[32];
    sprintf(temp_name, "t%d", temp_counter++);
    op.val.name = strdup(temp_name);
    return op;
}

Operand create_operand_label() {
    Operand op;
    op.type = OP_LABEL;
    char label_name[32];
    sprintf(label_name, "L%d", label_counter++);
    op.val.name = strdup(label_name);
    return op;
}

Operand create_operand_argument(int index) {
    Operand op;
    op.type = OP_IDENTIFIER; // Treat ARGx as a special kind of identifier
    char arg_name[32];
    sprintf(arg_name, "ARG%d", index);
    op.val.name = strdup(arg_name);
    return op;
}

Operand create_operand_label_named(const char *name) {
    Operand op;
    op.type = OP_LABEL;
    op.val.name = strdup(name);
    return op;
}

void emit(OpCode opcode, Operand result, Operand arg1, Operand arg2) {
    Instruction *instr = (Instruction*) malloc(sizeof(Instruction));
    if (!instr) {
        fprintf(stderr, "Out of memory for IR instruction.\n");
        exit(1);
    }
    instr->opcode = opcode;
    instr->result = result;
    instr->arg1 = arg1;
    instr->arg2 = arg2;
    instr->next = NULL;
    // printf(is_global_declaration?"In Global":"Not Global");
    // printf(is_in_main_function?" In Main\n":" In Other Func\n");
    // printf("DEBUG: Emitting global declaration IR instruction.\n");
    // printf("DEBUG: Opcode: %d\n", opcode);
    // printf("DEBUG: Result Operand Type: %d\n", result.type);
    // printf("DEBUG: Arg1 Operand Type: %d\n", arg1.type);
    // printf("DEBUG: Arg2 Operand Type: %d\n", arg2.type);
    if (is_global_declaration){
        if (global_declarations_head == NULL) {
            global_declarations_head = instr;
            global_declarations_tail = instr;
        } else {
            global_declarations_tail->next = instr;
            global_declarations_tail = instr;
        }
    }else{
        if (is_in_main_function) {
            if (main_ir_head == NULL) {
                main_ir_head = instr;
                main_ir_tail = instr;
            } else {
                main_ir_tail->next = instr;
                main_ir_tail = instr;
            }
        } else {
            if (other_funcs_ir_head == NULL) {
                other_funcs_ir_head = instr;
                other_funcs_ir_tail = instr;
            } else {
                other_funcs_ir_tail->next = instr;
                other_funcs_ir_tail = instr;
            }
        }
    }
}

// Function to print an operand
void print_operand(FILE *fp, Operand op) {
    switch (op.type) {
        case OP_NONE: break;
        case OP_INT_CONST: fprintf(fp, "%d", op.val.int_val); break;
        case OP_FLOAT_CONST: fprintf(fp, "%f", op.val.float_val); break;
        case OP_CHAR_CONST: fprintf(fp, "%d", op.val.char_val); break;
        case OP_STRING_LITERAL: fprintf(fp, "\"%s\"", op.val.str_val); break;
        case OP_IDENTIFIER:
        case OP_TEMPORARY:
        case OP_LABEL: fprintf(fp, "%s", op.val.name); break;
    }
}

// Helper to print a single IR list
void print_ir_list(FILE *fp, Instruction *head) {
    Instruction *current = head;
    while (current) {
        switch (current->opcode) {
            case IR_HALT:
                fprintf(fp,"\tHALT\n");
                break;
            case IR_LABEL:
                fprintf(fp, "%s:\n", current->result.val.name);
                break;
            case IR_ASSIGN:
                fprintf(fp, "\tASSIGN ");
                print_operand(fp, current->result);
                fprintf(fp, ", ");
                print_operand(fp, current->arg1);
                fprintf(fp, "\n");
                break;
            case IR_ADD:
            case IR_SUB:
            case IR_MUL:
            case IR_DIV:
            case IR_MOD:
            case IR_SHL:
            case IR_SHR:
            case IR_EQ:
            case IR_NE:
            case IR_LT:
            case IR_GT:
            case IR_LE:
            case IR_GE:
            case IR_AND:
            case IR_OR:
            case IR_BIT_AND:
            case IR_BIT_OR:
            case IR_XOR:
                fprintf(fp, "\t%s ",
                        current->opcode == IR_ADD ? "ADD" :
                        current->opcode == IR_SUB ? "SUB" :
                        current->opcode == IR_MUL ? "MUL" :
                        current->opcode == IR_DIV ? "DIV" :
                        current->opcode == IR_MOD ? "MOD" :
                        current->opcode == IR_SHL ? "SHL" :
                        current->opcode == IR_SHR ? "SHR" :
                        current->opcode == IR_EQ ? "EQ" :
                        current->opcode == IR_NE ? "NE" :
                        current->opcode == IR_LT ? "LT" :
                        current->opcode == IR_GT ? "GT" :
                        current->opcode == IR_LE ? "LE" :
                        current->opcode == IR_GE ? "GE" :
                        current->opcode == IR_AND ? "AND" :
                        current->opcode == IR_OR ? "OR" :
                        current->opcode == IR_BIT_AND ? "BIT_AND" :
                        current->opcode == IR_BIT_OR ? "BIT_OR" : "XOR");
                print_operand(fp, current->result);
                fprintf(fp, ", ");
                print_operand(fp, current->arg1);
                fprintf(fp, ", ");
                print_operand(fp, current->arg2);
                fprintf(fp, "\n");
                break;
            case IR_UNARY_MINUS:
            case IR_NOT:
            case IR_BIT_NOT:
            case IR_ADDR:
            case IR_DEREF:
                fprintf(fp, "\t%s ",
                        current->opcode == IR_UNARY_MINUS ? "UMINUS" :
                        current->opcode == IR_NOT ? "NOT" :
                        current->opcode == IR_BIT_NOT ? "BIT_NOT" :
                        current->opcode == IR_ADDR ? "ADDR" : "DEREF_LOAD");
                print_operand(fp, current->result);
                fprintf(fp, ", ");
                print_operand(fp, current->arg1);
                fprintf(fp, "\n");
                break;
            case IR_ALLOC_HEAP:
                fprintf(fp, "\tALLOC_HEAP ");
                print_operand(fp, current->result);
                fprintf(fp, ", ");
                print_operand(fp, current->arg1);
                fprintf(fp, "\n");
                break;
            case IR_FREE_HEAP:
                fprintf(fp, "\tFREE_HEAP ");
                print_operand(fp, current->arg1);
                fprintf(fp, "\n");
                break;
            case IR_ADDR_OF:
                fprintf(fp, "\tADDR_OF ");
                print_operand(fp, current->result);
                fprintf(fp, ", ");
                print_operand(fp, current->arg1);
                fprintf(fp, "\n");
                break;
            case IR_DEREF_LOAD:
                fprintf(fp, "\tDEREF_LOAD ");
                print_operand(fp, current->result);
                fprintf(fp, ", ");
                print_operand(fp, current->arg1);
                fprintf(fp, "\n");
                break;
            case IR_DEREF_STORE:
                fprintf(fp, "\tDEREF_STORE ");
                print_operand(fp, current->result);
                fprintf(fp, ", ");
                print_operand(fp, current->arg1);
                fprintf(fp, "\n");
                break;
            case IR_INDEX_LOAD:
            case IR_INDEX_STORE:
                fprintf(fp, "\t%s ", current->opcode == IR_INDEX_LOAD ? "INDEX_LOAD" : "INDEX_STORE");
                print_operand(fp, current->result);
                fprintf(fp, ", ");
                print_operand(fp, current->arg1);
                fprintf(fp, ", ");
                print_operand(fp, current->arg2);
                fprintf(fp, "\n");
                break;
            case IR_GOTO:
                fprintf(fp, "\tJUMP %s\n", current->result.val.name);
                break;
            case IR_IF_FALSE_GOTO:
                fprintf(fp, "\tJUMPF %s, ", current->result.val.name);
                print_operand(fp, current->arg1);
                fprintf(fp, "\n");
                break;
            case IR_IF_TRUE_GOTO:
                fprintf(fp, "\tJUMPT %s, ", current->result.val.name);
                print_operand(fp, current->arg1);
                fprintf(fp, "\n");
                break;
            case IR_CALL:
                fprintf(fp, "\tCALL ");
                fprintf(fp, "%s, %d,", current->arg1.val.name, current->arg2.val.int_val);
                print_operand(fp, current->result);
                fprintf(fp, "\n");
                break;
            case IR_PARAM:
                fprintf(fp, "\tPARAM ");
                print_operand(fp, current->arg1);
                fprintf(fp, "\n");
                break;
            case IR_RETURN:
                fprintf(fp, "\tRETURN ");
                print_operand(fp, current->arg1);
                fprintf(fp, "\n");
                break;
            case IR_NOP:
                fprintf(fp, "\tNOP\n");
                break;
            default:
                break;
        }
        current = current->next;
    }
}

// Function to print the IR to a file
void print_ir_to_file(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Could not open 3AC output file");
        return;
    }
    // First, print the main function's instructions
    fprintf(fp, "# --- Global DECLARATIONS ---\n");
    print_ir_list(fp, global_declarations_head);
    
    // Second, print the main function's instructions
    fprintf(fp, "# --- MAIN FUNCTION ---\n");
    print_ir_list(fp, main_ir_head);

    // Finaly, print the other functions' instructions
    fprintf(fp, "\n# --- OTHER FUNCTIONS ---\n");
    print_ir_list(fp, other_funcs_ir_head);
    fclose(fp);
}

// Helper to free a single operand's dynamically allocated name/string
void free_operand(Operand op) {
    switch (op.type) {
        case OP_STRING_LITERAL:
        case OP_IDENTIFIER:
        case OP_TEMPORARY:
        case OP_LABEL:
            if (op.val.name) {
                free(op.val.name);
            }
            break;
        default:
            // No dynamic memory for other operand types
            break;
    }
}

// Helper to free a list of instructions
void free_ir_list(Instruction *head) {
    Instruction *current = head;
    while (current) {
        Instruction *temp = current;
        current = current->next;

        free_operand(temp->result);
        free_operand(temp->arg1);
        free_operand(temp->arg2);
        free(temp);
    }
}

// Main cleanup function for all IR lists
void free_ir_lists() {
    free_ir_list(main_ir_head);
    main_ir_head = main_ir_tail = NULL;
    free_ir_list(other_funcs_ir_head);
    other_funcs_ir_head = other_funcs_ir_tail = NULL;
    free_ir_list(global_declarations_head);
    global_declarations_head=global_declarations_tail=NULL;
}

/* --- Peephole Optimizer --- */

/**
 * @brief Compares two operands to see if they are identical.
 * This is crucial for matching addresses in the peephole optimizer.
 */
int are_operands_equal(Operand op1, Operand op2) {
    if (op1.type != op2.type) {
        return 0; // Not equal if types differ
    }
    switch (op1.type) {
        case OP_INT_CONST:
            return op1.val.int_val == op2.val.int_val;
        case OP_CHAR_CONST:
            return op1.val.char_val == op2.val.char_val;
        case OP_FLOAT_CONST:
            return op1.val.float_val==op2.val.float_val;
        case OP_IDENTIFIER:
        case OP_TEMPORARY:
        case OP_LABEL:
            // Assumes names are properly managed (e.g., strdup'd)
            return strcmp(op1.val.name, op2.val.name) == 0;
        case OP_NONE:
            return 1; // OP_NONE is always equal to OP_NONE
        // Add other types (float, char, etc.) if needed for comparison
        default:
            return 0;
    }
}

/**
 * @brief Performs peephole optimization on a list of IR instructions.
 * Currently, it optimizes the pattern:
 *   INDEX_LOAD temp, base, offset
 *   INDEX_STORE base, offset, value
 * by removing the redundant INDEX_LOAD.
 * @param head A pointer to the head of the instruction list.
 * @return The head of the new, optimized instruction list.
 */
Instruction* optimize_ir(Instruction *head) {
    if (!head) {
        return NULL;
    }

    Instruction *current = head;

    while (current && current->next) {
        Instruction *next_instr = current->next;

        // Pattern: Redundant Load-Store
        // Look for an INDEX_LOAD followed by an INDEX_STORE to the same location.
        if (current->opcode == IR_INDEX_LOAD && next_instr->opcode == IR_INDEX_STORE) {
            // For INDEX_LOAD, the base is arg1 and offset is arg2.
            Operand load_base = current->arg1;
            Operand load_offset = current->arg2;

            // For INDEX_STORE, the base is result and offset is arg1.
            Operand store_base = next_instr->result;
            Operand store_offset = next_instr->arg1;

            // Check if the base and offset operands are the same.
            if (are_operands_equal(load_base, store_base) && are_operands_equal(load_offset, store_offset)) {
                Instruction *to_remove = current;
                
                // Find the instruction before 'current' to relink the list.
                Instruction *prev = head;
                if (to_remove == head) {
                    head = next_instr;
                } else {
                    while (prev->next != to_remove) {
                        prev = prev->next;
                    }
                    prev->next = next_instr;
                }
                current = next_instr; // Continue from the next instruction

                if(to_remove->result.type == OP_TEMPORARY) free(to_remove->result.val.name);
                free(to_remove);
                continue; // Restart loop to check for new patterns
            }
        }

        // Pattern: Redundant MUL -> INDEX_LOAD -> MUL -> INDEX_STORE for array assignments
        // Example: a[i] = val;
        //   MUL t0, i, 4
        //   INDEX_LOAD t1, a, t0  <-- Redundant load
        //   MUL t2, i, 4          <-- Redundant multiplication
        //   INDEX_STORE a, t2, val
        if (current->opcode == IR_MUL && current->next && current->next->opcode == IR_INDEX_LOAD &&
            current->next->next && current->next->next->opcode == IR_MUL &&
            current->next->next->next && current->next->next->next->opcode == IR_INDEX_STORE) {

            Instruction *mul1 = current;
            Instruction *load = current->next;
            Instruction *mul2 = current->next->next;
            Instruction *store = current->next->next->next;
            //printf("DEBUG: Found potential redundant MUL-LOAD-MUL-STORE pattern.\n");
            // Check if the pattern matches
            //printf("DEBUG: Comparing MUL1 result '%s' with LOAD result '%s'\n", mul1->result.val.name, load->result.val.name);
            //printf("DEBUG: Comparing STORE arg1 '%s' with MUL2 result '%s'\n", store->arg1.val.name, mul2->result.val.name);
            //printf("DEBUG: Comparing LOAD base '%s' with STORE result '%s'\n", load->arg1.val.name, store->result.val.name);
            if (are_operands_equal(mul1->arg1, mul2->arg1) && are_operands_equal(mul1->arg2, mul2->arg2) && // MULs are identical
                are_operands_equal(load->arg2, mul1->result) && // load uses result of mul1
                are_operands_equal(store->arg1, mul2->result) && // store uses result of mul2
                are_operands_equal(load->arg1, store->result)) { // load and store have same base
                // This is our redundant pattern. Remove the first two instructions.
                //printf("DEBUG: Optimizing redundant MUL-LOAD-MUL-STORE pattern.\n");
                current = mul2; // The next instruction to check will be mul2
                if (mul1 == head) head = mul2;
                
                // Free the removed instructions and their temporary results
                if(mul1->result.type == OP_TEMPORARY) free(mul1->result.val.name);
                if(load->result.type == OP_TEMPORARY) free(load->result.val.name);
                free(mul1);
                free(load);
                continue; // Restart loop
            }
        }

        current = current->next;
    }
    return head;
}

// Helper function to recursively process argument list and emit PARAMs
// This will emit PARAMs in the correct order (left to right)
static void emit_params_for_call(ASTNode* current_arg_list_node, int* count) {
    if (!current_arg_list_node || strcmp(current_arg_list_node->node_type, "EmptyExpression") == 0) {
        return;
    }

    if (strcmp(current_arg_list_node->node_type, "ArgumentList") == 0) {
        // The AST is left-recursive: ArgumentList -> ArgumentList, assignment_expression
        // We must process the list (left child) first to maintain left-to-right order.
        emit_params_for_call(current_arg_list_node->children[0], count);

        // Then process the actual argument on the right.
        if (current_arg_list_node->num_children > 1) {
            Operand param_op = Generate_IR(current_arg_list_node->children[1]);
            emit(IR_PARAM, create_operand_none(), param_op, create_operand_none());
            (*count)++;
        }

    } else {
        // This is the base case: a single assignment_expression.
        Operand param_op = Generate_IR(current_arg_list_node);
        emit(IR_PARAM, create_operand_none(), param_op, create_operand_none());
        (*count)++;
    }
}

// Main IR generation function
Operand Generate_IR(ASTNode *node) {
    if (!node) return create_operand_none();

    Operand result_op = create_operand_none();
    Operand arg1_op, arg2_op;

    // Handle nodes that are primarily statements or program structure
    // These nodes typically don't produce a value, but their children might.
    if (strcmp(node->node_type, "Program") == 0 ||
        strcmp(node->node_type, "ExternalDeclaration") == 0 ||
        strcmp(node->node_type, "BlockItemList") == 0 ||
        strcmp(node->node_type, "BlockItem") == 0 ||
        strcmp(node->node_type, "Statement") == 0 ||
        strcmp(node->node_type, "EmptyStatement") == 0 ||
        strcmp(node->node_type, "DeclarationSpecifiers") == 0 ||
        strcmp(node->node_type, "TypeSpecifier") == 0 ||
        strcmp(node->node_type, "TypeName") == 0 ||
        strcmp(node->node_type, "StorageClassSpecifier") == 0 ||
        strcmp(node->node_type, "TypeQualifier") == 0 ||
        strcmp(node->node_type, "StructSpecifier") == 0 ||
        strcmp(node->node_type, "StructToken") == 0 ||
        strcmp(node->node_type, "UnionToken") == 0 ||
        strcmp(node->node_type, "StructDeclarationList") == 0 ||
        strcmp(node->node_type, "StructDeclaration") == 0 ||
        strcmp(node->node_type, "SpecifierQualifierList") == 0 ||
        strcmp(node->node_type, "StructDeclaratorList") == 0 ||
        strcmp(node->node_type, "StructDeclarator") == 0 ||
        strcmp(node->node_type, "EnumSpecifier") == 0 ||
        strcmp(node->node_type, "EnumeratorList") == 0 ||
        strcmp(node->node_type, "Enumerator") == 0 ||
        strcmp(node->node_type, "Initializer") == 0 ||
        strcmp(node->node_type, "InitializerList") == 0 ||
        strcmp(node->node_type, "InitValues") == 0 ||
        strcmp(node->node_type, "PointerDeclarator") == 0 ||
        strcmp(node->node_type, "ParameterList") == 0 ||
        strcmp(node->node_type, "ParameterDeclaration") == 0 ||
        strcmp(node->node_type, "EmptyParameterList") == 0 ||
        strcmp(node->node_type, "Pointer") == 0 ||
        strcmp(node->node_type, "ParenthesizedAbstractDeclarator") == 0 ||
        strcmp(node->node_type, "AbstractArraySuffix") == 0 ||
        strcmp(node->node_type, "AbstractFunctionSuffix") == 0 ||
        strcmp(node->node_type, "EmptyExpression") == 0 
        ) {
        for (int i = 0; i < node->num_children; i++) {
            Generate_IR(node->children[i]);
        }
        return create_operand_none(); // These nodes don't produce a value
    }

    if (strcmp(node->node_type, "Declaration") == 0) {
        // This is the correct place to handle allocation for declared variables.
        ASTNode* declarator_list = node->children[1];
        while (declarator_list) {
            // The actual declarator can be inside an InitDeclarator or be the node itself.
            ASTNode* current_decl = declarator_list->children[0];
            if (strcmp(current_decl->node_type, "InitDeclarator") == 0) {
                // Case: int x = 5;
                Generate_IR(current_decl); // Handle the assignment part
            }else {
                // Case for uninitialized declarations: int x; struct Point p; etc.
                char* var_name = get_declarator_name(current_decl);
                if (var_name) {
                    Symbol* sym = lookup_symbol(var_name);
                    // If it's a struct, union, or array, allocate heap memory
                    if (sym && (sym->type->kind == TYPE_STRUCT || sym->type->kind == TYPE_UNION || sym->type->kind == TYPE_ARRAY)) {
                        int total_size = get_type_size(sym->type);
                        emit(IR_ALLOC_HEAP, create_operand_identifier(var_name), create_operand_int(total_size), create_operand_none());
                    }else{
                        // For basic types, no heap allocation needed; stack allocation assumed.
                        if(sym->type->kind == TYPE_BASE){
                            if(strcmp(sym->type->data.base_name,"int")==0){
                                // Initialize int to 0
                                emit(IR_ASSIGN, create_operand_identifier(var_name), create_operand_int(0), create_operand_none());
                            }else if(strcmp(sym->type->data.base_name,"char")==0){
                                // Initialize char to 0
                                emit(IR_ASSIGN, create_operand_identifier(var_name), create_operand_char(0), create_operand_none());
                            }else if((strcmp(sym->type->data.base_name,"float")==0)||(strcmp(sym->type->data.base_name,"double")==0)){
                                // Initialize float to 0.0  
                                emit(IR_ASSIGN, create_operand_identifier(var_name), create_operand_float(0.0), create_operand_none());
                            }else {
                                // Other basic types can be handled here

                            }
                        }
                        
                    }
                    free(var_name);
                }
            }
            declarator_list = (declarator_list->num_children > 1) ? declarator_list->children[1] : NULL;
        }
        return create_operand_none();
    }

    if (strcmp(node->node_type, "InitDeclaratorList") == 0) {
        // The list is reversed in the AST, so process children[1] then children[0]
        if (node->num_children > 1) {
            Generate_IR(node->children[1]);
        }
        Generate_IR(node->children[0]);
        return create_operand_none();
    }

    if (strcmp(node->node_type, "InitDeclarator") == 0) {
        // This node only exists for declarations with an initializer, e.g., `int x = 5;`
        ASTNode* declarator = node->children[0];
        ASTNode* initializer = node->children[1];
        char* var_name = get_declarator_name(declarator);
        if (var_name) {
            Symbol* sym = lookup_symbol(var_name);
            if (sym && (sym->type->kind == TYPE_STRUCT || sym->type->kind == TYPE_UNION || sym->type->kind == TYPE_ARRAY)) {
                int total_size = get_type_size(sym->type);
                emit(IR_ALLOC_HEAP, create_operand_identifier(var_name), create_operand_int(total_size), create_operand_none());
            }
            // Now handle the assignment part of the initialization
            // This requires creating a temporary assignment AST node and processing it.
            // For now, we assume simple assignment.
            Operand rhs_op = Generate_IR(initializer);
            emit(IR_ASSIGN, create_operand_identifier(var_name), rhs_op, create_operand_none());
            free(var_name);
        }
        return create_operand_none(); // The assignment is handled here.
    }

    if (strcmp(node->node_type, "ArrayDeclarator") == 0) {
        // This node is part of a declaration. We need to allocate memory.
        char* array_name = get_declarator_name(node);
        Symbol* sym = lookup_symbol(array_name);
        if (sym && sym->type->kind == TYPE_ARRAY && sym->type->data.array_info.size > 0) {
            int total_size = get_type_size(sym->type);
            emit(IR_ALLOC_HEAP, create_operand_identifier(array_name), create_operand_int(total_size), create_operand_none());
        }
        free(array_name);
    }

    if (strcmp(node->node_type, "FunctionDefinition") == 0) {
        char* func_name = get_declarator_name(node->children[1]);
        is_global_declaration=0;
        if (strcmp(func_name, "main") == 0) {
            is_in_main_function = 1;
        } else {
            is_in_main_function = 0;
        }
        emit(IR_LABEL, create_operand_label_named(func_name), create_operand_none(), create_operand_none());

        // Handle parameter assignments from arguments
        ASTNode* declarator_node = node->children[1];
        ASTNode* params_ast_node = get_function_parameters_node(declarator_node);
        Symbol* params_list = build_parameter_list_from_ast(params_ast_node);
        Symbol* current_param = params_list;
        int arg_index = 0;
        while (current_param) {
            emit(IR_ASSIGN, create_operand_identifier(current_param->name), create_operand_argument(arg_index), create_operand_none());
            current_param = current_param->next;
            arg_index++;
        }
        Generate_IR(node->children[2]); // CompoundStatement
        //If the return is void insert a return explicity at the end of the function.
        Symbol* function_entry=lookup_symbol(func_name);
        char* return_type=function_entry->type->data.function_info.return_type->data.base_name;
        if(strcmp(return_type,"void") == 0 ){
            emit(IR_RETURN,create_operand_none(),create_operand_none(),create_operand_none());
        }   
        free(func_name); // Free the strdup from get_declarator_name
        return create_operand_none();
    } else if (strcmp(node->node_type, "CompoundStatement") == 0) {
        if (node->num_children > 0) { // block_item_list
            Generate_IR(node->children[0]);
        }
        return create_operand_none();
    } else if (strcmp(node->node_type, "ExpressionStatement") == 0) {
        if (node->num_children > 0) { // expression
            Generate_IR(node->children[0]);
        }
        return create_operand_none();
    } else if (strcmp(node->node_type, "Return") == 0) {
        if (node->num_children > 0) { // expression
            if(!is_in_main_function){
                if(strcmp(node->children[0]->node_type, "EmptyExpression") == 0){
                    emit(IR_RETURN, create_operand_none(), create_operand_none(), create_operand_none());
                    is_global_declaration=1;
                    return create_operand_none();
                }
                arg1_op = Generate_IR(node->children[0]);
                emit(IR_RETURN, create_operand_none(), arg1_op, create_operand_none());
            }else{
                emit(IR_HALT, create_operand_none(), create_operand_none(), create_operand_none());
            }
        } else {
            emit(IR_RETURN, create_operand_none(), create_operand_none(), create_operand_none());
        }
        is_global_declaration=1;
        return create_operand_none();
    } else if (strcmp(node->node_type, "Identifier") == 0) {
        result_op = create_operand_identifier(node->value);
    } else if (strcmp(node->node_type, "IntConstant") == 0) {
        result_op = create_operand_int(atoi(node->value));
    } else if (strcmp(node->node_type, "FloatConstant") == 0) {
        result_op = create_operand_float(atof(node->value));
    } else if (strcmp(node->node_type, "CharConstant") == 0) {
        result_op = create_operand_char(node->value[1]); // Assuming node->value is "'c'"
    } else if (strcmp(node->node_type, "StringLiteral") == 0) {
        result_op = create_operand_string(node->value);
    } else if (strcmp(node->node_type, "Assignment") == 0) {
        arg1_op = Generate_IR(node->children[1]); // RHS
        result_op = Generate_IR(node->children[0]); // LHS (identifier or dereference)
        ASTNode* lhs = node->children[0];
        if (strcmp(lhs->node_type, "ArrayAccess") == 0) { // arr[i] = value
            //printf("DEBUG: Array assignment detected: %s\n", lhs->value);
            Operand array_op = Generate_IR(lhs->children[0]);
            Operand index_op = Generate_IR(lhs->children[1]);
            Type* element_type = lhs->type; // The type of the element

            if (element_type) {
                int element_size = get_type_size(element_type);
                Operand offset_op = create_operand_temp();
                emit(IR_MUL, offset_op, index_op, create_operand_int(element_size));
                emit(IR_INDEX_STORE, array_op, offset_op, arg1_op);
            } else {
                fprintf(stderr, "IR Generation Error: Attempting to index a non-array/pointer type for assignment.\n");
            }
            result_op = arg1_op; // The result of an assignment expression is the assigned value
        } else if (strcmp(lhs->node_type, "PointerMemberAccess") == 0) { // p->m = value
            //printf("DEBUG: Pointer to struct member assignment detected: %s\n", lhs->value);
            Operand struct_ptr_op = Generate_IR(lhs->children[0]);
            char* member_name = lhs->value;
            int offset = get_member_offset(lhs->children[0]->type, member_name);

            if (offset != -1) {
                // Use INDEX_STORE: base_ptr, index, value_to_store
                emit(IR_INDEX_STORE, struct_ptr_op, create_operand_int(offset), arg1_op);
            } else {
                fprintf(stderr, "IR Generation Error: Member '%s' not found for pointer assignment.\n", member_name);
            }
            result_op = arg1_op;
        } else if (strcmp(lhs->node_type, "MemberAccess") == 0) { // s.m = value
            //printf("DEBUG: Struct member assignment detected: %s\n", lhs->value);   
            // Get the base address of the struct variable 's'
            Operand struct_op = Generate_IR(lhs->children[0]);
            char* member_name = lhs->value;
            // Get the offset of member 'm'
            int offset = get_member_offset(lhs->children[0]->type, member_name);
            if (offset != -1) {
                // We need the address of the struct variable to use as a base.
                //Operand base_addr_op = create_operand_temp();
                //emit(IR_ADDR_OF, base_addr_op, struct_op, create_operand_none());
                // Use INDEX_STORE: base_addr, offset, value_to_store
                emit(IR_INDEX_STORE, struct_op, create_operand_int(offset), arg1_op);
            } else {
                fprintf(stderr, "IR Generation Error: Member '%s' not found for struct assignment.\n", member_name);
            }
            result_op = arg1_op;
        } else if((strcmp(lhs->node_type, "UnaryOp") == 0)&& (strcmp(lhs->value,"*")==0)){ // *ptr = value
            Operand ptr_op = Generate_IR(lhs->children[0]);
            emit(IR_DEREF_STORE, ptr_op, arg1_op, create_operand_none());
        } else {

            // Default case for simple variables: a = value
            result_op = Generate_IR(lhs); // LHS (identifier)
            emit(IR_ASSIGN, result_op, arg1_op, create_operand_none());
        }
    } else if (strcmp(node->node_type, "BinaryOp") == 0) {
        arg1_op = Generate_IR(node->children[0]);
        arg2_op = Generate_IR(node->children[1]);
        result_op = create_operand_temp();

        OpCode op_code;
        if (strcmp(node->value, "+") == 0) op_code = IR_ADD;
        else if (strcmp(node->value, "-") == 0) op_code = IR_SUB;
        else if (strcmp(node->value, "*") == 0) op_code = IR_MUL;
        else if (strcmp(node->value, "/") == 0) op_code = IR_DIV;
        else if (strcmp(node->value, "%") == 0) op_code = IR_MOD;
        else if (strcmp(node->value, "==") == 0) op_code = IR_EQ;
        else if (strcmp(node->value, "!=") == 0) op_code = IR_NE;
        else if (strcmp(node->value, "<") == 0) op_code = IR_LT;
        else if (strcmp(node->value, ">") == 0) op_code = IR_GT;
        else if (strcmp(node->value, "<=") == 0) op_code = IR_LE;
        else if (strcmp(node->value, ">=") == 0) op_code = IR_GE;
        else if (strcmp(node->value, "&&") == 0) op_code = IR_AND;
        else if (strcmp(node->value, "||") == 0) op_code = IR_OR;
        else if (strcmp(node->value, "&") == 0) op_code = IR_BIT_AND;
        else if (strcmp(node->value, "|") == 0) op_code = IR_BIT_OR;
        else if (strcmp(node->value, "^") == 0) op_code = IR_XOR;
        else if (strcmp(node->value, "<<") == 0) op_code = IR_SHL;
        else if (strcmp(node->value, ">>") == 0) op_code = IR_SHR;
        else if (strcmp(node->value, ",") == 0) { // Comma operator
            // Evaluate left, then right, result is right.
            // IR for left is already generated by arg1_op.
            result_op = arg2_op; // The result of the comma operator is the right operand's value
            return result_op;
        }
        else {
            fprintf(stderr, "IR Generation Error: Unknown binary operator '%s'\n", node->value);
            op_code = IR_NOP;
        }
        emit(op_code, result_op, arg1_op, arg2_op);
    } else if (strcmp(node->node_type, "UnaryOp") == 0) {
        //printf("DEBUG: Unary operator found!!");
        arg1_op = Generate_IR(node->children[0]);
        result_op = create_operand_temp();
        OpCode op_code;
        if (strcmp(node->value, "-") == 0) op_code = IR_UNARY_MINUS;
        else if (strcmp(node->value, "!") == 0) op_code = IR_NOT;
        else if (strcmp(node->value, "~") == 0) op_code = IR_BIT_NOT;
        else if (strcmp(node->value, "&") == 0) op_code = IR_ADDR_OF;
        else if (strcmp(node->value, "*") == 0) op_code = IR_DEREF;
        else {
            fprintf(stderr, "IR Generation Error: Unknown unary operator '%s'\n", node->value);
            op_code = IR_NOP;
        }
        emit(op_code, result_op, arg1_op, create_operand_none());
    } else if (strcmp(node->node_type, "PrefixIncrement") == 0 || strcmp(node->node_type, "PrefixDecrement") == 0) {
        Operand target_op = Generate_IR(node->children[0]);
        result_op = create_operand_temp();
        OpCode op_code = (strcmp(node->node_type, "PrefixIncrement") == 0) ? IR_ADD : IR_SUB;
        emit(op_code, result_op, target_op, create_operand_int(1)); // t1 = x + 1
        emit(IR_ASSIGN, target_op, result_op, create_operand_none()); // x = t1
    } else if (strcmp(node->node_type, "PostfixIncrement") == 0 || strcmp(node->node_type, "PostfixDecrement") == 0) {
        Operand target_op = Generate_IR(node->children[0]);
        result_op = create_operand_temp(); // Result is the value *before* increment/decrement
        emit(IR_ASSIGN, result_op, target_op, create_operand_none()); // t1 = x
        OpCode op_code = (strcmp(node->node_type, "PostfixIncrement") == 0) ? IR_ADD : IR_SUB;
        Operand temp_val_op = create_operand_temp();
        emit(op_code, temp_val_op, target_op, create_operand_int(1)); // t2 = x + 1
        emit(IR_ASSIGN, target_op, temp_val_op, create_operand_none()); // x = t2
    } else if (strcmp(node->node_type, "FunctionCall") == 0) {
        ASTNode* func_ident_node = node->children[0];
        char* func_name = func_ident_node->value; // Assuming func_ident_node is an Identifier
        ASTNode* arg_list_node = (node->num_children > 1) ? node->children[1] : NULL;
        int num_args = 0;

        emit_params_for_call(arg_list_node, &num_args);

        result_op = create_operand_temp();
        emit(IR_CALL, result_op, create_operand_identifier(func_name), create_operand_int(num_args));
    } //else if(strcmp(node->node_type, "ArgumentList") == 0){
      //  int num_args = 0;
      //  emit_params_for_call(node, &num_args);
      //  result_op = create_operand_none();
    //} 
    else if (strcmp(node->node_type, "ArrayAccess") == 0) { // arr[i]
        Operand array_op = Generate_IR(node->children[0]);
        Operand index_op = Generate_IR(node->children[1]);
        Type* array_type = node->children[0]->type; // Type of 'arr'
        
        // After semantic analysis, the node's type is the element type.
        Type* element_type = node->type; 

        if (array_type && (array_type->kind == TYPE_ARRAY || array_type->kind == TYPE_POINTER) && element_type) {
            int element_size = get_type_size(element_type); // Get size of element

            Operand offset_op = create_operand_temp();
            emit(IR_MUL, offset_op, index_op, create_operand_int(element_size));

            result_op = create_operand_temp();
            emit(IR_INDEX_LOAD, result_op, array_op, offset_op);
        } else {
            fprintf(stderr, "IR Generation Error: Attempting to index a non-array/pointer type.\n");
            // Return a NOP or default value to avoid cascading errors
            result_op = create_operand_int(0);
        }
    } else if (strcmp(node->node_type, "MemberAccess") == 0) { // s.m
        Operand struct_op = Generate_IR(node->children[0]);
        char* member_name = node->value;
        // The type of the struct is on the AST node from the semantic analysis phase.
        int offset = get_member_offset(node->children[0]->type, member_name);
        if (offset != -1) {
            //Operand base_addr_op = create_operand_temp();
            //emit(IR_ADDR_OF, base_addr_op, struct_op, create_operand_none());

            result_op = create_operand_temp();
            emit(IR_INDEX_LOAD, result_op, struct_op, create_operand_int(offset));
        } else {
            fprintf(stderr, "IR Generation Error: Member '%s' not found in struct.\n", member_name);
            // Handle error, maybe return a NOP or default value
        }

    } else if (strcmp(node->node_type, "PointerMemberAccess") == 0) { // s->m
        Operand struct_ptr_op = Generate_IR(node->children[0]);
        char* member_name = node->value; 
        // The type of the pointer is on the child node from the semantic analysis phase.
        int offset = get_member_offset(node->children[0]->type, member_name);

        if (offset != -1) {
            result_op = create_operand_temp();
            // The struct_ptr_op already holds the base address of the struct.
            // We emit: result = *(struct_ptr + offset)
            emit(IR_INDEX_LOAD, result_op, struct_ptr_op, create_operand_int(offset));
        } else {
            fprintf(stderr, "IR Generation Error: Member '%s' not found in struct pointed to.\n", member_name);
            result_op = create_operand_int(0); // Error recovery
        }

    } else if (strcmp(node->node_type, "IfStatement") == 0) {
        Operand cond_op = Generate_IR(node->children[0]);
        Operand label_end = create_operand_label();

        emit(IR_IF_FALSE_GOTO, label_end, cond_op, create_operand_none());
        Generate_IR(node->children[1]); // Then statement
        emit(IR_LABEL, label_end, create_operand_none(), create_operand_none());
        return create_operand_none();
    } else if (strcmp(node->node_type, "IfElseStatement") == 0) {
        Operand cond_op = Generate_IR(node->children[0]);
        Operand label_else = create_operand_label();
        Operand label_end = create_operand_label();

        emit(IR_IF_FALSE_GOTO, label_else, cond_op, create_operand_none());
        Generate_IR(node->children[1]); // Then statement
        emit(IR_GOTO, label_end, create_operand_none(), create_operand_none());
        emit(IR_LABEL, label_else, create_operand_none(), create_operand_none());
        Generate_IR(node->children[2]); // Else statement
        emit(IR_LABEL, label_end, create_operand_none(), create_operand_none());
        return create_operand_none();
    } else if (strcmp(node->node_type, "WhileStatement") == 0) {
        Operand label_loop_cond = create_operand_label();
        Operand label_loop_body = create_operand_label();
        Operand label_loop_end = create_operand_label();

        emit(IR_GOTO, label_loop_cond, create_operand_none(), create_operand_none());
        emit(IR_LABEL, label_loop_body, create_operand_none(), create_operand_none());
        Generate_IR(node->children[1]); // Loop body
        emit(IR_LABEL, label_loop_cond, create_operand_none(), create_operand_none());
        Operand cond_op = Generate_IR(node->children[0]); // Condition
        emit(IR_IF_FALSE_GOTO, label_loop_end, cond_op, create_operand_none());
        emit(IR_GOTO, label_loop_body, create_operand_none(), create_operand_none());
        emit(IR_LABEL, label_loop_end, create_operand_none(), create_operand_none());
        return create_operand_none();
    } else if (strcmp(node->node_type, "DoWhileStatement") == 0) {
        Operand label_loop_start = create_operand_label();
        Operand label_loop_end = create_operand_label();

        emit(IR_LABEL, label_loop_start, create_operand_none(), create_operand_none());
        Generate_IR(node->children[0]); // Loop body
        Operand cond_op = Generate_IR(node->children[1]); // Condition
        emit(IR_IF_FALSE_GOTO, label_loop_end, cond_op, create_operand_none());
        emit(IR_GOTO, label_loop_start, create_operand_none(), create_operand_none());
        emit(IR_LABEL, label_loop_end, create_operand_none(), create_operand_none());
        return create_operand_none();
    } else if (strcmp(node->node_type, "ForStatement") == 0) {
        // ForStatement: init_expr, cond_expr, update_expr, body
        Operand label_loop_cond = create_operand_label();
        Operand label_loop_incr = create_operand_label();
        Operand label_loop_body = create_operand_label();
        Operand label_loop_end = create_operand_label();
        current_continue_label = label_loop_incr;
        current_break_label = label_loop_end;

        Generate_IR(node->children[0]); // Initialization expression (expression_opt)
        emit(IR_GOTO, label_loop_cond, create_operand_none(), create_operand_none());
        emit(IR_LABEL, label_loop_body, create_operand_none(), create_operand_none());
        Generate_IR(node->children[3]); // Loop body (statement)
        emit(IR_LABEL, label_loop_incr, create_operand_none(), create_operand_none());
        Generate_IR(node->children[2]); // Update expression (expression_opt)
        emit(IR_LABEL, label_loop_cond, create_operand_none(), create_operand_none());
        Operand cond_op = Generate_IR(node->children[1]); // Condition (expression_opt)
        emit(IR_IF_FALSE_GOTO, label_loop_end, cond_op, create_operand_none());
        emit(IR_GOTO, label_loop_body, create_operand_none(), create_operand_none());
        current_continue_label.type = OP_NONE;
        current_break_label.type = OP_NONE;
        emit(IR_LABEL, label_loop_end, create_operand_none(), create_operand_none());
        return create_operand_none();
    } else if (strcmp(node->node_type, "ForDeclStatement") == 0) {
        // ForDeclStatement: declaration, cond_expr, update_expr, body
        Operand label_loop_cond = create_operand_label();
        Operand label_loop_body = create_operand_label();
        Operand label_loop_end = create_operand_label();
        current_continue_label = label_loop_cond;
        current_break_label = label_loop_end;

        Generate_IR(node->children[0]); // Declaration
        emit(IR_GOTO, label_loop_cond, create_operand_none(), create_operand_none());
        emit(IR_LABEL, label_loop_body, create_operand_none(), create_operand_none());
        Generate_IR(node->children[3]); // Loop body (statement)
        Generate_IR(node->children[2]); // Update expression (expression_opt)
        emit(IR_LABEL, label_loop_cond, create_operand_none(), create_operand_none());
        Operand cond_op = Generate_IR(node->children[1]); // Condition (expression_opt)
        emit(IR_IF_FALSE_GOTO, label_loop_end, cond_op, create_operand_none());
        emit(IR_GOTO, label_loop_body, create_operand_none(), create_operand_none());
        current_continue_label.type = OP_NONE;
        current_break_label.type = OP_NONE;
        emit(IR_LABEL, label_loop_end, create_operand_none(), create_operand_none());
        return create_operand_none();
    } else if (strcmp(node->node_type, "BreakStatement") == 0) {
        if (current_break_label.type == OP_NONE) {
            fprintf(stderr, "Error: Break statement outside of loop or switch.\n");
        } else {
            // Emit a GOTO to the break label
            emit(IR_GOTO, current_break_label, create_operand_none(), create_operand_none());
        }
        return create_operand_none();
    } else if (strcmp(node->node_type, "ContinueStatement") == 0) {
        if (current_continue_label.type == OP_NONE) {
            fprintf(stderr, "Error: Continue statement outside of loop.\n");
        } else {
            // Emit a GOTO to the continue label
            emit(IR_GOTO, current_continue_label, create_operand_none(), create_operand_none());
        }
        return create_operand_none();
    } else if (strcmp(node->node_type, "CaseStatement") == 0) {
        // This is a labeled statement. First, emit the label.
        // The label was created and stored on the node's type field during the SwitchStatement pass.
        if (node->type) {
            emit(IR_LABEL, create_operand_label_named((char*)node->type), create_operand_none(), create_operand_none());
        }
        // Then, generate code for the statement that follows the label.
        Generate_IR(node->children[1]);
        return create_operand_none();
    } else if (strcmp(node->node_type, "DefaultStatement") == 0) {
        if (node->type) {
            emit(IR_LABEL, create_operand_label_named((char*)node->type), create_operand_none(), create_operand_none());
        }
        Generate_IR(node->children[0]);
        return create_operand_none();
    } else if (strcmp(node->node_type, "SwitchStatement") == 0) {
        Operand switch_val = Generate_IR(node->children[0]);
        Operand end_label = create_operand_label();
        Operand default_label = create_operand_none();
        Operand old_break_label = current_break_label;
        current_break_label = end_label;

        ASTNode* body = node->children[1]; // This is a CompoundStatement
        ASTNode* block_item_list_node = (body->num_children > 0) ? body->children[0] : NULL;

        // Pass 1: Find all case/default statements and create IR labels for them.
        // We store the label's name string in the AST node's `type` field for later retrieval.
        if (block_item_list_node) {
            for (int i = 0; i < block_item_list_node->num_children; i++) {
                ASTNode* stmt = block_item_list_node->children[i];
                if (strcmp(stmt->node_type, "CaseStatement") == 0) {
                    stmt->type = (Type*)create_operand_label().val.name;
                } else if (strcmp(stmt->node_type, "DefaultStatement") == 0) {
                    default_label = create_operand_label();
                    stmt->type = (Type*)default_label.val.name;
                }
            }
        }

        // Pass 2: Generate the series of conditional jumps (the "switch" logic).
        if (block_item_list_node) {
            for (int i = 0; i < block_item_list_node->num_children; i++) {
                ASTNode* stmt = block_item_list_node->children[i];
                if (strcmp(stmt->node_type, "CaseStatement") == 0) {
                    Operand case_val = Generate_IR(stmt->children[0]);
                    Operand case_label = create_operand_label_named((char*)stmt->type);
                    Operand condition = create_operand_temp();
                    emit(IR_EQ, condition, switch_val, case_val);
                    // If the condition is true, jump to the case label.
                    emit(IR_IF_TRUE_GOTO, case_label, condition, create_operand_none());
                }
            }
        }

        // After all case comparisons, jump to the default label or the end.
        if (default_label.type != OP_NONE) {
            emit(IR_GOTO, default_label, create_operand_none(), create_operand_none());
        } else {
            emit(IR_GOTO, end_label, create_operand_none(), create_operand_none());
        }

        // Pass 3: Generate the IR for the switch body. This will emit the labels and statements in order.
        Generate_IR(body);
        emit(IR_LABEL, end_label, create_operand_none(), create_operand_none());
        current_break_label = old_break_label; // Restore old break label
        return create_operand_none();
    }
    // Default: If a node type is not explicitly handled, recursively process its children.
    else {
        fprintf(stderr, "Warning: Unhandled AST node type for IR generation: %s\n", node->node_type);
        for (int i = 0; i < node->num_children; i++) {
            Generate_IR(node->children[i]);
        }
        return create_operand_none();
    }

    return result_op;
}