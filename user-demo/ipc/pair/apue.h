#ifndef _APUE_H     
#define _APUE_H     
    
#define _XOPEN_SOURCE   600  /* Single UNIX Specification, Version 3 */     
    
#include <sys/types.h>       /* some systems still require this */     
#include <sys/stat.h>     
#include <sys/termios.h>     /* for winsize */     
#ifndef TIOCGWINSZ     
#include <sys/ioctl.h>     
#endif     
#include <stdio.h>     /* for convenience */     
#include <stdlib.h>    /* for convenience */     
#include <stddef.h>    /* for offsetof */     
#include <string.h>    /* for convenience */     
#include <unistd.h>    /* for convenience */     
#include <signal.h>    /* for SIG_ERR */     
    
    
#define MAXLINE 4096               /* max line length */     
    
/*  
 * Default file access permissions for new files.  
 */    
#define FILE_MODE   (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)     
    
/*  
 * Default permissions for new directories.  
 */    
#define DIR_MODE    (FILE_MODE | S_IXUSR | S_IXGRP | S_IXOTH)     
    
typedef void   Sigfunc(int);   /* for signal handlers */    
    
#if defined(SIG_IGN) && !defined(SIG_ERR)     
#define SIG_ERR ((Sigfunc *)-1)     
#endif     
    
#define min(a,b)     ((a) < (b) ? (a) : (b))     
#define max(a,b)     ((a) > (b) ? (a) : (b))     
    
/*  
 * Prototypes for our own functions.  
 */    
    
#endif  /* _APUE_H */    
