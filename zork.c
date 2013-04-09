/*
* zork - a dir and file indexing program for linux
* Author :: pointer.dev [at] gmail.com
*           Copyright (C) 2013 
* Released under           :: GPL v3
* Release date (v0.4 beta) :: 2013-04-10
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#define MAX_DIR_LEN 512
#define DEF_DIR_PERM 0775
#define MAX_ABS_PATH_LEN 2048
#define MAX_LIST_ENT 50
#define MAX_RECURSE 500
#define ABS(x) x < 0 ? -(x) : x

void init(void);
char *get_line(char *looper, char *buff);
void chop_shit(char *buff);
int get_dir_depth(char *buff);
int push(DIR **dp_stack, DIR *item, int *dp_stack_top);
DIR *pop(DIR **dp_stack, int *dp_stack_top);
int add(char *dirname);
int update(bool full);
int search(const char *srch_key, bool p, char **list);
int rm_ent(const char *d_entry);
void show_ent(void);

char *dir_file_name = "dir_list";
char *db_file_name = "file_db";
char prog_dir_path[MAX_DIR_LEN];
char d_file_abs[MAX_ABS_PATH_LEN];
char db_file_abs[MAX_ABS_PATH_LEN];

bool recurse;
int recurse_l;

void usage(char *progname, int er_code)
{
  printf("Usage: %s <options> <arguments>\n\
Available options:\n\
\t-r Make the particular directory listing recursive. Optionaly give a recursion count.\n\
\t   Using -r without the recursion count results in recursion to the innermost directory.\n\
\t-a <dir name> Add a directory entry and update database\n\
\t-u Update current database\n\
\t-s <search string> Search\n\
\t-p <program name> Open selected search result in external program\n\
\t-R <dir name> Remove a directory entry and update database\n\
\t-h This help text\n\
Note: Multiple options can be used at once, but order of the options matter.\n\
\t-p should be used with -s and placed before -s.\n\
\t-r should be used with -a and placed before -a.\n", progname);

  exit(er_code);
}

void malloc_bailout(void)
{
  perror("malloc");
  exit(EXIT_FAILURE);
}

/* init global variables */
void init(void)
{
  bzero(prog_dir_path, MAX_DIR_LEN);
  strncpy(prog_dir_path, getenv("HOME"), MAX_DIR_LEN);
  strncat(prog_dir_path, ".mveman/", MAX_DIR_LEN-strlen(prog_dir_path)-1);

  struct stat buff;
  if(stat((const char*)&prog_dir_path, &buff) == -1)
  {
    if(mkdir(prog_dir_path, DEF_DIR_PERM))
      exit(EXIT_FAILURE);
  }

  strcpy(d_file_abs, prog_dir_path);
  strncat(d_file_abs, dir_file_name, MAX_ABS_PATH_LEN-1);

  strcpy(db_file_abs, prog_dir_path);
  strncat(db_file_abs, db_file_name, MAX_ABS_PATH_LEN-1);

  recurse = false;
}

/* get a single line from read file */
char *get_line(char *looper, char *buff)
{
  bzero(buff, MAX_ABS_PATH_LEN);
  int indx=0;
  while(*looper!='\n' && indx<MAX_ABS_PATH_LEN)
    buff[indx++] = *looper++;

  buff[indx] = '\0';
  return looper;
}

