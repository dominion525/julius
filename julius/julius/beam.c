/**
 * @file   beam.c
 * @author Akinobu LEE
 * @date   Tue Feb 22 17:00:45 2005
 * 
 * <JA>
 * @brief  第１パス：フレーム同期ビーム探索
 *
 * 静的木構造辞書を用いて，入力特徴量ベクトル列に対して，Juliusの第１パス
 * であるフレーム同期ビーム探索を行います．
 *
 * 入力データ全体があらかじめ得られている場合は，一括で計算を
 * 行う関数 get_back_trellis() がメインから呼ばれます．オンライン認識
 * の場合は realtime_1stpass.c から get_back_trellis_init(),
 * get_back_trellis_proceed(), get_back_trellis_end() などがそれぞれ
 * 入力の進行状況にあわせて個別に呼ばれます．
 *
 * 単語履歴近似は 1-best 近似がデフォルトですが，単語対近似も使用可能です．
 *
 * Julius では単語間の接続制約は 1-gram factoring (2-gram factoring も
 * 選択可)を用いて計算されます．Julianの場合，木構造化辞書は文法の
 * カテゴリ単位で作成され，単語間の接続(単語対制約)は単語間遷移で
 * 適用されます．
 * </JA>
 * 
 * <EN>
 * @brief  The first pass: frame-synchronous beam search
 *
 * These functions perform a frame-synchronous beam search using a static
 * lexicon tree, as the first pass of Julius/Julian.
 *
 * When the whole input is already obtained, get_back_trellis() simply does
 * all the processing of the 1st pass.  When performing online
 * real-time recognition with concurrent speech input, get_bcak_trellis_init(),
 * get_back_trellis_proceed(), get_back_trellis_end() will be called
 * separately from realtime_1stpass.c according on the basis of
 * input processing.
 *
 * 1-best approximation will be performed for word context approximation,
 * but normal word-pair approximation is also supported.
 *
 * With word/class N-gram, Julius computes the language score using 1-gram
 * factoring (can be changed to 2-gram factoring if you want).  With
 * DFA grammar, Julian can compute the connection constraint of words
 * using the category-pair constraint on the beginning of the words, since
 * Julian makes a per-category tree lexicon.
 * </EN>
 * 
 * $Revision: 1.10 $
 * 
 */
/*
 * Copyright (c) 1991-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <julius.h>

#undef DEBUG

static boolean idc_on;		///< set to TRUE when activating tty progress indicator 
static int progout_interval_frame; ///< Local work area to hold the output interval in frames for progressive output


/* -------------------------------------------------------------------- */
/*                     第１パスの結果出力と終了処理                     */
/*              result output and end procedure of 1st pass             */
/* -------------------------------------------------------------------- */

#ifdef WORD_GRAPH
/* 第１パス結果から単語グラフを生成 */
/* generate word graphs from result of 1st pass (=backtrellis) */
static int glevel;		///< Current call depth of generate_lattice()
static int gnodes;		///< Number of generated nodes in generate_lattice()
static int garcs;		///< Number of generated arcs in generate_lattice()

/** 
 * <JA>
 * @brief  認識結果の単語トレリスから単語グラフを抽出する
 *
 * (WORD_GRAPH 指定時)
 * この関数は第１パスの結果の単語トレリスを終端からバックトレースし，
 * パス上にあるトレリス単語を単語グラフとして抽出する．実際には，
 * 単語トレリス上でグラフ上に残るもののみにマークを付け，
 * 第2パスでは，マークのついた単語のみを展開する．
 * 
 * @param endtime [in] 単語トレリス上で単語末端を検索するフレーム
 * @param bt [i/o] 単語トレリス構造体(グラフ上の単語がマークされる)
 * @param winfo [in] 単語辞書
 * </JA>
 * <EN>
 * @brief  Extract word graph from the resulting word trellis
 *
 * If WORD_GRAPH is defined, this function trace back through the
 * word trellis from the end point, to extract the trellis words on
 * the path as a word graph.  Actually, this function only marks
 * which trellis words are included in the word graph.  On the 2nd pass,
 * only the words in the word graph will be expanded.
 * 
 * @param endtime [in] frame to lookup for word ends in the word trellis
 * @param bt [i/o] word trellis structure, the words on the graph will be marked
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
generate_lattice(int frame, BACKTRELLIS *bt, WORD_INFO *winfo)
{
  TRELLIS_ATOM *ta;
  int i, j;
  boolean new_node = FALSE;
  LOGPROB l;

  glevel++;

  if (frame >= 0) {
    for (i=0;i<bt->num[frame];i++) {
      ta = bt->rw[frame][i];
      /* words will be saved as a part of graph only if any of its
	 following word has been survived in a beam */
      if (! ta->within_context) continue; /* not a candidate */
      if (ta->within_wordgraph) continue; /* already marked */
      /* mark  */
      ta->within_wordgraph = TRUE;
      new_node = TRUE;
      if (debug2_flag) {
	for(j=0;j<glevel;j++) j_printf(" ");
	j_printf("%s: [%d..%d]\n", winfo->wname[ta->wid], ta->begintime, ta->endtime);
      }
      if (verbose_flag) {
	j_printf("%d: [%d..%d] wid=%d name=\"%s\" lname=\"%s\" score=%f", garcs, ta->begintime, ta->endtime, ta->wid, winfo->woutput[ta->wid], winfo->wname[ta->wid], ta->backscore);
      }
#ifdef USE_NGRAM
      j_printf(" lscore=%f", ta->lscore);
#endif
      l = ta->backscore;
      if (ta->last_tre->wid != WORD_INVALID) {
	l -= ta->last_tre->backscore;
      }
#ifdef USE_NGRAM
      l -= ta->lscore;
#endif
      j_printf(" AMavg=%f\n", l / (ta->endtime - ta->begintime + 1));

      garcs++;
      /* recursive call */
      generate_lattice(ta->last_tre->endtime, bt, winfo);
    }
  }
  if (new_node) {
    gnodes++;
  }
  glevel--;
}
#endif

/** 
 * <JA>
 * あるトレリス単語の情報をテキストで出力 (デバッグ用)
 * 
 * @param atom [in] 出力するトレリス単語
 * @param winfo [in] 単語辞書
 * </JA>
 * <EN>
 * Output a trellis word information in text (for debug)
 * 
 * @param atom [in] trellis word to output
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
put_atom(TRELLIS_ATOM *atom, WORD_INFO *winfo)
{
  int i;
  j_printf("%3d,%3d %f %16s (id=%5d)", atom->begintime, atom->endtime,
	 atom->backscore, winfo->wname[atom->wid], atom->wid);
  for (i=0;i<winfo->wlen[atom->wid]; i++) {
    j_printf(" %s",winfo->wseq[atom->wid][i]->name);
  }
  j_printf("\n");
}

/** 
 * <JA>
 * @brief 認識結果の単語トレリス上の最尤単語系列を求める
 * 
 * 与えられたトレリス単語から入力始端に向かって単語トレリス上を
 * トレースバックし, その最尤単語系列候補およびその言語スコアを返す．
 * 起点となる最初のトレリス単語が与えられる必要がある．
 * 
 * @param wordseq_rt [out] 結果の最尤単語系列が格納されるバッファ
 * @param rt_wordlen [out] @a wordseq_rt の長さ
 * @param atom [in] バックトレースの起点となるトレリス単語
 * @param backtrellis [in] 単語トレリス構造体
 * @param winfo [in] 単語辞書
 * 
 * @return 得られた最尤単語系列の言語スコア.
 * </JA>
 * <EN>
 * @brief Find the best word sequence in the word trellis
 *
 * This function trace back through the word trellis to the beginning
 * of input, to find the best word sequence.  The traceback starting point
 * should be specified as a trellis word.
 * 
 * @param wordseq_rt [out] buffer to store the best word sequence as result
 * @param rt_wordlen [out] length of @a wordseq_rt
 * @param atom [in] a trellis word as the starting point of the traceback
 * @param backtrellis [in] word trellis structure
 * @param winfo [in] word dictionary
 * 
 * @return the total N-gram language score of the word sequence.
 * </EN>
 */
static LOGPROB
trace_backptr(WORD_ID wordseq_rt[MAXSEQNUM], int *rt_wordlen, TRELLIS_ATOM *atom, BACKTRELLIS *backtrellis, WORD_INFO *winfo)
{
  int wordlen = 0;		/* word length of best sentence hypothesis */
  TRELLIS_ATOM *tretmp;
  LOGPROB langscore = 0.0;
  static WORD_ID wordseq[MAXSEQNUM];	/* temporal: in reverse order */
  int i;
  
  /* initialize */
  wordseq[0] = atom->wid;	/* start from specified atom */
  wordlen = 1;
  tretmp = atom;
#ifdef USE_NGRAM
  langscore += tretmp->lscore;
#endif
  if (debug2_flag) {
    put_atom(tretmp, winfo);
  }
  
  /* trace the backtrellis */
  while (tretmp->begintime > 0) {/* until beginning of input */
    tretmp = tretmp->last_tre;
/*    t = tretmp->boundtime - 1;
    tretmp = bt_binsearch_atom(backtrellis, tretmp->boundtime-1, tretmp->last_wid);*/
    if (tretmp == NULL) {	/* should not happen */
      j_error("ERROR: BackTrellis Pass missing??\n");
    }
#ifdef USE_NGRAM
    langscore += tretmp->lscore;
#endif
    wordseq[wordlen] = tretmp->wid;
    wordlen++;
    if (debug2_flag) {
      put_atom(tretmp, winfo);
    }
    if (wordlen >= MAXSEQNUM) {
      j_error("sentence length exceeded ( > %d)\n",MAXSEQNUM);
    }
  }
  *rt_wordlen = wordlen;
  /* reverse order -> normal order */
  for(i=0;i<wordlen;i++) wordseq_rt[i] = wordseq[wordlen-i-1];
  return(langscore);
}

/** 
 * <JA>
 * @brief  第１パスの認識処理結果を出力する
 *
 * 第１パスの計算結果である単語トレリスから，第１パスでの最尤候補を求めて
 * 出力する．内部では，最終フレームに残った中から起点となるトレリス単語
 * を求め，trace_backptr() を呼び出して第1パスの尤仮説系列を求めて
 * その結果を出力する．
 *
 * この第１パスの最尤仮説列は
 * 大域変数 pass1_wseq, pass1_wnum, pass1_scoreにも保存される．
 * これらは第２パスで探索が失敗したときに第１パスの結果を最終結果として
 * 出力する際に参照される．
 *
 * またWORD_GRAPH 定義時は，この関数内でさらに generate_lattice() を呼び出し
 * 単語グラフの抽出を行う．
 * 
 * 
 * @param backtrellis [i/o] 単語トレリス構造体
 * @param framelen [in] 第１パスで処理が到達したフレーム数
 * @param winfo [in] 単語辞書
 * 
 * @return 第１パスの最尤仮説系列のスコア，あるいは第１パスで有効な仮説系列が
 * 見つからなければ NULL.
 * </JA>
 * <EN>
 * @brief  Output the result of the first pass
 *
 * This function output the best word sequence on the 1st pass.  The last
 * trellis word will be determined by linguistic property or scores from
 * words on the last frame, and trace_backptr() will be called to find
 * the best path from the word.  The resulting word sequence will be output
 * as a result of the 1st pass.
 *
 * The informations of the resulting best word sequence will also be stored
 * to global variables such as pass1_wseq, pass1_wnum, pass1_score.  They will
 * be referred on the 2nd pass as a fallback result when the 2nd pass failed
 * with no sentence hypothesis found.
 *
 * Also, if WORD_GRAPH is defined, this function also calls generate_lattice() to
 * extract word graph in the word trellis.
 * 
 * @param backtrellis [i/o] word trellis structure
 * @param framelen [in] frame length that has been processed
 * @param winfo [in] word dictionary
 * 
 * @return the best score of the resulting word sequence on the 1st pass.
 * </EN>
 */
