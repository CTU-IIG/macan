/*
 * syscalls.c
 */
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <reent.h>

#include "syscalls.h"


#if defined(MALLOC_HEAP_SIZE) && (MALLOC_HEAP_SIZE > 0)
static unsigned char my_heap[MALLOC_HEAP_SIZE] __attribute__ ((aligned (MALLOC_HEAP_SIZE)));
#endif

caddr_t _sbrk(int incr) {
#if defined(MALLOC_HEAP_SIZE) && (MALLOC_HEAP_SIZE > 0)
    static unsigned char *heap = NULL;
    unsigned char *prev_heap;

    /* initialize */
    if (heap == NULL) {
        heap = my_heap;
    }

    prev_heap = heap;
    heap += incr;
    if ((heap - my_heap) > MALLOC_HEAP_SIZE) {
        /* heap overflow—announce on stderr */
        //print_assert("SYS", "Heap overflow!", "MALLOC_HEAP_SIZE", heap - my_heap - MALLOC_HEAP_SIZE);
        //halt();       
    }

    return (caddr_t) prev_heap;
#else
    //print_assert("SYS", "Heap overflow!", "MALLOC_HEAP_SIZE", incr);
    //halt();       
#endif
}

void _exit(int status) {
    //print_assert("SYS", "Exit!", NULL,  status);
    //halt();
}

int _getpid(void) {
    return 1;
}

int _kill(int pid, int sig) {
    pid = pid;
    sig = sig; /* avoid warnings */
    errno = EINVAL;
    return -1;
}

int link(const char *old, const char *new) {
    (void) old;
    (void) new;
    return -1;
}

int _close(int file) {
    (void) file;
    return -1;
}

int _fstat(int file, struct stat *st) {
    (void) file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file) {
    (void) file;
    return 1;
}

int _lseek(int file, int ptr, int dir) {
    (void) file;
    (void) ptr;
    (void) dir;
    return 0;
}

int _read(int file, char *ptr, int len) {
    int i;
    for (i = 0; i < len; i++) {

        //ptr[i] = fdgetc(file);
    }
    return len;
}

int _write(int file, char *ptr, int len) {
    int i;
    for (i = 0; i < len; i++) {
        //fdputc(file, ptr[i]);
    }
    return len;
}

int _open(const char * filename, int flags, int mode) {
    (void) filename;
    (void) flags;
    (void) mode;
    return 1;
}
