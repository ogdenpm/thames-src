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

static int isis_close_file(unsigned short h);


ISIS_FILE* handles[MAXHANDLE];        // max number of handles supported

static char const* isis_clean_name(char const* name);

/* [Mark Ogden]
    const char *xlt_device(const char *dev);
    int isis_drive_exists(int n);
    moved to cmdline.c to tie in with disk name caching
*/

/* Is this filename for an ISIS device?
 *
 * Returns:
 * 0: It is not
 * 1: It is a character device eg :CI: :BB:
 * 2: It is a block device     eg :F0: :F2:
 */
int isis_isdev(char const* name) {
    char const* isisdev = isis_clean_name(name);

    /* All devices have 4-character names */
    if (strlen(isisdev) != 4)
        return 0;

    /* All devices have names of the form :??: */
    if (isisdev[0] != ':' || isisdev[3] != ':')
        return 0;

    /* Block devices of the form :F<digit>: */
    if (isisdev[1] == 'F' && isdigit(isisdev[2]))
        return 2;

    /* Everything else is a character device */
    return 1;
}

/* Find out which drive a file is on.
 * Returns 0-9, or -1 for a character device */
int isis_getdrive(const char* name) {
    char const* isisname = isis_clean_name(name);

    if (isisname[0] == ':') {
        if (isisname[1] == 'F' && isdigit(isisname[2]) && isisname[3] == ':')
            return isisname[2] - '0';
        else
            return -1;
    }
    return 0;	/* If no device spec, assume :F0: */
}


/* clean up passed in isis name to remove leading spaces, upper case and terminate with '\0' */
// returns a statically allocated clean name
static char const* isis_clean_name(char const* name) {
    static char isisName[ISIS_PATH_MAX + 1];
    int i;
    while (*name && isspace(*name))
        name++;
    for (i = 0; i < ISIS_PATH_MAX; i++)
        if (isalnum(*name) || *name == '.' || *name == ':')
            isisName[i] = toupper(name[i]);
        else
            break;
    isisName[i] = '\0';
    return isisName;

}


/* [Mark Ogden] - For consistent handling the isis name/ext are forced to lower case
* The path names and device path names from environment variables are NOT mapped
* Caution is needed with POSIX systems with mixed case filenames.
*
*/

int isis_name2unix(const char* name, char* unixname) {
     char isisdev[5];
    const char* src;
    char* dest;
    int isisNameOffset;
    int osNameOffset = 0;
    char isisname[ISIS_PATH_MAX + 1];
    strcpy(isisname, isis_clean_name(name));        // make a safe local copy


    /* If the name doesn't start with a device
     * specifier, assume :F0: */
    if (isisname[0] != ':') {
        strcpy(isisdev, ":F0:");
        isisNameOffset = 0;
    }
    else if (strlen(isisname) < 4 || isisname[3] != ':')
        return ERROR_BADFILENAME;
    else {
        sprintf(isisdev, "%-4.4s", isisname);
        isisNameOffset = 4;
    }

    /* The bit bucket (:BB:) is always defined, and is host null devvice*/
    if (strcmp(isisdev, ":BB:") == 0) {
#ifdef _WIN32		// [Mark Ogden] added dos nul device support
        strcpy(unixname, "nul");
#else
        strcpy(unixname, "/dev/null");
#endif
        return ERROR_SUCCESS;
    }
    /* Check for other mapped devices */
    if (isis_isdev(isisdev) == 1)	/* Character device */
    {
        src = xlt_device(isisdev);
        if (!src) {
            fprintf(stderr, "No UNIX mapping for ISIS "
                "character device %s\n", isisdev);
            return ERROR_BADDEVICE;
        }
        if (strlen(src) >= PATH_MAX)	// catch file name too long
            return ERROR_BADFILENAME;
        strcpy(unixname, src);
        return ERROR_SUCCESS;
    }
    /* isisdev had just better be a valid block device by now */
    if (isis_isdev(isisdev) != 2)
        return ERROR_BADFILENAME;

    if (tOption) {
        char* s = mapTmpFile(isisname + isisNameOffset);
        if (s && strlen(s) < PATH_MAX) {
            strcpy(unixname, s);
            return ERROR_SUCCESS;
        }
    }

    src = xlt_device(isisdev);
    if (!src) {
        fprintf(stderr, "No UNIX mapping for ISIS "
            "block device %s\n", isisdev);
        return ERROR_BADFILENAME;
    }
    // if src contains a final / or \ this may limit name size by 1 char
    if (strlen(src) + strlen(isisname + isisNameOffset) + 1 >= PATH_MAX) 	// catch file name too long
        return ERROR_BADFILENAME;


    strcpy(unixname, src);

#ifdef _WIN32		// [Mark Ogden] map \ to / and handle x: for dos
    for (char* s = strchr(unixname, '\\'); s; s = strchr(s + 1, '\\'))
        *s = '/';
    if (unixname[1] == ':')
        osNameOffset = 2;
#endif

    /* Append a path separator if there isn't one */
    dest = strchr(unixname, '\0');
    if (dest > unixname + osNameOffset && dest[-1] != '/') {
        strcpy(dest, "/");
        dest++;
    }
    src = &isisname[isisNameOffset];
    while (*dest++ = tolower(*src++))
        ;

    return ERROR_SUCCESS;
}




