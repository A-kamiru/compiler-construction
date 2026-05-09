#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokens.h"

extern int yylex();
extern char* yytext;
extern FILE *yyin;

int line_num = 1; 
int lookahead;
int indent = 0;
int error_count = 0; // Tracks total errors instead of crashing
const char* current_context = "program"; // Tracks the active BNF rule

// --- ICG INFRASTRUCTURE ---
FILE *icg_file;       // Global file pointer for the TAC output
int temp_count = 0;   // Keeps track of t1, t2, t3...

// Generates a new temporary variable name (e.g., "t1")
char* new_temp() {
    temp_count++;
    char* temp = (char*)malloc(10);
    sprintf(temp, "t%d", temp_count);
    return temp;
}

int label_count = 0;  // Keeps track of L1, L2, L3...

// Generates a new label name (e.g., "L1")
char* new_label() {
    label_count++;
    char* label = (char*)malloc(10);
    sprintf(label, "L%d", label_count);
    return label;
}

// Helper function for tree structure
void print_indent() {
    for(int i = 0; i < indent; i++) {
        printf("    "); 
    }
}

// Token dictionary for pretty printing
const char* get_token_name(int token) {
    switch(token) {
        case INT: return "INT";
        case CHAR: return "CHAR";
        case RETURN: return "RETURN";
        case IF: return "IF";
        case ELSE: return "ELSE";
        case WHILE: return "WHILE";
        case FOR: return "FOR";
        case PRINTF: return "PRINTF";
        case NUMBER: return "NUMBER";
        case IDENTIFIER: return "IDENTIFIER";
        case STRING: return "STRING";
        case REL_OP: return "REL_OP";
        case ASSIGN_OP: return "ASSIGN_OP";
        case ARITH_OP: return "ARITH_OP";
        case '(': return "LPAREN";
        case ')': return "RPAREN";
        case '{': return "LBRACE";
        case '}': return "RBRACE";
        case ';': return "SEMICOLON";
        case ',': return "COMMA";
        case 0: return "EOF";
        default: return "UNKNOWN";
    }
}

// The Error Reporter
void syntax_error(const char* expected_desc) {
    printf("\n\n======================================================\n");
    printf("              🚨 SYNTAX ERROR DETECTED 🚨             \n");
    printf("======================================================\n");
    printf("Location   : Line %d\n", line_num);
    printf("BNF Rule   : <%s>\n", current_context);
    printf("Expected   : %s\n", expected_desc);
    printf("Found      : %s ['%s']\n", get_token_name(lookahead), yytext);
    printf("------------------------------------------------------\n");
    printf("DIAGNOSIS  : The parser was trying to construct a\n");
    printf("             <%s> but the token '%s' breaks\n", current_context, yytext);
    printf("             the grammar rules for this structure.\n");
    printf("======================================================\n\n");
    error_count++; // Tally the error instead of dying
}

// PANIC MODE RECOVERY
void synchronize() {
    printf("    [RECOVERY] Panic Mode! Skipping tokens to find a safe resumption point...\n");
    while (lookahead != ';' && lookahead != '}' && lookahead != 0) {
        lookahead = yylex();
    }
    if (lookahead == ';') {
        lookahead = yylex();
    }
}

// THE MATCH FUNCTION
void match(int expected_token) {
    if (lookahead == expected_token) {
        print_indent();
        printf("|_ %s ['%s']\n", get_token_name(expected_token), yytext);
        lookahead = yylex(); 
    } else {
        syntax_error(get_token_name(expected_token));
        synchronize(); 
    }
}

void parse_program();
void parse_function();
void parse_type();
void parse_parameters();
void parse_statement();
void parse_if_statement();
void parse_while_statement();
void parse_for_statement();
char* parse_expression(); 
char* parse_factor();

void parse_program() {
    current_context = "program";
    printf("\n--- STAGE 2: PARSE TREE GENERATION ---\n");
    printf("PROGRAM_START\n");
    indent++;
    
    while (lookahead != 0) {
        parse_function();
    }
    
    indent--;
    printf("PROGRAM_END\n");
}

void parse_type() {
    current_context = "type";
    if (lookahead == INT) match(INT);
    else if (lookahead == CHAR) match(CHAR);
    else {
        syntax_error("type 'int' or 'char'");
        synchronize(); 
    }
}

