#include <stdio.h>
#include <ctype.h>
#include <string.h>

char* symbol[0x10000];

void logcall(unsigned short pc) {
	if (symbol[pc])
		printf("%s\n", symbol[pc]);
	else
		printf("%04X\n", pc);

}

void loadsymbols(char* fname) {
	FILE* fp;
	unsigned short loc;
	char sym[254];
	char* s;
	char line[256];
	if ((fp = fopen(fname, "rt"))) {
		if (s = strchr(line, '\n'))
			*s = 0;
		while (fgets(line, 256, fp)) {
			if (sscanf(line, "%04hx %s", &loc, sym) == 2)
				symbol[loc] = strdup(sym);
		}
	}

}