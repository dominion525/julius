/**
 * @file   multi-gram.h
 * @author Akinobu Lee
 * @date   Fri Jul  8 14:47:05 2005
 * 
 * <JA>
 * @brief  ʣ����ʸˡ��Ʊ���˰�������������
 * </JA>
 * 
 * <EN>
 * @brief  Definitions for managing multiple grammars.
 * </EN>
 * 
 * $Revision: 1.3 $
 * 
 */
/*
 * Copyright (c) 1991-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#ifndef __MULTI_GRAM__
#define __MULTI_GRAM__

#include <julius.h>

#ifdef USE_DFA

/// Maximum length of grammar name
#define MAXGRAMNAMELEN 512

/// Grammar holder
typedef struct __multi_gram__ {
  char name[MAXGRAMNAMELEN];	///< Unique name given by user
  unsigned short id;		///< Unique ID 
  DFA_INFO *dfa;		///< DFA describing syntax of this grammar
  WORD_INFO *winfo;		///< Dictionary of this grammar
  int hook;			///< Work area to store command hook
  boolean newbie;		///< TRUE if just read and not yet configured
  boolean active;		///< TRUE if active for recognition
  ///< below vars holds the location of this grammar within the global grammar */
  int state_begin;		///< Location of DFA states in the global grammar
  int cate_begin;		///< Location of category entries in the global grammar
  int word_begin;		///< Location of words in the dictionary of global grammar
  struct __multi_gram__ *next;	///< Link to the next grammar entry
} MULTIGRAM;

/// List of grammars to be read at startup
typedef struct __gram_list__ {
  char *dfafile;		///< DFA file path
  char *dictfile;		///< Dict file path
  struct __gram_list__ *next;   ///< Link to next entry
} GRAMLIST;

/* for command hook */
#define MULTIGRAM_DEFAULT    0	///< Grammar hook value of no operation
#define MULTIGRAM_DELETE     1  ///< Grammar hook value specifying that this grammar is to be deleted
#define MULTIGRAM_ACTIVATE   2  ///< Grammar hook value specifying that this grammar is to be activated
#define MULTIGRAM_DEACTIVATE 3  ///< Grammar hook value specifying that this grammar is to be deactivated

void multigram_add(DFA_INFO *dfa, WORD_INFO *winfo, char *name);
boolean multigram_delete(int gid);
void multigram_delete_all();
boolean multigram_exec();
void multigram_activate(int gid);
void multigram_deactivate(int gid);

void multigram_read_file(char *dfa_file, char *dict_file);
void multigram_add_gramlist(char *dfafile, char *dictfile);
void multigram_remove_gramlist();
void multigram_read_all_gramlist();
void multigram_add_prefix_list(char *prefix_list, char *cwd);
void multigram_add_prefix_filelist(char *listfile);
int multigram_get_active_num();
int multigram_get_gram_from_category(int category);

#endif /* USE_DFA */

#endif /*  __MULTI_GRAM__ */