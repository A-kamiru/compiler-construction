#include <stdio.h>

extern int yylex();
extern FILE *yyin;

int line_num = 1; // Your scanner needs this variable to exist

int main(int argc, char **argv) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            printf("Could not open file %s\n", argv[1]);
            return 1;
        }
    }

    printf("--- STARTING SCANNER TEST ---\n");
    
    // yylex() returns 0 when it hits the end of the file
    int token;
    while ((token = yylex()) != 0) {
        // We don't need to do anything here because your scanner.l 
        // already has printf statements inside its rules!
    }
    
    printf("--- END OF FILE ---\n");
    return 0;
}