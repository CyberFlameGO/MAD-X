#include "madx.h"

static void
exec_delete_sequ(char* name)
{
  struct sequence* keep = current_sequ;
  int spos;
  if ((spos = name_list_pos(name, sequences->list)) >= 0) {
    current_sequ = sequences->sequs[spos];
    if (current_sequ->ex_start != NULL) { /* delete expanded */
      current_sequ->ex_nodes = delete_node_list(current_sequ->ex_nodes);
      current_sequ->ex_start = delete_node_ring(current_sequ->ex_start);
      current_sequ->orbits = delete_vector_list(current_sequ->orbits);
    }
    sequences->sequs[spos] = delete_sequence(current_sequ);
    remove_from_sequ_list(current_sequ, sequences);
    current_sequ = keep;  
  }
  else warning("sequence to be deleted does not exist:", name);
  return;
}

void
exec_delete_table(char* name)
{
  struct table_list* tl;
  int j, k, pos;
  for (j = 0; j < all_table_lists->curr; j++) {
    tl = all_table_lists->table_lists[j];
    if ((pos = name_list_pos(name, tl->names)) >= 0) {
      tl->tables[pos] = delete_table(tl->tables[pos]);
      k = remove_from_name_list(name, tl->names);
      tl->tables[k] = tl->tables[--tl->curr];
      return;
    }
  }
}

// public interface

void
exec_option(void)
{
  if (get_option("reset")) set_defaults("option");
  if (get_option("tell")) print_command(options);

}

void
exec_help(struct in_cmd* cmd)
  /* prints list of commands */
{
  char** toks = cmd->tok_list->p;
  int i, k = 0, pos, n = cmd->tok_list->curr;
  if (n == 1)
  {
    while (special_comm_cnt[k] > 0) k++;
    puts("special commands - no further help:");
    puts(" ");
    for (i = 0; i < k-1; i++)
    {
      if (strchr(special_comm_desc[i], '(') != NULL)
        fprintf(prt_file, "%s<condition>){<statements(s)>}\n",
                &special_comm_desc[i][0]);
      else if (strchr(special_comm_desc[i], '{') != NULL)
        fprintf(prt_file, "%s<statements(s)>}\n",
                &special_comm_desc[i][0]);
      else fprintf(prt_file, "%s{<statements(s)>}\n",
                   &special_comm_desc[i][0]);
    }
    fprintf(prt_file, "<name>:line(...);\n");
    puts(" ");
    puts("normal commands or predefined particles:");
    dump_name_list(defined_commands->list);
  }
  else
  {
    for (i = 1; i < n; i++)
    {
      if ((pos = name_list_pos(toks[i], defined_commands->list)) > -1)
        dump_command(defined_commands->commands[pos]);
      else puts("no help for this command - try help; (no arguments)");
    }
  }
}

void
exec_assign(struct in_cmd* cmd)
  /* executes output unit assignment */
{
  char* p;
  char tmp[FNAME_L];
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  int pos = name_list_pos("echo", nl);
  int cut = name_list_pos("truncate", nl);

  if (prt_file != stdout)  fclose(prt_file);
  if (nl->inform[pos]) {
    p = pl->parameters[pos]->string; strcpy(tmp, p);
    if (strcmp(stolower(tmp), "terminal") == 0)
      prt_file = stdout;
    else {
      if (assign_start == 0) {
        assign_start = 1;
        prt_file = fopen(p, "w");
      }
      else if (!nl->inform[cut] || !pl->parameters[cut]->double_value)
        prt_file = fopen(p, "a");
      else
        prt_file = fopen(p, "w");
    }
  }
  else prt_file = stdout;
}

void
exec_removefile(struct in_cmd* cmd)
{
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  int pos = name_list_pos("file", nl);

  if (nl->inform[pos]) {
    if (remove(pl->parameters[pos]->string))
      warning("unable to remove file: ", pl->parameters[pos]->string);
  }
}

void
exec_renamefile(struct in_cmd* cmd)
{
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  int pos = name_list_pos("file", nl);
  int new = name_list_pos("name", nl);

  if (nl->inform[pos] && nl->inform[new]) {
    if (rename(pl->parameters[pos]->string, pl->parameters[new]->string))
      warning("unable to rename file: ", pl->parameters[pos]->string);
  }
}

