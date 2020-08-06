#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <libgen.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>

#ifndef EXEC_SEARCH_PATH
#define EXEC_SEARCH_PATH
#endif

#ifndef SELF_PATH_HOLDER
#define SELF_PATH_HOLDER
#endif

#define QUOTE(x) STRFY(x)
#define STRFY(x) #x

#define SUM(a,b) (a+b)

#define EXISTS_VAL(arr, len, cond) ({\
    bool _exists = false;\
    typeof(len) _len = (len);\
    \
    for (typeof(_len) _i = 0; _i < _len; ++_i) {\
        if (cond((arr)[_i])) {\
            _exists = true;\
            \
            break;\
        }\
    }\
    \
    _exists;\
})

#define MAP_VAL(dest, arr, len, map) ({\
    typeof(dest) _res = (dest);\
    typeof(len) _len = (len);\
    \
    for (typeof(_len) _i = 0; _i < (len); ++_i) {\
       _res[_i] = (map((arr)[_i]));\
    }\
    \
})

#define REDUCE_VAL(init, arr, len, reduce) ({\
    typeof(init) _res = (init);\
    typeof(len) _len = (len);\
    \
    for (typeof(_len) _i = 0; _i < (len); ++_i) {\
       _res = (reduce(_res, (arr)[_i]));\
    }\
    \
    _res;\
})


extern char **environ;

/*
 * Caution: pathList is modifiable
 */
static char pathList[] = QUOTE(EXEC_SEARCH_PATH);
static char const * const pathDelim = ":";
static char const * const selfPathHolder = QUOTE(SELF_PATH_HOLDER);

static char const * const argsNotSane =         "command not sane";
static char const * const noSrchPaths =         "no search paths detected";
static char const * const selfReqRelative =     "self directory required when "
                                                "relative search paths";
static char const * const pathNotAllocated = "command path not allocated";

/*
 * Pre: path != NULL;
 */
static bool isRelativePath(char const *path) 
{
    return path[0] != '/';
}

/*
 *
 */
static bool checkArgs(int argc, char const *argv[]) 
{
    return argc >= 2 && isRelativePath(argv[1]);
}

static char * getExeDir(char **dirHandler) 
{
    char *resolvedPath = *dirHandler = realpath(selfPathHolder, NULL);

    if (!resolvedPath) {
        errno = ENOTRECOVERABLE;

        return NULL;
    }

    return dirname(resolvedPath);
}

/*
 * Pre: pathList != NULL;
 */
static size_t pathNum(char const *pathList, char const *delim) 
{
    size_t pathCount = 0;
    const char *nextPath;

    if (!*pathList) {
        return 0;
    }

    do {
        nextPath = strchr(pathList, *delim);

        if (nextPath != pathList) {
            ++pathCount;
        }

        pathList = nextPath + 1;

    } while (nextPath);

    return pathCount;
}

/*
 * Pre: dest is spacious enough for all entries (see pathNum above)
 * Pre: pathList != NULL;
 */
static void getSearchPaths(char **dest, char *pathList, char const *delim) 
{
    char *currPath;

    while ((currPath = *dest = strsep(&pathList, delim))) {
        if (*currPath) {
            ++dest;
        }
    };
}


/*
 * Pre: for every p in paths, p != NULL
 */
static char *concatPaths(char const * paths[], size_t num) 
{
    size_t lengths[num];

    MAP_VAL((size_t *)lengths, paths, num, strlen);

    size_t  fullSize = REDUCE_VAL((size_t) 0, lengths, num, SUM) + num;
    char * resPath = malloc(fullSize);

    if (!resPath) {
        return NULL;
    }

    char * currPos = resPath;

    for (size_t idx = 0; idx < num; ++idx) {
        currPos = stpncpy(currPos, paths[idx], lengths[idx]);
        *(currPos++) = (idx == num - 1) ? '\0' : '/';
    }

    return resPath;
}

/*
 * Intended usage: <executable path> <command name> [<command args>...]
 */
int main(int argc, char *argv[]) 
{
    if (!checkArgs(argc, (char const **)argv)) {
        error(EXIT_FAILURE, EINVAL, argsNotSane);
    }

    char const *commName = argv[1];
    char *selfDirHandler;
    char *selfDir = getExeDir(&selfDirHandler);

    size_t pathCount = pathNum((char const *)pathList, pathDelim);

    if (!pathCount) {
        error(EXIT_FAILURE, ENOTRECOVERABLE, noSrchPaths);
    }

    char *searchPaths[pathCount + 1];

    getSearchPaths(searchPaths, pathList, pathDelim);
    searchPaths[pathCount] = NULL;

    if (EXISTS_VAL(searchPaths, pathCount, isRelativePath) && !selfDir) {
        error(EXIT_FAILURE, ENOTRECOVERABLE, selfReqRelative);
    }

    for (char const**path = (char const**)searchPaths; *path; ++path) {
        char *absPath;


        if (isRelativePath(*path)) {
            char const *paths[3] = { selfDir, *path, commName };
            
            absPath = concatPaths(paths, 3);
        }
        else {
            char const *paths[2] = { *path, commName };

            absPath = concatPaths(paths, 2);
        }

        if (!absPath) {
            error(EXIT_FAILURE, ENOMEM, pathNotAllocated);
        }

        char *const *childEnv = (char *const *)environ;
        char *const *childArgs = (char *const *)(argv+1);

        execve((char const *)absPath, childArgs, childEnv);

        // We have failed, next
        // error(0, errno, "Tried %s", absPath);
        free(absPath);
    }

    free(selfDirHandler);
    error(EXIT_FAILURE, 0, "Could not invoke the required exe. Aborting");
}
