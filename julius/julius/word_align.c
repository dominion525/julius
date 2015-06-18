/**
 * @file   word_align.c
 * @author Akinobu Lee
 * @date   Sat Sep 24 16:09:46 2005
 * 
 * <JA>
 * @brief  単語・音素・状態アラインメントを取る．
 *
 * ここでは，認識結果に対する入力音声のアラインメントを出力するための
 * 関数が定義されています．
 *
 * Julius/Julian では，認識結果においてその単語や音素，あるいはHMMの状態が
 * それぞれ入力音声のどの区間にマッチしたのかを知ることができます．
 * より正確なアラインメントを求めるために，Julius/Julian では認識中の
 * 近似を含む情報は用いずに，認識が終わった後に得られた認識結果の単語列に
 * 対して，あらためて forced alignment を実行しています．
 * </JA>
 * 
 * <EN>
 * @brief  Do Viterbi alignment per word / phoneme/ state.
 *
 * This file defines functions for performing forced alignment of
 * recognized words.  The forced alignment is implimented in Julius/Julian
 * to get the best matching segmentation of recognized word sequence
 * upon input speech.  Word-level, phoneme-level and HMM state-level
 * alignment can be obtained.
 *
 * Julius/Julian performs the forced alignment as a post-processing of
 * recognition process.  Recomputation of Viterbi path on the recognized
 * word sequence toward input speech will be done after the recognition
 * to get better alignment.
 *
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

#include <julius.h>

#define PER_WORD 1
#define PER_PHONEME 2
#define PER_STATE 3

/** 
 * <JA>
 * 与えられた単語列からHMMを連結して文全体のHMMを構築する．
 * 
 * @param wseq [in] 単語列
 * @param num [in] @a wseq の数
 * @param num_ret [out] 構築されたHMMに含まれる音素HMMの数
 * @param end_ret [out] アラインメントの区切りとなる状態番号の列
 * @param per_what [in] 単語・音素・状態のどの単位でアラインメントを取るかを指定
 * 
 * @return あらたに割り付けられた文全体をあらわすHMMモデル列へのポインタを返す．
 * </JA>
 * <EN>
 * Make the whole sentence HMM from given word sequence by connecting
 * each phoneme HMM.
 * 
 * @param wseq [in] word sequence to align
 * @param num [in] number of @a wseq
 * @param num_ret [out] number of HMM contained in the generated sentence HMM
 * @param end_ret [out] sequence of state location as alignment unit
 * @param per_what [in] specify the alignment unit (word / phoneme / state)
 * 
 * @return newly malloced HMM sequences.
 * </EN>
 */
static HMM_Logical **
make_phseq(WORD_ID *wseq, short num
#ifdef MULTIPATH_VERSION
	   , boolean **has_sp_ret
#endif
	   , int *num_ret, int **end_ret, int per_what)
{
  HMM_Logical **ph;		/* phoneme sequence */
#ifdef MULTIPATH_VERSION
  boolean *has_sp;
  int k;
#endif
  int phnum;			/* num of above */
  WORD_ID tmpw, w;
  int i, j, pn, st, endn;
  HMM_Logical *tmpp, *ret;

  /* make ph[] from wseq[] */
  /* 1. calc total phone num and malloc */
  phnum = 0;
  for (w=0;w<num;w++) phnum += winfo->wlen[wseq[w]];
  ph = (HMM_Logical **)mymalloc(sizeof(HMM_Logical *) * phnum);
#ifdef MULTIPATH_VERSION
  has_sp = (boolean *)mymalloc(sizeof(boolean) * phnum);
#endif
  /* 2. make phoneme sequence */
#ifdef MULTIPATH_VERSION
  st = 1;
#else
  st = 0;
#endif
  pn = 0;
  endn = 0;
  for (w=0;w<num;w++) {
    tmpw = wseq[w];
    for (i=0;i<winfo->wlen[tmpw];i++) {
      tmpp = winfo->wseq[tmpw][i];
      /* handle cross-word context dependency */
      if (ccd_flag) {
	if (w > 0 && i == 0) {	/* word head */
	  
	  if ((ret = get_left_context_HMM(tmpp, ph[pn-1]->name, hmminfo)) != NULL) {
	    tmpp = ret;
	  }
	  /* if triphone not found, fallback to bi/mono-phone  */
	  /* use pseudo phone when no bi-phone found in alignment... */
	}
	if (w < num-1 && i == winfo->wlen[tmpw] - 1) { /* word tail */
	  if ((ret = get_right_context_HMM(tmpp, winfo->wseq[wseq[w+1]][0]->name, hmminfo)) != NULL) {
	    tmpp = ret;
	  }
	}
      }
      ph[pn] = tmpp;
#ifdef MULTIPATH_VERSION
      if (enable_iwsp && i == winfo->wlen[tmpw] - 1) {
	has_sp[pn] = TRUE;
      } else {
	has_sp[pn] = FALSE;
      }
#endif
      if (per_what == PER_STATE) {
	for (j=0;j<hmm_logical_state_num(tmpp)-2;j++) {
	  (*end_ret)[endn++] = st + j;
	}
#ifdef MULTIPATH_VERSION
	if (enable_iwsp && has_sp[pn]) {
	  for (k=0;k<hmm_logical_state_num(hmminfo->sp)-2;k++) {
	    (*end_ret)[endn++] = st + j + k;
	  }
	}
#endif
      }
      st += hmm_logical_state_num(tmpp) - 2;
#ifdef MULTIPATH_VERSION
      if (enable_iwsp && has_sp[pn]) {
	st += hmm_logical_state_num(hmminfo->sp) - 2;
      }
#endif
      if (per_what == PER_PHONEME) (*end_ret)[endn++] = st - 1;
      pn++;
    }
    if (per_what == PER_WORD) (*end_ret)[endn++] = st - 1;
  }
  *num_ret = phnum;
#ifdef MULTIPATH_VERSION
  *has_sp_ret = has_sp;
#endif
  return ph;
}