/* Create a new ISIS file with a unique handle */
static int new_isis_handle(ISIS_FILE** pf) {
    int handle;

    for (handle = 2; handle < MAXHANDLE && handles[handle]; handle++)
        ;
    if (handle >= MAXHANDLE) {
        *pf = NULL;
        return ERROR_NOHANDLES;	/* no free handles */
    }
    if (!(handles[handle] = calloc(1, sizeof(ISIS_FILE)))) {
        *pf = NULL;
        return ERROR_NOMEM;
    }
    *pf = handles[handle];
    handles[handle]->handle = handle;
    return ERROR_SUCCESS;
}



void release_isis_handle(unsigned short h) {
    if (h >= MAXHANDLE)
        return;
    isis_close_file(h);
    free(handles[h]->buffer);                // safe even if null
    free(handles[h]);
    handles[h] = NULL;
    if (h < 2)                 // make sure CO/CI are always present
        isis_open_stdio(h);
}

static int isis_close_file(unsigned short h) {
    ISIS_FILE* self = handles[h];
    if (!self->fp) return ERROR_SUCCESS;

    /* don't close stdin or stdout */
    if (self->fp == stdout) {
        fflush(self->fp);
        return ERROR_SUCCESS;
    }
    else if (self->fp == stdin)
        return ERROR_SUCCESS;

    fclose(self->fp);
    self->fp = NULL;
    return ERROR_SUCCESS;
}


LINE_BUFFER* new_buffer(int len) {
    return calloc(1, len + sizeof(LINE_BUFFER));
}


int isis_open_stdio(int handle) {

    if (handle < 0 || handle > 1)
        return ERROR_BADDEVICE;
    ISIS_FILE* isf = handles[handle];

    if (isf) {
        if (isf->fp && isf->fp != (handle == ISISCO ? stdout : stdin))
            fclose(isf->fp);
        free(isf->buffer);      // safe even if NULL
        free(isf);
    }
    isf = handles[handle] = calloc(1, sizeof(ISIS_FILE));
    if (!isf)
        return ERROR_NOMEM;

    if (handle == ISISCO) {
        strcpy(isf->filename, ":CO:");
        isf->fp = stdout;
        isf->access = 2;
    }
    else {
        strcpy(isf->filename, ":CI:");
        isf->fp = stdin;
        conin->handle = 1;
        conin->access = 1;
    }
    return ERROR_SUCCESS;
}


/* Detect attempts to open/close/delete the console, which is already open */
ISIS_FILE* isis_check_console(const char* name) {
    char const* isisname = isis_clean_name(name);

    if (strcmp(isisname, ":CO:") == 0)
        return conout;
    if (strcmp(isisname, ":CI:") == 0)
        return conin;
    return NULL;
}