/* add an entry to dir_file */
int add(char *dirname)
{
  if(!dirname) return;

  struct stat buff;

  if(access(dirname, F_OK)) 
  { fputs("Directory doesn't exist.\n", stderr); exit(EXIT_FAILURE); }
  else
  {
    stat((const char*)dirname, &buff);
    if(!(buff.st_mode & S_IFDIR))
    { fputs("Not a dir.", stderr); exit(EXIT_FAILURE); }
  }
  
  FILE *fp;
  bool no_dfile_update = false;

  if(fp = fopen(d_file_abs, "r"))
  {
    fseek(fp, 0L, SEEK_END);
    long bytes = ftell(fp);
    rewind(fp);
    char *mem = malloc(bytes);
    if(!mem) malloc_bailout();
    fread(mem, 1, bytes, fp);
    fclose(fp);
    char *loop = mem;

    while(*loop && loop-mem != bytes) /* check if already exists */
    {
      char temp_buff[MAX_ABS_PATH_LEN];
      loop = get_line(loop, temp_buff);

      char *name_tok = strtok(temp_buff, "=");
      char *status_tok = strtok(NULL, "=");
      
      int rec_level = atoi(status_tok); 
      rec_level = rec_level < 0 ? MAX_RECURSE-1 : rec_level;

      if(strstr(temp_buff, dirname))
      { 
        if(!strcmp(name_tok, dirname))
        {
          if( (rec_level == 0 && !recurse) ||
              (rec_level == recurse_l) )
          {
            fputs("Already exists.\n", stderr);
            exit(EXIT_FAILURE);
          }
          else if(recurse_l > rec_level){
            rm_ent(name_tok);
          }
          else no_dfile_update = true; /* don't update dir file */
        }

        else if(sizeof(name_tok) >= sizeof(dirname))
        {
          if(recurse) {
            int offset = ABS(get_dir_depth(dirname) - get_dir_depth(name_tok));
            if(recurse_l > offset)
              rm_ent(name_tok);
          }
        }
        else
        {
          if(rec_level == -1)        /* if already exists and is rec. we ignore current */
            no_dfile_update = true;  /* command and just update file_db                 */
          else {
            int offset = ABS(get_dir_depth(dirname) - get_dir_depth(name_tok));
            if(rec_level > offset)
            no_dfile_update = true;
          }
        }
      }

      loop++;
    }
    free(mem);
  }

  if(!access(d_file_abs, F_OK))
    fp = fopen(d_file_abs, "a");
  else
    fp = fopen(d_file_abs, "w");
  
  short ret=0; 
  if(!no_dfile_update)
    fprintf(fp, "%s=%d\n", dirname, \
    recurse_l ? (recurse_l == MAX_RECURSE-1 ? -1 : recurse_l) : 0);
  else { update(1); ret = 1; }
  fclose(fp);

  return ret;
}

