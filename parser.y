%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symbol_table.h" // Include our new symbol table header
#include "semantics.h"    // Include our new semantics header
#include "ir_generator.h" // Include our new IR generator header


/* Function Prototypes */
ASTNode* create_node(const char *node_type, const char *value, int num_children, ...);
void print_ast(ASTNode *node, int level);
void free_ast(ASTNode *node);

/* External declarations from the lexer */
extern int yylex();
extern int line_count;
extern char* yytext;
extern FILE* yyin;

void yyerror(const char *s);

/* Root of the AST */
ASTNode *ast_root = NULL;
int is_typedef_declaration = 0; // Flag to track typedef context

%}

/* Yacc/Bison Union for semantic values */
%union {
    char *str;
    struct ASTNode *node;
}

/* Token Definitions */
/* The lexer will return these token types */
%token <str> IDENTIFIER INT_CONST FLOAT_CONST CHAR_CONST STRING_LITERAL TYPENAME

/* Keywords */
%token KEYWORD_BREAK KEYWORD_CASE KEYWORD_CONST
%token KEYWORD_CONTINUE KEYWORD_DEFAULT KEYWORD_DO KEYWORD_ELSE
%token KEYWORD_ENUM
%token KEYWORD_IF 
%token KEYWORD_RESTRICT KEYWORD_RETURN
%token KEYWORD_SIZEOF  KEYWORD_STRUCT KEYWORD_SWITCH
%token KEYWORD_TYPEDEF KEYWORD_UNION
%token KEYWORD_VOLATILE KEYWORD_WHILE
%token KEYWORD_FOR

/* Operators */
%token OP_INCREMENT OP_DECREMENT OP_ARROW
%token  OP_SHIFT_LEFT OP_SHIFT_RIGHT
%token OP_LE OP_GE OP_EQ OP_NE OP_AND OP_OR
%token OP_ADD_ASSIGN OP_SUB_ASSIGN OP_MUL_ASSIGN OP_DIV_ASSIGN OP_MOD_ASSIGN

/* Single-character tokens can be used directly as characters */
/* e.g. '+' '-' '*' '/' '%' '=' '<' '>' '&' '|' '^' '~' '!' */

/* Keywords with string values (for type specifiers) */
%token <str> KEYWORD_CHAR KEYWORD_DOUBLE KEYWORD_FLOAT KEYWORD_INT KEYWORD_LONG
%token <str> KEYWORD_SHORT KEYWORD_SIGNED KEYWORD_UNSIGNED KEYWORD_VOID

/* Type Definitions for grammar rules */
%type <node> program external_declaration function_definition declaration
%type <node> statement
%type <node> declaration_specifiers storage_class_specifier init_declarator declarator pointer
%type <node> direct_declarator parameter_type_list parameter_list parameter_declaration type_name abstract_declarator direct_abstract_declarator type_qualifier
%type <node> compound_statement block_item_list block_item expression_statement
%type <node> struct_or_union_specifier struct_or_union struct_declaration_list struct_declaration initializer initializer_list enum_specifier enumerator_list enumerator
%type <node> specifier_qualifier_list struct_declarator_list struct_declarator
%type <node> assignment_expression_opt parameter_type_list_opt
%type <node> selection_statement iteration_statement labeled_statement jump_statement expression assignment_expression
%type <node> expression_opt
%type <node> type_specifier_node
%type <node> conditional_expression logical_or_expression logical_and_expression
%type <node> inclusive_or_expression exclusive_or_expression and_expression
%type <node> equality_expression relational_expression shift_expression
%type <node> additive_expression multiplicative_expression cast_expression
%type <node> unary_expression postfix_expression primary_expression argument_expression_list 
%type <str> type_specifier unary_operator assignment_operator
%type <node> init_declarator_list

%nonassoc LOWER_THAN_ELSE
%nonassoc KEYWORD_ELSE

%%

/* Grammar Rules */

