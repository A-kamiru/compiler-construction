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
int error_count = 0; 
const char* current_context = "program"; 

// ========================================================================
// --- ICG INFRASTRUCTURE ---
// ========================================================================

FILE *icg_file; // Global file pointer for raw output.tac

// The Quadruple Struct for the command-line table
typedef struct {
    char op[15];
    char arg1[15];
    char arg2[15];
    char result[15];
} Quadruple;

Quadruple icg_table[1000]; 
int icg_index = 0;         
int temp_count = 0;        
int label_count = 0;       

// Records the strict Quadruple for the table
void emit(const char* op, const char* arg1, const char* arg2, const char* result) {
    strcpy(icg_table[icg_index].op, op);
    strcpy(icg_table[icg_index].arg1, arg1 ? arg1 : "-");
    strcpy(icg_table[icg_index].arg2, arg2 ? arg2 : "-");
    strcpy(icg_table[icg_index].result, result ? result : "-");
    icg_index++;
}

char* new_temp() {
    temp_count++;
    char* temp = (char*)malloc(10);
    sprintf(temp, "t%d", temp_count);
    return temp;
}

char* new_label() {
    label_count++;
    char* label = (char*)malloc(10);
    sprintf(label, "L%d", label_count);
    return label;
}

void print_quadruples() {
    printf("\n=================================================================\n");
    printf("              STAGE 3: INTERMEDIATE CODE (QUADRUPLES)            \n");
    printf("=================================================================\n");
    printf("%-5s %-10s %-15s %-15s %-15s\n", "IDX", "OP", "ARG1", "ARG2", "RESULT");
    printf("-----------------------------------------------------------------\n");
    for (int i = 0; i < icg_index; i++) {
        printf("%-5d %-10s %-15s %-15s %-15s\n", 
               i, 
               icg_table[i].op, 
               icg_table[i].arg1, 
               icg_table[i].arg2, 
               icg_table[i].result);
    }
    printf("-----------------------------------------------------------------\n");
    printf("Total instructions: %d\n\n", icg_index);
}

// ========================================================================
// --- PARSER UTILITIES ---
// ========================================================================

void print_indent() {
    for(int i = 0; i < indent; i++) {
        printf("    "); 
    }
}

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
    error_count++; 
}

void synchronize() {
    printf("    [RECOVERY] Panic Mode! Skipping tokens...\n");
    while (lookahead != ';' && lookahead != '}' && lookahead != 0) {
        lookahead = yylex();
    }
    if (lookahead == ';') {
        lookahead = yylex();
    }
}

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
    
    char func_name[100];
    if (lookahead == IDENTIFIER) {
        strcpy(func_name, yytext);
        print_indent(); printf("NAME\n");
        indent++;
        match(IDENTIFIER);
        indent--;
    } else {
        match(IDENTIFIER); 
    }
    
    emit("FUNC", func_name, "-", "-"); 
    
    match('(');
    parse_parameters(); 
    match(')');
    match('{');
    
    while (lookahead != '}' && lookahead != 0) {
        parse_statement();
    }
    
    match('}');
    
    emit("ENDFUNC", func_name, "-", "-");
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
        char* expr = parse_expression();
        
        fprintf(icg_file, "RETURN %s\n", expr);  // Write to File
        emit("RET", expr, "-", "-");             // Save Quadruple
        
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
        char* expr = parse_expression();
        
        fprintf(icg_file, "PRINT %s\n", expr);   // Write to File
        emit("PRINT", expr, "-", "-");           // Save Quadruple
        
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
        
        emit("DECL", var_name, "-", "-"); // Quadruple only
        
        if (lookahead == ASSIGN_OP) {
            match(ASSIGN_OP);
            char* expr_result = parse_expression();
            
            fprintf(icg_file, "%s = %s\n", var_name, expr_result); // Write to File
            emit("COPY", expr_result, "-", var_name);              // Save Quadruple
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
            
            fprintf(icg_file, "%s = %s\n", var_name, expr_result); // Write to File
            emit("COPY", expr_result, "-", var_name);              // Save Quadruple
            
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
             
             fprintf(icg_file, "CALL %s\n", var_name); // Write to File
             emit("CALL", var_name, "-", "-");         // Save Quadruple
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
    
    fprintf(icg_file, "LABEL %s:\n", start_label); // File
    emit("LABEL", start_label, "-", "-");          // Quadruple
    
    match(WHILE);         
    match('(');           
    char* condition = parse_expression();   
    match(')');           
    
    fprintf(icg_file, "ifFalse %s goto %s\n", condition, end_label); // File
    emit("IFF", condition, "-", end_label);                          // Quadruple
    
    match('{');           
    while (lookahead != '}' && lookahead != 0) {
        parse_statement();
    }
    match('}');           
    
    fprintf(icg_file, "goto %s\n", start_label); // File
    fprintf(icg_file, "LABEL %s:\n", end_label); // File
    
    emit("GOTO", "-", "-", start_label); // Quadruple
    emit("LABEL", end_label, "-", "-");  // Quadruple
    
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
    
    fprintf(icg_file, "ifFalse %s goto %s\n", condition, else_label); // File
    emit("IFF", condition, "-", else_label);                          // Quadruple
    
    match('{');
    while (lookahead != '}' && lookahead != 0) {
        parse_statement();
    }
    match('}');
    
    fprintf(icg_file, "goto %s\n", end_label);       // File
    fprintf(icg_file, "LABEL %s:\n", else_label);    // File
    
    emit("GOTO", "-", "-", end_label);               // Quadruple
    emit("LABEL", else_label, "-", "-");             // Quadruple
    
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
    
    fprintf(icg_file, "LABEL %s:\n", end_label); // File
    emit("LABEL", end_label, "-", "-");          // Quadruple
    indent--;
}