void
exec_call(struct in_cmd* cmd)
  /* handles calling external files */
{
  struct command_parameter_list* pl = cmd->clone->par;
  struct name_list* nl = cmd->clone->par_names;
  int pos = name_list_pos("file", nl);
  int top = in->curr;
  
  if (nl->inform[pos]) {
    if (down_unit(pl->parameters[pos]->string)) madx_input(top);
  }
  else warning("call without filename:", "ignored");
}

void
exec_cmd_delete(struct in_cmd* cmd)
/* handles all delete request through "delete" command */
{
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  int pos;
  char* name;

  pos = name_list_pos("sequence", nl);
  if (nl->inform[pos]) {
    name = pl->parameters[pos]->string;
    exec_delete_sequ(name);
  }

  pos = name_list_pos("table", nl);
  if (nl->inform[pos]) {
    name = pl->parameters[pos]->string;
    exec_delete_table(name);
  }
}

void
exec_show(struct in_cmd* cmd)
  /* executes "show" command */
{
  struct element* el;
  struct variable* var;
  char** toks = cmd->tok_list->p;
  int i, pos, n = cmd->tok_list->curr;

  for (i = 1; i < n; i++) {
    if (strcmp(toks[i],",")) {
      if (strncmp(toks[i], "beam", 4) == 0) show_beam(toks[i]);
      else if ((pos = name_list_pos(toks[i], defined_commands->list)) > -1) {
        if (strcmp(toks[i], "option") == 0) dump_command(options);
        else if (strcmp(toks[i], "eoption") == 0 && current_eopt != NULL)
          dump_command(current_eopt);
        else dump_command(defined_commands->commands[pos]);
      }
      else if ((pos = name_list_pos(toks[i], beta0_list->list)) > -1)
        dump_command(beta0_list->commands[pos]);
      else if ((el = find_element(toks[i], element_list)) != NULL)
        dump_element(el);
      else if ((var = find_variable(toks[i], variable_list))) {
        if (var->expr)  fprintf(prt_file, "%s := %s ;\n", toks[i], var->expr->string);
        else fprintf(prt_file, v_format("%s = %F ;\n"), toks[i], var->value);
      }
      else fprintf(prt_file, "%s not found\n", toks[i]);
    }
  }
  return;
}

void
exec_create_table(struct in_cmd* cmd)
  /* makes a user defined table */
{
  const char *rout_name = "exec_create_table";
  struct table* t;
  int* t_types;
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  struct char_p_array* m;
  char** t_c;
  int j, pos = name_list_pos("table", nl);
  char* name = NULL;
  int  ncols = 0;  /*number of columns*/

  if (nl->inform[pos] == 0) {
    warning("no table name:", "ignored");
    return;
  }

  if ((name = pl->parameters[pos]->string) == NULL) {
    warning("no table name: ", "ignored");
    return;
  }

  if ((pos = name_list_pos(name, table_register->names)) > -1) {
    warning("table already exists: ", "ignored");
    return;
  }

  pos = name_list_pos("column", nl);
  if (nl->inform[pos] == 0) {
    warning("table without columns: ", "ignored");
    return;
  }

  m = pl->parameters[pos]->m_string;
  ncols = m->curr;

  /* now make table */
  t_types = mymalloc_atomic(rout_name, ncols * sizeof *t_types);
  t_c = mymalloc(rout_name, (ncols+1) * sizeof *t_c);

  for (j = 0; j < m->curr; j++) {
    if (*m->p[j] == '_') {
      t_types[j] = 3; /* type string */
      t_c[j] = permbuff(&m->p[j][1]);
    }
    else {
      t_types[j] = 2; /* type double */
      t_c[j] = permbuff(m->p[j]);
    }
  }

  t_c[ncols] = blank;
  t = make_table(name, "user", t_c, t_types, USER_TABLE_LENGTH);
  t->org_cols = 0;  /* all entries are "added" */
  add_to_table_list(t, table_register);
  myfree(rout_name, t_c); myfree(rout_name, t_types);
  t->dynamic = 1;
  
  return;
}

void
exec_store_coguess(struct in_cmd* cmd)
  /* stores the initial orbit guess of the user */
{
  struct name_list* nl = cmd->clone->par_names;
  double tol, toldefault=1.e-6;

  int pos = name_list_pos("tolerance", nl);
  if (nl->inform[pos])  {
    tol = command_par_value("tolerance", cmd->clone);
    set_variable("twiss_tol", &tol);
  }
  store_orbit(cmd->clone, guess_orbit);
  guess_flag = 1;

  /* 2014-May-30  13:55:50  ghislain: clear option added to cancel coguess */
  if (log_val("clear", cmd->clone)) {
    set_variable("twiss_tol",&toldefault);
    zero_double(guess_orbit, 6);
    guess_flag=0;
  }

  return;
}