static LOGPROB
print_1pass_result(BACKTRELLIS *backtrellis, int framelen, WORD_INFO *winfo)
{
  WORD_ID wordseq[MAXSEQNUM];
  int wordlen;
  int i;
  TRELLIS_ATOM *best;
  int last_time;
  LOGPROB total_lscore;
  LOGPROB maxscore;
  TRELLIS_ATOM *tmp;

  /* look for the last trellis word */
  for (last_time = framelen - 1; last_time >= 0; last_time--) {
#ifdef USE_NGRAM
#ifdef SP_BREAK_CURRENT_FRAME	/*  in case of sp segmentation */
    /* 最終フレームに残った最大スコアの単語 */
    /* it should be the best trellis word on the last frame */
    maxscore = LOG_ZERO;
    for (i=0;i<backtrellis->num[last_time];i++) {
      tmp = backtrellis->rw[last_time][i];
#ifdef WORD_GRAPH
      /* treat only words on a graph path */
      if (!tmp->within_context) continue;
#endif
      if (maxscore < tmp->backscore) {
	maxscore = tmp->backscore;
	best = tmp;
      }
    }
    if (maxscore != LOG_ZERO) break;
#else  /* normal mode */
    /* 最終単語は winfo->tail_silwid に固定 */
    /* it is fixed to the tail silence model (winfo->tail_silwid) */
    maxscore = LOG_ZERO;
    for (i=0;i<backtrellis->num[last_time];i++) {
      tmp = backtrellis->rw[last_time][i];
#ifdef WORD_GRAPH
      /* treat only words on a graph path */
      if (!tmp->within_context) continue;
#endif
      if (tmp->wid == winfo->tail_silwid && maxscore < tmp->backscore) {
	maxscore = tmp->backscore;
	best = tmp;
	break;
      }
    }
    if (maxscore != LOG_ZERO) break;
#endif
#else  /* USE_DFA */
    /* 末尾に残った単語の中で最大スコアの単語(cp_endは使用しない) */
    /* the best trellis word on the last frame (not use cp_end[]) */
    maxscore = LOG_ZERO;
    for (i=0;i<backtrellis->num[last_time];i++) {
      tmp = backtrellis->rw[last_time][i];
#ifdef WORD_GRAPH
      /* treat only words on a graph path */
      if (!tmp->within_context) continue;
#endif
      /*      if (dfa->cp_end[winfo->wton[tmp->wid]] == TRUE) {*/
	if (maxscore < tmp->backscore) {
	  maxscore = tmp->backscore;
	  best = tmp;
	}
	/*      }*/
    }
    if (maxscore != LOG_ZERO) break;
#endif /* USE_NGRAM */
  }
  if (last_time < 0) {		/* not found */
    j_printerr("[no sentence-end word survived on last beam]\n");
    /* print null result */
    result_pass1_final(NULL, 0, LOG_ZERO, LOG_ZERO, winfo);
    return(LOG_ZERO);
  }
  
  /* traceback word trellis from the best word */
  total_lscore = trace_backptr(wordseq, &wordlen, best, backtrellis, winfo);

  if (progout_flag) {		/* just flush last progress output */
    result_pass1_current(last_time, wordseq, wordlen, best->backscore, total_lscore, winfo);
  }

  /* output 1st pass result */    
  if (verbose_flag || !progout_flag) {
    result_pass1_final(wordseq, wordlen, best->backscore, total_lscore, winfo);
  }
  result_pass1_end();
  
  /* store the result to global val (notice: in reverse order) */
  for(i=0;i<wordlen;i++) pass1_wseq[i] = wordseq[i];
  pass1_wnum = wordlen;
  pass1_score = best->backscore;

#ifdef WORD_GRAPH
  /* 単語トレリスから，ラティスを生成する */
  /* 実際には within_wordgraph を on にするだけ */
  /* generate word graph from the word trellis */
  /* actually generate_lattice() does not construct graph:
     it only marks words in trellis that are on the word graph */
  glevel = 0;
  gnodes = 0;			/* frame = 0 の分 */
  garcs = 0;
  j_printf("--- begin wordgraph data pass1 ---\n");
  generate_lattice(last_time, backtrellis, winfo);
  if (verbose_flag) j_printf("word graph generated (nodes=%d,arcs=%d)\n",gnodes, garcs);
  j_printf("--- end wordgraph data pass1 ---\n");
#endif

  /* return maximum score */
  return(best->backscore);
}

/** 
 * <JA>
 * 第１パスの処理中に，あるフレームまでのベストパスを表示する．
 * 
 * @param bt [in] 単語トレリス構造体
 * @param t [in] フレーム
 * @param winfo [in] 単語辞書
 * </JA>
 * <EN>
 * Output the current best word sequence ending
 * at a specified time frame in the course of the 1st pass.
 * 
 * @param bt [in] word trellis structure
 * @param t [in] frame
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
bt_current_max(BACKTRELLIS *bt, int t, WORD_INFO *winfo)
{
  static WORD_ID wordseq[MAXSEQNUM];
  int wordlen;
  TRELLIS_ATOM *tre;
  TRELLIS_ATOM *tremax;
  LOGPROB maxscore;
  LOGPROB lscore;

  /* bt->list is ordered by time frame */
  maxscore = LOG_ZERO;
  tremax = NULL;
  tre = bt->list;
  while (tre != NULL && tre->endtime == t) {
    if (maxscore < tre->backscore) {
      maxscore = tre->backscore;
      tremax = tre;
    }
    tre = tre->next;
  }

  if (maxscore != LOG_ZERO) {
    lscore = trace_backptr(wordseq, &wordlen, tremax, bt, winfo);
    result_pass1_current(t, wordseq, wordlen, tremax->backscore, lscore, winfo);
  } else {
    wordlen = 0;
    result_pass1_current(t, wordseq, wordlen, LOG_ZERO, LOG_ZERO, winfo);
  }
}

/** 
 * <JA>
 * 第１パスの処理中に，あるフレーム上の最尤単語を表示する(デバッグ用)
 * 
 * @param bt [in] 単語トレリス構造体
 * @param t [in] フレーム
 * @param winfo [in] 単語辞書
 * </JA>
 * <EN>
 * Output the current best word on a specified time frame in the course
 * of the 1st pass.
 * 
 * @param bt [in] word trellis structure
 * @param t [in] frame
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
bt_current_max_word(BACKTRELLIS *bt, int t, WORD_INFO *winfo)
{
  TRELLIS_ATOM *tre;
  TRELLIS_ATOM *tremax;
  LOGPROB maxscore;
  WORD_ID w;

  /* bt->list は時間順に格納されている */
  /* bt->list is order by time */
  maxscore = LOG_ZERO;
  tremax = NULL;
  tre = bt->list;
  while (tre != NULL && tre->endtime == t) {
    if (maxscore < tre->backscore) {
      maxscore = tre->backscore;
      tremax = tre;
    }
    tre = tre->next;
  }

  if (maxscore != LOG_ZERO) {
    j_printf("%3d: ",t);
    w = tremax->wid;
    j_printf("\"%s [%s]\"(id=%d)",
	   winfo->wname[w], winfo->woutput[w], w);
    j_printf(" [%d-%d] %f <- ", tremax->begintime, t, tremax->backscore);
    w = tremax->last_tre->wid;
    if (w != WORD_INVALID) {
      j_printf("\"%s [%s]\"(id=%d)\n",
	     winfo->wname[w], winfo->woutput[w], w);
    } else {
      j_printf("bgn\n");
    }
  }
}

/* -------------------------------------------------------------------- */
/*                 ビーム探索中のトークンを扱うサブ関数                 */
/*                functions to handle hypothesis tokens                  */
/* -------------------------------------------------------------------- */

/* Token structure: */
   
/* How tokens are managed:
   o  tlist[][] is a token stocker.  It holds all tokens in sequencial
      buffer.  They are malloced first on startup, and refered by ID while
      Viterbi procedure.  In word-pair mode, each token also has a link to
      another token to allow a node to have more than 1 token.
      
   o  token[n] holds the current ID number of a token associated to a
      lexicon tree node 'n'.

 */

/* Token space */
static TOKEN2 *tlist[2];	///< Token space to hold all token entities.
static TOKENID *tindex[2];	///< Token index corresponding to @a tlist for sort
static int maxtnum = 0;		///< Allocated number of tokens (will grow)
static int expand_step = 0;	///< Number of tokens to be increased per expansion
static int tnum[2];		///< Current number of tokens used in @a tlist
static int n_start;		///< Start index of in-beam nodes on @a tindex
static int n_end;		///< end index of in-beam nodes on @a tindex
static int tl;		///< Current work area id (0 or 1, swapped for each frame)
static int tn;		///< Next work area id (0 or 1, swapped for each frame)

/* Active token list */
static TOKENID *token;		///< Active token list that holds currently assigned tokens for each tree node
static int totalnodenum;	///< Allocated number of nodes in @a token


static TRELLIS_ATOM bos;	///< Special token for beginning-of-sentence
static boolean nodes_malloced = FALSE; ///< Flag to check if tokens already allocated

/** 
 * <JA>
 * 第１パスのビーム探索用の初期ワークエリアを確保する．
 * 足りない場合は探索中に動的に伸長される．
 * 
 * @param n [in] 木構造化辞書のノード数
 * @param ntoken_init [in] 最初に確保するトークンの数
 * @param ntoken_step [in] トークン不足で伸長する場合の1回あたりの伸長数
 * </JA>
 * <EN>
 * Allocate initial work area for beam search on the 1st pass.
 * If filled while search, they will be expanded on demand.
 * 
 * @param n [in] number of nodes in the tree lexicon
 * @param ntoken_init [in] number of token space to be allocated at first
 * @param ntoken_step [in] number of token space to be increased for each expansion
 * </EN>
 */
static void
malloc_nodes(int n, int ntoken_init, int ntoken_step)
{
  totalnodenum = n;
  token        = mymalloc(sizeof(TOKENID)*totalnodenum);
  if (maxtnum < ntoken_init) maxtnum = ntoken_init;
  tlist[0]     = mymalloc(sizeof(TOKEN2)*maxtnum);
  tlist[1]     = mymalloc(sizeof(TOKEN2)*maxtnum);
  tindex[0]     = mymalloc(sizeof(TOKENID)*maxtnum);
  tindex[1]     = mymalloc(sizeof(TOKENID)*maxtnum);
  tnum[0] = tnum[1] = 0;
  if (expand_step < ntoken_step) expand_step = ntoken_step;
  nodes_malloced = TRUE;
}

/** 
 * <JA>
 * 第１パスのビーム探索用のワークエリアを伸ばして再確保する．
 * </JA>
 * <EN>
 * Re-allocate work area for beam search on the 1st pass.
 * </EN>
 */
static void
expand_tlist()
{
  maxtnum += expand_step;
  tlist[0]     = myrealloc(tlist[0],sizeof(TOKEN2)*maxtnum);
  tlist[1]     = myrealloc(tlist[1],sizeof(TOKEN2)*maxtnum);
  tindex[0]     = myrealloc(tindex[0],sizeof(TOKENID)*maxtnum);
  tindex[1]     = myrealloc(tindex[1],sizeof(TOKENID)*maxtnum);
  /*j_printerr("warn: token space expanded\n");*/
}

/** 
 * <JA>
 * 第１パスのビーム探索用のワークエリアを全て解放する．
 * </JA>
 * <EN>
 * Free all the work area for beam search on the 1st pass.
 * </EN>
 */
static void
free_nodes()
{
  if (nodes_malloced) {
    free(token);
    free(tlist[0]);
    free(tlist[1]);
    free(tindex[0]);
    free(tindex[1]);
    nodes_malloced = FALSE;
  }
}

/** 
 * <JA>
 * トークンスペースをリセットする．
 * 
 * @param tt [in] ワークエリアID (0 または 1)
 * </JA>
 * <EN>
 * Reset the token space.
 * 
 * @param tt [in] work area id (0 or 1)
 * </EN>
 */
static void
clear_tlist(int tt)
{
  tnum[tt] = 0;
}

/** 
 * <JA>
 * アクティブトークンリストをクリアする．
 * 
 * @param tt [in] 直前のワークエリアID (0 または 1)
 * </JA>
 * <EN>
 * Clear the active token list.
 * 
 * @param tt [in] work area id of previous frame (0 or 1)
 * </EN>
 */
static void
clear_tokens(int tt)
{
  int j;
  /* initialize active token list: only clear ones used in the last call */
  for (j=0; j<tnum[tt]; j++) {
    token[tlist[tt][j].node] = TOKENID_UNDEFINED;
  }
}

/** 
 * <JA>
 * トークンスペースから新たなトークンを取りだす．
 * 
 * @return 新たに取り出されたトークンのID
 * </JA>
 * <EN>
 * Assign a new token from token space.
 * 
 * @return the id of the newly assigned token.
 * </EN>
 */
static TOKENID
create_token()
{
  TOKENID newid;
  newid = tnum[tn];
  tnum[tn]++;
  if (tnum[tn]>=maxtnum) expand_tlist();
  tindex[tn][newid] = newid;
#ifdef WPAIR
  /* initialize link */
  tlist[tn][newid].next = TOKENID_UNDEFINED;
#endif
  return(newid);
}

/** 
 * <JA>
 * @brief  木構造化辞書のノードにトークンを割り付ける．
 *
 * 木構造化辞書のノードのアクティブトークンリストにトークンを保存する．
 * またトークンスペースにおいてトークンからノード番号へのリンクを保存する．
 * 
 * 既にトークンがある場合は，新たなトークンによって上書きされる．なお
 * WPAIR 指定時はそのリストに新たなトークンを追加する．
 * 
 * @param node [in] 木構造化辞書のノード番号
 * @param tkid [in] トークン番号
 * </JA>
 * <EN>
 * @brief  Assign token to a node on tree lexicon
 *
 * Save the token id to the specified node in the active token list.
 * Also saves the link to the node from the token in token space.
 *
 * If a token already exist on the node, it will be overridden by the new one.
 * If WPAIR is defined, the new token will be simply added to the list of
 * active tokens on the node.
 * 
 * @param node [in] node id on the tree lexicon
 * @param tkid [in] token id to be assigned
 * </EN>
 */
