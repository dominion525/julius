/**
 * @file   cpair.c
 * @author Akinobu LEE
 * @date   Tue Feb 15 13:54:44 2005
 * 
 * <JA>
 * @brief  カテゴリ対制約へのアクセス関数およびメモリ管理
 *
 * カテゴリ対制約のメモリ確保，およびカテゴリ間の接続の可否を返す関数です．
 * </JA>
 * 
 * <EN>
 * @brief  Category-pair constraint handling
 *
 * Functions to allocate memory for category-pair constraint, and functions
 * to return whether the given category pairs can be connected or not are
 * defined here.
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

#include <sent/stddefs.h>
#include <sent/dfa.h>

/// Bit mask to access category-pair matrix
static unsigned char cp_table[] = {
  0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
};

/** 
 * Return whether the given two category can be connected or not.
 * 
 * @param dfa [in] DFA grammar holding category pair matrix
 * @param i [in] category id of left word
 * @param j [in] category id of right word
 * 
 * @return TRUE if connection is allowed by the grammar, FALSE if prohibited.
 */
boolean
dfa_cp(DFA_INFO *dfa, int i, int j)
{
  /*return(dfa->cp[i][j]);*/
  return((dfa->cp[i][j>>3] & cp_table[j&7]) ? TRUE : FALSE);
}

/** 
 * Return whether the category can be appear at the beginning of sentence.
 * 
 * @param dfa [in] DFA grammar holding category pair matrix
 * @param i [in] category id of the word
 * 
 * @return TRUE if it can appear at the beginning of sentence, FALSE if not.
 */
boolean
dfa_cp_begin(DFA_INFO *dfa, int i)
{
  /*return(dfa->cp_begin[i]);*/
  return((dfa->cp_begin[i>>3] & cp_table[i&7]) ? TRUE : FALSE);
}

/** 
 * Return whether the category can be appear at the end of sentence.
 * 
 * @param dfa [in] DFA grammar holding category pair matrix
 * @param i [in] category id of the word
 * 
 * @return TRUE if it can appear at the end of sentence, FALSE if not.
 */
boolean
dfa_cp_end(DFA_INFO *dfa, int i)
{
  /*return(dfa->cp_end[i]);*/
  return((dfa->cp_end[i>>3] & cp_table[i&7]) ? TRUE : FALSE);
}

/** 
 * Set the category-pair matrix bit
 * 
 * @param dfa [out] DFA grammar holding category pair matrix
 * @param i [in] category id of left word
 * @param j [in] category id of right word
 * @param value TRUE if connection allowed, FALSE if connection prohibited.
 */
void
set_dfa_cp(DFA_INFO *dfa, int i, int j, boolean value)
{
  /*dfa->cp[i][j] = value;*/
  if (value) {
    dfa->cp[i][j>>3] |= cp_table[j&7];
  } else {
    dfa->cp[i][j>>3] &= ~ cp_table[j&7];
  }
}

/** 
 * Set the category-pair matrix bit at the beginning of sentence
 * 
 * @param dfa [out] DFA grammar holding category pair matrix
 * @param i [in] category id of the word
 * @param value TRUE if the category can appear at the beginning of
 * sentence, FALSE if not.
 */
void
set_dfa_cp_begin(DFA_INFO *dfa, int i, boolean value)
{
  /*dfa->cp_begin[i] = value;*/
  if (value) {
    dfa->cp_begin[i>>3] |= cp_table[i&7];
  } else {
    dfa->cp_begin[i>>3] &= ~ cp_table[i&7];
  }
}

/** 
 * Set the category-pair matrix bit at the end of sentence
 * 
 * @param dfa [out] DFA grammar holding category pair matrix
 * @param i [in] category id of the word
 * @param value TRUE if the category can appear at the end of
 * sentence, FALSE if not.
 */
void
set_dfa_cp_end(DFA_INFO *dfa, int i, boolean value)
{
  /*dfa->cp_end[i] = value;*/
  if (value) {
    dfa->cp_end[i>>3] |= cp_table[i&7];
  } else {
    dfa->cp_end[i>>3] &= ~ cp_table[i&7];
  }
}

/** 
 * Initialize category pair matrix in the grammar data.
 * 
 * @param dfa [out] DFA grammar to hold category pair matrix
 */
