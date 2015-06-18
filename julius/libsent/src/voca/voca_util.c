/**
 * @file   voca_util.c
 * @author Akinobu LEE
 * @date   Fri Feb 18 21:41:41 2005
 * 
 * <JA>
 * @brief  単語辞書の情報をテキストで出力
 * </JA>
 * 
 * <EN>
 * @brief  Output text informations about the grammar
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
#include <sent/vocabulary.h>

/** 
 * Output overall word dictionary information to stdout.
 * 
 * @param winfo [in] word dictionary
 */
void
print_voca_info(WORD_INFO *winfo)
{
  int i,j;
  long models, states;
  int trnum;
  HMM_Logical *lg;

  models = 0;
  states = 0;
  trnum = 0;
  for (i=0;i<winfo->num;i++) {
    models += winfo->wlen[i];
    for (j=0;j<winfo->wlen[i];j++) {
      lg = winfo->wseq[i][j];
      states += hmm_logical_state_num(lg) - 2;
    }
    if (winfo->is_transparent[i]) trnum++;
  }

  j_printf("Dictionary Info:\n");
  /*j_printf("        vocabulary size  = %d words, %d models, %d states\n", winfo->num, models, states);*/
  j_printf("        vocabulary size  = %d words, %d models\n", winfo->num, models);
  j_printf("        average word len = %.1f models, %.1f states\n", (float)models/(float)winfo->num, (float)states/(float)winfo->num);
  j_printf("       maximum state num = %d nodes per word\n", winfo->maxwn);
  j_printf("       transparent words = ");
  if (trnum > 0) {
    j_printf("%d words\n", trnum);
  } else {
    j_printf("not exist\n");
  }
#ifdef CLASS_NGRAM
  j_printf("       words under class = ");
  if (winfo->cwnum > 0) {
    j_printf("%d words\n", winfo->cwnum);
  } else {
    j_printf("not exist\n");
  }    
#endif
}
	 
/** 
 * Output information of a word in dictionary to stdout.
 * 
 * @param winfo [in] word dictionary
 * @param wid [in] word id to be output
 */
void
put_voca(WORD_INFO *winfo, WORD_ID wid)
{
  int i;
  HMM_Logical *lg;
  
  j_printf("%d: \"%s", wid, winfo->wname[wid]);
#ifdef CLASS_NGRAM
  j_printf(" @%f", winfo->cprob[wid]);
#endif
  if (winfo->is_transparent[wid]) {
    j_printf(" {%s}", winfo->woutput[wid]);
  } else {
    j_printf(" [%s]", winfo->woutput[wid]);
  }
  for(i=0;i<winfo->wlen[wid];i++) {
    lg = winfo->wseq[wid][i];
    j_printf(" %s", lg->name);
    if (lg->is_pseudo) {
      j_printf("(pseudo)");
    } else {
      j_printf("(%s)", lg->body.defined->name);
    }
  }
  j_printf("\"\n");
}

/** 
 * Output information of a word in dictionary to stderr.
 * 
 * @param winfo [in] word dictionary
 * @param wid [in] word id to be output
 */
void
put_voca_err(WORD_INFO *winfo, WORD_ID wid)
{
  int i;
  HMM_Logical *lg;
  
  j_printerr("id%d: %s", wid, winfo->wname[wid]);
#ifdef CLASS_NGRAM
  j_printerr(" @%f", winfo->cprob[wid]);
#endif
  if (winfo->is_transparent) {
    j_printerr(" {%s}", winfo->woutput[wid]);
  } else {
    j_printerr(" [%s]", winfo->woutput[wid]);
  }
  for(i=0;i<winfo->wlen[wid];i++) {
    lg = winfo->wseq[wid][i];
    j_printerr(" %s", lg->name);
    if (lg->is_pseudo) {
      j_printerr("(pseudo)");
    } else {
      j_printerr("(%s)", lg->body.defined->name);
    }
  }
  j_printerr("\n");
}