static void
node_assign_token(int node, TOKENID tkid)
{
#ifdef WPAIR
  /* add to link list */
  tlist[tn][tkid].next = token[node];
#endif
  token[node] = tkid;
  tlist[tn][tkid].node = node;
}

/** 
 * <JA>
 * @brief  木構造化辞書上のあるノードが，現在なんらかのトークンを
 * 保持しているかをチェックする．
 *
 * WPAIR が定義されている場合，ノードは直前単語ごとに異なるトークンを複数
 * 保持する．この場合, 指定された単語IDを直前単語とするトークンが
 * そのノードに保持されているかどうかがチェックされる．すなわち，既にトークン
 * が存在しても，そのトークンの表すパスの直前単語が指定した単語と異なって
 * いれば未保持 (TOKENID_UNDEFINED) を返す．
 * 
 * @param tt [in] 直前のワークエリアID (0 または 1)
 * @param node [in] ノード番号
 * @param wid [in] 直前単語のID (WPAIR定義時のみ有効, 他では無視される)
 *
 * @return そのノードが既に保持するトークン番号，無ければ TOKENID_UNDEFINED．
 * </JA>
 * <EN>
 * @brief  Check if a node holds any token
 *
 * This function checks if a node on the tree lexicon already holds a token.
 *
 * If WPAIR is defined, a node has multiple tokens according to the previous
 * word context.  In this case, only token with the same previous word will be
 * checked.
 * 
 * @param tt [in] work area id (0 or 1)
 * @param node [in] node id of lexicon tree
 * @param wid [in] word id of previous word (ignored if WPAIR is not defined)
 * 
 * @return the token id on the node, or TOKENID_UNDEFINED if no token has
 * been assigned in this frame.
 * </EN>
 */
static TOKENID
node_exist_token(int tt, int node, WORD_ID wid)
{
#ifdef WPAIR
  /* In word-pair mode, multiple tokens are assigned to a node as a list.
     so we have to search for tokens with same last word ID */
#ifdef WPAIR_KEEP_NLIMIT
  /* 1ノードごとに保持するtoken数の上限を設定 */
  /* tokenが無いが上限に達しているときは一番スコアの低いtokenを上書きする */
  /* N-best: limit number of assigned tokens to a node */
  int i = 0;
  TOKENID lowest_token = TOKENID_UNDEFINED;
#endif
  TOKENID tmp;
  for(tmp=token[node]; tmp != TOKENID_UNDEFINED; tmp=tlist[tt][tmp].next) {
    if (tlist[tt][tmp].last_tre->wid == wid) {
      return(tmp);
    }
#ifdef WPAIR_KEEP_NLIMIT
    if (lowest_token == TOKENID_UNDEFINED ||
	tlist[tt][lowest_token].score < tlist[tt][tmp].score)
      lowest_token = tmp;
    if (++i >= wpair_keep_nlimit) break;
#endif
  }
#ifdef WPAIR_KEEP_NLIMIT
  if (i >= wpair_keep_nlimit) { /* overflow, overwrite lowest score */
    return(lowest_token);
  } else {
    return(TOKENID_UNDEFINED);
  }
#else 
  return(TOKENID_UNDEFINED);
#endif
  
#else  /* not WPAIR */
  /* 1つだけ保持,これを常に上書き */
  /* Only one token is kept in 1-best mode (default), so
     simply return the ID */
  return(token[node]);
#endif
}

#ifdef DEBUG
/* tlist と token の対応をチェックする(debug) */
/* for debug: check tlist <-> token correspondence
   where  tlist[tt][tokenID].node = nodeID and
          token[nodeID] = tokenID
 */
static void
node_check_token(int tt)
{
  int i;
  for(i=0;i<tnum[tt];i++) {
    if (node_exist_token(tt, tlist[tt][i].node, tlist[tt][i].last_tre->wid) != i) {
      j_printerr("token %d not found on node %d\n", i, tlist[tt][i].node);
    }
  }
}
#endif



/* -------------------------------------------------------------------- */
/*       トークンをソートし 上位 N トークンを判別する (heap sort)       */
/*        Sort generated tokens and get N-best (use heap sort)          */
/* -------------------------------------------------------------------- */
/* ビームの閾値として上位 N 番目のスコアが欲しいだけであり，実際にソート
   される必要はない */
/* we only want to know the N-th score for determining beam threshold, so
   order is not considered here */

#define SD(A) tindex[tn][A-1]	///< Index locater for sort_token_*()
#define SCOPY(D,S) D = S	///< Content copier for sort_token_*()
#define SVAL(A) (tlist[tn][tindex[tn][A-1]].score) ///< Score locater for sort_token_*()
#define STVAL (tlist[tn][s].score) ///< Indexed score locater for sort_token_*()

/** 
 * <JA>
 * @brief  トークンスペースをスコアの大きい順にソートする．
 *
 * heap sort を用いて現在のトークン集合をスコアの大きい順にソートする．
 * 上位 @a neednum 個のトークンがソートされればそこで処理を終了する．
 * 
 * @param neednum [in] 上位 @a neednum 個が得られるまでソートする
 * @param totalnum [in] トークンスペース内の有効なトークン数
 * </JA>
 * <EN>
 * @brief  Sort the token space upward by score.
 *
 * This function sort the whole token space in upward direction, according
 * to their accumulated score.
 * This function terminates sort as soon as the top
 * @a neednum tokens has been found.
 * 
 * @param neednum [in] sort until top @a neednum tokens has been found
 * @param totalnum [in] total number of assigned tokens in the token space
 * </EN>
 */
static void
sort_token_upward(int neednum, int totalnum)
{
  int n,root,child,parent;
  TOKENID s;
  for (root = totalnum/2; root >= 1; root--) {
    SCOPY(s, SD(root));
    parent = root;
    while ((child = parent * 2) <= totalnum) {
      if (child < totalnum && SVAL(child) < SVAL(child+1)) {
	child++;
      }
      if (STVAL >= SVAL(child)) {
	break;
      }
      SCOPY(SD(parent), SD(child));
      parent = child;
    }
    SCOPY(SD(parent), s);
  }
  n = totalnum;
  while ( n > totalnum - neednum) {
    SCOPY(s, SD(n));
    SCOPY(SD(n), SD(1));
    n--;
    parent = 1;
    while ((child = parent * 2) <= n) {
      if (child < n && SVAL(child) < SVAL(child+1)) {
	child++;
      }
      if (STVAL >= SVAL(child)) {
	break;
      }
      SCOPY(SD(parent), SD(child));
      parent = child;
    }
    SCOPY(SD(parent), s);
  }
}

/** 
 * <JA>
 * @brief  トークンスペースをスコアの小さい順にソートする．
 *
 * ビームのしきい値決定のために，heap sort を用いて
 * 現在のトークン集合をスコアの小さい順にソートする．
 * 下位 @a neednum 個のトークンがソートされればそこで処理を終了する．
 * 
 * @param neednum [in] 下位 @a neednum 個が得られるまでソートする
 * @param totalnum [in] トークンスペース内の有効なトークン数
 * </JA>
 * <EN>
 * @brief  Sort the token space downward by score.
 *
 * This function sort the whole token space in downward direction, according
 * to their accumulated score for hypothesis pruning.
 *
 * This function terminates sort as soon as the bottom
 * @a neednum tokens has been found.
 * 
 * @param neednum [in] sort until bottom @a neednum tokens has been found
 * @param totalnum [in] total number of assigned tokens in the token space
 * </EN>
 */
static void
sort_token_downward(int neednum, int totalnum)
{
  int n,root,child,parent;
  TOKENID s;
  for (root = totalnum/2; root >= 1; root--) {
    SCOPY(s, SD(root));
    parent = root;
    while ((child = parent * 2) <= totalnum) {
      if (child < totalnum && SVAL(child) > SVAL(child+1)) {
	child++;
      }
      if (STVAL <= SVAL(child)) {
	break;
      }
      SCOPY(SD(parent), SD(child));
      parent = child;
    }
    SCOPY(SD(parent), s);
  }
  n = totalnum;
  while ( n > totalnum - neednum) {
    SCOPY(s, SD(n));
    SCOPY(SD(n), SD(1));
    n--;
    parent = 1;
    while ((child = parent * 2) <= n) {
      if (child < n && SVAL(child) > SVAL(child+1)) {
	child++;
      }
      if (STVAL <= SVAL(child)) {
	break;
      }
      SCOPY(SD(parent), SD(child));
      parent = child;
    }
    SCOPY(SD(parent), s);
  }
}

/** 
 * <JA>
 * @brief トークンスペースをソートしてビーム内に残るトークンを決定する
 * 
 * heap sort を用いて現在のトークン集合をソートし，上位スコアのトークン
 * 集合を求める．上位 @a neednum 個のトークン集合が得られれば良いので，
 * 全体が完全にソートされている必要はない．よって
 * 上位 @a neednum 個のトークンのみをソートする．実際には，全体のトークン
 * 数と必要なトークン数から sort_token_upward()
 * と sort_token_downward() の早い方が用いられる．
 * 
 * @param neednum [in] 求める上位トークンの数
 * @param start [out] 上位 @a neednum のトークンが存在するトークンスペースの最初のインデックス番号
 * @param end [out] 上位 @a neednum のトークンが存在するトークンスペースの最後のインデックス番号
 * </JA>
 * <EN>
 * @brief Sort the token space to find which tokens to be survived in the beam
 *
 * This function sorts the currrent tokens in the token space to find
 * the tokens to be survived in the current frame.  Only getting the top
 * @a neednum tokens is required, so the sort will be terminate just after
 * the top @a neednum tokens are determined.  Actually, either
 * sort_token_upward() or sort_token_downward() will be used depending of
 * the number of needed tokens and total tokens.
 * 
 * @param neednum [in] number of top tokens to be found
 * @param start [out] start index of the top @a neednum nodes
 * @param end [out] end index of the top @a neednum nodes
 * </EN>
 */
static void
sort_token_no_order(int neednum, int *start, int *end)
{
  int totalnum = tnum[tn];
  int restnum;

  restnum = totalnum - neednum;

  if (neednum >= totalnum) {
    /* no need to sort */
    *start = 0;
    *end = totalnum - 1;
  } else if (neednum < restnum)  {
    /* needed num is smaller than rest, so sort for the needed tokens */
    sort_token_upward(neednum,totalnum);
    *start = totalnum - neednum;
    *end = totalnum - 1;
  } else {
    /* needed num is bigger than rest, so sort for the rest token */
    sort_token_downward(restnum,totalnum);
    *start = 0;
    *end = neednum - 1;
  }
}
    

#ifdef SP_BREAK_CURRENT_FRAME
/* -------------------------------------------------------------------- */
/*                     ショートポーズ・セグメンテーション               */
/*                     short pause segmentation                         */
/* -------------------------------------------------------------------- */
/* ====================================================================== */
/* sp segmentation */
/*
  |---************-----*********************-------*******--|
[input segments]
  |-------------------|
                  |-------------------------------|
		                            |---------------|

		     
  |-------------------|t-2
                       |t-1 ... token processed (= lastlen)
		        |t  ... outprob computed
		       
*/

static boolean in_sparea;	/* TRUE if we are within a pause area */
static int sparea_start;	/* start frame of pause area */
static int tmp_sparea_start;
#ifdef SP_BREAK_RESUME_WORD_BEGIN
static WORD_ID tmp_sp_break_last_word;
#else
static WORD_ID last_tre_word;
#endif
static boolean first_sparea;	/* TRUE if this is the first pause segment in a input stream */
static int sp_duration;		/* number of current successive sp frame */

/** 
 * <JA>
 * @brief  与えられた単語がショートポーズ単語であるかどうか調べる
 *
 * ショートポーズ単語は，無音部(ショートポーズ)の%HMM１つのみを読みとする
 * 単語である．
 * 
 * @param w [in] 単語ID
 * @param winfo [in] 単語辞書
 * @param hmm [in] 音響モデル
 * 
 * @return ショートポーズ単語であれば TRUE，そうでなければ FALSE．
 * </JA>
 * <EN>
 * @brief  check if the fiven word is a short-pause word.
 *
 * "Short-pause word" means a word whose pronunciation consists of
 * only one short-pause %HMM (ex. "sp").
 * 
 * @param w [in] word id
 * @param winfo [in] word dictionary
 * @param hmm [in] acoustic %HMM
 * 
 * @return TRUE if it is short pause word, FALSE if not.
 * </EN>
 */
boolean
is_sil(WORD_ID w, WORD_INFO *winfo, HTK_HMM_INFO *hmm)
{
  /* num of phones should be 1 */
  if (winfo->wlen[w] > 1) return FALSE;

  /* short pause (specified by "-spmodel") */
  if (winfo->wseq[w][0] == hmm->sp) return TRUE;

  /* head/tail sil */
  if (w == winfo->head_silwid || w == winfo->tail_silwid) return TRUE;

  /* otherwise */
  return FALSE;
}