void
init_dfa_cp(DFA_INFO *dfa)
{
  dfa->cp_root = NULL;
  dfa->cp = NULL;
  dfa->cp_begin = NULL;
  dfa->cp_end = NULL;
}

/** 
 * Allocate memory for category pair matrix and initialize it.
 * 
 * @param dfa [out] DFA grammar to hold category pair matrix
 * @param term_num [in] number of categories in the grammar
 */
void
malloc_dfa_cp(DFA_INFO *dfa, int term_num)
{
  int i, j, x;

  x = (term_num + 7) >> 3;
  dfa->cp_root = (unsigned char *)mymalloc(sizeof(unsigned char) * term_num * x);
  dfa->cp = (unsigned char **)mymalloc(sizeof(unsigned char *) * term_num);
  for(i=0;i<term_num;i++) {
    dfa->cp[i] = &(dfa->cp_root[x*i]);
    for(j=0;j<term_num;j++) {
      set_dfa_cp(dfa, i, j, FALSE);
    }
  }
  dfa->cp_begin = (unsigned char *)mymalloc(sizeof(unsigned char) * x);
  dfa->cp_end = (unsigned char *)mymalloc(sizeof(unsigned char) * x);
  for(i=0;i<term_num;i++) {
    set_dfa_cp_begin(dfa, i, FALSE);
    set_dfa_cp_end(dfa, i, FALSE);
  }
  dfa->term_num = term_num;
}

/** 
 * Re-allocate memory for category pair matrix, can be called when
 * the number of category is expanded.
 * 
 * @param dfa [I/O] DFA grammar holding category pair matrix
 * @param old_term_num [in] number of categories when the last category pair matrix was allocated
 * @param new_term_num [in] new number of categories in the grammar
 */
void
realloc_dfa_cp(DFA_INFO *dfa, int old_term_num, int new_term_num)
{
  int i, j, n, x, old_x;
  unsigned char *oldroot, *oldbegin, *oldend;
  unsigned char **oldcp;

  if (dfa->cp == NULL) {
    malloc_dfa_cp(dfa, new_term_num);
    return;
  }

  x = (new_term_num + 7) >> 3;
  old_x = (old_term_num + 7) >> 3;

  oldroot = dfa->cp_root;
  oldcp   = dfa->cp;
  
  dfa->cp_root = (unsigned char *)mymalloc(sizeof(unsigned char) * new_term_num * x);
  dfa->cp = (unsigned char **)mymalloc(sizeof(unsigned char *) * new_term_num);
  for(i=0;i<new_term_num;i++) {
    dfa->cp[i] = &(dfa->cp_root[x*i]);
  }
  for(i=0;i<old_term_num;i++) {
    for(n=0;n<old_x;n++) {
      dfa->cp[i][n] = oldcp[i][n];
    }
  }
  for(i=old_term_num;i<new_term_num;i++) {
    for(j=0;j<old_term_num;j++) {
      set_dfa_cp(dfa, i, j, FALSE);
      set_dfa_cp(dfa, j, i, FALSE);
    }
    set_dfa_cp(dfa, i, i, FALSE);
  }
  free(oldcp);
  free(oldroot);

  oldbegin = dfa->cp_begin;
  oldend = dfa->cp_end;
  dfa->cp_begin = (unsigned char *)mymalloc(sizeof(unsigned char) * x);
  dfa->cp_end = (unsigned char *)mymalloc(sizeof(unsigned char) * x);
  for(n=0;n<old_x;n++) {
    dfa->cp_begin[n] = oldbegin[n];
    dfa->cp_end[n] = oldend[n];
  }
  for(i=old_term_num;i<new_term_num;i++) {
    set_dfa_cp_begin(dfa, i, FALSE);
    set_dfa_cp_end(dfa, i, FALSE);
  }
  free(oldbegin);
  free(oldend);

  dfa->term_num = new_term_num;
}

/** 
 * Free the category pair matrix from DFA grammar.
 * 
 * @param dfa [i/o] DFA grammar holding category pair matrix
 */
void
free_dfa_cp(DFA_INFO *dfa)
{
  if (dfa->cp != NULL) {
    free(dfa->cp_begin);
    free(dfa->cp_end);
    free(dfa->cp);
    free(dfa->cp_root);
  }
}
