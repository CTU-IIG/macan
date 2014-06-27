/* 
 * File:   syscalls.h
 * Author: breuejan
 *
 * Created on 20. leden 2012, 14:01
 */

#ifndef SYSCALLS_H
#define	SYSCALLS_H

#ifdef	__cplusplus
extern "C" {
#endif


int fdputc(int fd, int ch);
int fdgetc(int fd);

#define fdprintf dprintf
#define vfdprintf vdprintf

#ifdef	__cplusplus
}
#endif

#endif	/* SYSCALLS_H */