/** 
 * <JA>
 * 発話区間の終了を検出する．
 * ショートポーズ単語が連続して最尤候補になっている区間を発見し,
 * 入力の切目を検出する．第１パスにおいて１フレーム進むごとに呼ばれる．
 * 
 * @param backtrellis [in] 単語トレリス構造体(構築途中)
 * @param time [in] 現在のフレーム
 * @param wchmm [in] 木構造辞書
 * 
 * @return TRUE (このフレームでの終了を検出したら), FALSE (終了でない場合)
 * </JA>
 * <EN>
 * Detect end-of-input by duration of short-pause words.
 * This function will be called for every frame at the 1st pass.
 * 
 * @param backtrellis [in] word trellis structure being built
 * @param time [in] current frame
 * @param wchmm [in] tree lexicon
 * 
 * @return TRUE if end-of-input detected at this frame, FALSE if not.
 * </EN>
 */
static boolean
detect_end_of_segment(BACKTRELLIS *backtrellis, int time, WCHMM_INFO *wchmm)
{
  TRELLIS_ATOM *tre;
  LOGPROB maxscore = LOG_ZERO;
  TRELLIS_ATOM *tremax = NULL;
  int count = 0;
  boolean detected = FALSE;

  /* look for the best trellis word on the given time frame */
  for(tre = backtrellis->list; tre != NULL && tre->endtime == time; tre = tre->next) {
    if (maxscore < tre->backscore) {
      maxscore = tre->backscore;
      tremax = tre;
    }
    count++;
  }
  if (tremax == NULL) {	/* no word end: possible in the very beggining of input*/
    detected = TRUE;		/* assume it's in the short-pause duration */
  } else if (count > 0) {	/* many words found --- check if maximum is sp */
    if (is_sil(tremax->wid, wchmm->winfo, wchmm->hmminfo)) {
      detected = TRUE;
    }
  }
  
  /* sp区間持続チェック */
  /* check sp segment duration */
  if (in_sparea && detected) {	/* we are already in sp segment and sp continues */
    sp_duration++;		/* increment count */
#ifdef SP_BREAK_RESUME_WORD_BEGIN
    /* resume word at the "beggining" of sp segment */
    /* if this segment has triggered by (tremax == NULL) (in case the first
       several frame of input), the sp word (to be used as resuming
       word in the next segment) is not yet set. it will be detected here */
    if (tmp_sp_break_last_word == WORD_INVALID) {
      if (tremax != NULL) tmp_sp_break_last_word = tremax->wid;
    }
#else
    /* resume word at the "end" of sp segment */
    /* simply update the best sp word */
    if (tremax != NULL) last_tre_word = tremax->wid;
#endif
  }

  /* sp区間開始チェック */
  /* check if sp segment begins at this frame */
  else if (!in_sparea && detected) {
    /* 一時的に開始フレームとしてマーク */
    /* mark this frame as "temporal" begging of short-pause segment */
    tmp_sparea_start = time;
#ifdef SP_BREAK_RESUME_WORD_BEGIN
    /* sp 区間開始時点の最尤単語を保存 */
    /* store the best word in this frame as resuming word */
    tmp_sp_break_last_word = tremax ? tremax->wid : WORD_INVALID;
#endif
    in_sparea = TRUE;		/* yes, we are in sp segment */
    sp_duration = 1;		/* initialize duration count */
#ifdef SP_BREAK_DEBUG
    printf("sp start %d\n", time);
#endif /* SP_BREAK_DEBUG */
  }
  
  /* sp 区間終了チェック */
  /* check if sp segment ends at this frame */
  else if (in_sparea && !detected) {
    /* (time-1) is end frame of pause segment */
    in_sparea = FALSE;		/* we are not in sp segment */
#ifdef SP_BREAK_DEBUG
    printf("sp end %d\n", time);
#endif /* SP_BREAK_DEBUG */
    /* sp 区間長チェック */
    /* check length of the duration*/
    if (sp_duration < sp_frame_duration) {
      /* 短すぎる: 第１パスを中断せず続行 */
      /* too short segment: not break, continue 1st pass */
#ifdef SP_BREAK_DEBUG
      printf("too short (%d<%d), ignored\n", sp_duration, sp_frame_duration);
#endif /* SP_BREAK_DEBUG */
    } else if (first_sparea) {
      /* 最初のsp区間は silB にあたるので,第１パスを中断せず続行 */
      /* do not break at first sp segment: they are silB */
      first_sparea = FALSE;
#ifdef SP_BREAK_DEBUG
      printf("first silence, ignored\n");
#endif /* SP_BREAK_DEBUG */
    } else {
      /* 区間終了確定, 第１パスを中断して第2パスへ */
      /* break 1st pass */
#ifdef SP_BREAK_DEBUG
      printf(">> segment [%d..%d]\n", sparea_start, time-1);
#else
      if (idc_on) j_printerr("|");
#endif /* SP_BREAK_DEBUG */
      /* store begging frame of the segment */
      sparea_start = tmp_sparea_start;
#ifdef SP_BREAK_RESUME_WORD_BEGIN
      /* resume word = most likely sp word on beginning frame of the segment */
      sp_break_last_word = tmp_sp_break_last_word;
#else
      /* resume word = most likely sp word on end frame of the segment */
      sp_break_last_word = last_tre_word;
#endif

      /*** segment: [sparea_start - time-1] ***/
      return(TRUE);
    }
  }
    
#ifdef SP_BREAK_EVAL
  printf("[%d %d %d]\n", time, count, (detected) ? 50 : 0);
#endif
  return (FALSE);
}

#endif /* SP_BREAK_CURRENT_FRAME */



/* -------------------------------------------------------------------- */
/*             第１パス(フレーム同期ビームサーチ) メイン                */
/*           main routines of 1st pass (frame-synchronous beam search)  */
/* -------------------------------------------------------------------- */

/** 
 * <JA>
 * ビームサーチ用の初期化を行う．出力確率キャッシュの
 * 初期化および初期仮説の生成を行う．初期仮説の初期スコア設定のため,
 * 最初の１フレーム目の入力が得られている必要がある．
 * 
 * @param param [in] 入力ベクトル列情報(最初のフレームのみ必要)
 * @param wchmm [in] 木構造化辞書
 * </JA>
 * <EN>
 * Initialize work areas, prepare output probability cache, and
 * set initial hypotheses.  The first frame of input parameter
 * should be specified to compute the scores of the initial hypotheses.
 * 
 * @param param [in] input vectors (only the first frame will be used)
 * @param wchmm [in] tree lexicon
 * </EN>
 */
static void
init_nodescore(HTK_Param *param, WCHMM_INFO *wchmm)
{
  TOKENID newid;
  TOKEN2 *new;
#ifdef USE_NGRAM
  WORD_ID beginword;
#endif
  int node;
#ifdef USE_DFA
  int i;
#endif

  /* 初期仮説用単語履歴 */
  /* setup initial word context */
#ifdef SP_BREAK_CURRENT_FRAME
  /* initial word context = last non-sp word of previous 2nd pass at last segment*/
  if (sp_break_last_nword == wchmm->winfo->tail_silwid) {
    /* if end with silE, initialize as normal start of sentence */
    bos.wid = WORD_INVALID;
  } else {
    bos.wid = sp_break_last_nword;
  }
#else
  bos.wid = WORD_INVALID;	/* no context */
#endif
  bos.begintime = bos.endtime = -1;

  /* ノード・トークンを初期化 */
  /* clear tree lexicon nodes and tokens */
  for(node=0;node<totalnodenum;node++) {
    token[node] = TOKENID_UNDEFINED;
  }
  tnum[0] = tnum[1]  = 0;
  
#ifdef PASS1_IWCD
  /* 出力確率計算キャッシュを初期化 */
  /* initialize outprob cache */
  outprob_style_cache_init(wchmm);
#endif

  /* 初期仮説の作成: 初期単語の決定と初期トークンの生成 */
  /* initial word hypothesis */
#ifdef USE_NGRAM
  
#ifdef SP_BREAK_CURRENT_FRAME
  if (sp_break_last_word != WORD_INVALID) { /* last segment exist */
    /* 開始単語＝前のセグメント計算時の最後の最尤単語 */
    /* 文終了単語(silE,句点(IPAモデル))なら，silB で開始 */
    /* initial word = best last word hypothesis on the last segment */
    /* if silE or sp, begin with silB */
    /*if (is_sil(sp_break_last_word, wchmm->winfo, wchmm->hmminfo)) {*/
    if (sp_break_last_word == wchmm->winfo->tail_silwid) {
      beginword = wchmm->winfo->head_silwid;
      bos.wid = WORD_INVALID;	/* reset initial word context */
    } else {
      beginword = sp_break_last_word;
    }
  } else {
    /* initial word fixed to silB */
    beginword = wchmm->winfo->head_silwid;
  }
#else
  /* initial word fixed to silB */
  beginword = wchmm->winfo->head_silwid;
#endif
#ifdef SP_BREAK_DEBUG
  printf("startword=[%s], last_nword=[%s]\n",
	 (beginword == WORD_INVALID) ? "WORD_INVALID" : wchmm->winfo->wname[beginword],
	 (bos.wid == WORD_INVALID) ? "WORD_INVALID" : wchmm->winfo->wname[bos.wid]);
#endif
  /* create the first token at the first node of initial word */
  newid = create_token();
  new = &(tlist[tn][newid]);
  /* initial node = head node of the beginword */
#ifdef MULTIPATH_VERSION
  node = wchmm->wordbegin[beginword];
#else
  node = wchmm->offset[beginword][0];
#endif /* MULTIPATH_VERSION */
  /* set initial LM score */
  if (wchmm->state[node].scid != 0) {
    /* if initial node is on a factoring branch, use the factored score */
    new->last_lscore = max_successor_prob(wchmm, bos.wid, node);
  } else {
    /* else, set to 0.0 */
    new->last_lscore = 0.0;
  }
#ifdef FIX_PENALTY
  new->last_lscore = new->last_lscore * lm_weight;
#else
  new->last_lscore = new->last_lscore * lm_weight + lm_penalty;
#endif
  /* set initial word history */
  new->last_tre = &bos;
  new->last_cword = bos.wid;
#ifdef MULTIPATH_VERSION
  /* set initial score using the initial LM score */
  new->score = new->last_lscore;
#else
  /* set initial score using the initial LM score and AM score of the first state */
  new->score = outprob_style(wchmm, node, bos.wid, 0, param) + new->last_lscore;
#endif /* MULTIPATH_VERSION */
  /* assign the initial node to token list */
  node_assign_token(node, newid);
  
#else  /* USE_DFA */
  
  /* 初期仮説: 文法上文頭に接続しうる単語集合 */
  /* initial words: all words that can be begin of sentence grammatically */
  /* アクティブな文法に属する単語のみ許す */
  /* only words in active grammars are allowed to be an initial words */
  {
    MULTIGRAM *m;
    int t,tb,te;
    WORD_ID iw;
    boolean flag;

    flag = FALSE;
    /* for all active grammar */
    for(m = gramlist; m; m = m->next) {
      if (m->active) {
	tb = m->cate_begin;
	te = tb + m->dfa->term_num;
	for(t=tb;t<te;t++) {
	  /* for all word categories that belong to the grammar */
	  if (dfa_cp_begin(dfa, t) == TRUE) {
	    /* if the category can appear at beginning of sentence, */
	    flag = TRUE;
	    for (iw=0;iw<dfa->term.wnum[t];iw++) {
	      /* create the initial token at the first node of all words that belongs to the category */
	      i = dfa->term.tw[t][iw];
#ifdef MULTIPATH_VERSION
	      node = wchmm->wordbegin[i];
#else
	      node = wchmm->offset[i][0];
#endif /* MULTIPATH_VERSION */
	      /* in tree lexicon, words in the same category may share the same root node, so skip it if the node has already existed */
	      if (node_exist_token(tn, node, bos.wid) != TOKENID_UNDEFINED) continue;
	      newid = create_token();
	      new = &(tlist[tn][newid]);
	      new->last_tre = &bos;
#ifdef MULTIPATH_VERSION
	      new->score = 0.0;
#else
	      new->score = outprob_style(wchmm, node, bos.wid, 0, param);
#endif /* MULTIPATH_VERSION */
	      node_assign_token(node, newid);
	    }
	  }
	}
      }
    }
    if (!flag) {
      j_printerr("Error: no initial state found in active DFA grammar!\n");
    }
  }
/* 
 *   for (i=0;i<wchmm->winfo->num;i++) {
 *     if (dfa->cp_begin[wchmm->winfo->wton[i]] == TRUE) {
 *	 node = wchmm->offset[i][0];
 *	 if (node_exist_token(tn, node, bos.wid) != TOKENID_UNDEFINED) continue;
 *	 newid = create_token();
 *	 new = &(tlist[tn][newid]);
 *	 new->last_tre = &bos;
 *	 new->score = outprob_style(wchmm, node, bos.wid, 0, param);
 *	 node_assign_token(node, newid);
 *     }
 *   }
 */
  
#endif /* USE_DFA */
}