void parse_parameters() {
    current_context = "parameters";
    if (lookahead == INT || lookahead == CHAR) {
        parse_type();
        match(IDENTIFIER);
        while (lookahead == ',') {
            match(',');
            parse_type();
            match(IDENTIFIER);
        }
    }
}

void parse_function() {
    current_context = "function_definition";
    print_indent(); printf("FUNCTION_DEFINITION\n");
    indent++;
    
    parse_type();
    
    if (lookahead == IDENTIFIER) {
        print_indent(); printf("NAME\n");
        indent++;
        match(IDENTIFIER);
        indent--;
    } else {
        match(IDENTIFIER); 
    }
    
    match('(');
    parse_parameters(); 
    match(')');
    match('{');
    
    while (lookahead != '}' && lookahead != 0) {
        parse_statement();
    }
    
    match('}');
    indent--;
}

void parse_statement() {
    current_context = "statement";
    if (lookahead == IF) {
        parse_if_statement();
    } 
    else if (lookahead == WHILE) {
        parse_while_statement();
    } 
    else if (lookahead == FOR) {
        parse_for_statement();
    }
    else if (lookahead == RETURN) {
        current_context = "return_statement";
        print_indent(); printf("RETURN_STATEMENT\n");
        indent++;
        match(RETURN);
        parse_expression();
        match(';');
        indent--;
    }
    else if (lookahead == PRINTF) {
        current_context = "print_statement";
        print_indent(); printf("PRINTF_STATEMENT\n");
        indent++;
        match(PRINTF);
        match('(');
        match(STRING);
        match(',');
        parse_expression();
        match(')');
        match(';');
        indent--;
    }
    else if (lookahead == INT || lookahead == CHAR) {
        current_context = "declaration";
        print_indent(); printf("DECLARATION\n");
        indent++;
        parse_type();
        
        char var_name[100];
        strcpy(var_name, yytext); 
        match(IDENTIFIER);
        
        if (lookahead == ASSIGN_OP) {
            match(ASSIGN_OP);
            char* expr_result = parse_expression();
            fprintf(icg_file, "%s = %s\n", var_name, expr_result); // Write to file
        }
        match(';');
        indent--;
    }
    else {
        current_context = "assignment_or_call";
        print_indent(); printf("ASSIGNMENT_OR_CALL\n");
        indent++;
        
        char var_name[100];
        strcpy(var_name, yytext);
        match(IDENTIFIER);
        
        if (lookahead == ASSIGN_OP) {
            match(ASSIGN_OP);
            char* expr_result = parse_expression();
            fprintf(icg_file, "%s = %s\n", var_name, expr_result); // Write to file
        } else if (lookahead == '(') {
             match('(');
             if (lookahead != ')') {
                 parse_expression();
                 while (lookahead == ',') {
                     match(',');
                     parse_expression();
                 }
             }
             match(')');
             fprintf(icg_file, "CALL %s\n", var_name); // Write to file
        }
        match(';');
        indent--;
    }
}

void parse_while_statement() {
    current_context = "while_statement";
    print_indent(); printf("WHILE_LOOP\n");
    indent++;
    
    char* start_label = new_label();
    char* end_label = new_label();
    
    fprintf(icg_file, "LABEL %s:\n", start_label); // Write to file
    
    match(WHILE);         
    match('(');           
    char* condition = parse_expression();   
    match(')');           
    
    fprintf(icg_file, "ifFalse %s goto %s\n", condition, end_label); // Write to file
    
    match('{');           
    while (lookahead != '}' && lookahead != 0) {
        parse_statement();
    }
    match('}');           
    
    fprintf(icg_file, "goto %s\n", start_label); // Write to file
    fprintf(icg_file, "LABEL %s:\n", end_label); // Write to file
    indent--;
}

void parse_if_statement() {
    current_context = "if_statement";
    print_indent(); printf("IF_STATEMENT\n");
    indent++;
    
    match(IF);
    match('(');
    char* condition = parse_expression(); 
    match(')');
    
    char* else_label = new_label();
    char* end_label = new_label();
    
    fprintf(icg_file, "ifFalse %s goto %s\n", condition, else_label); // Write to file
    
    match('{');
    while (lookahead != '}' && lookahead != 0) {
        parse_statement();
    }
    match('}');
    
    fprintf(icg_file, "goto %s\n", end_label); // Write to file
    fprintf(icg_file, "LABEL %s:\n", else_label); // Write to file
    
    if (lookahead == ELSE) {
        print_indent(); printf("ELSE_BLOCK\n");
        indent++;
        match(ELSE);
        match('{');
        while (lookahead != '}' && lookahead != 0) {
            parse_statement();
        }
        match('}');
        indent--;
    }
    
    fprintf(icg_file, "LABEL %s:\n", end_label); // Write to file
    indent--;
}

