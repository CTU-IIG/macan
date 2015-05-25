#ifndef DUMMY_KLEE_HEADER
#define DUMMY_KLEE_HEADER

//dummy declaration of klee_make_symbolic function, to avoid dragging in klee header.
void klee_make_symbolic(void * addr, unsigned size, const char* name);
//FixMe: Current KLEE puts its header in PATH to klee/klee.h

#endif