/** 
 * <JA>
 * 文全体のHMMを構築し，Viterbiアラインメントを実行し，結果を出力する．
 * 
 * @param words [in] 文仮説をあらわす単語列
 * @param wnum [in] @a words の長さ
 * @param param [in] 入力特徴パラメータ列
 * @param per_what [in] 単語・音素・状態のどの単位でアラインメントを取るかを指定
 * </JA>
 * <EN>
 * Build sentence HMM, call viterbi_segment() and output result.
 * 
 * @param words [in] word sequence of the sentence
 * @param wnum [in] number of words in @a words
 * @param param [in] input parameter vector
 * @param per_what [in] specify the alignment unit (word / phoneme / state)
 * </EN>
 */
static void
do_align(WORD_ID *words, short wnum, HTK_Param *param, int per_what)
{
  HMM_Logical **phones;		/* phoneme sequence */
#ifdef MULTIPATH_VERSION
  boolean *has_sp;		/* whether phone can follow short pause */
  int k;
#endif
  int phonenum;			/* num of above */
  HMM *shmm;			/* sentence HMM */
  int *end_state;		/* state number of word ends */
  int *end_frame;		/* segmented last frame of words */
  LOGPROB *end_score;		/* normalized score of each words */
  LOGPROB allscore;		/* total score of this word sequence */
  WORD_ID w;
  int i, rlen;
  int end_num = 0;
  int *id_seq, *phloc = NULL, *stloc = NULL;
  int j,n,p;

  /* initialize result storage buffer */
  switch(per_what) {
  case PER_WORD:
    j_printf("=== word alignment begin ===\n");
    end_num = wnum;
    phloc = (int *)mymalloc(sizeof(int)*wnum);
    i = 0;
    for(w=0;w<wnum;w++) {
      phloc[w] = i;
      i += winfo->wlen[words[w]];
    }
    break;
  case PER_PHONEME:
    j_printf("=== phoneme alignment begin ===\n");
    end_num = 0;
    for(w=0;w<wnum;w++) end_num += winfo->wlen[words[w]];
    break;
  case PER_STATE:
    j_printf("=== state alignment begin ===\n");
    end_num = 0;
    for(w=0;w<wnum;w++) {
      for (i=0;i<winfo->wlen[words[w]]; i++) {
	end_num += hmm_logical_state_num(winfo->wseq[words[w]][i]) - 2;
      }
#ifdef MULTIPATH_VERSION
      if (enable_iwsp) {
	end_num += hmm_logical_state_num(hmminfo->sp) - 2;
      }
#endif
    }
    phloc = (int *)mymalloc(sizeof(int)*end_num);
    stloc = (int *)mymalloc(sizeof(int)*end_num);
    {
      n = 0;
      p = 0;
      for(w=0;w<wnum;w++) {
	for(i=0;i<winfo->wlen[words[w]]; i++) {
	  for(j=0; j<hmm_logical_state_num(winfo->wseq[words[w]][i]) - 2; j++) {
	    phloc[n] = p;
	    stloc[n] = j + 1;
	    n++;
	  }
#ifdef MULTIPATH_VERSION
	  if (enable_iwsp && i == winfo->wlen[words[w]] - 1) {
	    for(k=0;k<hmm_logical_state_num(hmminfo->sp)-2;k++) {
	      phloc[n] = p;
	      stloc[n] = j + 1 + k + end_num;
	      n++;
	    }
	  }
#endif 
	  p++;
	}
      }
    }
    
    break;
  }
  end_state = (int *)mymalloc(sizeof(int) * end_num);

  /* make phoneme sequence word sequence */
  phones = make_phseq(words, wnum
#ifdef MULTIPATH_VERSION
		      , &has_sp
#endif
		      , &phonenum, &end_state, per_what);
  /* build the sentence HMMs */
  shmm = new_make_word_hmm(hmminfo, phones, phonenum
#ifdef MULTIPATH_VERSION
			   , has_sp
#endif
			   );

  /* call viterbi segmentation function */
  allscore = viterbi_segment(shmm, param, end_state, end_num, &id_seq, &end_frame, &end_score, &rlen);

  /* print result */
  {
    int i,p,n;
    j_printf("id: from  to    n_score    applied HMMs (logical[physical] or {pseudo})\n");
    j_printf("------------------------------------------------------------\n");
    for (i=0;i<rlen;i++) {
      j_printf("%2d: %4d %4d  %f ", id_seq[i], (i == 0) ? 0 : end_frame[i-1]+1, end_frame[i], end_score[i]);
      switch(per_what) {
      case PER_WORD:
	for(p=0;p<winfo->wlen[words[id_seq[i]]];p++) {
	  n = phloc[id_seq[i]] + p;
	  if (phones[n]->is_pseudo) {
	    j_printf(" %s{%s}", phones[n]->name, phones[n]->body.pseudo->name);
	  } else if (strmatch(phones[n]->name, phones[n]->body.defined->name)) {
	    j_printf(" %s", phones[n]->name);
	  } else {
	    j_printf(" %s[%s]", phones[n]->name, phones[n]->body.defined->name);
	  }
	}
	break;
      case PER_PHONEME:
	n = id_seq[i];
	if (phones[n]->is_pseudo) {
	  j_printf(" {%s}", phones[n]->name);
	} else if (strmatch(phones[n]->name, phones[n]->body.defined->name)) {
	  j_printf(" %s", phones[n]->name);
	} else {
	  j_printf(" %s[%s]", phones[n]->name, phones[n]->body.defined->name);
	}
	break;
      case PER_STATE:
	n = phloc[id_seq[i]];
	if (phones[n]->is_pseudo) {
	  j_printf(" {%s}", phones[n]->name);
	} else if (strmatch(phones[n]->name, phones[n]->body.defined->name)) {
	  j_printf(" %s", phones[n]->name);
	} else {
	  j_printf(" %s[%s]", phones[n]->name, phones[n]->body.defined->name);
	}
#ifdef MULTIPATH_VERSION
	if (enable_iwsp && stloc[id_seq[i]] > end_num) {
	  j_printf(" #%d (sp)", stloc[id_seq[i]] - end_num);
	} else {
	  j_printf(" #%d", stloc[id_seq[i]]);
	}
#else
	j_printf(" #%d", stloc[id_seq[i]]);
#endif
	break;
      }
      j_printf("\n");
    }
  }
  j_printf("re-computed AM score: %f\n", allscore);

  free_hmm(shmm);
  free(id_seq);
  free(phones);
#ifdef MULTIPATH_VERSION
  free(has_sp);
#endif
  free(end_score);
  free(end_frame);
  free(end_state);

  switch(per_what) {
  case PER_WORD:
    free(phloc);
    j_printf("=== word alignment end ===\n");
    break;
  case PER_PHONEME:
    j_printf("=== phoneme alignment end ===\n");
    break;
  case PER_STATE:
    free(phloc);
    free(stloc);
    j_printf("=== state alignment end ===\n");
  }
  
}

