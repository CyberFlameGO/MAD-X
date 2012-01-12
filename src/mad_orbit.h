#ifndef MAD_ORBIT_H
#define MAD_ORBIT_H

// types

struct in_cmd;
struct command;

// constants

enum Match_Mode{ kMatch_NoMatch = 0, kMatch_Std, kMatch_UseMacro, kMatch_PTCknobs };

// interface

void    pro_correct(struct in_cmd*);
void    store_orbit(struct command*, double* orbit);

// for orbf.f90
uintptr_t locf_(char *iadr);
void      f_ctof(int *j, char *string, int *nel);

#endif // MAD_ORBIT_H

