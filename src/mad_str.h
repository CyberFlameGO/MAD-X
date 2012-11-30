#ifndef MAD_STR_H
#define MAD_STR_H

// types

struct int_array;
struct char_array;
struct char_p_array;

// interface

void  mycpy(char* sout, const char* sin);
char* mystrchr(char* string, char c);
void  mystrcpy(struct char_array* target, char* source);
char* mystrstr(char* string, char* s);
void  myrepl(char* in, char* out, char* string_in, char* string_out);
int   mysplit(char* buf, struct char_p_array* list);

void  conv_char(char* string, struct int_array* tint);
void  stolower_nq(char* s);
char* strip(char* name);
int   supp_lt(char* inbuf, int flag);
void  supp_mul_char(char c, char* string);
char* supp_tb(char* string);      /* suppress trailing blanks in string */
int   zero_string(char* string);  /* returns 1 if string defaults to '0', else 0 */
char* buffer(char* string);       /* obsolete, replaced by permbuff */
char* permbuff(char* string);     /* copy string to permanent buffer */
char* tmpbuff(const char* string);      /* copy string to temporary allocated buffer */
// int   compare_no_case(char* string_1, char* string_2); // replaced by string_icmp
int   is_token(char* pb, char* string, int slen);
char* join(char** it_list, int n);
char* join_b(char** it_list, int n);
char  next_non_blank(char* string);
int   next_non_blank_pos(char* string);
char* noquote(char* string);
int   quote_level(char* string, char* send);
int   square_to_colon(char* string);

// inline functions

#include <ctype.h>

static inline int
str_pos(const char s[], char c)
{
  int i;
  for (i = 0; s[i]; i++)
    if (s[i] == c) return i;
  return -1;
}

static inline int
char_cnt(char c, const char* s)
  /* returns number of occurrences of character c in string */
{
  int i, k = 0;
  for (i = 0; s[i]; i++)
    if(s[i] == c) k++;
  return k;
}

static inline int
next_char(char c, char** toks, int start, int nitem)
  /* returns the number of the token starting with c after token start */
{
  int i;
  for (i = start; i < nitem; i++)
    if(*toks[i] == c) return i;
  return -1;
}

static inline void
replace(char* buf, char in, char out)
  /* replaces character in by character out in string buf */
{
  int j;
  for (j = 0; buf[j]; j++)
    if (buf[j] == in) buf[j] = out;
}

static inline char*
stolower(char* s)  /* converts string to lower in place */
{
  int j;
  for (j = 0; s[j]; j++) {
    unsigned char c = s[j];
    s[j] = tolower(c);
  }
  return s;
}

static inline char*
stoupper(char* s)  /* converts string to upper in place */
{
  int j;
  for (j = 0; s[j]; j++) {
    unsigned char c = s[j];
    s[j] = toupper(c);
  }
  return s;
}

static inline int
string_icmp(const char* s1, const char *s2)  /* case insitive string compare */
{
  int j;
  for (j = 0; s1[j] && s2[j]; j++) {
    unsigned char c1 = s1[j], c2 = s2[j];
    int ic1 = tolower(c1), ic2 = tolower(c2);
    if (ic1 != ic2) return ic2-ic1;
  }
  return s2[j]-s1[j];
}

static inline int
string_cnt(char c, int n, char* toks[])
  /* returns number of strings in toks starting with character c */
{
  int i, k = 0;
  for (i = 0; i < n; i++)
    if(*toks[i] == c) k++;
  return k;
}

static inline void
supp_char(char c, char* s)
  /* suppresses character c in string */
{
  char *p;
  for (p = s; *s; s++)
    if (*s != c) *p++ = *s;
  *p = '\0';
}

static inline int
all_blank(char* s)
{
  int i;
  for (i = 0; s[i]; i++)
    if(s[i] != ' ') return 0;
  return 1;
}

#endif // MAD_STR_H

