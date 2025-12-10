#ifndef SEMANTICS_H
#define SEMANTICS_H

#include "symbol_table.h"

/* AST Node Structure */
typedef struct ASTNode {
    char *node_type;
    char *value;
    int num_children;
    struct ASTNode **children;
    Type *type; // Add this field to carry type information
} ASTNode;

/* Function Prototypes from parser.y that are needed by semantics.c */
void yyerror(const char *s);

/* Semantic Analysis Function Prototypes */
Type* get_base_type_from_specifiers(ASTNode* specifiers_node);
int is_function_declarator(ASTNode* declarator_node);
void check_semantics(ASTNode *node);
char* get_declarator_name(ASTNode* declarator_node);
Type* build_declarator_type(Type* base_type, ASTNode* declarator_node);
ASTNode* get_function_parameters_node(ASTNode* declarator_node);
Symbol* build_parameter_list_from_ast(ASTNode* param_list_node);
void calculate_struct_layout(Type* struct_type, ASTNode* decl_list_node);
int get_member_offset(Type* struct_type, const char* member_name);
int evaluate_constant_expression(ASTNode* expr_node);

#endif // SEMANTICS_H