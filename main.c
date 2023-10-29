#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sysexits.h>

#define BRAINFUCK_MAX_TAPE_SIZE 30000

typedef struct {
    char* contents;
    size_t size;
} SourceFile;

// load file from filepath into SourceFile struct
const SourceFile readSourceFile(const char* filePath) {
    FILE* fp;
    size_t lSize;
    char *buffer;
    
    fp = fopen (filePath , "rb");

    if(!fp) {
        perror(filePath);
        exit(EX_IOERR);
    }

    fseek(fp,0L,SEEK_END);
    lSize = ftell(fp);
    rewind(fp);

    buffer = calloc(1, lSize+1);
    if(!buffer) {
        fclose(fp);
        fputs("memory alloc fails",stderr);
        exit(EX_IOERR);
    }

    if(fread( buffer , lSize, 1 , fp) != 1) {
        fclose(fp);
        free(buffer);
        fputs("entire read fails",stderr);
        exit(EX_IOERR);
    }
    fclose(fp);

    SourceFile sourceFile = {
        buffer,
        lSize
    };
    return sourceFile;
}

// check there are valid parenthesis
bool checkSourceFileValidity(const SourceFile* sourceFile){
    size_t stack = 0;
    for (size_t i = 0; i < sourceFile->size; i++){
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
    uint8_t tape[BRAINFUCK_MAX_TAPE_SIZE] = {0};

    // Index of current memory cell
    size_t tapePosition = 0;

    // Keeps track of nested braces
    size_t braceCount = 0;

    // Load source file into a char array
    SourceFile sourceFile = readSourceFile(pathToSource);

    // Check the source file is valid brainfuck code
    if (!checkSourceFileValidity(&sourceFile)) {
        fputs("Brainfuck source file is invalid.", stderr);
        exit(EX_DATAERR);
    }

    // iterate over source file symbols, performing different operations for each
    size_t i = 0;
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