program
    : external_declaration { $$ = create_node("Program", NULL, 1, $1); ast_root = $$; }
    | program external_declaration {
        // Append the new external_declaration to the existing Program node's flat list of children.
        $1->children = (ASTNode**) realloc($1->children, ($1->num_children + 1) * sizeof(ASTNode*));
        $1->children[$1->num_children] = $2;
        $1->num_children++;
        $$ = $1; // The root Program node remains the same.
      }
    ;

external_declaration  
    : function_definition { $$ = $1; }
    | declaration         { $$ = $1; }
    ;

function_definition
    : declaration_specifiers declarator compound_statement
      {
        char* func_name = get_declarator_name($2);
        if (func_name) {
            Type* return_type = get_base_type_from_specifiers($1);
            ASTNode* params_node = get_function_parameters_node($2);
            Symbol* params_list = build_parameter_list_from_ast(params_node);
            Type* func_type = create_function_type(return_type, params_list);
            // Insert function into the *current* (likely global) scope
            insert_symbol(func_name, func_type, SYM_FUNCTION);
            // The scope for the body will be handled during semantic analysis.
            if (params_list) {
                // Parameters will also be added to the scope during semantic analysis.
                
            }
            free(func_name);
        }
        $$ = create_node("FunctionDefinition", NULL, 3, $1, $2, $3);
      }
    ;


declaration
    : declaration_specifiers init_declarator_list ';'
      { 
        //printf("DEBUG: Processing declaration.\n");
        // This is now the central point for variable insertion.
        // We have both the type specifiers ($1) and the list of declarators ($2).
        Type* base_type = get_base_type_from_specifiers($1);
        ASTNode* declarator_list = $2;
        // The list is built backwards, so we traverse it to insert symbols.
        //printf("DEBUG: Base type determined for declaration.\n");
        while(declarator_list) {
            ASTNode* declarator = declarator_list->children[0];
            char* name = get_declarator_name(declarator);
            //printf("DEBUG: Found declarator for '%s'.\n", name ? name : "unnamed");
            if (name) {
                 if (is_function_declarator(declarator)) {
                    // It's a function declaration (prototype)
                    ASTNode* params_node = get_function_parameters_node(declarator);
                    Symbol* params_list = build_parameter_list_from_ast(params_node);
                    Type* func_type = create_function_type(base_type, params_list);
                    insert_symbol(name, func_type, SYM_FUNCTION);
                    // The symbol table takes ownership of func_type, so we don't free it here.
                } else {
                    //printf("DEBUG: Inserting variable '%s' into symbol table.\n", name);
                    // It's a variable, pointer, or array declaration.
                    // Build the full type from the base and the declarator structure.
                    Type* full_type = build_declarator_type(base_type, declarator);
                    insert_symbol(name, full_type, is_typedef_declaration ? SYM_TYPENAME : SYM_VARIABLE);
                    // The symbol table now owns the 'full_type' structure. 
                    // We should not free it here. If base_type was copied inside
                    // build_declarator_type, we might need to free the original base_type
                    // after the loop, but since it's shared, we don't.
                }
                free(name);
            }
            if (declarator_list->num_children > 1) {
                declarator_list = declarator_list->children[1];
            } else {
                break;
            }
        }
        // Note: We don't free base_type here because multiple variables might share it.
        $$ = create_node("Declaration", NULL, 2, $1, $2);
      }
    ;


declaration_specifiers
    : storage_class_specifier { $$ = $1; }
    | type_specifier_node { $$ = $1; }
    | type_qualifier { $$ = $1; }
    | storage_class_specifier declaration_specifiers { is_typedef_declaration = 1; $$ = create_node("DeclarationSpecifiers", NULL, 2, $1, $2); }
    | type_specifier_node declaration_specifiers { if (strcmp($1->node_type, "TypeName") != 0) is_typedef_declaration = 0; $$ = create_node("DeclarationSpecifiers", NULL, 2, $1, $2); }
    | type_qualifier declaration_specifiers { $$ = create_node("DeclarationSpecifiers", NULL, 2, $1, $2); }
    ;
 

storage_class_specifier
    : KEYWORD_TYPEDEF { $$ = create_node("StorageClassSpecifier", "typedef", 0); }
    ;