/******************************************************/
/* フレーム同期ビーム探索の実行 --- 最初のフレーム用  */
/* frame synchronous beam search --- first frame only */
/******************************************************/

/** 
 * <JA>
 * @brief  フレーム同期ビーム探索：最初の１フレーム目
 *
 * ここではビームサーチに用いるノードやトークン，単語トレリス構造体などの
 * 初期化，および最初のフレームの計算を行う．2フレーム目以降は
 * get_back_trellis_proceed() を用いる．
 * 
 * @param param [in] 入力ベクトル列情報 (最初の１フレーム目のみ用いられる)
 * @param wchmm [in] 木構造化辞書
 * @param backtrellis [i/o] 単語トレリス構造体 (この関数内で初期化される)
 * </JA>
 * <EN>
 * @brief  Frame synchronous beam search: the fist frame
 *
 * This function will initialize nodes, tokens and the resulting word trellis
 * for the 1st pass, and then compute for the 1st frame by setting the
 * initial hypotheses.  For later frames other than the first one,
 * get_back_trellis_proceed() should be called instead.
 * 
 * @param param [in] input vectors (onlyt the first frame will be used)
 * @param wchmm [in] tree lexicon
 * @param backtrellis [i/o] word trellis structure (will be initialized)
 * </EN>
 */
void
get_back_trellis_init(HTK_Param *param,	WCHMM_INFO *wchmm, BACKTRELLIS *backtrellis)
{
  
  /* Viterbi演算用ワークエリアのスイッチャー tl,tn の初期値設定 */
  /* tn: このフレーム用ID   tl: １フレーム前のID */
  /* initialize switch tl, tn for Viterbi computation */
  /* tn: this frame  tl: last frame */
  tn = 0;
  tl = 1;

  /* 結果の単語トレリスを格納するバックトレリス構造体を初期化 */
  /* initialize backtrellis structure to store resulting word trellis */
  bt_prepare(backtrellis);

  /* ワークエリアを確保 */
  /* malloc work area */
  /* 使用するトークン量 = viterbi時に遷移先となる状態候補の数
   * 予測: ビーム数 x 2 (自己遷移+次状態) + 木構造化辞書のルートノード数
   */
  /* assumed initial number of needed tokens: beam width x 2 (self + next trans.)
   * + root node on the tree lexicon (for inter-word trans.)
   */
  /*malloc_nodes(wchmm->n);*/
  malloc_nodes(wchmm->n, trellis_beam_width * 2 + wchmm->startnum, trellis_beam_width);
  
  /* 初期スコアを nodescore[tn] にセット */
  /* set initial score to nodescore[tn] */
  init_nodescore(param, wchmm);
  sort_token_no_order(trellis_beam_width, &n_start, &n_end);

  /* テキスト出力を初期化 */
  /* initialize message output */
  result_pass1_begin();
  /* 漸次出力を行なう場合のインターバルを計算 */
  /* set interval frame for progout */
  progout_interval_frame = (int)((float)progout_interval / ((float)param->header.wshift / 10000.0));

  /* 進行状況表示を初期化 */
  /* progress bar setting */
  if (!realtime_flag && verbose_flag && (!progout_flag) && isatty(1)) {
    idc_on = TRUE;
  } else { 
    idc_on = FALSE;
  }
  
#ifdef SP_BREAK_CURRENT_FRAME
  /* ショートポーズセグメンテーション用パラメータの初期化 */
  /* initialize parameter for short pause segmentation */
  in_sparea = TRUE;		/* assume beginning is silence */
  sparea_start = tmp_sparea_start = 0; /* set start frame to 0 */
#ifdef SP_BREAK_RESUME_WORD_BEGIN
  tmp_sp_break_last_word = WORD_INVALID;
#endif
  sp_break_last_word = WORD_INVALID;
  /* 最初のセグメント: 次の非ポーズフレームで第2パスへ移行しない */
  /* the first end of pause segment should be always silB, so
     skip the first segment */
  first_sparea = TRUE;
  sp_break_2_begin_word = WORD_INVALID;
#endif

  if (gmm != NULL) {
    /* GMM 計算の初期化 */
    gmm_prepare(gmm);
  }
}

/* local work areas for get_back_trellis_proceed() */
static TRELLIS_ATOM *tre; ///< Local workarea to hold the generated trellis word
static int node; ///< Temporal work to hold the current node number on the lexicon tree
static int stid; ///< Temporal worl to specify beginning-of-word nodes on the lexicon tree
static A_CELL *ac; ///< Temporal work to hold the next states of a node
static int next_node2;		///< Temporal work to hold the next node
#ifdef USE_NGRAM
static LOGPROB tmpprob; ///< Temporal work for computing LM likelihood
static LOGPROB *iwparray; ///< Temporal pointer to hold inter-word cache array
#endif
#ifdef UNIGRAM_FACTORING
/* for wordend processing with 1-gram factoring */
static LOGPROB wordend_best_score; ///< Best score of word-end nodes
static int wordend_best_node;	///< Node id of the best wordend nodes
static TRELLIS_ATOM *wordend_best_tre; ///< Trellis word corresponds to above
static WORD_ID wordend_best_last_cword;	///< Last context-aware word of above
static int isoid; ///< Temporal work to hold isolated node
#endif

/*****************************************************/
/* frame synchronous beam search --- proceed 1 frame */
/* フレーム同期ビーム探索の実行 --- 1フレーム進める  */
/*****************************************************/

/** 
 * <JA>
 * @brief  フレーム同期ビーム探索：２フレーム目以降
 *
 * フレーム同期ビーム探索のメイン部分です．与えられた１フレーム分計算を
 * 進め，そのフレーム内に残った単語を単語トレリス構造体の中に保存する．
 * 第1フレームに対しては get_back_trellis_init() を用いる．
 * 詳細は関数内のコメントを参照のこと．
 * 
 * @param t [in] 現在のフレーム (このフレームについて計算が進められる)
 * @param param [in] 入力ベクトル列構造体 (@a t 番目のフレームのみ用いられる)
 * @param wchmm [in] 木構造化辞書
 * @param backtrellis [i/o] 単語トレリス構造体 (@a t 番目のフレーム上に残った単語が追加される)
 * 
 * @return TRUE (通常どおり終了) あるいは FALSE (ここで探索を中断する
 * 場合: 逐次デコーディング時にショートポーズ区間を検出したか，ビーム内の
 * アクティブノード数が0になったとき)
 * </JA>
 * <EN>
 * @brief  Frame synchronous beam search: proceed for 2nd frame and later.
 *
 * This is the main function of beam search on the 1st pass.  Given a
 * input vector of a frame, it proceeds the computation for the one frame,
 * and store the words survived in the beam width to the word trellis
 * structure.  get_back_trellis_init() should be used for the first frame.
 * For detailed procedure, please see the comments in this
 * function.
 * 
 * @param t [in] current frame to be computed in @a param
 * @param param [in] input vector structure (only the vector at @a t will be used)
 * @param wchmm [in] tree lexicon
 * @param backtrellis [i/o] word trellis structure to hold the survived words
 * on the @a t frame.
 * 
 * @return TRUE if processing ended normally, or FALSE if the search was
 * terminated (in case of short pause segmentation in successive decoding
 * mode, or active nodes becomes zero).
 * </EN>
 */