int  isis_open(int* handle, const char* name, int fmode, int echo) {
    ISIS_FILE* isf;
    char unixname[PATH_MAX + 1];

    /* Access modes:
    1	"r"
    2	"w"
    3	If file exists, "r+" else "w+"
*/
    if (fmode < 1 || fmode > 3) return ERROR_BADACCESS;
    /* XXX Handle Echo.
        If Echo is nonzero, the file is put in buffered mode and its low
        byte is the handle of the file to which input should be echoed.
        (So 0xFF00 -> file 0, :CO: )
    */


    /* Check for attempts to open the console, which is always open */
    if ((isf = isis_check_console(name))) {
        if (isf == conin && fmode != 1) return ERROR_BADACCESS;
        if (isf == conout && fmode != 2) return ERROR_BADACCESS;
        isf->access = fmode;
        isf->echo = echo;
        *handle = isf == conout ? ISISCO : ISISCI;
        return ERROR_SUCCESS;
    }
    char const *isisname = isis_clean_name(name);
    for (int i = 0; i < MAXHANDLE; i++) {
        if (handles[i] && strcmp(handles[i]->filename, isisname) == 0)
            return ERROR_ALREADYOPEN;
    }
    int err;
    if ((err = new_isis_handle(&isf)))
            return err;
    strcpy(isf->filename, isis_clean_name(name));
    isf->access = fmode;
    isf->echo = echo;

    if (isf->echo) {
        /* Give the file a dummy buffer; isis_read() will
         * populate it for real. */
        isf->buffer = new_buffer(1);
        isf->buffer->len = 1;
        isf->buffer->pos = 1;
    }

    err = isis_name2unix(name, unixname);
    if (err) {
        release_isis_handle(isf->handle);
        return err;
    }
    addFileRef(unixname, fmode);
    switch (fmode) {
    case 1: isf->fp = fopen(unixname, "rb");
        break;
    case 2: isf->fp = fopen(unixname, "wb");
        break;
    case 3: isf->fp = fopen(unixname, "r+b");
        if (!isf->fp) {
            isf->fp = fopen(unixname, "w+b");
        }
        break;
    }
    if (!isf->fp) {
        if (trace)
            fprintf(stderr, "Can't open '%s'\n", unixname);
        release_isis_handle(isf->handle);
        return ERROR_FILENOTFOUND;
    }
    *handle = isf->handle;
    return ERROR_SUCCESS;
}



int isis_close(int handle) {
    if (handle >= MAXHANDLE)
        return ERROR_BADPARAM;

    if (!handles[handle])
        return ERROR_NOTOPEN;

    int err = isis_close_file(handle);
    release_isis_handle(handle);
    return err;
}


int isis_delete(const char* name) {
    char realname[PATH_MAX];
    int err;


    /* If the filename refers to a device, it can't be deleted */
    if (isis_isdev(name)) return ERROR_ISDEVICE;

    /* This should never return true, because isis_isdev() ought to
     * have caught attempts to delete the console. But just in case... */
    if (isis_check_console(name)) return ERROR_ISDEVICE;

    err = isis_name2unix(name, realname);
    if (err) return err;

    /* Check for attempts to delete an open file. Unix is happy with this,
     * but ISIS is not. */
    for (int i = 0; i < MAXHANDLE; i++) {
        if (handles[i] && !strcmp(realname, handles[i]->filename))
            return ERROR_FILEINUSE;
    }

    deleteFileRef(realname);
    if (remove(realname)) return ERROR_PERMISSIONS;
    /* Saves deleted files for future comparison.
        {
            int ok = 0;
            char altname[PATH_MAX + 1];
            int ver = 0;
            struct stat st;

            while (!ok)
            {
                sprintf(altname, "%s.%d", realname, ver);
                if (stat(altname, &st) < 0)
                {
                    rename(realname, altname);
                    return ERROR_SUCCESS;
                }
                ++ver;
            }

        }
    */

    return ERROR_SUCCESS;
}



