#include "common.h"

#include <stdlib.h>

#include <stdarg.h>

#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>

#define mkdir _mkdir
#else
#include <sys/stat.h>

#include <unistd.h>  // For POSIX mkdir
#endif

#include <dirent.h>

void panic(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    printf("\nPANIC: %s\n\n", buffer);

    va_end(args);

    exit(1);
}

void warn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    printf("\nWARN: %s\n\n", buffer);

    va_end(args);
}

char* getFilename(char* path) {
    char* lastSlash = strrchr(path, '/');
    if (!lastSlash)
        lastSlash = strrchr(path, '\\');

    if (!lastSlash)
        return path;

    return (char*)(lastSlash + 1);
}

char* getExtension(char* path) {
    char* lastDot = strrchr(path, '.');
    if (!lastDot)
        return path + strlen(path);

    return lastDot;
}

void createDirectory(const char* path) {
    #ifdef _WIN32
    struct _stat st = { 0 };
    #else
    struct stat st = { 0 };
    #endif

    if (stat(path, &st) == -1) {
        #ifdef _WIN32
        if (mkdir(path) != 0)
        #else
        if (mkdir(path, 0700) != 0)
        #endif
            panic("createDirectory: mkdir failed");
    }
}

void createDirectoryTree(const char* path) {
    char tempPath[1024];
    char* c = NULL;
    u64 len;

    snprintf(tempPath, sizeof(tempPath), "%s", path);
    len = strlen(tempPath);

    for (c = tempPath + 1; c < tempPath + len; c++) {
        if (*c == PATH_SEPARATOR_C) {
            *c = '\0'; // Temporarily null-terminate string

            createDirectory(tempPath);

            *c = PATH_SEPARATOR_C;
        }
    }

    createDirectory(tempPath);
}

static int _isDirectory(const char* path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        return 0; // Cannot access path, assume not a directory
    }
    return S_ISDIR(statbuf.st_mode);
}

static void _getAllFiles(const char* path, const char* root, unsigned rootLength, GetAllFilesResult* result) {
    struct dirent* entry;
    DIR* dir = opendir(path);

    if (dir == NULL)
        panic("_getAllFiles: unable to open directory");

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char fullPath[1024];
            snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);

            if (_isDirectory(fullPath))
                _getAllFiles(fullPath, root, rootLength, result);
            else {
                result->fullFilenames[result->fileCount] = strdup(fullPath);
                result->truncatedFilenames[result->fileCount] = strdup(fullPath + rootLength + 1); // Skip root and trailing slash

                result->fileCount++;
            }
        }
    }

    closedir(dir);
}

GetAllFilesResult getAllFiles(const char* rootPath) {
    GetAllFilesResult result;
    result.fileCount = 0;
    result.fullFilenames = (char**)malloc(sizeof(char*) * 1024); // Allocating space for 1024 filenames
    result.truncatedFilenames = (char**)malloc(sizeof(char*) * 1024);

    _getAllFiles(rootPath, rootPath, strlen(rootPath), &result);

    return result;
}