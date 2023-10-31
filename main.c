#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <errno.h>

#ifdef __unix__
    #include <sysexits.h>
    #define IO_ERR EX_IOERR
    #define USAGE_ERR EX_USAGE
    #define DATA_ERR EX_DATAERR
#else
    #define IO_ERR EXIT_FAILURE
    #define USAGE_ERR EXIT_FAILURE
    #define DATA_ERR EXIT_FAILURE
#endif

#define DEFAULT_TAPE_SIZE 30000

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
        exit(IO_ERR);
    }

    fseek(fp,0L,SEEK_END);
    lSize = ftell(fp);
    rewind(fp);

    buffer = calloc(1, lSize+1);
    if(!buffer) {
        fclose(fp);
        fputs("memory alloc fails",stderr);
        exit(IO_ERR);
    }

    if(fread( buffer , lSize, 1 , fp) != 1) {
        fclose(fp);
        free(buffer);
        fputs("entire read fails",stderr);
        exit(IO_ERR);
    }
    fclose(fp);

    SourceFile sourceFile = {
        buffer,
        lSize
    };
    
    return sourceFile;
}

// check validity of a brainfuck program
bool checkSourceFileValidity(const SourceFile* sourceFile){
    /* 
    Keep track of how many lines we have scanned and the amount of chars in those lines
    */
    size_t newlines = 0;
    size_t lineCharsScanned = 0;

    // Using an integer to simulate a stack popping and emplacing when we find square  brackets
    size_t stack = 0;
    for (size_t i = 0; i < sourceFile->size; i++){
        uint8_t symbol = sourceFile->contents[i];
        if (symbol == '[') {
            stack++;
        }
        else if (symbol == ']'){
            // if stack is already empty, can't have a closing bracket
            if (stack == 0) {
                fprintf(stderr, "Line %zu: Character %zu :: Closing bracket found with no opening bracket!", newlines + 1, i - lineCharsScanned + 1);
                return false;
            }
            stack--;
        }
        else if (symbol == '\n'){
            newlines++;
            lineCharsScanned = i;
        }
    }

    bool valid = stack == 0;
    if (!valid) {
        fprintf(stderr, "Found %zu opening brackets without closing brackets!", stack);
    }
    return valid;
}

int main(int argc, char *argv[]) {
    // Get source file from command line args
    if (argc < 2) {
        puts("Error! Expected path to source file as command line argument.");
        return USAGE_ERR;
    }

    /* 
    if we have 3 arguments, 3rd argument will be the tape size in bytes
    we will try and parse it as a 
    */
    size_t tapeSize;
    if (argc == 3) {
        char* str = argv[2];
        char* end;
        tapeSize = (size_t)strtoumax(str, &end, 10);
        if (errno == ERANGE || tapeSize == 0) {
            fprintf(stderr, "Tape size out of range. Tape size must be greater than 0 and less than or equal to %zu", SIZE_MAX);
            return USAGE_ERR;
        }
    } else {
        tapeSize = DEFAULT_TAPE_SIZE;
    }

    const char* pathToSource = argv[1];

    // Initial tape of memory and zero it all
    uint8_t* tape = (uint8_t*)malloc(tapeSize * sizeof(uint8_t));
    memset(tape, 0, tapeSize * sizeof(uint8_t));

    // Index of current memory cell
    size_t tapePosition = 0;

    // Keeps track of nested braces
    size_t braceCount = 0;

    // Load source file into a char array
    SourceFile sourceFile = readSourceFile(pathToSource);

    // Check the source file is valid brainfuck code
    if (!checkSourceFileValidity(&sourceFile)) {
        return DATA_ERR;
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

    // Free resources used
    free(sourceFile.contents);
    free(tape);
    return EXIT_SUCCESS;
}