/* full updates the entire db file, !full adds listing for only the last dir in dir_file */
int update(bool full)
{
  FILE *db;

  if(access(d_file_abs, F_OK))
  { fputs("No dirs added to create index.", stderr); exit(EXIT_FAILURE); }

  if(full)
    db = fopen(db_file_abs,"w"); /* open db file */
  else
    db = fopen(db_file_abs,"a");

  if(!db) { perror("fopen"); exit(EXIT_FAILURE); }

  FILE *dir_names;
  if(!(dir_names = fopen(d_file_abs,"r"))) /* open dir_file to get dir names to list */
  { perror("fopen"); exit(EXIT_FAILURE); }

  fseek(dir_names, 0L, SEEK_END);
  long bytes = ftell(dir_names);
  rewind(dir_names);
  char *mem = malloc(bytes);
  if(!mem) malloc_bailout();
  fread(mem, 1, bytes, dir_names);
  char *loop = mem;
  fclose(dir_names);

  fputs("Updating db...\n", stdout);
  while(*loop && loop-mem != bytes)
  {
    char temp_dir_name[MAX_ABS_PATH_LEN];
    loop = get_line(loop, temp_dir_name);
    loop++;
    
    if(!full) /* partial update */
    {
      while(*loop && loop-mem != bytes) /* get the last dir name */
      {  
         loop = get_line(loop, temp_dir_name);
         loop++;
      }
    }

    DIR *dp_stack[MAX_RECURSE], *dp;
    struct dirent *ep;
    int dp_stack_top=0;
    char *name_token = strtok(temp_dir_name, "=");
    char *stat_token = strtok(NULL, "=");
    int rec_level = atoi(stat_token);
    
    if(rec_level == -1) rec_level = MAX_RECURSE-1;
    if(rec_level) recurse = true;
    else recurse = false;
    int preserve_r = rec_level;

    if(!(dp = opendir(name_token))) {
      perror("opendir");
      fclose(db); fclose(dir_names);
      exit(EXIT_FAILURE);
    }
    
    printf("%s\n", name_token); 
    char temp[MAX_ABS_PATH_LEN];
    strncpy(temp, name_token, MAX_ABS_PATH_LEN-1);
    while((ep = readdir(dp)) || dp_stack_top)
    {
      if(!ep && recurse) {
        closedir(dp);
        dp = pop(dp_stack, &dp_stack_top);
        if(!dp) break;
        chop_shit(temp);
        rec_level++;
        continue;
      }
      if(strcmp(ep->d_name, ".") && strcmp(ep->d_name, ".."))
      {
        bool is_dir = true;
        strncat(temp, ep->d_name, MAX_ABS_PATH_LEN-strlen(temp)-1);

        if(recurse && rec_level > 0) {
          struct stat buff;
          stat((const char*)temp, &buff);

          if(buff.st_mode & S_IFDIR) {
            strcat(temp, "/");
            if(push(dp_stack, dp, &dp_stack_top) != -1) {
              dp = opendir(temp);
              --rec_level;
              if(!dp) {
                fprintf(stderr, "Failed to open directory: \"%s\"\n", temp);
                dp = pop(dp_stack, &dp_stack_top);
                is_dir = false;
              }
            }
            else {
              fputs("Warning: Maximum recursion level reached.\n", stderr);
              is_dir = false;
            }
          }
          else is_dir = false;
        }
        else 
          is_dir = false;

        fprintf(db, "%s\n", temp);
        if(!is_dir) chop_shit(temp);
      }
    }
    closedir(dp);

    if(!full) break;
  }
  free(mem);
  fclose(db);
}

/* chop upto previous instance of '/' */
void chop_shit(char *buff)
{
  int len = strlen(buff)-1;
  if(buff[len] == '/') len--;
  while(buff[len] != '/' && len)
    buff[len--] = '\0';
}

int push(DIR **dp_stack, DIR *item, int *dp_stack_top)
{
  if(*dp_stack_top == MAX_RECURSE+1) return -1;
  dp_stack[(*dp_stack_top)++] = item;
  return 0;
}

DIR *pop(DIR **dp_stack, int *dp_stack_top)
{
  if(*dp_stack_top == 0) return NULL;
  return dp_stack[--(*dp_stack_top)];
}

/* number of slashes in the dir name indicating depth */
int get_dir_depth(char *buff)
{
  int len = strlen(buff);
  int i=0;
  int slash_count=0;
  for(; i<len; i++) {
    if(buff[i] == '/') slash_count++;
  }

  return slash_count;
}

/* search for given key in db_file */
int search(const char *srch_key, bool p, char **list)
{
  if(access(db_file_abs, F_OK))
  { fputs("Database file doesn't exist.\n", stderr); exit(EXIT_FAILURE); }

  FILE *db = fopen(db_file_abs, "r");

  fseek(db, 0L, SEEK_END);
  long bytes = ftell(db);
  rewind(db);
  char *mem = malloc(bytes);
  if(!mem) malloc_bailout();
  fread(mem, 1, bytes, db);
  char *loop = mem;

  bool match_f = false;
  int indx=0;
  while(*loop && loop-mem != bytes)
  {
    char temp_entry[MAX_ABS_PATH_LEN];
    loop = get_line(loop, temp_entry);
    if(strcasestr(temp_entry, srch_key)) /* returns null if not found */
    { 
      if(!p) printf("%s\n", temp_entry); 
      else if(indx<MAX_LIST_ENT) /* if to be run with a program */
      {
        char *data = malloc(strlen(temp_entry)+1);
        if(!data) malloc_bailout();
        strcpy(data, temp_entry);
        list[indx++] = data;
      }
      match_f = true; 
    }
    
    loop++;
  }
  if(!match_f) printf("Not found.\n");
  free(mem);
  fclose(db);
  return indx;
}