boolean
get_back_trellis_proceed(int t, HTK_Param *param, WCHMM_INFO *wchmm, BACKTRELLIS *backtrellis
#ifdef MULTIPATH_VERSION
			 , boolean final /* TRUE if this is final frame */
#endif
			 )
{
  LOGPROB tmpsum;
  int j, next_node, sword;
  TOKEN2  *tk, *tknext;
  TOKENID  tknextid;
#ifdef USE_NGRAM
  LOGPROB ngram_score_cache;
#endif

  /*********************/
  /* 1. 初期化         */
  /*    initialization */
  /*********************/

  /* tl と tn を入れ替えて作業領域を切り替え */
  /* tl (= 直前の tn) は直前フレームの結果を持つ */
  /* swap tl and tn to switch work buffer */
  /* tl (= last tn) holds result of the previous frame */
  tl = tn;
  if (tn == 0) tn = 1; else tn = 0;

  /* 進行状況表示を更新 */
  /* update progress bar */
  if (idc_on) {
#ifdef SP_BREAK_CURRENT_FRAME
    if (in_sparea) j_printerr("."); else j_printerr("-");
#else  /* normal */
    j_printerr(".");
#endif /* SP_BREAK_CURRENT_FRAME */
  }

#ifdef UNIGRAM_FACTORING
  /* 1-gram factoring では単語先頭での言語確率が一定で直前単語に依存しない
     ため，単語間 Viterbi において選ばれる直前単語は,次単語によらず共通である．
     よって単語終端からfactoring値のある単語先頭への遷移は１つにまとめられる．
     ただし，木から独立した単語については, 単語先頭で履歴に依存した2-gramが
     与えられるため, 最尤の単語間 Viterbi パスは次単語ごとに異なる．
     よってそれらについてはまとめずに別に計算する */
  /* In 1-gram factoring, the language score on the word-head node is constant
     and independent of the previous word.  So, the same word hypothesis will
     be selected as the best previous word at the inter-word Viterbi
     processing.  So, in inter-word processing, we can (1) select only
     the best word-end hypothesis, and then (2) process viterbi from the node
     to each word-head node.  On the other hand, the isolated words,
     i.e. words not sharing any node with other word, has unique word-head
     node and the true 2-gram language score is determined at the top node.
     In such case the best word hypothesis prior to each node will differ
     according to the language scores.  So we have to deal such words separately. */
  /* initialize max value to delect best word-end hypothesis */
  wordend_best_score = LOG_ZERO;
#endif

#ifdef DEBUG
  /* debug */
  /* node_check_token(tl); */
#endif

  /* トークンバッファを初期化: 直前フレームで使われた部分だけクリアすればよい */
  /* initialize token buffer: for speedup, only ones used in the last call will be cleared */
  clear_tokens(tl);

  /**************************/
  /* 2. Viterbi計算         */
  /*    Viterbi computation */
  /**************************/
  /* 直前フレームからこのフレームへの Viterbi 計算を行なう */
  /* tindex[tl][n_start..n_end] に直前フレーム上位ノードのIDが格納されている */
  /* do one viterbi computation from last frame to this frame */
  /* tindex[tl][n_start..n_end] holds IDs of survived nodes in last frame */
  for (j=n_start;j<=n_end;j++) {

    /* tk: 対象トークン  node: そのトークンを持つ木構造化辞書ノードID */
    /* tk: token data  node: lexicon tree node ID that holds the 'tk' */
    tk = &(tlist[tl][tindex[tl][j]]);
    node = tk->node;
    if (tk->score <= LOG_ZERO) continue; /* invalid node */

    /*********************************/
    /* 2.1. 単語内遷移               */
    /*      word-internal transition */
    /*********************************/
    for (ac = wchmm->state[node].ac; ac; ac = ac->next) {
      next_node = ac->arc;
      /* now, 'node' is the source node, 'next_node' is the destication node,
         and ac-> holds transition probability */
      /* tk->score is the accumulated score at the 'node' on previous frame */

      /******************************************************************/
      /* 2.1.1 遷移先へのスコア計算(遷移確率＋言語スコア)               */
      /*       compute score of destination node (transition prob + LM) */
      /******************************************************************/
      tmpsum = tk->score + ac->a;
#ifdef USE_NGRAM
      ngram_score_cache = LOG_ZERO;
#endif
      /* the next score at 'next_node' will be computed on 'tmpsum', and
	 the new LM probability (if updated) will be stored on 'ngram_score_cache' at below */

#ifndef CATEGORY_TREE
      /* 言語スコア factoring:
         arcが自己遷移でない単語内の遷移で，かつ遷移先にsuccessorリスト
	 があれば，lexicon tree の分岐部分の遷移である */
      /* LM factoring:
	 If this is not a self transition and destination node has successor
	 list, this is branching transition
       */
      if (next_node != node) {
	if (wchmm->state[next_node].scid != 0
#ifdef UNIGRAM_FACTORING
	    /* 1-gram factoring 使用時は, 複数で共有される枝では
	       wchmm->state[node].scid は負の値となり，その絶対値を
	       添字として wchmm->fscore[] に単語集合の1-gramの最大値が格納
	       されている．末端の枝(複数単語で共有されない)では，
	       wchmm->state[node].scid は正の値となり，
	       １単語を sc として持つのでそこで正確な2-gramを計算する */
	    /* When uni-gram factoring,
	       wchmm->state[node].scid is below 0 for shared branches.
	       In this case the maximum uni-gram probability for sub-tree
	       is stored in wchmm->fscore[- wchmm->state[node].scid].
	       Leaf branches (with only one successor word): the scid is
	       larger than 0, and has
	       the word ID in wchmm->sclist[wchmm->state[node].scid].
	       So precise 2-gram is computed in this point */
#endif
	    ){
#ifdef USE_NGRAM
	  /* ここで言語モデル確率を更新する */
	  /* LM value should be update at this transition */
	  /* N-gram確率からfactoring 値を計算 */
	  /* compute new factoring value from N-gram probabilities */
#ifdef FIX_PENALTY
	  /* if at the beginning of sentence, not add lm_penalty */
	  if (tk->last_cword == WORD_INVALID) {
	    ngram_score_cache = max_successor_prob(wchmm, tk->last_cword, next_node) * lm_weight;
	  } else {
	    ngram_score_cache = max_successor_prob(wchmm, tk->last_cword, next_node) * lm_weight + lm_penalty;
	  }
#else
	  ngram_score_cache = max_successor_prob(wchmm, tk->last_cword, next_node) * lm_weight + lm_penalty;
#endif
	  /* スコアの更新: tk->last_lscore に単語内での最後のfactoring値が
	   入っているので, それをスコアから引いてリセットし, 新たなスコアを
	   セットする */
	  /* update score: since 'tk->last_lscore' holds the last LM factoring
	     value in this word, we first remove the score from the current
	     score, and then add the new LM value computed above. */
	  tmpsum -= tk->last_lscore;
	  tmpsum += ngram_score_cache;
	  
#else  /* USE_DFA --- not CATEGORY_TREE */
	  /* 文法を用いる場合, カテゴリ単位の木構造化がなされていれば,
	     接続制約は単語間遷移のみで扱われるので，factoring は必要ない．
	     カテゴリ単位木構造化が行われない場合, 文法間の接続制約はここ
	     で factoring で行われることになる．*/
	  /* With DFA, we use category-pair constraint extracted from the DFA
	     at this 1st pass.  So if we compose a tree lexicon per word's
	     category, the each category tree has the same connection
	     constraint and thus we can apply the constraint on the cross-word
	     transition.  This per-category tree lexicon is enabled by default,
	     and in the case the constraint will be applied at the word end.
	     If you disable per-category tree lexicon by undefining
	     'CATEGORY_TREE', the word-pair contrained will be applied in a
	     factoring style at here.
	  */
	  
          /* 決定的factoring: 直前単語に対して,sub-tree内にカテゴリ対制約上
	     接続しうる単語が１つもなければ, この遷移は不可 */
	  /* deterministic factoring in grammar mode:
	     transition disabled if there are totally no sub-tree word that can
	     grammatically (in category-pair constraint) connect
	     to the previous word.
	   */
          if (!can_succeed(wchmm, tk->last_tre->wid, next_node)) {
            tmpsum = LOG_ZERO;
          }
	  
#endif /* USE_NGRAM */
	}
      }
#endif /* CATEGORY_TREE */
      /* factoring not needed when DFA mode and uses category-tree */

      /****************************************/
      /* 2.1.2 遷移先ノードへトークン伝搬     */
      /*       pass token to destination node */
      /****************************************/

      if ((tknextid = node_exist_token(tn, next_node, tk->last_tre->wid)) != TOKENID_UNDEFINED) {
	/* 遷移先ノードには既に他ノードから伝搬済み: スコアが高いほうを残す */
	/* the destination node already has a token: compare score */
	tknext = &(tlist[tn][tknextid]);
	if (tknext->score < tmpsum) {
	  /* その遷移先ノードが持つトークンの内容を上書きする(新規トークンは作らない) */
	  /* overwrite the content of existing destination token: not create a new token */
	  tknext->last_tre = tk->last_tre; /* propagate last word info */
#ifdef USE_NGRAM
	  tknext->last_cword = tk->last_cword; /* propagate last context word info */
	  tknext->last_lscore = (ngram_score_cache != LOG_ZERO) ? ngram_score_cache : tk->last_lscore; /* set new LM score */
#endif /* USE_NGRAM */
	  tknext->score = tmpsum; /* set new score */
	}
      } else {
	/* 遷移先ノードは未伝搬: 新規トークンを作って割り付ける */
	/* token unassigned: create new token and assign */
	if (tmpsum > LOG_ZERO) { /* valid token */
	  tknextid = create_token(); /* get new token */
	  tknext = &(tlist[tn][tknextid]);
	  /* if work area has been expanded at 'create_token()' above,
	     the inside 'remalloc()' will destroy the 'tk' pointer.
	     so, here we again set tk from token index */
	  tk = &(tlist[tl][tindex[tl][j]]);
	  tknext->last_tre = tk->last_tre; /* propagate last word info */
#ifdef USE_NGRAM
	  tknext->last_cword = tk->last_cword; /* propagate last context word info */
	  tknext->last_lscore = (ngram_score_cache != LOG_ZERO) ? ngram_score_cache : tk->last_lscore; /* set new LM score */
#endif /* USE_NGRAM */
	  tknext->score = tmpsum; /* set new score */
	  node_assign_token(next_node, tknextid); /* assign this new token to the next node */
	}
      }
    } /* END OF WORD-INTERNAL TRANSITION */

#ifdef MULTIPATH_VERSION
    
  }
  /*******************************************************/
  /* 3. スコアでトークンをソートしビーム幅分の上位を決定 */
  /*    sort tokens by score up to beam width            */
  /*******************************************************/
  /* ヒープソートを用いてこの段のノード集合から上位(bwidth)個を得ておく */
  /* (上位内の順列は必要ない) */
  sort_token_no_order(trellis_beam_width, &n_start, &n_end);
  
  /*************************/
  /* 4. 単語間Viterbi計算  */
  /*    cross-word viterbi */
  /*************************/
  
  for(j=n_start; j<=n_end; j++) {
    tk = &(tlist[tn][tindex[tn][j]]);
    node = tk->node;
     
#endif /* MULTIPATH_VERSION */

    /* 遷移元ノードが単語終端ならば */
    /* if source node is end state of a word, */
    if (wchmm->stend[node] != WORD_INVALID) {

      sword = wchmm->stend[node];

      /**************************/
      /* 2.2. トレリス単語保存  */
      /*      save trellis word */
      /**************************/

      /* この遷移元の単語終端ノードは「直前フレームで」生き残ったノード．
	 (「このフレーム」でないことに注意！！)
	 よってここで, 時間(t-1) を単語終端とするトレリス上の単語仮説
	 (TRELLIS_ATOM)として，単語トレリス構造体に保存する．*/
      /* This source node (= word end node) has been survived in the
	 "last" frame (notice: not "this" frame!!).  So this word end
	 is saved here to the word trellis structure (BACKTRELLIS) as a
	 trellis word (TRELLIS_ATOM) with end frame (t-1). */
      tre = (TRELLIS_ATOM *)mybmalloc2(sizeof(TRELLIS_ATOM), &(backtrellis->root));
      tre->wid = sword;		/* word ID */
      tre->backscore = tk->score; /* log score (AM + LM) */
      tre->begintime = tk->last_tre->endtime + 1; /* word beginning frame */
      tre->endtime   = t-1;	/* word end frame */
      tre->last_tre  = tk->last_tre; /* link to previous trellis word */
#ifdef USE_NGRAM
      tre->lscore    = tk->last_lscore;	/* log score (LM only) */
#endif
      bt_store(backtrellis, tre); /* save to backtrellis */
#ifdef WORD_GRAPH
      if (tre->last_tre != NULL) {
	/* mark to indicate that the following words was survived in beam */
	tre->last_tre->within_context = TRUE;
      }
#ifdef MULTIPATH_VERSION
      if (final) {
	/* last node */
	if (tre->wid == winfo->tail_silwid) {
	  tre->within_context = TRUE;
	}
      }
#endif
#endif /* WORD_GRAPH */
      
      /******************************/
      /* 2.3. 単語間遷移            */
      /*      cross-word transition */
      /******************************/

#ifdef MULTIPATH_VERSION
      /* 最終フレームであればここまで：遷移はさせない */
      /* If this is a final frame, does not do cross-word transition */
      if (final) continue;
#endif

      /* 先ほどのトレリス単語treが，ここからの遷移先の単語先頭ノード以降の
	 直前単語情報としても参照される */
      /* The trellis atom 'tre' will be refered as the word context
	 information for next word-beginning nodes */

#ifdef UNIGRAM_FACTORING
      /* ここで処理されるのは isolated words のみ，
	 shared nodes はまとめてこのループの外で計算する */
      /* Only the isolated words will be processed here.
	 The shared nodes with constant factoring values will be computed
	 after this loop */
#endif

#ifdef USE_NGRAM
      /* 遷移元単語が末尾単語の終端なら，どこへも遷移させない */
      /* do not allow transition if the source word is end-of-sentence word */
      if (sword == wchmm->winfo->tail_silwid) continue;

#ifdef UNIGRAM_FACTORING
      /* あとで共有単語先頭ノードに対して単語間遷移をまとめて計算するため，*/
      /* このループ内では最大尤度を持つ単語終端ノードを記録しておく */
      /* here we will record the best wordend node of maximum likelihood
	 at this frame, to compute later the cross-word transitions toward
	 shared factoring word-head node */
      if (wordend_best_score < tk->score
#ifndef MULTIPATH_VERSION
	  + wchmm->wordend_a[sword]
#endif
	  ) {
	wordend_best_score = tk->score
#ifndef MULTIPATH_VERSION
	  + wchmm->wordend_a[sword]
#endif
	  ;
	wordend_best_node = node;
	wordend_best_tre = tre;
	wordend_best_last_cword = tk->last_cword;
      }
#endif
      
      /* N-gramにおいては常に全単語の接続を考慮する必要があるため，
	 ここで単語間の言語確率値をすべて計算しておく．
	 キャッシュは max_successor_prob_iw() 内で考慮．*/
      /* As all words are possible to connect in N-gram, we first compute
	 all the inter-word LM probability here.
	 Cache is onsidered in max_successor_prob_iw(). */
      if (wchmm->winfo->is_transparent[sword]) {
	iwparray = max_successor_prob_iw(wchmm, tk->last_cword);
      } else {
	iwparray = max_successor_prob_iw(wchmm, sword);
      }
#endif

      /* すべての単語始端ノードに対して以下を実行 */
      /* for all beginning-of-word nodes, */
      /* wchmm->startnode[0..stid-1] ... 単語始端ノードリスト */
      /* wchmm->startnode[0..stid-1] ... list of word start node (shared) */
      for (stid = wchmm->startnum - 1; stid >= 0; stid--) {
	next_node = wchmm->startnode[stid];
#ifdef MULTIPATH_VERSION
#ifdef USE_NGRAM
	/* connection to the head silence word is not allowed */
	if (wchmm->wordbegin[wchmm->winfo->head_silwid] == next_node) continue;
#endif
#endif

	/*****************************************/
	/* 2.3.1. 単語間言語制約を適用           */
	/*        apply cross-word LM constraint */
	/*****************************************/
	
#ifdef USE_NGRAM
	/* N-gram確率を計算 */
	/* compute N-gram probability */
#ifdef UNIGRAM_FACTORING
	/* 1-gram factoring における単語間言語確率キャッシュの効率化:
	   1-gram factoring は単語履歴に依存しないので，
	   ここで参照する factoring 値の多くは
	   wchmm->fscore[] に既に格納され, 探索中も不変である．
	   よって計算が必要な単語(どの単語ともノードを共有しない単語)
	   についてのみ iwparray[] で計算・キャッシュする． */
	/* Efficient cross-word LM cache:
	   As 1-gram factoring values are independent of word context,
	   they remain unchanged while search.  So, in cross-word LM
	   computation, beginning-of-word states which share nodes with
	   others and has factoring value in wchmm does not need cache.
	   So only the unshared beginning-of-word states are computed and
	   cached here in iwparray[].
	 */
	/* wchmm,start2isolate[0..stid-1] ... ノードを共有しない単語は
	   その通しID, 共有する(キャッシュの必要のない)単語は -1 */
	/* wchmm->start2isolate[0..stid-1] ... isolate ID for
	   beginning-of-word state.  value: -1 for states that has
	   1-gram factoring value (share nodes with some other words),
	   and ID for unshared words
	 */
	isoid = wchmm->start2isolate[stid];
	/* 計算が必要でない単語先頭ノードはパスをまとめて後に計算するので
	   ここではスキップ */
	/* the shared nodes will be computed afterward, so just skip them
	   here */
	if (isoid == -1) continue;
	tmpprob = iwparray[isoid];
#else
	tmpprob = iwparray[stid];
#endif
#endif

#ifdef USE_NGRAM
	/* 遷移先の単語が先頭単語なら遷移させない．
	   これは wchmm.c で該当単語に stid を割り振らないことで対応
	   しているので，ここでは何もしなくてよい */
	/* do not allow transition if the destination word is
	   beginning-of-sentence word.  This limitation is realized by
	   not assigning 'stid' for the word in wchmm.c, so we have nothing
	   to do here.
	*/
#endif
	
#ifdef CATEGORY_TREE
	/* 文法の場合, 制約は決定的: カテゴリ対制約上許されない場合は遷移させない */
	/* With DFA and per-category tree lexicon,
	   LM constraint is deterministic:
	   do not allow transition if the category connection is not allowed
	   (with category tree, constraint can be determined on top node) */
	if (dfa_cp(dfa, wchmm->winfo->wton[sword],
#ifdef MULTIPATH_VERSION
		   wchmm->winfo->wton[wchmm->start2wid[stid]]
#else
		   wchmm->winfo->wton[wchmm->ststart[next_node]]
#endif /* MULTIPATH_VERSION */
		   ) == FALSE) continue;
#endif

	/*******************************************************************/
	/* 2.3.2. 遷移先の単語先頭へのスコア計算(遷移確率＋言語スコア)     */
	/*        compute score of destination node (transition prob + LM) */
	/*******************************************************************/
	tmpsum = tk->score
#ifndef MULTIPATH_VERSION
	  + wchmm->wordend_a[sword]
#endif
	  ;
	/* 'tmpsum' now holds outgoing score from the wordend node */
#ifdef USE_NGRAM
	/* 言語スコアを追加 */
	/* add LM score */
	ngram_score_cache = tmpprob * lm_weight + lm_penalty;
	tmpsum += ngram_score_cache;
	if (wchmm->winfo->is_transparent[sword] && wchmm->winfo->is_transparent[tk->last_cword]) {
	  
	  tmpsum += lm_penalty_trans;
	}
#else  /* USE_DFA */
	/* grammar: 単語挿入ペナルティを追加 */
	/* grammar: add insertion penalty */
	tmpsum += penalty1;

	/* grammar: deterministic factoring (in case category-tree not enabled) */
#ifdef CATEGORY_TREE
	/* no need to factoring */
#else
	if (!can_succeed(wchmm, sword, next_node)) {
	  tmpsum = LOG_ZERO;
	}
#endif /* CATEGORY_TREE */
#endif /* USE_NGRAM */


	/*********************************************************************/
	/* 2.3.3. 遷移先ノードへトークン伝搬(単語履歴情報は更新)             */
	/*        pass token to destination node (updating word-context info */
	/*********************************************************************/

#ifndef MULTIPATH_VERSION
	next_node2 = next_node;
#else
	for(ac=wchmm->state[next_node].ac; ac; ac=ac->next) {
	  next_node2 = ac->arc;
#endif
	  
	  if ((tknextid = node_exist_token(tn, next_node2, tre->wid)) != TOKENID_UNDEFINED) {
	    /* 遷移先ノードには既に他ノードから伝搬済み: スコアが高いほうを残す */
	    /* the destination node already has a token: compare score */
	    tknext = &(tlist[tn][tknextid]);
	    if (tknext->score < tmpsum) {
	      /* その遷移先ノードが持つトークンの内容を上書きする(新規トークンは作らない) */
	      /* overwrite the content of existing destination token: not create a new token */
#ifdef USE_NGRAM
	      tknext->last_lscore = ngram_score_cache; /* set new LM score */
	      /* set last context word */
	      if (wchmm->winfo->is_transparent[sword]) {
		/* if this word is a "transparent" word, this word should be
		   excluded from a word history as a context for LM. In the case,
		   propagate the last context word to the new node */
		tknext->last_cword = tk->last_cword;
	      } else {
		/* otherwise, set last context word to this word */
		tknext->last_cword = sword;
	      }
#endif
	      /* set new score */
	      tknext->score = tmpsum;
#ifdef MULTIPATH_VERSION
	      tknext->score += ac->a;
#endif
	      /* 遷移先に，先ほど保存したswordのトレリス単語へのポインタを
		 「直前単語履歴情報」として伝搬 */
	      /* pass destination the pointer to the saved trellis atom
		 corresponds to 'sword' as "previous word-context info". */
	      tknext->last_tre = tre;
	    }
	  } else {
	    /* 遷移先ノードは未伝搬: 新規トークンを作って割り付ける */
	    /* token unassigned: create new token and assign */
	    if (tmpsum > LOG_ZERO) { /* valid token */
	      tknextid = create_token();
	      tknext = &(tlist[tn][tknextid]);
	      /* if work area has been expanded at 'create_token()' above,
		 the inside 'remalloc()' will destroy the 'tk' pointer.
		 so, here we again set tk from token index */
#ifdef MULTIPATH_VERSION
	      tk = &(tlist[tn][tindex[tn][j]]);
#else
	      tk = &(tlist[tl][tindex[tl][j]]);
#endif
#ifdef USE_NGRAM
	      tknext->last_lscore = ngram_score_cache; /* set new LM score */
	      /* set last context word */
	      if (wchmm->winfo->is_transparent[sword]) {
		/* if this word is a "transparent" word, this word should be
		   excluded from a word history as a context for LM. In the case,
		   propagate the last context word to the new node */
		tknext->last_cword = tk->last_cword;
	      } else {
		/* otherwise, set last context word to this word */
		tknext->last_cword = sword;
	      }
#endif
	      tknext->score = tmpsum;
#ifdef MULTIPATH_VERSION
	      tknext->score += ac->a;
#endif
	    /* 遷移先に，先ほど保存したswordのトレリス単語へのポインタを
	       「直前単語履歴情報」として伝搬 */
	    /* pass destination the pointer to the saved trellis atom
	       corresponds to 'sword' as "previous word-context info". */
	      tknext->last_tre = tre;
	      /* assign to the next node */
	      node_assign_token(next_node2, tknextid);
	    }
	  }
#ifdef MULTIPATH_VERSION
	}
#endif
	
      }	/* end of next word heads */
    } /* end of cross-word processing */
    
  } /* end of main viterbi loop */
#ifdef UNIGRAM_FACTORING
  /***********************************************************/
  /* 2.4 単語終端からfactoring付き単語先頭への遷移 ***********/
  /*    transition from wordend to shared (factorized) nodes */
  /***********************************************************/
  /* wordend_best_* holds the best word ends at this frame. */
  if (wordend_best_score > LOG_ZERO) {
    node = wordend_best_node;
    sword = wchmm->stend[node];
    for (stid = wchmm->startnum - 1; stid >= 0; stid--) {
      next_node = wchmm->startnode[stid];
      /* compute transition from 'node' at end of word 'sword' to 'next_node' */
      /* skip isolated words already calculated in the above main loop */
      if (wchmm->start2isolate[stid] != -1) continue;
      /* rest should have 1-gram factoring score at wchmm->fscore */
      if (wchmm->state[next_node].scid >= 0) {
	j_error("InternalError: scid mismatch at 1-gram factoring of shared states\n");
      }
      tmpprob = wchmm->fscore[- wchmm->state[next_node].scid];
      ngram_score_cache = tmpprob * lm_weight + lm_penalty;
      tmpsum = wordend_best_score;
      tmpsum += ngram_score_cache;
      if (wchmm->winfo->is_transparent[sword] && wchmm->winfo->is_transparent[wordend_best_last_cword]) {
	tmpsum += lm_penalty_trans;
      }
      
#ifndef MULTIPATH_VERSION
      next_node2 = next_node;
#else
      for(ac=wchmm->state[next_node].ac; ac; ac=ac->next) {
	next_node2 = ac->arc;
#endif
	if ((tknextid = node_exist_token(tn, next_node2, sword)) != TOKENID_UNDEFINED) {
	  tknext = &(tlist[tn][tknextid]);
	  if (tknext->score < tmpsum) {
	    tknext->last_lscore = ngram_score_cache;
	    if (wchmm->winfo->is_transparent[sword]) {
	      tknext->last_cword = wordend_best_last_cword;
	    } else {
	      tknext->last_cword = sword;
	    }
	    tknext->score = tmpsum;
#ifdef MULTIPATH_VERSION
	    tknext->score += ac->a;
#endif
	    tknext->last_tre = wordend_best_tre;
	  }
	} else {
	  if (tmpsum > LOG_ZERO) { /* valid token */
	    tknextid = create_token();
	    tknext = &(tlist[tn][tknextid]);
	    tknext->last_lscore = ngram_score_cache;
	    if (wchmm->winfo->is_transparent[sword]) {
	      tknext->last_cword = wordend_best_last_cword;
	    } else {
	      tknext->last_cword = sword;
	    }
	    tknext->score = tmpsum;
#ifdef MULTIPATH_VERSION
	    tknext->score += ac->a;
#endif
	    tknext->last_tre = wordend_best_tre;
	    node_assign_token(next_node2, tknextid);
	  }
	}
#ifdef MULTIPATH_VERSION
      }
#endif
      
    }
  }
#endif /* UNIGRAM_FACTORING */

  /***************************************/
  /* 3. 状態の出力確率計算               */
  /*    compute state output probability */
  /***************************************/

  /* 次段の有効ノードについて出力確率を計算してスコアに加える */
  /* compute outprob for new valid (token assigned) nodes and add to score */
#ifdef MULTIPATH_VERSION
  /* 今扱っているのが入力の最終フレームの場合出力確率は計算しない */
  /* don't calculate the last frame (transition only) */
  if (! final) {
#endif
    for (j=0;j<tnum[tn];j++) {
      tk = &(tlist[tn][tindex[tn][j]]);
#ifdef MULTIPATH_VERSION
      /* skip non-output state */
      if (wchmm->state[tk->node].out.state == NULL) continue;
#endif
      tk->score += outprob_style(wchmm, tk->node, tk->last_tre->wid, t, param);
    }
#ifdef MULTIPATH_VERSION
  }
#endif

  if (gmm != NULL) {
    /* GMM 計算を行う */
    gmm_proceed(gmm, param, t);
  }

  /*******************************************************/
  /* 4. スコアでトークンをソートしビーム幅分の上位を決定 */
  /*    sort tokens by score up to beam width            */
  /*******************************************************/

  /* tlist[tl]を次段のためにリセット */
  clear_tlist(tl);

  /* ヒープソートを用いてこの段のノード集合から上位(bwidth)個を得ておく */
  /* (上位内の順列は必要ない) */
  sort_token_no_order(trellis_beam_width, &n_start, &n_end);

  /* check for sort result */
/* 
 *     j_printf("%d: vwidth=%d / %d\n",t, vwidth[tn], wchmm->n);
 */
  /* make sure LOG_ZERO not exist width vwidth[tn] */
/* 
 *     for (j=0;j<vwidth[tn];j++) {
 *	 if (nodescore[tn][hsindex[tn][j+1]] <= LOG_ZERO) {
 *	   j_error("dsadsadas %d %d\n",hsindex[tn][j+1], nodescore[tn][hsindex[tn][j+1]]);
 *	 }
 *     }
 */
  
  /***************/
  /* 5. 終了処理 */
  /*    finalize */
  /***************/
  if (progout_flag) {
    /* 漸次出力: 現フレームのベストパスを一定時間おきに上書き出力 */
    /* progressive result output: output current best path in certain time interval */
    if ((t % progout_interval_frame) == 0) {
      bt_current_max(backtrellis, t-1, wchmm->winfo);
    }
  }
  /* j_printf("%d: %d\n",t,tnum[tn]); */
  /* for debug: output current max word */
  if (debug2_flag) {
    bt_current_max_word(backtrellis, t-1, wchmm->winfo);
  }

#ifdef SP_BREAK_CURRENT_FRAME
  /* ショートポーズセグメンテーション: 直前フレームでセグメントが切れたかどうかチェック */
  if (detect_end_of_segment(backtrellis, t-1, wchmm)) {
    /* セグメント終了検知: 第１パスここで中断 */
    return FALSE;		/* segment: [sparea_start..t-2] */
  }
#endif

  /* ビーム内ノード数が 0 になってしまったら，強制終了 */
  if (tnum[tn] == 0) {
    j_printerr("Error: %dth frame: no nodes left in beam! model mismatch or wrong input?\n", t);
    return(FALSE);
  }

  return(TRUE);
    
}