type_specifier
    : KEYWORD_VOID   { $$ = $1; }
    | KEYWORD_CHAR   { $$ = $1; }
    | KEYWORD_SHORT  { $$ = $1; }
    | KEYWORD_INT    { $$ = $1; }
    | KEYWORD_LONG   { $$ = $1; }
    | KEYWORD_FLOAT  { $$ = $1; }
    | KEYWORD_DOUBLE { $$ = $1; }
    | KEYWORD_SIGNED { $$ = $1; }
    | KEYWORD_UNSIGNED { $$ = $1; }
    ;


type_specifier_node
    : type_specifier { $$ = create_node("TypeSpecifier", $1, 0); }
    | TYPENAME { $$ = create_node("TypeName", $1, 0); }
    | struct_or_union_specifier { $$ = $1; }
    | enum_specifier { $$ = $1; }
    ;


init_declarator_list
    : init_declarator { $$ = create_node("InitDeclaratorList", NULL, 1, $1); }
    | init_declarator_list ',' init_declarator { $$ = create_node("InitDeclaratorList", NULL, 2, $3, $1); }
    ;

init_declarator
    : declarator {
        // Simply pass the declarator up. Insertion is handled in the 'declaration' rule.
        $$ = $1;
    }
    | declarator '=' initializer { $$ = create_node("InitDeclarator", "=", 2, $1, $3); }
    ;


declarator
    : pointer direct_declarator { $$ = create_node("PointerDeclarator", NULL, 2, $1, $2); }
    | direct_declarator { $$ = $1; }
    ;


type_name
   : specifier_qualifier_list { $$ = create_node("TypeName", NULL, 1, $1); }
    | specifier_qualifier_list abstract_declarator { $$ = create_node("TypeName", NULL, 2, $1, $2); }
    ;

abstract_declarator
    : pointer { $$ = $1; }
    | direct_abstract_declarator { $$ = $1; }
    | pointer direct_abstract_declarator { $$ = create_node("PointerAbstractDeclarator", NULL, 2, $1, $2); }
    ;


direct_abstract_declarator
    : '(' abstract_declarator ')' { $$ = create_node("ParenthesizedAbstractDeclarator", NULL, 1, $2); }
    | direct_abstract_declarator '[' assignment_expression_opt ']' { $$ = create_node("AbstractArraySuffix", NULL, 2, $1, $3); }
    | '[' assignment_expression_opt ']' { $$ = create_node("AbstractArraySuffix", NULL, 1, $2); } // Base case for array suffix
    | direct_abstract_declarator '(' parameter_type_list_opt ')' { $$ = create_node("AbstractFunctionSuffix", NULL, 2, $1, $3); }
    | '(' parameter_type_list_opt ')' { $$ = create_node("AbstractFunctionSuffix", NULL, 1, $2); } // Base case for function suffix
    ;


assignment_expression_opt
    : assignment_expression { $$ = $1; }
    | /* empty */ { $$ = create_node("EmptyExpression", NULL, 0); }
    ;


parameter_type_list_opt
    : parameter_type_list { $$ = $1; }
    | /* empty */ { $$ = create_node("EmptyParameterList", NULL, 0); }
    ;


/*
 * Unary Expressions
 */
unary_expression
    : postfix_expression { $$ = $1; }
    | OP_INCREMENT unary_expression { $$ = create_node("PrefixIncrement", "++", 1, $2); }
    | OP_DECREMENT unary_expression { $$ = create_node("PrefixDecrement", "--", 1, $2); }
    | unary_operator cast_expression { $$ = create_node("UnaryOp", $1, 1, $2); }
    | KEYWORD_SIZEOF unary_expression { $$ = create_node("SizeofUnaryExpr", NULL, 1, $2); }
    | KEYWORD_SIZEOF '(' type_name ')' { $$ = create_node("SizeofTypeExpr", NULL, 1, $3); }
    ;

/*
 * Primary Expressions
 */

pointer
    : '*' { $$ = create_node("Pointer", "*", 0); }
    | '*' pointer { $$ = create_node("Pointer", "*", 1, $2); }
    ;


