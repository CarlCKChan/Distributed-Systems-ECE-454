/* 
 * Mahesh V. Tripunitara
 * University of Waterloo
 * You are NOT allowed to modify this file.
 */

#ifndef _ECE_454_FS_H_
#define _ECE_454_FS_H_
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#include "fsOtherIncludes.h" /* At the minimum, this should define
				the type FSDIR. */

struct fsDirent {
    char entName[256];
    unsigned char entType; /* 0 for file, 1 for folder,
			      -1 otherwise. */
};

extern int fsMount(const char *srvIpOrDomName, const unsigned int srvPort, const char *localFolderName);
extern int fsUnmount(const char *localFolderName);
extern FSDIR* fsOpenDir(const char *folderName);
extern int fsCloseDir(FSDIR *);
extern struct fsDirent *fsReadDir(FSDIR *);
extern int fsOpen(const char *fname, int mode);
extern int fsClose(int);
extern int fsRead(int fd, void *buf, const unsigned int count);
extern int fsWrite(int fd, const void *buf, const unsigned int count);
extern int fsRemove(const char *name);
#endif