/* entry functions */
/** 
 * <JA>
 * 単語ごとの forced alignment を行う．
 * 
 * @param words [in] 単語列
 * @param wnum [in] @a words の単語数
 * @param param [in] 入力特徴ベクトル列
 * </JA>
 * <EN>
 * Do forced alignment per word for the given word sequence.
 * 
 * @param words [in] word sequence
 * @param wnum [in] length of @a words
 * @param param [in] input parameter vectors
 * </EN>
 */
void
word_align(WORD_ID *words, short wnum, HTK_Param *param)
{
  do_align(words, wnum, param, PER_WORD);
}

/** 
 * <JA>
 * 単語ごとの forced alignment を行う（単語が逆順で与えられる場合）
 * 
 * @param revwords [in] 単語列（逆順）
 * @param wnum [in] @a revwords の単語数
 * @param param [in] 入力特徴ベクトル列
 * </JA>
 * <EN>
 * Do forced alignment per word for the given word sequence (reversed order).
 * 
 * @param revwords [in] word sequence in reversed direction
 * @param wnum [in] length of @a revwords
 * @param param [in] input parameter vectors
 * </EN>
 */
void
word_rev_align(WORD_ID *revwords, short wnum, HTK_Param *param)
{
  WORD_ID *words;		/* word sequence (true order) */
  int w;
  words = (WORD_ID *)mymalloc(sizeof(WORD_ID) * wnum);
  for (w=0;w<wnum;w++) words[w] = revwords[wnum-w-1];
  do_align(words, wnum, param, PER_WORD);
  free(words);
}