struct_or_union_specifier
    : struct_or_union IDENTIFIER '{' struct_declaration_list '}'';' 
        { 
            // This is a struct/union definition with a tag.
            //printf("DEBUG: Parsing tagged struct/union definition: %s %s\n", $1->value, $2);
            char type_name[256];
            sprintf(type_name, "%s %s",$1->value, $2);
            Symbol* sym = lookup_symbol(type_name);
            Type* struct_type = NULL;

            if (sym) { // Found a previous declaration (possibly forward)
                struct_type = sym->type;
                if (struct_type->size > 0 || struct_type->data.record_info.members != NULL) {
                    yyerror("Redefinition of struct/union");
                }
                //printf("DEBUG: Found forward declaration for '%s'. Completing it.\n", type_name);
            } else { // No previous declaration found, create a new type.
                struct_type = create_aggregate_type(($1->value[0] == 's') ? TYPE_STRUCT : TYPE_UNION, type_name);
                insert_symbol(type_name, struct_type, SYM_TYPENAME);
                //printf("DEBUG: Created new aggregate type for '%s'.\n", type_name);
            }
            
            // Now, complete the type by calculating its layout.
            calculate_struct_layout(struct_type, $4);
            //printf("DEBUG: Calculated layout for '%s'. Size: %d\n", type_name, struct_type->size);

            $$ = create_node("StructSpecifier", $2, 1, $1); 
            $$->type = struct_type; // Attach the type to the node.
        }
    | struct_or_union '{' struct_declaration_list '}'
        { 
            //printf("DEBUG: Parsing anonymous struct/union definition.\n");
            // Anonymous struct/union definition. Always create a new type.
            Type* struct_type = create_aggregate_type(($1->value[0] == 's') ? TYPE_STRUCT : TYPE_UNION, "anonymous");
            calculate_struct_layout(struct_type, $3);
            //printf("DEBUG: Calculated layout for anonymous struct. Size: %d\n", struct_type->size);
            $$ = create_node("StructSpecifier", "anonymous", 1, $1); 
            $$->type = struct_type;
            //printf("DEBUG: Anonymous struct node created and type attached.\n");
        }
    | struct_or_union IDENTIFIER
        { 
            // This is a forward declaration or a usage of an already declared struct.
            char type_name[256];
            sprintf(type_name, "%s %s",$1->value,  $2);
            Symbol* sym = lookup_symbol(type_name);
            //printf("DEBUG: Parsing usage of struct/union: %s\n", type_name);
            Type* struct_type = NULL;
            if (!sym) { // Not found, so this is a forward declaration.
                struct_type = create_aggregate_type(($1->value[0] == 's') ? TYPE_STRUCT : TYPE_UNION, type_name);
                insert_symbol(type_name, struct_type, SYM_TYPENAME);
                // printf("DEBUG: Forward-declaring struct/union '%s'.\n", type_name);
            } else {
                struct_type = sym->type;
                // printf("DEBUG: Found existing struct/union type for '%s'.\n", type_name);
            }
            $$ = create_node("StructSpecifier", $2, 1, $1); 
            $$->type = struct_type; // Attach the (possibly incomplete) type.
        }
    ;


struct_or_union
    : KEYWORD_STRUCT { $$ = create_node("StructToken", "struct", 0); }
    | KEYWORD_UNION  { $$ = create_node("UnionToken", "union", 0); }
    ;


struct_declaration_list
    : struct_declaration { $$ = create_node("StructDeclarationList", NULL, 1, $1); }
    | struct_declaration_list struct_declaration { $$ = create_node("StructDeclarationList", NULL, 2, $1, $2); }
    ;

struct_declaration
    : specifier_qualifier_list struct_declarator_list ';' { $$ = create_node("StructDeclaration", NULL, 2, $1, $2); }
    ;

specifier_qualifier_list
    : type_specifier_node { $$ = create_node("SpecifierQualifierList", NULL, 1, $1); }
    | type_qualifier { $$ = create_node("SpecifierQualifierList", NULL, 1, $1); }
    | type_specifier_node specifier_qualifier_list { $$ = create_node("SpecifierQualifierList", NULL, 2, $1, $2); }
    | type_qualifier specifier_qualifier_list { $$ = create_node("SpecifierQualifierList", NULL, 2, $1, $2); }
    ;


