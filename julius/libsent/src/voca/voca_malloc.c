/**
 * @file   voca_malloc.c
 * @author Akinobu LEE
 * @date   Fri Feb 18 21:33:29 2005
 * 
 * <JA>
 * @brief  ñ�켭��¤�ΤΥ������դ��Ȳ���
 * </JA>
 * 
 * <EN>
 * @brief  Memory allocation of word dictionary information
 * </EN>
 * 
 * $Revision: 1.4 $
 * 
 */
/*
 * Copyright (c) 1991-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/vocabulary.h>


/** 
 * Allocate a new word dictionary structure.
 * 
 * @return pointer to the newly allocated WORD_INFO.
 */
WORD_INFO *
word_info_new()
{
  WORD_INFO *new;

  new = (WORD_INFO *)mymalloc(sizeof(WORD_INFO));
  new->mroot = NULL;

  return(new);
}

/** 
 * Free all informations in the WORD_INFO.
 * 
 * @param winfo [i/o] word dictionary data to be freed.
 */
void
word_info_free(WORD_INFO *winfo)
{
  WORD_ID w;
  /* free each word info */
  if (winfo->mroot != NULL) mybfree2(&(winfo->mroot));
  /* free word info */
  free(winfo->wname);
  free(winfo->woutput);
  free(winfo->wseq);
  free(winfo->wlen);
  free(winfo->wton);
#ifdef CLASS_NGRAM
  free(winfo->cprob);
#endif
  free(winfo->is_transparent);
  /* free whole */
  free(winfo);
}

/** 
 * Initialize a new word dictionary structure.
 * 
 * @param winfo [i/o] word dictionary to be initialized.
 */
void
winfo_init(WORD_INFO *winfo)
{
  int n;
  
  n = MAXWSTEP;
  winfo->wlen = (unsigned char *)mymalloc(sizeof(unsigned char)*n);
  winfo->wname = (char **)mymalloc(sizeof(char *)*n);
  winfo->woutput = (char **)mymalloc(sizeof(char *)*n);
  winfo->wseq = (HMM_Logical ***)mymalloc(sizeof(HMM_Logical **)*n);
  winfo->wton = (WORD_ID *)mymalloc(sizeof(WORD_ID)*n);
#ifdef CLASS_NGRAM
  winfo->cprob = (LOGPROB *)mymalloc(sizeof(LOGPROB)*n);
  winfo->cwnum = 0;
#endif
  winfo->is_transparent = (boolean *)mymalloc(sizeof(boolean)*n);
  winfo->maxnum = n;
  winfo->num = 0;
  winfo->head_silwid = winfo->tail_silwid = WORD_INVALID;
  winfo->maxwn = 0;
  winfo->maxwlen = 0;
  winfo->errnum = 0;
  winfo->errph_root = NULL;
}

/** 
 * Expand the word dictionary.
 * 
 * @param winfo [i/o] word dictionary to be expanded.
 */
void
winfo_expand(WORD_INFO *winfo)
{
  int n;

  n = winfo->maxnum;
  if (n >= MAX_WORD_NUM) {
    j_error("Error: maximum dict size exceeded limit (%d)\n", MAX_WORD_NUM);
  }
  n += MAXWSTEP;
  if (n > MAX_WORD_NUM) n = MAX_WORD_NUM;

  winfo->wlen = (unsigned char *)myrealloc(winfo->wlen, sizeof(unsigned char)*n);
  winfo->wname = (char **)myrealloc(winfo->wname, sizeof(char *)*n);
  winfo->woutput = (char **)myrealloc(winfo->woutput, sizeof(char *)*n);
  winfo->wseq = (HMM_Logical ***)myrealloc(winfo->wseq, sizeof(HMM_Logical **)*n);
  winfo->wton = (WORD_ID *)myrealloc(winfo->wton, sizeof(WORD_ID)*n);
#ifdef CLASS_NGRAM
  winfo->cprob = (LOGPROB *)myrealloc(winfo->cprob, sizeof(LOGPROB)*n);
#endif
  winfo->is_transparent = (boolean *)myrealloc(winfo->is_transparent, sizeof(boolean)*n);
  winfo->maxnum = n;
}
