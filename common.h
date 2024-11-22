#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>

#ifdef _WIN32
#define PATH_SEPARATOR_C '\\'
#define PATH_SEPARATOR_S "\\"
#else
#define PATH_SEPARATOR_C '/'
#define PATH_SEPARATOR_S "/"
#endif

typedef unsigned long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef long s64;
typedef int s32;
typedef short s16;
typedef char s8;

#define IDENTIFIER_TO_U32(char1, char2, char3, char4) ( \
    ((u32)char4 << 24) | ((u32)char3 << 16) | \
    ((u32)char2 <<  8) | ((u32)char1 <<  0) \
)
#define MAX(a,b) ((a) > (b) ? a : b)
#define MIN(a,b) ((a) < (b) ? a : b)

#define STR_LIT_LEN(string_literal) (sizeof(string_literal) - 1)

#define INDENT_SPACE "    "

#define LOG_OK printf(" OK\n")

// Does not return
void panic(const char* fmt, ...);
// Similar to panic, but does not halt
void warn(const char* fmt, ...);

char* getFilename(char* path);

char* getExtension(char* path);

void createDirectory(const char* path);

void createDirectoryTree(const char* path);

typedef struct {
    unsigned fileCount;
    char** fullFilenames;
    char** truncatedFilenames;
} GetAllFilesResult;
GetAllFilesResult getAllFiles(const char* rootPath);

#endif