struct_declarator_list
    : struct_declarator { $$ = create_node("StructDeclaratorList", NULL, 1, $1); }
    | struct_declarator_list ',' struct_declarator { $$ = create_node("StructDeclaratorList", NULL, 2, $1, $3); }
    ;

struct_declarator
    : declarator { $$ = $1; }
    ;


type_qualifier
    : KEYWORD_CONST { $$ = create_node("TypeQualifier", "const", 0); }
    | KEYWORD_VOLATILE { $$ = create_node("TypeQualifier", "volatile", 0); }
    | KEYWORD_RESTRICT { $$ = create_node("TypeQualifier", "restrict", 0); }
    ;

enum_specifier
    : KEYWORD_ENUM IDENTIFIER '{' enumerator_list '}' { $$ = create_node("EnumSpecifier", $2, 1, $4); }
    | KEYWORD_ENUM '{' enumerator_list '}' { $$ = create_node("EnumSpecifier", "anonymous", 1, $3); }
    | KEYWORD_ENUM IDENTIFIER '{' enumerator_list ',' '}' { $$ = create_node("EnumSpecifier", $2, 1, $4); }
    | KEYWORD_ENUM '{' enumerator_list ',' '}' { $$ = create_node("EnumSpecifier", "anonymous", 1, $3); }
    | KEYWORD_ENUM IDENTIFIER { $$ = create_node("EnumSpecifier", $2, 0); }
    ;

enumerator_list
    : enumerator { $$ = create_node("EnumeratorList", NULL, 1, $1); }
    | enumerator_list ',' enumerator { $$ = create_node("EnumeratorList", NULL, 2, $1, $3); }
    ;

enumerator
    : IDENTIFIER { $$ = create_node("Enumerator", $1, 0); }
    | IDENTIFIER '=' conditional_expression { $$ = create_node("Enumerator", $1, 1, $3); }
    ;

initializer
    : assignment_expression { $$ = $1; }
    | '{' initializer_list '}' { $$ = create_node("InitializerList", NULL, 1, $2); }
    ;


initializer_list
    : assignment_expression { $$ = create_node("InitValues", NULL, 1, $1); }
    | initializer_list ',' assignment_expression { $$ = create_node("InitValues", NULL, 2, $1, $3); }
    ;

direct_declarator
    : IDENTIFIER { $$ = create_node("Identifier", $1, 0); }
    | '(' declarator ')' { $$ = $2; }
    | direct_declarator '(' parameter_type_list ')' { $$ = create_node("FunctionDeclarator", NULL, 2, $1, $3); }
    | direct_declarator '(' ')' { $$ = create_node("FunctionDeclarator", "()", 1, $1); }
    | direct_declarator '[' assignment_expression ']' { $$ = create_node("ArrayDeclarator", NULL, 2, $1, $3); }
    | direct_declarator '[' ']' { $$ = create_node("ArrayDeclarator", "unspecified_size", 1, $1); }
    ;


parameter_type_list
    : parameter_list { $$ = $1; }
    ;


parameter_list
    : parameter_declaration { $$ = $1; }
    | parameter_list ',' parameter_declaration { $$ = create_node("ParameterList", NULL, 2, $1, $3); }
    ;
 
parameter_declaration
    : declaration_specifiers declarator { $$ = create_node("ParameterDeclaration", NULL, 2, $1, $2); }
    | declaration_specifiers { $$ = create_node("ParameterDeclaration", NULL, 1, $1); } /* For void */
    ;

compound_statement
    : '{' '}' { 
        $$ = create_node("CompoundStatement", "{}", 0); 
      }
    | '{' block_item_list '}' { 
        $$ = create_node("CompoundStatement", NULL, 1, $2); 
      }
    ;


