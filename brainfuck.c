// So we can use fopen on windows platforms
#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <errno.h>

// Create macros depending on operating system for exit failiure
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

// Terminal Colors
#define RED   "\x1B[31m"
#define YEL   "\x1B[33m"
#define RESET "\x1B[0m"

#define DEFAULT_TAPE_SIZE 30000

typedef struct {
    char* contents;
    size_t size;
} SourceFile;

// Load file from filepath into SourceFile struct
SourceFile readSourceFile(const char* filePath) {
    FILE* fp;
    size_t size;
    char  *buffer;

    // try open the file
    errno = 0;
    fp = fopen(filePath, "rb");
    if (!fp) {
        if (errno) {
            fprintf(stderr, RED "%s: %s" RESET, strerror(errno), filePath);
        } else {
            fprintf(stderr, RED "Error :: failed to open %s.\n" RESET, filePath);
        }
        exit(IO_ERR); 
    }

    // Determine file size
    fseek(fp,0L,SEEK_END); // UNDEFINED BEHAVIOUR MUST FIX
    long lsize = ftell(fp);
    if (lsize == -1L){
        fprintf(stderr, RED "%s" RESET, strerror(errno));
        exit(EXIT_FAILURE);
    }
    size = (size_t)lsize;
    rewind(fp);

    // Allocate enough memory for the char buffer
    buffer = calloc(1, size+1);
    if(!buffer) {
        fclose(fp);
        fputs(RED "Source file memory allocation fails!\n" RESET,stderr);
        exit(IO_ERR);
    }

    // Read file into newly allocated char buffer
    if(fread(buffer, size, 1, fp) != 1) {
        fclose(fp);
        free(buffer);  
        fputs(RED "Entire source file read fails!\n" RESET,stderr);
        exit(IO_ERR);
    }
    fclose(fp);

    SourceFile sourceFile = {
        buffer,
        size
    };
    
    return sourceFile;
}

// Check validity of a brainfuck program (ensure square brackets match up)
bool checkSourceFileValidity(const SourceFile* sourceFile){
    /* 
    Keep track of how many lines we have scanned and the amount of chars in those lines
    */
    size_t newlines = 0;
    size_t lineCharsScanned = 0;

    // Using an integer to simulate a stack popping and emplacing brackets when we find them
    size_t stack = 0;
    for (size_t i = 0; i < sourceFile->size; i++){
        uint8_t symbol = (uint8_t)sourceFile->contents[i];
        if (symbol == '[') {
            stack++;
        }
        else if (symbol == ']'){
            // if stack is already empty, can't have a closing bracket
            if (stack == 0) {
                fprintf(stderr, RED "Program validiation error (Line %zu Character %zu) :: Closing bracket found with no opening bracket!\n" RESET, newlines + 1, i - lineCharsScanned + 1);
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
        fprintf(stderr, RED "Program validiation error :: Found %zu opening brackets without closing brackets!\n" RESET, stack);
    }
    return valid;
}

int main(int argc, char *argv[]) {
    // Get source file from command line args
    if (argc < 2) {
        fputs(RED "Error :: Expected path to source file as command line argument.\n" RESET, stderr);
        return USAGE_ERR;
    }

    size_t tapeSize;
    if (argc == 3) {
        char* str = argv[2];
        char* end;
        tapeSize = (size_t)strtoumax(str, &end, 10);
        if (errno) {
            fprintf(stderr, RED "Error :: %s :: Input must be a number x: 0 < x < %tu.\n" RESET, strerror(errno), SIZE_MAX);
            return USAGE_ERR;
        }
        else if (tapeSize == 0) {
            fprintf(stderr, RED "Error :: Cannot allocate 0 bytes of tape!\n" RESET);
            return USAGE_ERR;
        }
    }else{
        tapeSize = DEFAULT_TAPE_SIZE;
    }

    const char* pathToSource = argv[1];

    // Initial tape of memory and zero it all
    uint8_t* tape = calloc(tapeSize, sizeof(uint8_t));
    if (!tape) {
        fprintf(stderr, RED "Error :: Failed allocate %tu bytes of tape. (Not enough memory available)\n" RESET, tapeSize);
        return EXIT_FAILURE;
    }

    // Current memory cell index
    size_t tapePosition = 0;

    // Keeps track of nested braces
    size_t braceCount = 0;

    // Load source file into a char array
    SourceFile sourceFile = readSourceFile(pathToSource);

    // Check the source file is valid brainfuck code
    if (!checkSourceFileValidity(&sourceFile)) {
        free(sourceFile.contents);
        free(tape);
        return DATA_ERR;
    }

    // iterate over source file symbols, performing different operations for each
    size_t i = 0;
    while(i < sourceFile.size) {
        uint8_t symbol = (uint8_t)sourceFile.contents[i];
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
                #ifdef NDEBUG
                    tape[tapePosition]++;
                #else
                    if (tape[tapePosition] == UINT8_MAX) {
                        tape[tapePosition] = 0;
                        fputs(YEL "Warning :: Runtime integer overflow!\n" RESET, stderr);
                    }else{
                        tape[tapePosition]++;
                    }
                #endif
                break;
            }
            case '-': {
                #ifdef NDEBUG
                    tape[tapePosition]--;
                #else
                    if (tape[tapePosition] == 0) {
                        tape[tapePosition] = UINT8_MAX;
                        fputs(YEL "Warning :: Runtime integer underflow!\n" RESET, stderr);
                    }else{
                        tape[tapePosition]--;
                    }
                #endif
                break;
            }
            case ',': {
                tape[tapePosition] = (uint8_t)getchar(); // TODO: Should we warn user about truncation?
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
