#ifndef MAD_BEAM_H
#define MAD_BEAM_H

// types

struct in_cmd;
struct command;
struct sequence;

// interface

void    exec_beam(struct in_cmd* cmd, int flag);        // used by mad_cmd.c
void    save_beam(struct sequence* sequ, FILE* file);   // used by mad_seq.c
void    expand_line(struct char_p_array* l_buff);       // used by mad_seq.c
void    show_beam(char* tok);                           // used by mad_exec.c
void    update_beam(struct command* comm);              // used by mad_option.c
void    adjust_beam(void);                              // many uses
int     attach_beam(struct sequence* sequ);             // many uses

#endif // MAD_BEAM_H

