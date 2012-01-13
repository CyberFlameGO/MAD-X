#ifndef MAD_SELECT_H
#define MAD_SELECT_H

// types

struct node;
struct node_list;
struct table;
struct sequence;
struct command;
struct command_list;

// interface

void  store_select(struct in_cmd*);
void  store_deselect(struct in_cmd*);
int   pass_select(char* name, struct command*);
int   pass_select_list(char* name, struct command_list*);
void  get_select_t_ranges(struct command_list* select, struct command_list* deselect, struct table*);
int   get_select_ranges(struct sequence* sequ, struct command_list* select, struct node_list* s_ranges);
int   get_ex_range(char* range, struct sequence*, struct node**);
int   get_sub_range(char* range, struct sequence*, struct node**);
int   get_range(char* range, struct sequence*, struct node**);
void  set_selected_errors(void);
void  set_range(char* range, struct sequence*);
void  set_sector(void);

#endif // MAD_SELECT_H