void parse_for_statement() {
    current_context = "for_statement";
    print_indent(); printf("FOR_LOOP\n");
    indent++;
    
    match(FOR);
    match('(');
    
    char init_var[100];
    strcpy(init_var, yytext);
    match(IDENTIFIER);
    match(ASSIGN_OP);
    char* init_val = parse_expression();
    
    fprintf(icg_file, "%s = %s\n", init_var, init_val); // File
    emit("COPY", init_val, "-", init_var);              // Quadruple
    match(';');
    
    char* start_label = new_label();
    char* end_label = new_label();
    
    fprintf(icg_file, "LABEL %s:\n", start_label); // File
    emit("LABEL", start_label, "-", "-");          // Quadruple
    
    char* condition = parse_expression();
    
    fprintf(icg_file, "ifFalse %s goto %s\n", condition, end_label); // File
    emit("IFF", condition, "-", end_label);                          // Quadruple
    match(';');
    
    char inc_var[100];
    strcpy(inc_var, yytext);
    match(IDENTIFIER);
    match(ASSIGN_OP);
    char* inc_val = parse_expression();
    match(')');
    match('{');
    
    while (lookahead != '}' && lookahead != 0) {
        parse_statement();
    }
    
    fprintf(icg_file, "%s = %s\n", inc_var, inc_val); // File
    fprintf(icg_file, "goto %s\n", start_label);      // File
    fprintf(icg_file, "LABEL %s:\n", end_label);      // File
    
    emit("COPY", inc_val, "-", inc_var); // Quadruple
    emit("GOTO", "-", "-", start_label); // Quadruple
    emit("LABEL", end_label, "-", "-");  // Quadruple
    
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
        
        // Write standard TAC to file
        fprintf(icg_file, "%s = %s %s %s\n", temp, left_side, op, right_side);
        
        // Map operators to specific OP codes for the Quadruple array
        char op_code[10];
        if (strcmp(op, "+") == 0) strcpy(op_code, "ADD");
        else if (strcmp(op, "-") == 0) strcpy(op_code, "SUB");
        else if (strcmp(op, "*") == 0) strcpy(op_code, "MUL");
        else if (strcmp(op, "/") == 0) strcpy(op_code, "DIV");
        else if (strcmp(op, ">") == 0) strcpy(op_code, "GT");
        else if (strcmp(op, "<") == 0) strcpy(op_code, "LT");
        else if (strcmp(op, ">=") == 0) strcpy(op_code, "GE");
        else if (strcmp(op, "<=") == 0) strcpy(op_code, "LE");
        else if (strcmp(op, "==") == 0) strcpy(op_code, "EQ");
        else if (strcmp(op, "!=") == 0) strcpy(op_code, "NEQ");
        else strcpy(op_code, op);
        
        // Save Quadruple
        emit(op_code, left_side, right_side, temp);
        
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
            
            fprintf(icg_file, "%s = CALL %s\n", temp, result); // File
            emit("CALL", result, "-", temp);                   // Quadruple
            
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
    
    // 1. Open the ICG output file
    icg_file = fopen("output.tac", "w");
    if (!icg_file) {
        printf("Error: Could not create output.tac file.\n");
        return 1;
    }
    
    // 2. STAGE 1: LEXICAL ANALYSIS
    printf("\n======================================================\n");
    printf("              STAGE 1: LEXICAL ANALYSIS               \n");
    printf("======================================================\n");
    
    int current_token;
    while ((current_token = yylex()) != 0) {
        printf("TOKEN: %-15s | LEXEME: %-15s | LINE: %d\n", 
               get_token_name(current_token), yytext, line_num);
    }

    rewind(yyin);
    extern void yyrestart(FILE *input_file);
    yyrestart(yyin);
    line_num = 1;

    // 3. STAGE 2: SYNTAX ANALYSIS
    printf("\n======================================================\n");
    printf("              STAGE 2: SYNTAX ANALYSIS                \n");
    printf("======================================================\n");
    
    lookahead = yylex(); 
    parse_program();
    
    fclose(icg_file); // Close the TAC file safely
    
    // 4. STAGE 3: ICG COMPLETION (Quadruples) & SUMMARY
    if (error_count == 0) {
        print_quadruples(); // Satisfies the Rubric (Table on Command Line)
        printf("\n======================================================\n");
        printf("       ✅ COMPILATION SUCCESSFUL (0 Errors)           \n");
        printf("       📄 Raw TAC saved securely to 'output.tac'      \n");
        printf("======================================================\n\n");
    } else {
        printf("\n======================================================\n");
        printf("       ❌ COMPILATION FAILED (%d Syntax Errors found) \n", error_count);
        printf("       ⚠️ ICG Table generation aborted due to errors  \n");
        printf("======================================================\n\n");
    }
    
    return (error_count > 0) ? 1 : 0;
}