/* remove an entry from dir_list */
int rm_ent(const char *d_entry)
{
  if(access(d_file_abs, F_OK))
  { fputs("No dirs added yet.", stderr); exit(EXIT_FAILURE); }

  FILE *dir_names = fopen(d_file_abs, "r");
  fseek(dir_names, 0L, SEEK_END);
  long bytes = ftell(dir_names);
  rewind(dir_names);
  char *mem = malloc(bytes);
  if(!mem) malloc_bailout();
  fread(mem, 1, bytes, dir_names);
  char *loop = mem;

  fclose(dir_names);
  if(!(dir_names = fopen(d_file_abs, "w")))
  { perror("fopen"); exit(EXIT_FAILURE); }
  
  bool found=false;
  while(*loop && loop-mem != bytes)
  {
    char temp_ent[MAX_ABS_PATH_LEN];
    loop = get_line(loop, temp_ent);
    if(!*temp_ent) break;

    char *name_token = strtok(temp_ent, "=");
    char *stat_tok = strtok(NULL, "=");

    if(strncmp(name_token, d_entry, strlen(name_token)))
      fprintf(dir_names, "%s=%s\n", name_token, stat_tok);
    else {
      if(!strcmp(stat_tok, "r"))
         recurse=1;
      found = true;
    }
    
    loop++;
  }
  free(mem);
  fclose(dir_names);

  if(found) return 0;
  else {
    fputs("Entry not found.\n", stderr);
    return 1;
  }
}

/* show currently indexed dirs */
void show_ent(void)
{
  FILE *dir_names = fopen(d_file_abs, "r");

  fseek(dir_names, 0L, SEEK_END);
  long bytes = ftell(dir_names);
  rewind(dir_names);
  char *mem = malloc(bytes);
  if(!mem) malloc_bailout();
  fread(mem, 1, bytes, dir_names);
  char *loop = mem;
  fclose(dir_names);

  unsigned int ind=1;
  while(*loop && loop-mem != bytes)
  {
    char temp_buff[MAX_ABS_PATH_LEN];
    loop = get_line(loop, temp_buff);

    char *ent_name = strtok(temp_buff, "=");
    char *ent_stat = strtok(NULL, "=");
    
    if(ent_name && ent_stat)
      printf("%d. %s %s\n", ind, ent_name, *ent_stat ? ent_stat : "");
    else
      printf("Malformed entry on line %d : %s %s\n", ind,
              ent_name ? ent_name:"Unknown dir", ent_stat ? ent_stat:"(Recurse status unknown)");
    loop++; ind++;
  }

  free(mem);
}

/* flush both dir_list and file_db */
void flush(void)
{
  FILE *fp;

  if(!(fp = fopen(d_file_abs, "w")))
    perror("fopen");
  fclose(fp);

  if(!(fp = fopen(db_file_abs, "w")))
    perror("fopen");
  fclose(fp);
  
  fputs("All files flushed.\n", stderr);
}

