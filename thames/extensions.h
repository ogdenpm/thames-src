#pragma once

void addFileRef(char *fname, int flags);
void deleteFileRef(char *fname);
void genDependencies(char *depfile);
char * mapTmpFile(char *fname);