int isis_read(int handle, byte* buffer, int count, int* actual) {
    FILE* fp;
    ISIS_FILE* fd;
    int avail = 0;
    char input[ISIS_LINE_MAX + 1];
    int err = ERROR_SUCCESS;


    fd = find_handle(handle);

    if (!fd) return ERROR_NOTOPEN;

    if (fd->access == 2) return ERROR_NOREAD;

    if (fd->buffer)	/* File is in line mode */
    {
        avail = fd->buffer->len - fd->buffer->pos;

        if (avail == 0) /* Reload buffer */
        {
            if (!fgets(input, ISIS_LINE_MAX, fd->fp)) {
                if (fd->echo)
                    err = isis_write(fd->echo, "\x1a\r\n", 3);  // isis writes control Z, cr, lf
                *actual = 0;
                return err;
            }

            if (input[0] != 0 && input[strlen(input) - 1] == '\n') {
                input[strlen(input) - 1] = 0;
            }
            if (input[0] != 0 && input[strlen(input) - 1] == '\r') {	// [Mark Ogden] non unix systems may include \r as well
                input[strlen(input) - 1] = 0;
            }
            free(fd->buffer);
            fd->buffer = new_buffer(2 + (int)strlen(input));		// [Mark Ogden] note new_buffer allocates 1 additional byte
            strcpy(fd->buffer->data, input);
            strcat(fd->buffer->data, "\r\n");
            fd->buffer->pos = 0;
            /* [Mark Ogden] avail was not set */
            avail = fd->buffer->len = (int)strlen(fd->buffer->data);
        }
        if (avail > count) avail = count;
        for (int i = 0; i < avail; i++)						// [Mark Ogden] modified to handle multiple lines in buffer
            if ((buffer[i] = fd->buffer->data[fd->buffer->pos++]) == '\n')
                avail = i + 1;
        err = ERROR_SUCCESS;
        /* If there is an echo file set up, write to it */
        if (fd->echo != 0)
            err = isis_write(fd->echo, buffer, avail);

        *actual = avail;									// [Mark Ogden] pos updated in copy loop above
        return err;
    }
    /* Unbuffered read */
    fp = fd->fp;

    if (fp == stdout) fp = stdin;

    avail = (int)fread(buffer, 1, count, fp);
    *actual = avail;
    return ERROR_SUCCESS;
}


int isis_write(int handle, byte* buffer, int count) {
    FILE* fp;
    ISIS_FILE* fd;
    int done;

    fd = find_handle(handle);
    if (!fd) return ERROR_NOTOPEN;

    if (strcmp(fd->filename, "PROMPT.LST") == 0)
        if (count > 0 && !*buffer)
            printf("oops\n");

    if (fd->access == 1) return ERROR_NOWRITE;

    fp = fd->fp;

    if (fp == stdin) fp = stdout;
    if (fp == stdout)
        errCheck(buffer, count);		// intercept to check if app error

    done = (int)fwrite(buffer, 1, count, fp);
    fflush(fp);
    if (done < count) return ERROR_DISKFULL;
    return ERROR_SUCCESS;
}




int isis_seek(int handle, int seekMode, long* offset) {
    ISIS_FILE* fd;
    long pos;

    fd = find_handle(handle);
    if (!fd) return ERROR_NOTOPEN;

    if (fd->access == 2) return ERROR_SEEKWRITE;
    if (isatty(fileno(fd->fp)))
        return ERROR_CANTSEEKDEV;
    switch (seekMode) {
    case 0:	 /* Get position */
        pos = ftell(fd->fp);
        /* Can't get current position */
        if (pos < 0) return ERROR_CANTSEEKDEV;
        *offset = pos;
        break;

    case 1: /* Seek backward */
        pos = ftell(fd->fp);
        if (pos < 0) return ERROR_CANTSEEKDEV;
        if (fd->access == 2)  return ERROR_SEEKWRITE;
        if (pos - (*offset) < 0) {
            /* [Mark Ogden] Seeking off beginning is an error, but nevertheless
             * rewinds the file */
            fseek(fd->fp, 0L, SEEK_SET);
            return ERROR_OFFBEGINNING;
        }
        if (fseek(fd->fp, -(*offset), SEEK_CUR) < 0)
            return ERROR_CANTSEEKDEV;
        break;


    case 2: /* Seek absolute */
        if (fd->access == 2) return ERROR_SEEKWRITE;
        if (fseek(fd->fp, (*offset), SEEK_SET) < 0)
            return ERROR_CANTSEEKDEV;
        break;

    case 3: /* Seek forward */
        if (fd->access == 2) return ERROR_SEEKWRITE;
        if (fseek(fd->fp, (*offset), SEEK_CUR) < 0)
            return ERROR_CANTSEEKDEV;
        break;

    case 4:	/* Seek EOF */
        if (fd->access == 2) return ERROR_SEEKWRITE;
        if (fseek(fd->fp, 0, SEEK_END) < 0)
            return ERROR_CANTSEEKDEV;
        break;

    default: return ERROR_BADMODE;
    }
    return ERROR_SUCCESS;
}

