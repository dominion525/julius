/**
 * @file   ngram_util.c
 * @author Akinobu LEE
 * @date   Wed Feb 16 17:18:55 2005
 * 
 * <JA>
 * @brief  N-gramの情報をテキスト出力する
 * </JA>
 * 
 * <EN>
 * @brief  Output some N-gram information to stdout
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
 * Estimate the total size of 1-gram part on memory.  The resulting value may
 * differ from the actual value, since the data sizes are hard coded.
 * 
 * @param ndata [in] N-gram data
 * 
 * @return the estimated size on bytes.
 */
static int
get_unigram_size(NGRAM_INFO *ndata)
{
  int unitsize;
  unitsize = sizeof(LOGPROB) * 3 + sizeof(NNID) + sizeof(WORD_ID);
  return(unitsize * ndata->ngram_num[0]);
}

/** 
 * Estimate the total size of 2-gram part on memory.  The resulting value may
 * differ from the actual value, since the data sizes are hard coded.
 * 
 * @param ndata [in] N-gram data
 * 
 * @return the estimated size on bytes.
 */
static int
get_bigram_size(NGRAM_INFO *ndata)
{
  int unitsize;
  int size;

  switch(ndata->version) {
  case 4:
    unitsize = sizeof(WORD_ID) + sizeof(LOGPROB) * 2 + sizeof(NNID_UPPER) + sizeof(NNID_LOWER);
    size = unitsize * ndata->ngram_num[1];
    unitsize = sizeof(LOGPROB) + sizeof(NNID_UPPER) + sizeof(NNID_LOWER) + sizeof(WORD_ID);
    size += unitsize * ndata->bigram_bo_num;
    break;
  case 3:
    unitsize = sizeof(WORD_ID) * 2 + sizeof(LOGPROB) * 3 + sizeof(NNID);
    size = unitsize * ndata->ngram_num[1];
    break;
  }
    
  return(size);
}
  
/** 
 * Estimate the total size of 3-gram part on memory.  The resulting value may
 * differ from the actual value, since the data sizes are hard coded.
 * 
 * @param ndata [in] N-gram data
 * 
 * @return the estimated size on bytes.
 */
static int
get_trigram_size(NGRAM_INFO *ndata)
{
  int unitsize;

  unitsize = sizeof(WORD_ID) + sizeof(LOGPROB);
  return(unitsize * ndata->ngram_num[2]);
}
  

/** 
 * Output misccelaneous information of N-gram to standard output.
 * 
 * @param ndata [in] N-gram data
 */
void
print_ngram_info(NGRAM_INFO *ndata)
{
  j_printf("N-gram info:\n");
  j_printf("\t  struct version = %d\n", ndata->version);
  if (ndata->isopen) {
    j_printf("\t        OOV word = %s(id=%d)\n", ndata->wname[ndata->unk_id],ndata->unk_id);
    j_printf("\t        OOV size = %d words in dict\n", ndata->unk_num);
  } else {
    j_printf("\t        OOV word = none\n");
  }
  j_printf("\t   wordset size  = %8d\n", ndata->max_word_num);
  j_printf("\tuni-gram entries = %8d (%8d bytes)\n",
	 ndata->ngram_num[0], get_unigram_size(ndata));
  j_printf("\t  bi-gram tuples = %8d (%8d bytes)\n",
	 ndata->ngram_num[1], get_bigram_size(ndata));
  j_printf("\t tri-gram tuples = %8d (%8d bytes)\n",
	 ndata->ngram_num[2], get_trigram_size(ndata));
}