void parse_for_statement() {
    current_context = "for_statement";
    print_indent(); printf("FOR_LOOP\n");
    indent++;
    
    match(FOR);
    match('(');
    
    match(IDENTIFIER);
    match(ASSIGN_OP);
    parse_expression();
    match(';');
    
    parse_expression();
    match(';');
    
    match(IDENTIFIER);
    match(ASSIGN_OP);
    parse_expression();
    match(')');
    match('{');
    
    while (lookahead != '}' && lookahead != 0) {
        parse_statement();
    }
    match('}');
    indent--;
}

char* parse_expression() {
    current_context = "expression";
    
    char* left_side = parse_factor();
    
    while (lookahead == ARITH_OP || lookahead == REL_OP) {
        char op[10];
        strcpy(op, yytext); 
        match(lookahead);
        
        char* right_side = parse_factor();
        char* temp = new_temp();
        
        fprintf(icg_file, "%s = %s %s %s\n", temp, left_side, op, right_side); // Write to file
        
        strcpy(left_side, temp);
    }
    
    return left_side;
}

char* parse_factor() {
    current_context = "factor";
    char* result = (char*)malloc(100);

    if (lookahead == NUMBER) {
        print_indent(); printf("NUMBER_LITERAL\n");
        indent++;
        strcpy(result, yytext); 
        match(NUMBER);
        indent--;
        return result;
    } 
    else if (lookahead == IDENTIFIER) {
        print_indent(); printf("VARIABLE_OR_CALL\n");
        indent++;
        strcpy(result, yytext); 
        match(IDENTIFIER);
        
        if (lookahead == '(') {
            match('(');
            if (lookahead != ')') {
                parse_expression();
                while(lookahead == ',') {
                    match(',');
                    parse_expression();
                }
            }
            match(')');
            
            char* temp = new_temp();
            fprintf(icg_file, "%s = CALL %s\n", temp, result); // Write to file
            indent--;
            return temp;
        }
        indent--;
        return result;
    } 
    else if (lookahead == '(') {
        match('(');
        char* temp = parse_expression();
        match(')');
        return temp;
    } 
    else {
        syntax_error("NUMBER, IDENTIFIER, or '('");
        synchronize();         
        strcpy(result, "ERROR"); 
        return result;           
    }
}

int main(int argc, char **argv) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            perror(argv[1]);
            return 1;
        }
    } else {
        printf("Usage: ./compiler sample.mini\n");
        return 1;
    }
    
    // Open the ICG output file
    icg_file = fopen("output.tac", "w");
    if (!icg_file) {
        printf("Error: Could not create output.tac file.\n");
        return 1;
    }
    
    // --- STAGE 1: LEXICAL ANALYSIS ---
    printf("\n======================================================\n");
    printf("              STAGE 1: LEXICAL ANALYSIS               \n");
    printf("======================================================\n");
    
    int current_token;
    while ((current_token = yylex()) != 0) {
        printf("TOKEN: %-15s | LEXEME: %-15s | LINE: %d\n", 
               get_token_name(current_token), yytext, line_num);
    }

    // --- REWIND THE TAPE ---
    rewind(yyin);
    extern void yyrestart(FILE *input_file);
    yyrestart(yyin);
    line_num = 1;

    // --- STAGE 2: SYNTAX ANALYSIS ---
    printf("\n======================================================\n");
    printf("              STAGE 2: SYNTAX ANALYSIS                \n");
    printf("======================================================\n");
    
    lookahead = yylex(); 
    parse_program();
    
    // --- STAGE 3: ICG COMPLETION ---
    fclose(icg_file); // Close the file safely
    
    // --- COMPILATION SUMMARY ---
    printf("\n======================================================\n");
    if (error_count == 0) {
        printf("       ✅ COMPILATION SUCCESSFUL (0 Errors)           \n");
        printf("       📄 ICG saved successfully to 'output.tac'      \n");
    } else {
        printf("       ❌ COMPILATION FAILED (%d Syntax Errors found) \n", error_count);
        printf("       ⚠️ ICG generation incomplete due to errors     \n");
    }
    printf("======================================================\n\n");
    
    return (error_count > 0) ? 1 : 0;
}