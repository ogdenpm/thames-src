/***************************************************************************
 *                                                                         *
 *    THAMES: Partial ISIS-II emulator                                     *
 *    Copyright (C) 2011 John Elliott <seasip.webmaster@gmail.com>         *
 *                                                                         *
 *    This library is free software; you can redistribute it and/or        *
 *    modify it under the terms of the GNU Library General Public          *
 *    License as published by the Free Software Foundation; either         *
 *    version 2 of the License, or (at your option) any later version.     *
 *                                                                         *
 *    This library is distributed in the hope that it will be useful,      *
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    *
 *    Library General Public License for more details.                     *
 *                                                                         *
 *    You should have received a copy of the GNU Library General Public    *
 *    License along with this library; if not, write to the Free           *
 *    Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,      *
 *    MA 02111-1307, USA                                                   *
 *                                                                         *
 ***************************************************************************/

#include "thames.h"

/* Remove trailing spaces */
void trim(char *s) {
    for (char *p = strchr(s, '\0'); p >= s && isspace(*p); p--)
        *p = 0;
}



void capitals(char *s) {
    while (*s)     {
        if (islower(*s)) *s = toupper(*s);
        ++s;
    }
}

char *getExt(char *path) {
    char *s = getName(path);

    return (s = strrchr(path, '.')) ? s : strchr(path, 0);

}


char *getName(char *path) {
    char *s;
    while ((s = strchr(path, '/')) || (s = strchr(path, '\\')))
        path = s + 1;
    return path;
}

void *safeMalloc(size_t size) {
    void *p = malloc(size);
    if (!p) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    return p;
}