block_item_list
    : block_item { $$ = create_node("BlockItemList", NULL, 1, $1); }
    | block_item_list block_item {
        // Append the new block_item to the existing flat list
        $1->children = (ASTNode**) realloc($1->children, ($1->num_children + 1) * sizeof(ASTNode*));
        $1->children[$1->num_children] = $2;
        $1->num_children++;
        $$ = $1;
      }
    ;

block_item
    : declaration { $$ = $1; }
    | statement   { $$ = $1; }
    ;

statement
    : expression_statement { $$ = $1; }
    | compound_statement   { $$ = $1; } 
    | selection_statement  { $$ = $1; } 
    | iteration_statement  { $$ = $1; }
    | jump_statement       { $$ = $1; }
    | labeled_statement    { $$ = $1; }
    ;

expression_statement
    : ';' { $$ = create_node("EmptyStatement", ";", 0); }
    | expression ';' { $$ = create_node("ExpressionStatement", NULL, 1, $1); }
    ;

expression_opt
    : expression { $$ = $1; }
    | /* empty */ { $$ = create_node("EmptyExpression", NULL, 0); }
    ;


iteration_statement
    : KEYWORD_WHILE '(' expression ')' statement { $$ = create_node("WhileStatement", NULL, 2, $3, $5); }
    | KEYWORD_DO statement KEYWORD_WHILE '(' expression ')' ';' { $$ = create_node("DoWhileStatement", NULL, 2, $2, $5); }
    | KEYWORD_FOR '(' expression_opt ';' expression_opt ';' expression_opt ')' statement { $$ = create_node("ForStatement", NULL, 4, $3, $5, $7, $9); }
    | KEYWORD_FOR '(' declaration expression_opt ';' expression_opt ')' statement { $$ = create_node("ForDeclStatement", NULL, 4, $3, $4, $6, $8); }
    | error statement { yyerror("Invalid iteration statement"); $$ = $2; }
    ;

selection_statement
    : KEYWORD_IF '(' expression ')' statement %prec LOWER_THAN_ELSE { $$ = create_node("IfStatement", NULL, 2, $3, $5); }
    | KEYWORD_IF '(' expression ')' statement KEYWORD_ELSE statement { $$ = create_node("IfElseStatement", NULL, 3, $3, $5, $7); }
    | KEYWORD_SWITCH '(' expression ')' statement { $$ = create_node("SwitchStatement", NULL, 2, $3, $5); }
    ;



labeled_statement
    : IDENTIFIER ':' statement { $$ = create_node("LabeledStatement", $1, 1, $3); }
    | KEYWORD_CASE conditional_expression ':' statement { $$ = create_node("CaseStatement", NULL, 2, $2, $4); }
    | KEYWORD_DEFAULT ':' statement { $$ = create_node("DefaultStatement", NULL, 1, $3); }
    ;

jump_statement
    : KEYWORD_RETURN ';' { $$ = create_node("Return", NULL, 0); }
    | KEYWORD_RETURN expression ';' { $$ = create_node("Return", NULL, 1, $2); }
    | KEYWORD_BREAK ';' { $$ = create_node("BreakStatement", NULL, 0); }
    | KEYWORD_CONTINUE ';' { $$ = create_node("ContinueStatement", NULL, 0); }
    ;

expression
    : assignment_expression { $$ = $1; }
    | expression ',' assignment_expression { $$ = create_node("BinaryOp", ",", 2, $1, $3); }
    ;

assignment_expression
    : conditional_expression { $$ = $1; }
    | unary_expression assignment_operator assignment_expression { $$ = create_node("Assignment",NULL, 2, $1,$3); }
    ;


assignment_operator
    : '=' { $$ = "="; } | OP_MUL_ASSIGN { $$ = "*="; } | OP_DIV_ASSIGN { $$ = "/="; }
    | OP_MOD_ASSIGN { $$ = "%="; } | OP_ADD_ASSIGN { $$ = "+="; } | OP_SUB_ASSIGN { $$ = "-="; }
    ;

conditional_expression
    : logical_or_expression { $$ = $1; }
    ;


logical_or_expression
    : logical_and_expression { $$ = $1; }
    | logical_or_expression OP_OR logical_and_expression { $$ = create_node("BinaryOp", "||", 2, $1, $3); }
    ;


