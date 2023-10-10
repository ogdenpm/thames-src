#include "thames.h"

#define MAXTMP  40
struct {
    char *isisTmpFile;
    char *tmpFile;
} tmpFiles[MAXTMP];



// track file usage to allow dependency file to be generated if requested
typedef struct _flist {
    struct _flist *next;
    char *fname;
    int flags;
} flist_t;

flist_t headFList;

void addFileRef(char *fname, int flags) {
    flist_t *p = &headFList;
    while (p->next) {
        p = p->next;
        if (strcasecmp(p->fname, fname) == 0) {
            p->flags |= flags;
            return;
        }
    }
    p->next = (flist_t *)safeMalloc(sizeof(flist_t));
    p = p->next;
    p->next = NULL;
    p->fname = strdup(fname);
    p->flags = flags;
}

void deleteFileRef(char *fname) {
    for (flist_t *p = &headFList; p->next; p = p->next) {
        flist_t *q = p->next;
        if (strcasecmp(q->fname, fname) == 0) {
            p->next = q->next;
            free(q);
            return;
        }
    }
}

void mkpath(char *depFile) {
    char dir[_MAX_DIR + 1];
    char *s = dir;
    while (1) {
        while (*depFile && *depFile != '/' && *depFile != '\\')
            *s++ = *depFile++;
        if (*depFile == 0)
            return;
        *s = 0;
        if (access(dir, 0) < 0)
            if (mkdir(dir, 0777) != 0 && errno != EEXIST) {
                fprintf(stderr, "cannot create directory %s\n", dir);
                exit(1);
            }
        *s++ = *depFile++;
    }
}


void genDependencies(char *depFile) {
    FILE *fp;
    if (!depFile)
        return;
    if (strcmp(depFile, ".") == 0)
        fp = stdout;
    else {
        mkpath(depFile);
        if ((fp = fopen(depFile, "wt")) == 0) {
            fprintf(stderr, "can't create dep file %s\n", depFile);
            return;
        }
    }
    for (flist_t *p = &headFList; p->next;) {
        p = p->next;
        char *ext = getExt(p->fname);
        if (*ext == '.' && *outputExt != '.')   /* be flexible in case user has missed off . in the -ME option */
            ext++;
        if ((p->flags & 2) && strcasecmp(ext, outputExt) == 0)
            fprintf(fp, "%s ", (p->fname[0] == '.' && p->fname[1] == '/') ? p->fname + 2 : p->fname);
    }
    putc(':', fp);
    for (flist_t *p = &headFList; p->next;) {
        p = p->next;
        if (p->flags == 1)
            fprintf(fp, " %s", (p->fname[0] == '.' && p->fname[1] == '/') ? p->fname + 2 : p->fname);
    }
    putc('\n', fp);
    if (fp != stdout)
        fclose(fp);
}



// to support parallel builds map isis tmp files to unix/windows tmp files that don't clash
// currently all files ending .tmp and the asm86.nam and asm86.ent files are mapped
// a dynamic table is built up to allow for close/reopen of tmp files
// currently deleted files are not removed but not found to be a problem
char *mapTmpFile(char *path) {
    int i;

    if (strcmp(getExt(path), ".TMP") && strcmp(path, "ASM86.NAM") && strcmp(path, "ASM86.ENT"))
        return NULL;

    for (i = 0; i < MAXTMP && tmpFiles[i].isisTmpFile && strcmp(tmpFiles[i].isisTmpFile, path); i++)
        ;
    if (i >= MAXTMP) {
        fprintf(stderr, "too many tmp files (%d) recompile thames with increased MAXTMP\n", i);
        return NULL;
    }
    if (!tmpFiles[i].isisTmpFile)
        tmpFiles[i].isisTmpFile = strdup(path);
    if (!tmpFiles[i].tmpFile) {
        char *tmp = tmpnam(NULL);
        if (tmp == NULL) {
            fprintf(stderr, "failed to map temporary %s\n", path);
            return NULL;
        }
        tmpFiles[i].tmpFile = strdup(tmp);
    }
#if _DEBUG
    printf("mapping %s to %s\n", path, tmpFiles[i].tmpFile);
#endif
    return tmpFiles[i].tmpFile;

}