int isis_rename(const char* oldname, const char* newname) {
    char unixold[1 + PATH_MAX];
    char unixnew[1 + PATH_MAX];
    int err;
    struct stat st;

    if (isis_isdev(oldname) || isis_isdev(newname)) return ERROR_ISDEVICE;

    err = isis_name2unix(oldname, unixold); if (err) return err;
    err = isis_name2unix(newname, unixnew); if (err) return err;

    if (isis_getdrive(oldname) != isis_getdrive(newname))
        return ERROR_RENACROSS;	/* Can't rename across drives */

    if (!stat(unixnew, &st)) return ERROR_EXISTS;	/* Target file exists */

    if (!rename(unixold, unixnew)) return ERROR_SUCCESS;

    return ERROR_PERMISSIONS;
}

int isis_console(const char* ciname, const char* coname) {
    char unixname[1 + PATH_MAX];
    FILE* fp;
    int err;

    /* reset any existing connections */
    isis_open_stdio(ISISCO);
    isis_open_stdio(ISISCI);

    ISIS_FILE* isf;


    if ((isf = isis_check_console(ciname)) != conin) {
        if (isf == conout)
            return ERROR_BADPARAM;
        if ((err = isis_name2unix(ciname, unixname)))
            return err;
        if (!(fp = fopen(unixname, "rb")))
            return ERROR_FILENOTFOUND;
        conin->fp = fp;
    }

    if ((isf = isis_check_console(coname)) != conout) {
        if (isf == conin)
            return ERROR_BADPARAM;
        if ((err = isis_name2unix(coname, unixname)))
            return err;
        if (!(fp = fopen(unixname, "wb")))
            return ERROR_DIRFULL;
        conout->fp = fp;
    }
    return ERROR_SUCCESS;
}


int isis_attrib(const char* isisname, int attr, int value) {
    char unixname[1 + PATH_MAX];
    int err;
    struct stat st;
    mode_t access;

    if (attr < 0 || attr > 3) return ERROR_BADATTRIB;
    /* UNIX only supports the read-only attribute. */
    if (attr != 2) return ERROR_SUCCESS;
    if (isis_isdev(isisname)) return ERROR_ISDEVICE;
    err = isis_name2unix(isisname, unixname);

    if (stat(unixname, &st) < 0) return ERROR_FILENOTFOUND;

    access = st.st_mode;
    if (value & 1) /* Read-only */
        access &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
    else	access |= S_IWUSR;

    if (access == st.st_mode) return ERROR_SUCCESS;	/* Already set */
    if (chmod(unixname, access) == 0) return ERROR_SUCCESS;
    return ERROR_PERMISSIONS;
}


int isis_rescan(int handle) {
    ISIS_FILE* fd;

    fd = find_handle(handle);

    if (!fd || !fd->buffer) return ERROR_NOTLINEMODE;

    fd->buffer->pos = 0;
    return ERROR_SUCCESS;
}


int isis_whocon(int handle, char* isisname) {
    if (handle & 1) strcpy(isisname, conin->filename);
    else		strcpy(isisname, conout->filename);
    return ERROR_SUCCESS;
}

// Mark Ogden - updated spath to follow isis 4.3 behaviour