int main(int argc, char **argv)
{
  char *progname = argv[0];
 /* char last_arg = argv[argc-1][1];*/

  if(argc < 2)
   usage(progname, 1);
  
  init();

  if(argc < 2)
    usage(progname, EXIT_FAILURE);

  bool prog_start = 0;
  bool valid_args = 0;
  char *list[MAX_LIST_ENT];
  char *launch_prog=NULL;
  char temp, *tmp;
  int list_ents=0; /* number of list entries in case -p is used */

  while((temp = getopt(argc, argv, "ur::vhFp:a:s:R:")) != -1)
  {

    switch(temp)
    {
      case 'p': if(optarg) {
                  launch_prog = optarg; prog_start = 1;
                  }
                else usage(progname, 1);
                break;

      case 'u': update(1); break;

     /* case 'U': if(optarg) {
                  tmp = optarg;
                  if(!rm_ent((const char*)tmp)) {
                    add(argv[1]);
                    update(0);
                  }
                  valid_args = true;
                }
                else usage(progname, 1);
                break;*/

      case 'a': if(optarg) { 
                  tmp = optarg;
                  if(!add(tmp)) update(0);
                  valid_args = true;
                }
                else usage(progname, 1);
                break;

      case 'r': recurse = true;
                if(optarg) {
                  tmp = optarg;
                  recurse_l = ABS(atoi(tmp));
                }
                else recurse_l = MAX_RECURSE-1;
                break;

      case 's': if(optarg) {
                  tmp = optarg;
                  if(!prog_start) search((const char*)tmp, 0, NULL);
                  else list_ents = search((const char*)tmp, 1, list);
                  valid_args = true;
                }
                else usage(progname, 1);
                break;

      case 'R': if(optarg) {
                  tmp = optarg;
                  if(!rm_ent((const char*)tmp)) 
                    update(1);
                  valid_args = true;
                }
                else usage(progname, 1); 
                break;

      case 'v': show_ent();
                break;

      case 'F': flush(); break;
      case 'h': usage(progname, 0); break;
      default : usage(progname, 1); break;
    }

  }

  if((temp == -1 && optind == 1) || !valid_args)
    usage(progname, EXIT_FAILURE);
  /*
  while(argc>0 && argv)
  {
    char *temp;
    if(*argv[0] == '-')
      { temp = argv[0]+1; valid_args=1; }
    else { argv++; argc--; continue; }

    switch(*temp)
    {
      case 'p': if(argv[1] && argv[1][0] != '-') { launch_prog = argv[1]; prog_start = 1; }
                else usage(progname, 1);
                break;
      case 'u': update(1); break;
      case 'U': if(!rm_ent((const char*)argv[1])) {
                  add(argv[1]);
                  update(0);
                  break;
                }
      case 'a': if(argv[1]) { if(!add(argv[1])) update(0);}
                else usage(progname, 1);
                break;
      case 'r': recurse = true;
                break;
      case 's': if(argv[1])
                {
                  if(!prog_start) search((const char*)argv[1], 0, NULL);
                  else list_ents = search((const char*)argv[1], 1, list);
                }
                else usage(progname, 1);
                break;
      case 'R': if(argv[1]) {
                  if(!rm_ent((const char*)argv[1])) 
                    update(1);
                  }
                else usage(progname, 1); 
                break;
      case 'v': show_ent();
                break;
      case 'F': flush(); break;
      case 'h': usage(progname, 0); break;
      default : usage(progname, 1); break;
    }

    argc--;
    argv++;
  }*/

  int indx=0;
  if(prog_start && list_ents)
  {
    int resp;
    puts("");
    if(list_ents > 1)
    {
      if(list_ents == MAX_LIST_ENT)
        fputs(":: Showing first 50 matches only\n", stdout);
      while(indx<list_ents)
      { printf("%d. %s\n", indx+1, list[indx]); indx++; }

      fputs("\nChoose [0 to exit]: ", stdout);
      scanf("%d", &resp);
    }
    else { printf("%s\n", list[0]); resp=1; }

    if(resp>0 && resp<=list_ents)
    {
      char temp[MAX_ABS_PATH_LEN];
           bzero(temp, MAX_ABS_PATH_LEN);
      strncpy(temp, launch_prog, MAX_ABS_PATH_LEN);
      strcat(temp, " \"");
      strncat(temp, list[resp-1], MAX_ABS_PATH_LEN-strlen(temp)-1);
      strcat(temp, "\" 2>/dev/null &"); /* shut up and go to bg */
      /*printf("%s\n", temp);*/
      system(temp); 
    }
    for(indx=0; indx<list_ents; indx++)
      free(list[indx]);
  }
  exit(EXIT_SUCCESS);
}