logical_and_expression
    : inclusive_or_expression { $$ = $1; }
    | logical_and_expression OP_AND inclusive_or_expression { $$ = create_node("BinaryOp", "&&", 2, $1, $3); }
    ;


inclusive_or_expression
    : exclusive_or_expression { $$ = $1; }
    | inclusive_or_expression '|' exclusive_or_expression { $$ = create_node("BinaryOp", "|", 2, $1, $3); }
    ;

exclusive_or_expression
    : and_expression { $$ = $1; }
    | exclusive_or_expression '^' and_expression { $$ = create_node("BinaryOp", "^", 2, $1, $3); }
    ;

and_expression
    : equality_expression { $$ = $1; }
    | and_expression '&' equality_expression { $$ = create_node("BinaryOp", "&", 2, $1, $3); }
    ;

equality_expression
    : relational_expression { $$ = $1; }
    | equality_expression OP_EQ relational_expression { $$ = create_node("BinaryOp", "==", 2, $1, $3); }
    | equality_expression OP_NE relational_expression { $$ = create_node("BinaryOp", "!=", 2, $1, $3); }
    ;


relational_expression
    : shift_expression { $$ = $1; }
    | relational_expression '<' shift_expression { $$ = create_node("BinaryOp", "<", 2, $1, $3); }
    | relational_expression '>' shift_expression { $$ = create_node("BinaryOp", ">", 2, $1, $3); }
    | relational_expression OP_LE shift_expression { $$ = create_node("BinaryOp", "<=", 2, $1, $3); }
    | relational_expression OP_GE shift_expression { $$ = create_node("BinaryOp", ">=", 2, $1, $3); }
    ;


shift_expression
    : additive_expression { $$ = $1; }
    | shift_expression OP_SHIFT_LEFT additive_expression { $$ = create_node("BinaryOp", "<<", 2, $1, $3); }
    | shift_expression OP_SHIFT_RIGHT additive_expression { $$ = create_node("BinaryOp", ">>", 2, $1, $3); }
    ;

additive_expression
    : multiplicative_expression { $$ = $1; }
    | additive_expression '+' multiplicative_expression { $$ = create_node("BinaryOp", "+", 2, $1, $3); }
    | additive_expression '-' multiplicative_expression { $$ = create_node("BinaryOp", "-", 2, $1, $3); }
    ;

multiplicative_expression
    : cast_expression { $$ = $1; }
    | multiplicative_expression '*' cast_expression { $$ = create_node("BinaryOp", "*", 2, $1, $3); }
    | multiplicative_expression '/' cast_expression { $$ = create_node("BinaryOp", "/", 2, $1, $3); }
    | multiplicative_expression '%' cast_expression { $$ = create_node("BinaryOp", "%", 2, $1, $3); }
    ;


cast_expression
    : unary_expression { $$ = $1; }
    ;

unary_operator
    : '&' { $$ = "&"; } | '*' { $$ = "*"; } | '+' { $$ = "+"; } | '-' { $$ = "-"; } | '~' { $$ = "~"; } | '!' { $$ = "!"; }
    ;

postfix_expression
    : postfix_expression '(' ')' { $$ = create_node("FunctionCall", NULL, 1, $1); }
    | postfix_expression '(' argument_expression_list ')' { $$ = create_node("FunctionCall", NULL, 2, $1, $3); }
    | postfix_expression OP_INCREMENT { $$ = create_node("PostfixIncrement", "++", 1, $1); }
    | postfix_expression OP_DECREMENT { $$ = create_node("PostfixDecrement", "--", 1, $1); }
    | postfix_expression '[' expression ']' { $$ = create_node("ArrayAccess", NULL, 2, $1, $3); }
    | postfix_expression '.' IDENTIFIER { $$ = create_node("MemberAccess", $3, 1, $1); }
    | postfix_expression OP_ARROW IDENTIFIER { $$ = create_node("PointerMemberAccess", $3, 1, $1); }
    | primary_expression { $$ = $1; }
    ;