void
exec_dump(struct in_cmd* cmd)
  /* write a table out */
{
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  int pos = name_list_pos("table", nl);
  char* name = NULL;
  char *f, filename[FNAME_L];

  // get "table" command parameter
  if (pos < 0 || nl->inform[pos] == 0 || (name = pl->parameters[pos]->string) == NULL) {
    warning("dump without table name:", "ignored");
    return;
  }

  // get "file" command parameter
  pos = name_list_pos("file", nl);
  if (nl->inform[pos] == 0)
    strcpy(filename, "terminal"); // write to console
  else if ((f = pl->parameters[pos]->string) == NULL || *f == '\0')
    strcpy(filename, name); // write to file with same name as table
  else
    strcpy(filename,f);


  // get table from registered tables
  if ((pos = name_list_pos(name, table_register->names)) < 0) {
    warning("table name not found:", "ignored");
    return;
  }

  struct table* t = table_register->tables[pos];
  out_table(name, t, filename);

  return;
}

void
exec_fill_table(struct in_cmd* cmd)
  /* adds variables to a table */
{
  struct table* t;
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  int pos = name_list_pos("table", nl);
  char* name = NULL;
  int row,curr;
  if (nl->inform[pos] == 0) {
    warning("no table name:", "ignored");
    return;
  }

  if ((name = pl->parameters[pos]->string) == NULL) {
    warning("no table name: ", "ignored");
    return;
  }

  if ((pos = name_list_pos(name, table_register->names)) < 0) {
    warning("table name not found:", "ignored");
    return;
  }
  t = table_register->tables[pos];

  pos=name_list_pos("row", nl);
  row=(int) pl->parameters[pos]->double_value;

  if (row==0 || row == t->curr + 1) { // add row to table and fill
    add_vars_to_table(t);
    if (++t->curr == t->max) grow_table(t);
    return;
  }

  if (abs(row) > t->curr) { // bounds check
    // note: cases row=0 and row=t->curr+1 already treated
    warning("row index out of bounds:", " ignored");
    return;
  }

  // 2014-Aug-18  17:05:33  ghislain: allow for negative row numbers; 
  // -1 indexes last row and negative numbers count row numbers backwards from end
  // -2 denoting the one before last and so on
  if (row<0) row=t->curr + 1 + row; 
  
  curr=t->curr;
  t->curr=row-1;
  add_vars_to_table(t);
  t->curr=curr;
  
  return;
}

void
exec_setvars_table(struct in_cmd* cmd)
  /* set variables from a table */
{
  struct table* t;
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  int pos = name_list_pos("table", nl);
  char* name = NULL;
  int row,curr;

  if (nl->inform[pos] == 0) {
    warning("no table name:", "ignored");
    return;
  }

  if ((name = pl->parameters[pos]->string) == NULL) {
    warning("no table name: ", "ignored");
    return;
  }

  current_node = NULL; /* to distinguish from other table fills */

  if ((pos = name_list_pos(name, table_register->names)) < 0) {
    warning("table name not found:", "ignored");
    return;
  }
  t = table_register->tables[pos];

  pos=name_list_pos("row", nl);
  row=(int) pl->parameters[pos]->double_value;

  if (abs(row) > t->curr || row == 0){
    warning("row index out of bounds:", " ignored");
    return;
  }

  // 2014-Aug-18  17:05:33  ghislain: allow for negative row numbers; 
  // -1 indexes last row and negative numbers count row numbers backwards from end
  // -2 denoting the one before last and so on
  if (row<0) row=t->curr + 1 + row; 
  
  curr=t->curr;
  t->curr=row-1;
  set_vars_from_table(t);
  t->curr=curr;
  
  return;
}