/** 
 * <JA>
 * 音素ごとの forced alignment を行う．
 * 
 * @param words [in] 単語列
 * @param num [in] @a words の単語数
 * @param param [in] 入力特徴ベクトル列
 * </JA>
 * <EN>
 * Do forced alignment per phoneme for the given word sequence.
 * 
 * @param words [in] word sequence
 * @param num [in] length of @a words
 * @param param [in] input parameter vectors
 * </EN>
 */
void
phoneme_align(WORD_ID *words, short num, HTK_Param *param)
{
  do_align(words, num, param, PER_PHONEME);
}

/** 
 * <JA>
 * 音素ごとの forced alignment を行う（単語が逆順で与えられる場合）
 * 
 * @param revwords [in] 単語列（逆順）
 * @param num [in] @a revwords の単語数
 * @param param [in] 入力特徴ベクトル列
 * </JA>
 * <EN>
 * Do forced alignment per phoneme for the given word sequence (reversed order).
 * 
 * @param revwords [in] word sequence in reversed direction
 * @param num [in] length of @a revwords
 * @param param [in] input parameter vectors
 * </EN>
 */
void
phoneme_rev_align(WORD_ID *revwords, short num, HTK_Param *param)
{
  WORD_ID *words;		/* word sequence (true order) */
  int p;
  words = (WORD_ID *)mymalloc(sizeof(WORD_ID) * num);
  for (p=0;p<num;p++) words[p] = revwords[num-p-1];
  do_align(words, num, param, PER_PHONEME);
  free(words);
}

/** 
 * <JA>
 * HMM状態ごとの forced alignment を行う．
 * 
 * @param words [in] 単語列
 * @param num [in] @a words の単語数
 * @param param [in] 入力特徴ベクトル列
 * </JA>
 * <EN>
 * Do forced alignment per HMM state for the given word sequence.
 * 
 * @param words [in] word sequence
 * @param num [in] length of @a words
 * @param param [in] input parameter vectors
 * </EN>
 */
void
state_align(WORD_ID *words, short num, HTK_Param *param)
{
  do_align(words, num, param, PER_STATE);
}

/** 
 * <JA>
 * HMM状態ごとの forced alignment を行う（単語が逆順で与えられる場合）
 * 
 * @param revwords [in] 単語列（逆順）
 * @param num [in] @a revwords の単語数
 * @param param [in] 入力特徴ベクトル列
 * </JA>
 * <EN>
 * Do forced alignment per state for the given word sequence (reversed order).
 * 
 * @param revwords [in] word sequence in reversed direction
 * @param num [in] length of @a revwords
 * @param param [in] input parameter vectors
 * </EN>
 */
void
state_rev_align(WORD_ID *revwords, short num, HTK_Param *param)
{
  WORD_ID *words;		/* word sequence (true order) */
  int p;
  words = (WORD_ID *)mymalloc(sizeof(WORD_ID) * num);
  for (p=0;p<num;p++) words[p] = revwords[num-p-1];
  do_align(words, num, param, PER_STATE);
  free(words);
}
