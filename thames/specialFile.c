#include "specialFile.h"

#ifdef _MSC_VER
#include <io.h>
#else
#include <dirent.h>
#endif


char label[] = "THAMESF0 43";
char dirEntry[16];

char* expandPath(char const* token);

FILE* mkIsisLab(int drive) {
    if (drive < 0 || drive > 9)
        return NULL;
    FILE* fp = tmpfile();
    if (!fp)
        return NULL;
    label[7] = '0' + drive;
    fwrite(label, 1, sizeof(label) - 1, fp);
    rewind(fp);
    return fp;
}


#ifdef _MSC_VER
FILE* mkIsisDir(char const* path) {

    FILE* fp = tmpfile();
    if (!fp)
        return NULL;

    char pattern[PATH_MAX];
    strcpy(pattern, path);

    struct _finddata_t file;

    strcpy(basename(pattern), "??????.???");        // file pattern to match
    intptr_t hfile = _findfirst(pattern, &file);


    if (hfile != -1) {
        do {
            char* s = file.name;

            memset(dirEntry + 1, 0, 9);

            if (isalnum(*s)) {
                for (int i = 1; i < 7 && isalnum(*s); i++)
                    dirEntry[i] = toupper(*s++);
                if (*s == '.' && isalnum(*++s)) {
                    for (int i = 7; i < 10 && isalnum(*s); i++)
                        dirEntry[i] = toupper(*s++);
                }
                if (*s == 0)
                    fwrite(dirEntry, 1, 16, fp);
            }
        } while (_findnext(hfile, &file) == 0);
        _findclose(hfile);
    }
    memset(dirEntry + 1, 0, 9);
    dirEntry[0] = 0x7f;
    fwrite(dirEntry, 1, 16, fp);
    rewind(fp);
    return fp;
}

#else

FILE* mkIsisDir(char const* path) {

    FILE* fp = tmpfile();
    if (!fp)
        return NULL;

    char pattern[PATH_MAX];
    strcpy(pattern, path);

    *basename(pattern) = '\0';  // remove isis.dir
    DIR* dir = opendir(pattern);

    struct dirent* entry;
    if (dir) {
        while (entry = readdir(dir)) {
            char* s = entry->d_name;

            memset(dirEntry + 1, 0, 9);

            if (isalnum(*s)) {
                for (int i = 1; i < 7 && isalnum(*s); i++)
                    dirEntry[i] = toupper(*s++);
                if (*s == '.' && isalnum(*++s)) {
                    for (int i = 7; i < 10 && isalnum(*s); i++)
                        dirEntry[i] = toupper(*s++);
                }
                if (*s == 0)
                    fwrite(dirEntry, 1, 16, fp);
            }
        }
        closedir(dir);
    }
    memset(dirEntry + 1, 0, 9);
    dirEntry[0] = 0x7f;
    fwrite(dirEntry, 1, 16, fp);
    rewind(fp);
    return fp;
}
#endif