void
exec_setvars_lin_table(struct in_cmd* cmd)
  /* set variables from a table by linear interpolation between values in two rows */
{
  struct table* t;
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  int pos,row1,row2,i;
  char* name = NULL;
  char* param = NULL;
  char* colname = NULL;
  double val1,val2;
  char expr[10*NAME_L];
  i=0;

  pos = name_list_pos("table", nl);
  if (nl->inform[pos] == 0) {
    warning("no table name:", "ignored");
    return;
  }

  if ((name = pl->parameters[pos]->string) == NULL) {
    warning("no table name: ", "ignored");
    return;
  }

  /*current_node = NULL;  to distinguish from other table fills ????*/
  pos=name_list_pos("row1", nl);
  row1=(int) pl->parameters[pos]->double_value; 
  pos=name_list_pos("row2", nl);
  row2=(int) pl->parameters[pos]->double_value; 

  pos = name_list_pos("param", nl);
  param = pl->parameters[pos]->string;

  if ((pos = name_list_pos(name, table_register->names)) < 0) {
    warning("table name not found:", "ignored");
    return;
  }
  t = table_register->tables[pos];

  if (abs(row1) > t->curr || row1 == 0){
    warning("row1 index out of bounds:", " ignored");
    return;
  } else if (abs(row2) > t->curr || row2 == 0){
    warning("row2 index out of bounds:", " ignored");
    return;
  }
  
  /* negative row numbers are counting backwards from last row */
  /* transform into positive values */
  if (row1<0) row1=t->curr + 1 + row1; 
  if (row2<0) row2=t->curr + 1 + row2;

  for (i = 0; i < t->num_cols; i++) {
    if (t->columns->inform[i] <3){
      colname=t->columns->names[i];
      val1=t->d_cols[i][row1-1];
      val2=t->d_cols[i][row2-1];
      // 2014-Aug-18  17:15:08  ghislain: 
      // value := val1*param + val2*(1-param) ; 
      // sprintf(expr,"%s:=%10.16g*(%s)%+10.16g*(1-(%s));", colname,val1,param,val2,param);
      // is counterintuitve for interpolation between val1 and val2 and should instead be 
      // value := val1 + param*(val2-val1) = val1*(1-param) + val2*param;
      sprintf(expr,"%s:=%10.16g*(1-(%s))%+10.16g*(%s);", colname,val1,param,val2,param);
      pro_input(expr);
    }
  }
 
 return;
}

void
exec_print(struct in_cmd* cmd)
  /* prints text from "print" command to current output unit */
{
  struct command_parameter_list* pl = cmd->clone->par;
  struct name_list* nl = cmd->clone->par_names;
  int pos = name_list_pos("text", nl);
  if (nl->inform[pos]) fprintf(prt_file,"%s\n", pl->parameters[pos]->string);
}

void // this function extend print_value from mad_eval.c
exec_printf(struct in_cmd* cmd)
{
  struct command_parameter_list* pl = cmd->clone->par;
  struct name_list* nl = cmd->clone->par_names;

  // retrieve output format from text=""
  int txt_pos = name_list_pos("text", nl);
  if (!nl->inform[txt_pos]) { warning("missing text=:",""); return; }
  char *txt_str = v_format(pl->parameters[txt_pos]->string);

  // check for value=...
  int val_pos = name_list_pos("value", nl);
  if (!nl->inform[val_pos]) { warning("missing value=:",""); return; }

  // retrieve vector of values from value=...
  int val_n = command_par_vector("value", cmd->clone, NULL);
  if (val_n < 100) val_n = 100;
  double val[val_n]; 
  command_par_vector("value", cmd->clone, val);

  // enough to print a full twiss row, anyway C limits is 127, and var_form is long enough
  fprintf(prt_file, txt_str,
    val[ 0], val[ 1], val[ 2], val[ 3], val[ 4], val[ 5], val[ 6], val[ 7], val[ 8], val[ 9],
    val[10], val[11], val[12], val[13], val[14], val[15], val[16], val[17], val[18], val[19],
    val[20], val[21], val[22], val[23], val[24], val[25], val[26], val[27], val[28], val[29],
    val[30], val[31], val[32], val[33], val[34], val[35], val[36], val[37], val[38], val[39],
    val[40], val[41], val[42], val[43], val[44], val[45], val[46], val[47], val[48], val[49],
    val[50], val[51], val[52], val[53], val[54], val[55], val[56], val[57], val[58], val[59],
    val[60], val[61], val[62], val[63], val[64], val[65], val[66], val[67], val[68], val[69],
    val[70], val[71], val[72], val[73], val[74], val[75], val[76], val[77], val[78], val[79],
    val[80], val[81], val[82], val[83], val[84], val[85], val[86], val[87], val[88], val[89],
    val[90], val[91], val[92], val[93], val[94], val[95], val[96], val[97], val[98], val[99]);
  fprintf(prt_file, "\n");
}

