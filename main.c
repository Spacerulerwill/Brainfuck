#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sysexits.h>

typedef struct {
    char* contents;
    long size;
} SourceFile;

// load file from filepath into SourceFile struct
const SourceFile readSourceFile(const char* filePath) {
    FILE* fp;
    long lSize;
    char *buffer;
    
    fp = fopen (filePath , "rb");
    if( !fp ) perror(filePath),exit(1);

    fseek( fp , 0L , SEEK_END);
    lSize = ftell( fp );
    rewind( fp );

    buffer = calloc( 1, lSize+1 );
    if( !buffer ) fclose(fp),fputs("memory alloc fails",stderr),exit(1);

    if( 1!=fread( buffer , lSize, 1 , fp) )
    fclose(fp),free(buffer),fputs("entire read fails",stderr),exit(1);
    fclose(fp);

    SourceFile sourceFile = {
        buffer,
        lSize
    };
    return sourceFile;
}

// check there are valid parenthesis
bool checkSourceFileValidity(const SourceFile* sourceFile){
    uint16_t stack = 0;
    for (uint16_t i = 0; i < sourceFile->size; i++){
        uint8_t symbol = sourceFile->contents[i];
        if (symbol == '[') {
            stack++;
        }
        else if (symbol == ']'){
            // if stack is already empty, can't have a closing bracket
            if (stack == 0) {
                return false;
            }
            stack--;
        }
    }
    return stack == 0;
}

int main(int argc, char *argv[]) {
    // Get source file from command line args
    if (argc < 2) {
        puts("Error! Expected path to source file as command line argument.");
        exit(EX_USAGE);
    }

    const char* pathToSource = argv[1];

    // Initial 30kb tape of memory
    uint8_t tape[30000];
    memset(tape, 0, 30000);

    // Index of current memory cell
    uint16_t tapePosition = 0;

    // Keeps track of nested braces
    uint16_t braceCount = 0;

    // Load source file into a char array
    SourceFile sourceFile = readSourceFile(pathToSource);

    // Check the source file is valid brainfuck code
    if (!checkSourceFileValidity(&sourceFile)) {
        fputs("Brainfuck source file is invalid.", stderr);
        exit(EX_DATAERR);
    }

    // iterate over source file symbols, performing different operations for each
    uint16_t i = 0;
    while(i < sourceFile.size) {
        uint8_t symbol = sourceFile.contents[i];
        switch (symbol) {
            case '>': {
                tapePosition++;
                break;
            }
            case '<': {
                tapePosition--;
                break;
            } 
            case '+': {
                tape[tapePosition]++;
                break;
            }
            case '-': {
                tape[tapePosition]--;
                break;
            }
            case ',': {
                tape[tapePosition] = getchar();
                break;
            }
            case '.': {
                putchar(tape[tapePosition]);
                break;
            }
            case '[': {
                if (tape[tapePosition] == 0) {
                    ++braceCount;
                    while (sourceFile.contents[i] != ']' || braceCount != 0) {
                        ++i;
                        if (sourceFile.contents[i] == '[') {
                            ++braceCount;
                        }
                        else if (sourceFile.contents[i] == ']') {
                            --braceCount;
                        }
                    }
                }
                break;
            }
            case ']': {
                if (tape[tapePosition] != 0) {
                    ++braceCount;
                    while (sourceFile.contents[i] != '[' || braceCount != 0) {
                        --i;
                        if (sourceFile.contents[i] == ']') {
                            ++braceCount;
                        }
                        else if (sourceFile.contents[i] == '[') {
                            --braceCount;
                        }
                    }
                }
                break;
            }
        }
        ++i;
    }

    // Free char array
    free(sourceFile.contents);
    return EXIT_SUCCESS;
}