struct {
    char* dev;
    unsigned char devtype;
} deviceMap[] = {
     {"F0", 3}, {"F1", 3}, {"F2", 3}, {"F3", 3},
     {"F4", 3}, {"F5", 3}, {"TI", 0}, {"TO", 1},
     {"VI", 0}, {"VO", 1}, {"I1", 0}, {"O1", 1},
     {"TR", 0}, {"HR", 0}, {"R1", 0}, {"R2", 0},
     {"TP", 1}, {"HP", 1}, {"P1", 1}, {"P2", 1},
     {"LP", 1}, {"L1", 1}, {"BB", 2}, {"CI", 0},
     {"CO", 1}, {"F6", 3}, {"F7", 3}, {"F8", 3},
     {"F9", 3} };

int isis_spath(const char* isisname, ISIS_STAT* result) {
    const char* pathname;
    char devname[3];

    memset(result, 0, sizeof(*result));

    for (pathname = isisname; *pathname == ' '; pathname++);	// skip leading space

    if (pathname[0] == ':') {
        result->device = 0xff;
        if (pathname[3] != ':')
            return ERROR_BADFILENAME;
        devname[0] = toupper(pathname[1]);
        devname[1] = toupper(pathname[2]);
        devname[2] = 0;
        pathname += 4;

        for (int i = 0; i < sizeof(deviceMap) / sizeof(deviceMap[0]); i++) {
            if (isisname[1] == deviceMap[i].dev[0] && isisname[2] == deviceMap[i].dev[1]) {
                result->device = i;
                break;
            }
        }
        if (result->device == 0xff)
            return ERROR_BADDEVICE;

    }
    result->drivetype = 0xff;
    if ((result->devtype = deviceMap[result->device].devtype) == 3)		/* random access */
        result->drivetype = isis_drive_exists(result->device < 6 ? result->device : result->device - 19) ? 4 : 0;

    for (int i = 0; i < 6 && isalnum(*pathname); i++)
        result->filename[i] = toupper(*pathname++);

    if (*pathname == '.') {
        pathname++;
        for (int i = 0; i < 3 && isalnum(*pathname); i++)
            result->ext[i] = toupper(*pathname++);
        if (result->ext[0] == 0)
            return ERROR_BADEXT;
    }
    if (result->device <= 9 && result->filename[0] == 0)
        return ERROR_NOFILENAME;

    if (isalnum(*pathname) || *pathname == '.' || *pathname == ':')
        return ERROR_BADFILENAME;

    return ERROR_SUCCESS;
}




ISIS_FILE* find_handle(unsigned short h) {
    return h < MAXHANDLE ? handles[h] : NULL;
}

const char* isis_filename(int h) {
    ISIS_FILE* f = find_handle(h);

    return f ? f->filename : "[Unknown handle]";
}


static const char* error_strings[] =
{
    "Success",
    "No memory available for buffer",
    "File is not open",
    "No more file handles available",
    "Invalid pathname",
    "Bad device name in filename",
    "Trying to write a file opened in read mode",
    "Disk is full",
    "Trying to read a file opened in write mode",
    "Cannot create file",
    "Cannot rename across devices",
    "Destination file exists",
    "File is already open",
    "File not found",
    "Permissions error",
    "Attempting to overwrite operating system",
    "Invalid executable image",
    "Attempt to rename or delete a device",
    "Invalid function number",
    "Can't seek on a device",
    "Can't seek to before start of file",
    "File is not open in line mode",
    "Bad access mode",
    "No filename specified",
    "Disk error",
    "Invalid echo file specified",
    "Bad attribute",
    "Bad seek mode",
    "Null file extension",
    "End of file on console input",
    "Drive not ready",
    "Can't seek in a write-only file",
    "Can't delete an open file",
    "Bad system call parameter",
    "Bad switch argument in load",
    "Cannot seek past EOF in file open for read",
};

#define MAXERROR (sizeof(error_strings) / sizeof(error_strings[0]))


void isis_perror(const char* s, int err) {
    fprintf(stderr, "%s: ", s);

    if (err >= 0 && err < MAXERROR) {
        fprintf(stderr, "%s.\n", error_strings[err]);
    }
    else {
        fprintf(stderr, "Unknown error %d.\n", err);
    }
}


