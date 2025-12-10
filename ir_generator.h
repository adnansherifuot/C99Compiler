#ifndef IR_GENERATOR_H
#define IR_GENERATOR_H

#include <stdio.h>
#include "semantics.h" // For ASTNode definition

/* --- Intermediate Representation (3-Address Code) --- */

typedef enum {
    IR_ADD, IR_SUB, IR_MUL, IR_DIV, IR_MOD,
    IR_ASSIGN,
    IR_EQ, IR_NE, IR_LT, IR_GT, IR_LE, IR_GE,
    IR_AND, IR_OR, IR_BIT_AND, IR_BIT_OR, IR_XOR,
    IR_SHL, IR_SHR,
    IR_UNARY_MINUS, IR_NOT, IR_BIT_NOT, IR_ADDR, IR_DEREF,
    IR_GOTO, IR_IF_FALSE_GOTO, IR_IF_TRUE_GOTO, 
    IR_ALLOC_HEAP, IR_FREE_HEAP,
    IR_ADDR_OF, IR_DEREF_LOAD, IR_DEREF_STORE,
    IR_INDEX_LOAD, IR_INDEX_STORE,
    IR_CALL, IR_PARAM, IR_RETURN,
    IR_LABEL,
    IR_NOP, // No operation
    IR_HALT // Special opcode for halting (added)
} OpCode;

typedef enum {
    OP_NONE, OP_INT_CONST, OP_FLOAT_CONST, OP_CHAR_CONST, OP_STRING_LITERAL,
    OP_IDENTIFIER, OP_TEMPORARY, OP_LABEL
} OperandType;

typedef struct {
    OperandType type;
    union {
        int int_val;
        double float_val;
        char char_val;
        char *str_val;
        char *name; // For identifiers, temporaries, labels
    } val;
} Operand;

typedef struct Instruction {
    OpCode opcode;
    Operand result;
    Operand arg1;
    Operand arg2;
    struct Instruction *next;
} Instruction;

/* --- Global Variables for IR --- */
extern Instruction *main_ir_head;
extern Instruction *other_funcs_ir_head;
extern Operand current_break_label;
extern Operand current_continue_label;


/* --- Function Prototypes for IR Generation --- */

// Main IR generation function, recursively traverses the AST
Operand Generate_IR(ASTNode *node);

// Performs peephole optimization on the generated IR
Instruction* optimize_ir(Instruction *head);

// Prints the generated IR to a file
void print_ir_to_file(const char *filename);

// Frees all memory associated with the IR lists
void free_ir_lists();

#endif // IR_GENERATOR_H