/*************************************************/
/* frame synchronous beam search --- last frame  */
/* フレーム同期ビーム探索の実行 --- 最終フレーム */
/*************************************************/

/** 
 * <JA>
 * @brief  フレーム同期ビーム探索：最終フレーム
 *
 * 第１パスのフレーム同期ビーム探索を終了するために，
 * (param->samplenum -1) の最終フレームに対する終了処理を行う．
 * 
 * 
 * @param param [in] 入力ベクトル列 (param->samplenum の値のみ用いられる)
 * @param wchmm [in] 木構造化辞書
 * @param backtrellis [i/o] 単語トレリス構造体 (最終フレームの単語候補が格納される)
 * </JA>
 * <EN>
 * @brief  Frame synchronous beam search: last frame
 *
 * This function should be called at the end of the 1st pass.
 * The last procedure will be done for the (param->samplenum - 1) frame.
 * 
 * @param param [in] input vectors (only param->samplenum is referred)
 * @param wchmm [in] tree lexicon
 * @param backtrellis [i/o] word trellis structure (the last survived words will be stored to this)
 * </EN>
 */
void
get_back_trellis_end(HTK_Param *param, WCHMM_INFO *wchmm, BACKTRELLIS *backtrellis)
{
  int t, node, j;
  TOKEN2 *tk;

  /* 最後にビーム内に残った単語終端トークンを処理する */
  /* process the last wordend tokens */

#ifdef MULTIPATH_VERSION
  
  /* 単語末ノードへの遷移のみ計算 */
  /* only arcs to word-end node is calculated */
  get_back_trellis_proceed(param->samplenum, param, wchmm, backtrellis, TRUE);
  
#else  /* ~MULTIPATH_VERSION */
  
  t = param->samplenum;
  tl = tn;
  if (tn == 0) tn = 1; else tn = 0;
  for (j=n_start; j<=n_end; j++) {
    tk = &(tlist[tl][tindex[tl][j]]);
    node = tk->node;
    if (wchmm->stend[node] != WORD_INVALID) {
      tre = (TRELLIS_ATOM *)mybmalloc2(sizeof(TRELLIS_ATOM), &(backtrellis->root));
      tre->wid = wchmm->stend[node];
      tre->backscore = tk->score;
      tre->begintime = tk->last_tre->endtime + 1;
      tre->endtime   = t-1;
      tre->last_tre  = tk->last_tre;
#ifdef USE_NGRAM
      tre->lscore    = tk->last_lscore;
#endif
      bt_store(backtrellis, tre);
#ifdef WORD_GRAPH
      if (tre->last_tre != NULL) {
	/* mark to indicate that the following words was survived in beam */
	tre->last_tre->within_context = TRUE;
      }
      /* last node */
      if (tre->wid == winfo->tail_silwid) {
	tre->within_context = TRUE;
      }
#endif
    }
  }

#endif /* MULTIPATH_VERSION */

#ifdef SP_BREAK_CURRENT_FRAME
  /*if (detect_end_of_segment(backtrellis, param->samplenum-1, winfo)) {
    return;
    }*/
#endif
}