argument_expression_list
    : assignment_expression { $$ = $1; }
    | argument_expression_list ',' assignment_expression { $$ = create_node("ArgumentList", NULL, 2, $1, $3); }
    ;

primary_expression
    : IDENTIFIER { $$ = create_node("Identifier", $1, 0); }
    | INT_CONST { $$ = create_node("IntConstant", $1, 0); }
    | FLOAT_CONST { $$ = create_node("FloatConstant", $1, 0); }
    | CHAR_CONST { $$ = create_node("CharConstant", $1, 0); }
    | STRING_LITERAL { $$ = create_node("StringLiteral", $1, 0); }
    | '(' expression ')' { $$ = $2; }
    ;

%%

/* C Code Section */
#include <stdarg.h>

ASTNode* create_node(const char *node_type, const char *value, int num_children, ...) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    if (!node) {
        yyerror("Out of memory");
        exit(1);
    }
    node->node_type = strdup(node_type);
    node->value = value ? strdup(value) : NULL;
    node->num_children = num_children;
    if (num_children > 0) {
        node->children = NULL; // Initialize to NULL
        node->children = (ASTNode**) malloc(num_children * sizeof(ASTNode*));
        va_list ap;
        va_start(ap, num_children);
        for (int i = 0; i < num_children; i++) {
            node->children[i] = va_arg(ap, ASTNode*);
        }
        va_end(ap);
    } else {
        node->children = NULL;
    }
    return node;
}

void print_ast(ASTNode *node, int level) {
    if (!node) return;
    for (int i = 0; i < level; i++) printf("  ");
    printf("%s", node->node_type);
    if (node->value) {
        if(node->type)
            printf(" (%s,%s)", node->value, node->type->data.base_name);
        else
            printf(" (%s,%s)", node->value, "unknown");
    }
    printf("\n");

    for (int i = 0; i < node->num_children; i++) {
        print_ast(node->children[i], level + 1);
    }
}

void free_ast(ASTNode *node) {
    if (!node) return;
    for (int i = 0; i < node->num_children; i++) {
        free_ast(node->children[i]);
    }
    // If the node has a type that was created just for it (e.g., for constants), free it.
    if (node->type && (strcmp(node->node_type, "IntConstant") == 0 || strcmp(node->node_type, "FloatConstant") == 0 || strcmp(node->node_type, "CharConstant") == 0)) {
        free_type(node->type);
    }
    free(node->node_type);
    if (node->value) free(node->value);
    if (node->children) free(node->children);
    free(node);
}

void yyerror(const char *s) {
    fprintf(stderr, "Parse error on line %d near '%s': %s\n", line_count, yytext, s);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <sourcefile.c> <destinationfile.3ac>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Cannot open input file");
        return 1;
    }

    yyin = file;
    init_symbol_table(); // Initialize the symbol table
    yyparse();
    fclose(file);

    if (ast_root) {
        printf("\n--- Abstract Syntax Tree ---\n");
        print_ast(ast_root, 0);
        printf("----------------------------\n\n");
        printf("--- Semantic Analysis ---\n");
        check_semantics(ast_root); // Perform semantic checks
        printf("--- Semantic Analysis Complete ---\n\n");
        print_ast(ast_root, 0);
        printf("----------------------------\n");
        printf("\n--- Generating 3-Address Code ---\n");
        Generate_IR(ast_root); // Generate IR
        printf("--- 3-Address Code Generated ---\n");
        printf("\n--- Performing Peephole Optimization ---\n");
        main_ir_head = optimize_ir(main_ir_head);
        other_funcs_ir_head = optimize_ir(other_funcs_ir_head);
        printf("--- Optimization Complete ---\n\n");

        print_ir_to_file(argv[2]); // Save IR to file
        printf("--- 3-Address Code Generated to %s ---\n", argv[2]);
        printf("---------------------------\n");
        //free_ir_lists(); // Free the IR lists
        //free_ast(ast_root);
    } else {
        printf("Parsing failed, no AST generated.\n");
    }
    print_symbol_table(); // Print symbol table contents
    cleanup_symbol_table(); // Free all remaining symbols and types

    return 0;
}