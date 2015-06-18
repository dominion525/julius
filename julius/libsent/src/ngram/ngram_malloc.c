/**
 * @file   ngram_malloc.c
 * @author Akinobu LEE
 * @date   Wed Feb 16 16:48:56 2005
 * 
 * <JA>
 * @brief  N-gram構造体の初期メモリ確保と解放
 * </JA>
 * 
 * <EN>
 * @brief  Initial memory allocation and free for N-gram stucture
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
#include <sent/ngram2.h>

/** 
 * Allocate a new N-gram structure.
 * 
 * @return pointer to the newly allocated structure.
 */
NGRAM_INFO *
ngram_info_new()
{
  NGRAM_INFO *new;

  new = (NGRAM_INFO *)mymalloc(sizeof(NGRAM_INFO));

  return(new);
}

/** 
 * Free N-gram data.
 * 
 * @param ndata [in] N-gram data
 */
void
ngram_info_free(NGRAM_INFO *ndata)
{
  /* bin test only */
  /* free word names */
  if (ndata->from_bin) {
    free(ndata->wname[0]);
    free(ndata->wname);
  } else {
    WORD_ID w;
    for(w=0;w<ndata->max_word_num;w++) {
      free(ndata->wname[w]);
    }
    free(ndata->wname);
  }
  /* free 1-gram */
  free(ndata->p);
  free(ndata->bo_wt_lr);
  free(ndata->bo_wt_rl);
  free(ndata->n2_bgn);
  free(ndata->n2_num);
  /* free 2-gram */
  free(ndata->n2tonid);
  free(ndata->p_lr);
  free(ndata->p_rl);
  switch(ndata->version) {
  case 3:
    free(ndata->bo_wt_rrl);
    free(ndata->n3_bgn);
    free(ndata->n3_num);
    break;
  case 4:
    free(ndata->n2bo_upper);
    free(ndata->n2bo_lower);
    free(ndata->bo_wt_rrl);
    free(ndata->n3_bgn_upper);
    free(ndata->n3_bgn_lower);
    free(ndata->n3_num);
    break;
  }
  /* free 3-gram */
  free(ndata->n3tonid);
  free(ndata->p_rrl);
  /* free index tree */
  free_ptree(ndata->root);
  /* free whole */
  free(ndata);
}