/*************************/
/* 探索終了 --- 終了処理 */
/* end of search         */
/*************************/
/** 
 * <JA>
 * @brief  第１パスの終了処理を行う．
 *
 * この関数は get_back_trellis_end() の直後に呼ばれ，第１パスの終了処理を
 * 行う．生成した単語トレリス構造体の最終的な後処理を行い第２パスで
 * アクセス可能な形に内部を変換する．また，
 * 仮説のバックトレースを行い第１パスのベスト仮説を出力する．
 * 
 * @param backtrellis [i/o] 単語トレリス構造体 (後処理が行われる)
 * @param winfo [in] 単語辞書
 * @param len [in] 第１パスで処理された最終的なフレーム長
 * 
 * @return 第１パスの最尤仮説の累積尤度，あるいは仮説が見つからない場合
 * は LOG_ZERO．
 * </JA>
 * <EN>
 * @brief  Finalize the 1st pass.
 *
 * This function will be called just after get_back_trellis_end() to
 * finalize the 1st pass.  It processes the resulting word trellis structure
 * to be accessible from the 2nd pass, and output the best sentence hypothesis
 * by backtracing the word trellis.
 * 
 * @param backtrellis [i/o] word trellis structure (will be reconstructed internally
 * @param winfo [in] word dictionary
 * @param len [in] total number of processed frames
 * 
 * @return the maximum score of the best hypothesis, or LOG_ZERO if search
 * failed.
 * </EN>
 */
LOGPROB
finalize_1st_pass(BACKTRELLIS *backtrellis, WORD_INFO *winfo, int len)
{
  LOGPROB lastscore;
  int mseclen;
  boolean ok_p;
 
  backtrellis->framelen = len;

  /* rejectshort 指定時, 入力が短ければここで第1パス結果を出力しない */
  /* ここでは reject はまだ行わず, 後で reject する */
  /* suppress 1st pass output if -rejectshort and input shorter than specified */
  /* the actual rejection will be done later at main */
  ok_p = TRUE;
  if (rejectshortlen > 0) {
    mseclen = (float)len * (float)para.smp_period * (float)para.frameshift / 10000.0;
    if (mseclen < rejectshortlen) {
      ok_p = FALSE;
    }
  }
  
  /* 単語トレリス(backtrellis) を整理: トレリス単語の再配置とソート */
  /* re-arrange backtrellis: index them by frame, and sort by word ID */
  if (ok_p) {
    bt_relocate_rw(backtrellis);
    bt_sort_rw(backtrellis);
    if (backtrellis->num == NULL) {
      if (backtrellis->framelen > 0) j_printerr("no survived word!\n");
      ok_p = FALSE;
    }
  }

  /* 結果を表示する (best 仮説)*/
  /* output 1st pass result (best hypothesis) */
  if (verbose_flag && (!progout_flag)) j_printerr("\n");
  if (ok_p) {
    lastscore = print_1pass_result(backtrellis, len, winfo);
  } else {
    lastscore = LOG_ZERO;
  }

#ifdef USE_NGRAM
  /* free succesor cache */
  /* 次の認識でwchmm->winfoともに無変更の場合 free する必要なし */
  /* no need to free if wchmm and wchmm are not changed in the next recognition */
  /* max_successor_cache_free(); */
#endif
  /* free nodes */
  free_nodes();
  
  if (ok_p) {
    if (gmm != NULL) {
      /* GMM 計算の終了 */
      gmm_end(gmm);
    }
  }

  /* return the best score */
  return(lastscore);
}

#ifdef SP_BREAK_CURRENT_FRAME
/*******************************************************************/
/* 第１パスセグメント終了処理 (ショートポーズセグメンテーション用) */
/* end of 1st pass for a segment (for short pause segmentation)    */
/*******************************************************************/
/** 
 * <JA>
 * @brief  逐次デコーディングのための第１パス終了時の追加処理
 *
 * 逐次デコーディング使用時，この関数は finalize_1st_pass() 後に呼ばれ，
 * そのセグメントの第１パスの終了処理を行う．具体的には，
 * 続く第２パスのための始終端単語のセット，および
 * 次回デコーディングを再開するときのために，入力ベクトル列の未処理部分の
 * コピーを rest_param に残す．
 * 
 * @param backtrellis [in] 単語トレリス構造体
 * @param param [i/o] 入力ベクトル列．頭から @a len フレームだけが切り出される．
 * @param len [in] セグメント長
 * </JA>
 * <EN>
 * @brief  Additional finalize procedure for successive decoding
 *
 * When successive decoding mode is enabled, this function will be
 * called just after finalize_1st_pass() to finish the beam search
 * of the last segment.  The beginning and ending words for the 2nd pass
 * will be set according to the 1st pass result.  Then the current
 * input will be shrinked to the segmented length and the unprocessed
 * region are copied to rest_param for the next decoding.
 * 
 * @param backtrellis [in] word trellis structure
 * @param param [in] input vectors (will be shrinked to @a len frames)
 * @param len [in] length of the last segment
 * </EN>
 */
void
finalize_segment(BACKTRELLIS *backtrellis, HTK_Param *param, int len)
{
  int t;

  /* トレリス始終端における最尤単語を第2パスの始終端単語として格納 */
  /* fix initial/last word hypothesis of the next 2nd pass to the best
     word hypothesis at the first/last frame in backtrellis*/
  set_terminal_words(backtrellis);

  /* パラメータを, 今第１パスが終了したセグメント区間と残りの区間に分割する．
     ただし接続部のsp区間部分(sparea_start..len-1)は「のりしろ」として両方に
     コピーする */
  /* Divide input parameter into two: the last segment and the rest.
     The short-pause area (sparea_start..len-1) is considered as "tab",
     copied in both parameters
   */
  /* param[sparea_start..framelen] -> rest_param
     param[0..len-1] -> param
     [sparea_start...len-1] overlapped
  */

  if (len != param->samplenum) {

    VERMES("segmented: processed length=%d\n", len);
    VERMES("segmented: next decoding will restart from %d\n", sparea_start);
    
    /* copy rest parameters for next process */
    rest_param = new_param();
    memcpy(&(rest_param->header), &(param->header), sizeof(HTK_Param_Header));
    rest_param->samplenum = param->samplenum - sparea_start;
    rest_param->header.samplenum = rest_param->samplenum;
    rest_param->veclen = param->veclen;
    rest_param->parvec = (VECT **)mymalloc(sizeof(VECT *) * rest_param->samplenum);
    /* copy 1: overlap area (copy) */
    for(t=sparea_start;t<len;t++) {
      rest_param->parvec[t-sparea_start] = (VECT *)mymalloc(sizeof(VECT) * rest_param->veclen);
      memcpy(rest_param->parvec[t-sparea_start], param->parvec[t], sizeof(VECT) * rest_param->veclen);
    }
    /* copy 2: rest area (move) */
    for(t=len;t<param->samplenum;t++) {
      rest_param->parvec[t-sparea_start] = param->parvec[t];
    }

    /* shrink original param to [0..len-1] */
    /* just shrink the parvec */
    param->samplenum = len;
    param->parvec = (VECT **)myrealloc(param->parvec, sizeof(VECT *) * param->samplenum);
    sp_break_last_nword_allow_override = TRUE;
    
  } else {
    
    /* last segment is on end of input: no rest parameter */
    rest_param = NULL;
    /* reset last_word info */
    sp_break_last_word = WORD_INVALID;
    sp_break_last_nword = WORD_INVALID;
    sp_break_last_nword_allow_override = FALSE;
  }
}
#endif /* SP_BREAK_CURRENT_FRAME */
  
/********************************************************************/
/* 第１パスを実行するメイン関数                                     */
/* 入力をパイプライン処理する場合は realtime_1stpass.c を参照のこと */
/* main function to execute 1st pass                                */
/* the pipeline processing is not here: see realtime_1stpass.c      */
/********************************************************************/

/** 
 * <JA>
 * @brief  フレーム同期ビーム探索：メイン
 *
 * 与えられた入力ベクトル列に対して第１パス(フレーム同期ビーム探索)を
 * 行い，その結果を出力する．また全フレームに渡る単語終端を，第２パス
 * のために単語トレリス構造体に格納する．
 * 
 * この関数は入力ベクトル列があらかじめ得られている場合に用いられる．
 * 第１パスが入力と並列して実行されるオンライン認識の場合，
 * この関数は用いられず，代わりにこのファイルで定義されている各サブ関数が
 * 直接 realtime-1stpass.c 内から呼ばれる．
 * 
 * @param param [in] 入力ベクトル列
 * @param wchmm [in] 木構造化辞書
 * @param backtrellis [out] 単語トレリス情報が書き込まれる構造体へのポインタ
 * @param backmax [out] 第１パスの最尤スコア (探索失敗なら LOG_ZERO)
 * </JA>
 * <EN>
 * @brief  Frame synchronous beam search: the main
 *
 * This function perform the 1st recognition pass of frame-synchronous beam
 * search and output the result.  It also stores all the word ends in every
 * input frame to word trellis structure.
 *
 * This function will be called if the whole input vector is already given
 * to the end.  When online recognition, where the 1st pass will be
 * processed in parallel with input, this function will not be used.
 * In that case, functions defined in this file will be directly called
 * from functions in realtime-1stpass.c.
 * 
 * @param param [in] input vectors
 * @param wchmm [in] tree lexicon
 * @param backtrellis [out] pointer to structure in which the resulting word trellis data should be stored
 * @param backmax [out] the maximum score of the 1st pass, or LOG_ZERO if search failed.
 * </EN>
 */
void
get_back_trellis(HTK_Param *param, WCHMM_INFO *wchmm, BACKTRELLIS *backtrellis,
LOGPROB *backmax)
{
  int t;

  /* 初期化及び t=0 を計算 */
  /* initialize and compute frame = 0 */
  get_back_trellis_init(param, wchmm, backtrellis);

  /* メインループ */
  /* main loop */
  for (
#ifdef MULTIPATH_VERSION
       /* t = 0 should be computed here */
       t = 0
#else
       /* t = 0 is already computed in get_back_trellis_init() */
       t = 1
#endif
	 ; t < param->samplenum; t++) {
    if (get_back_trellis_proceed(t, param, wchmm, backtrellis
#ifdef MULTIPATH_VERSION
				 ,FALSE
#endif
				 ) == FALSE
	|| (module_mode && module_wants_terminate())) {
      /* 探索中断: 処理された入力は 0 から t-2 まで */
      /* search terminated: processed input = [0..t-2] */
      /* この時点で第1パスを終了する */
      /* end the 1st pass at this point */
      *backmax = finalize_1st_pass(backtrellis, wchmm->winfo, t-1);
#ifdef SP_BREAK_CURRENT_FRAME
      /* ショートポーズセグメンテーションの場合,
	 入力パラメータ分割などの最終処理も行なう */
      /* When short-pause segmentation enabled */
      finalize_segment(backtrellis, param, t-1);
#endif
      /* terminate 1st pass here */
      return;
    }
  }
  /* 最終フレームを計算 */
  /* compute the last frame of input */
  get_back_trellis_end(param, wchmm, backtrellis);
  /* 第1パス終了処理 */
  /* end of 1st pass */
  *backmax = finalize_1st_pass(backtrellis, wchmm->winfo, param->samplenum);
#ifdef SP_BREAK_CURRENT_FRAME
  /* ショートポーズセグメンテーションの場合,
     入力パラメータ分割などの最終処理も行なう */
  /* When short-pause segmentation enabled */
  finalize_segment(backtrellis, param, param->samplenum);
#endif
}

/* end of file */
