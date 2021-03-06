/**
 * @file   factoring_sub.c
 * @author Akinobu LEE
 * @date   Mon Mar  7 23:20:26 2005
 * 
 * <JA>
 * @brief  言語スコアのfactoring計算
 *
 * このファイルには，第１パスにおいて言語スコアの factoring を行うための
 * 関数が含まれています．木構造化辞書上でのサブツリー内の単語リスト
 * (successor list) の構築，および認識中の言語スコア計算ルーチンが
 * 含まれます．
 *
 * successor list は，木構造化辞書の各ノードに割り付けられる，
 * そのノードを共有する単語のリストです．木構造化辞書において，
 * 枝部分の次のノードがこのリストを保持します．実際にはリストが変化する
 * 場所，すなわち木構造化辞書の枝の分岐点に割り付けられます．
 * 例えば，以下のような木構造化辞書の場合，数字の書いてあるノードに
 * successor list が割り付けられます．
 *
 *        2-o-o - o-o-o - o-o-o          word "A" 
 *       /
 *  1-o-o
 *       \       4-o-o                   word "B"
 *        \     /   
 *         3-o-o - 5-o-o - 7-o-o         word "C"
 *              \        \ 
 *               \        8-o-o          word "D"
 *                6-o-o                  word "E"
 *
 * 各 successor list はそのサブツリーに含まれる単語のリストです．
 * この例では以下のようになります．
 *
 *   node  | successor list (wchmm->state[node].sc)
 *   =======================
 *     1   | A B C D E
 *     2   | A
 *     3   |   B C D E
 *     4   |   B
 *     5   |     C D
 *     6   |         E
 *     7   |     C
 *     8   |       D
 *
 * ある successor list に含まれる単語が１つになったとき，その時点で
 * 単語が確定する．上記の場合，単語 "A" はノード 2 の位置ですでに
 * その後続単語として "A" 以外無いので，そこで確定する．
 * すなわち，単語 A の正確な言語スコアは，単語終端を待たずノード 2 で決まる．
 *
 * 第１パスにおける factoring の計算は，実際には beam.c で行なわれる．
 * 2-gram factoringの場合，次ノードに successor list が存在すれば,
 * その successor list の単語の 2-gram の最大値を求め, 伝搬してきている
 * factoring 値を更新する．successor list に単語が1つのノードでは，
 * 正しい2-gramが自動的に割り当てられる．
 * 1-gram factoringの場合，次ノードに successor list が存在する場合，
 * その successor list の単語の 1-gram の最大値を求め，伝搬してきている
 * factoring 値を更新する．successor list に単語が１つのノードで，はじめて
 * 2-gram を計算する．
 *
 * 実際では 1-gram factoring では各 successor list における factoring 値
 * は単語履歴に非依存なので，successor list 構築時に全てあらかじめ計算して
 * おく．すなわち，エンジン起動時に木構造化辞書を構築後，successor list
 * を構築したら，単語を2個以上含む successor list についてはその 1-gram の
 * 最大値を計算して，それをそのノードの fscore メンバに格納しておき，その
 * successor list は free してしまえばよい．単語が１つのみの successor list
 * についてはその単語IDを残しておき，探索時にパスがそこに到達したら
 * 正確な2-gramを計算すれば良い．
 *
 * DFA文法使用時は，デフォルトでは言語制約(カテゴリ対制約)を
 * カテゴリ単位で木を構築することで静的に表現する．このため，
 * これらの factoring 機構は用いられない．ただし，
 * CATEGORY_TREE が undefined であれば，決定的 factoring を用いた言語制約
 * 適用を行うことも可能である．
 * すなわち，次ノードに successor list が存在すれば,
 * その successor list 内の各単語と直前単語の単語対制約を調べ,
 * そのうち一つでも接続可能な単語があれば，その遷移を許し，一つも
 * なければ遷移させない．この機能は技術参考のために残されているのみである．
 * </JA>
 * 
 * <EN>
 * @brief  Build successor tree and compute LM factoring values.
 * </EN>
 *
 * This file contains functions to do language score factoring on the 1st
 * pass.  They build a successor lists which holds the successive words in
 * each sub tree on the tree lexicon, and also provide a factored LM
 * probability on each nodes on the tree lexicon.
 *
 * The "successor list" will be assigned for each lexicon tree node to
 * represent a list of words that exist in the sub-tree and share the node.
 * Actually they will be assigned to the branch node.
 * Below is the example of successor lists on a tree lexicon, in which
 * the lists is assigned to the numbered nodes.
 *
 *         2-o-o - o-o-o - o-o-o          word "A" 
 *        /
 *   1-o-o
 *        \       4-o-o                   word "B"
 *         \     /   
 *          3-o-o - 5-o-o - 7-o-o         word "C"
 *           \            \ 
 *            \            8-o-o          word "D"
 *             6-o-o                      word "E"
 *
 * The contents of the successor lists are the following:
 *
 *  node  | successor list (wchmm->state[node].sc)
 *  =======================
 *    1   | A B C D E
 *    2   | A
 *    3   |   B C D E
 *    4   |   B
 *    5   |     C D
 *    6   |         E
 *    7   |     C
 *    8   |       D
 *
 * When the 1st pass proceeds, if the next going node has a successor list,
 * all the word 2-gram scores in the successor list on the next node
 * will be computed, and the propagating LM value in the token on
 * the current node will be replaced by the maximum value of the scores
 * when copied to the next node.  Appearently, if the successor list has
 * only one word, it means that the word can be determined on that point,
 * and the precise 2-gram value will be assigned as is.
 *
 * When using 1-gram factoring, the computation will be slightly different.
 * Since the factoring value (maximum value of 1-gram scores on each successor
 * list) is independent of the word context, they can be computed statically
 * before the search.  Thus, for all the successor lists that have more than
 * two words, the maximum 1-gram value is computed and stored to
 * "fscore" member in tree lexicon, and the successor lists will be freed.
 * The successor lists with only one word should still remain in the
 * tree lexicon, to compute the precise 2-gram scores for the words.
 *
 *
 * When using DFA grammar, Julian builds separated lexicon trees for every
 * word categories, to statically express the catergory-pair constraint.
 * Thus these factoring scheme is not used by default.
 * However you can still force Julian to use the grammar-based
 * deterministic factoring scheme by undefining CATEGORY_TREE.
 * If CATEGORY_TREE is undefined, the word connection constraint will be
 * performed based on the successor list at the middle of tree lexicon.
 * This enables single tree search on Julian.  This function is left
 * only for technical reference.
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

#ifndef CATEGORY_TREE
   
/*----------------------------------------------------------------------*/

/** 
 * <JA>
 * @brief  木構造化辞書上のあるノードの successor list に単語を追加する．
 * 
 * すでに同じ単語が登録されていれば，新たに登録はされない．
 * 単語はIDで昇順に保存される．
 * 
 * @param wchmm [i/o] 木構造化辞書
 * @param node [in] ノード番号
 * @param w [in] 単語ID
 * </JA>
 * <EN>
 * @brief  Add a word to the successor list on a node in tree lexicon.
 * Words in lists should be ordered by ID.
 * 
 * @param wchmm [i/o] tree lexicon
 * @param node [in] node id
 * @param w [in] word id
 * </EN>
 */
static void
add_successor(WCHMM_INFO *wchmm, int node, WORD_ID w)
{
  S_CELL *sctmp, *sc;

  /* malloc a new successor list element */
  sctmp=(S_CELL *) mymalloc(sizeof(S_CELL));
  if (sctmp == NULL) {
    j_error("malloc fault at add_succesor(%d,%d)\n",node,w);
  }
  /* assign word ID to the new element */
  sctmp->word = w;
  /* add the new element to existing list (keeping order) */
  if (wchmm->state[node].scid == 0) {
    j_error("InternalError: sclist id not assigned to branch node?\n");
  }
  sc = wchmm->sclist[wchmm->state[node].scid];
  if (sc == NULL || sctmp->word < sc->word) {
    sctmp->next = sc;
    wchmm->sclist[wchmm->state[node].scid] = sctmp;
  } else {
    for(;sc;sc=sc->next) {
      if (sc->next == NULL || sctmp->word < (sc->next)->word) {
	if (sctmp->word == sc->word) break; /* avoid duplication */
	sctmp->next = sc->next;
	sc->next = sctmp;
	break;
      }
    }
  }
}

/** 
 * <JA>
 * ２つのノード上の successor list が一致するかどうかチェックする
 * 
 * @param wchmm [in] 木構造化辞書
 * @param node1 [in] １つめのノードID
 * @param node2 [in] ２つめのノードID
 * 
 * @return 完全に一致すれば TRUE，一致しなければ FALSE．
 * </JA>
 * <EN>
 * Check if successor lists on two nodes are the same.
 * 
 * @param wchmm [in] tree lexicon
 * @param node1 [in] 1st node id
 * @param node2 [in] 2nd node id
 * 
 * @return TRUE if they have the same successor list, or FALSE if they differ.
 * </EN>
 */
static boolean
match_successor(WCHMM_INFO *wchmm, int node1, int node2)
{
  S_CELL *sc1,*sc2;

  /* assume successor is sorted by ID */
  if (wchmm->state[node1].scid == 0 || wchmm->state[node2].scid == 0) {
    j_error("InternalError: sclist id not assigned to branch node?\n");
  }
  sc1 = wchmm->sclist[wchmm->state[node1].scid];
  sc2 = wchmm->sclist[wchmm->state[node2].scid];
  for (;;) {
    if (sc1 == NULL || sc2 == NULL) {
      if (sc1 == NULL && sc2 == NULL) {
	return TRUE;
      } else {
	return FALSE;
      }
    } else if (sc1->word != sc2->word) {
      return FALSE;
    }
    sc1 = sc1->next;
    sc2 = sc2->next;
  }
}

/** 
 * <JA>
 * 指定ノード上の successor list を空にする．
 * 
 * @param wchmm [i/o] 木構造化辞書
 * @param scid [in] node id
 * </JA>
 * <EN>
 * Free successor list at the node
 * 
 * @param wchmm [i/o] tree lexicon
 * @param scid [in] node id
 * </EN>
 */
static void
free_successor(WCHMM_INFO *wchmm, int scid)
{
  S_CELL *sc;
  S_CELL *sctmp;

  /* free sclist */
  sc = wchmm->sclist[scid];
  while (sc != NULL) {
    sctmp = sc;
    sc = sc->next;
    free(sctmp);
  }
}

/** 
 * <JA>
 * 木構造化辞書上からリンクが消された successor list について，
 * その実体を削除してリストを詰めるガーベージコレクションを行う．
 * 
 * @param wchmm [i/o] 木構造化辞書
 * </JA>
 * <EN>
 * Garbage collection of the successor list, by deleting successor lists
 * to which the link was deleted on the lexicon tree.
 * 
 * @param wchmm [i/o] tree lexiton
 * </EN>
 */
static void
compaction_successor(WCHMM_INFO *wchmm)
{
  int src, dst;

  dst = 1;
  for(src=1;src<wchmm->scnum;src++) {
    if (wchmm->state[wchmm->sclist2node[src]].scid <= 0) {
      /* already freed, skip */
      continue;
    }
    if (dst != src) {
      wchmm->sclist[dst] = wchmm->sclist[src];
      wchmm->sclist2node[dst] = wchmm->sclist2node[src];
      wchmm->state[wchmm->sclist2node[dst]].scid = dst;
    }
    dst++;
  }
  if (debug2_flag) j_printerr("  successor list shrinked from %d to %d...\n", wchmm->scnum, dst);
  wchmm->scnum = dst;
}

/** 
 * <JA>
 * successor list 用に割り付けられたメモリ領域を有効な長さに縮める．
 * 初期構築時や，1-gram factoring のために削除された successor list 分の
 * メモリを解放する．
 * 
 * @param wchmm [i/o] 木構造化辞書
 * </JA>
 * <EN>
 * Shrink the memory area that has been allocated for building successor list.
 * 
 * @param wchmm [i/o] tree lexicon
 * </EN>
 */
static void
shrink_successor(WCHMM_INFO *wchmm)
{
  wchmm->sclist = (S_CELL **)myrealloc(wchmm->sclist, sizeof(S_CELL *) * wchmm->scnum);
  wchmm->sclist2node = (int *)myrealloc(wchmm->sclist2node, sizeof(int) * wchmm->scnum);
}

/** 
 * <JA>
 * 木構造化辞書上の全ノードに successor list を構築するメイン関数
 * 
 * @param wchmm [i/o] 木構造化辞書
 * </JA>
 * <EN>
 * Main function to build whole successor list to lexicon tree.
 * 
 * @param wchmm [i/o] tree lexicon
 * </EN>
 */
void
make_successor_list(WCHMM_INFO *wchmm)
{
  int node;
  WORD_ID w;
  int i;
  boolean *freemark;
  int s;

  VERMES("  make successor lists for factoring...\n");

  /* 1. initialize */
  /* initialize node->sclist index */
  for (node=0;node<wchmm->n;node++) wchmm->state[node].scid = 0;
  /* parse tree to get the possible maximum number of successor list */
  s = 1;
  for (w=0;w<wchmm->winfo->num;w++) {
    for (i=0;i<wchmm->winfo->wlen[w];i++) {
      if (wchmm->state[wchmm->offset[w][i]].scid == 0) {
	wchmm->state[wchmm->offset[w][i]].scid = s;
	s++;
      }
    }
    if (wchmm->state[wchmm->wordend[w]].scid == 0) {
      wchmm->state[wchmm->wordend[w]].scid = s;
      s++;
    }
  }
  wchmm->scnum = s;
  if (debug2_flag) j_printerr("  initial successor list size = %d...\n", wchmm->scnum);
  /* allocate successor list */
  wchmm->sclist = (S_CELL **)mymalloc(sizeof(S_CELL *) * wchmm->scnum);
  for (i=1;i<wchmm->scnum;i++) wchmm->sclist[i] = NULL;
  wchmm->sclist2node = (int *)mymalloc(sizeof(int) * wchmm->scnum);
  /* allocate misc. work area */
  freemark = (boolean *)mymalloc(sizeof(boolean) * wchmm->scnum);
  for (i=1;i<wchmm->scnum;i++) freemark[i] = FALSE;

  /* 2. make initial successor list: assign at all possible nodes */
  for (w=0;w<wchmm->winfo->num;w++) {
    /* at each start node of phonemes */
    for (i=0;i<wchmm->winfo->wlen[w];i++) {
      wchmm->sclist2node[wchmm->state[wchmm->offset[w][i]].scid] = wchmm->offset[w][i];
      add_successor(wchmm, wchmm->offset[w][i], w);
    }
    /* at word end */
    wchmm->sclist2node[wchmm->state[wchmm->wordend[w]].scid] = wchmm->wordend[w];
    add_successor(wchmm, wchmm->wordend[w], w);
  }
  
  /* 3. erase unnecessary successor list */
  /* sucessor list same as the previous node is not needed, so */
  /* parse lexicon tree from every leaf to find the same succesor list */
  for (w=0;w<wchmm->winfo->num;w++) {
    node = wchmm->wordend[w];	/* begin from the word end node */
    i = wchmm->winfo->wlen[w]-1;
    while (i >= 0) {		/* for each phoneme start node */
      if (node == wchmm->offset[w][i]) {
	/*	printf("same:w=%d,phoneloc=%d,node=%d,%d\n", w, i, wchmm->offset[w][i], node);
	{
	  S_CELL *sc;
	  for(sc=wchmm->state[node].sc;sc;sc=sc->next) {
	    printf("%d[%s],", sc->word, ngram->wname[sc->word]);
	  }
	  printf("\n");
	  }*/
	/* word with only 1 state: skip */
	i--;
	continue;
      }
      if (match_successor(wchmm, node, wchmm->offset[w][i])) {
	freemark[wchmm->state[node].scid] = TRUE;	/* mark the node */
      }
/* 
 *	 if (freemark[wchmm->offset[w][i]] != FALSE) {
 *	   break;
 *	 }
 */
      node = wchmm->offset[w][i];
      i--;
    }
  }
  /* really free */
  for (i=1;i<wchmm->scnum;i++) {
    if (freemark[i] == TRUE) {
      free_successor(wchmm, i);
      /* reset node -> sclist link */
      wchmm->state[wchmm->sclist2node[i]].scid = 0;
    }
  }
  /* garbage collection of deleted sclist */
  compaction_successor(wchmm);

  free(freemark);

  VERMES("done\n");
}

#ifndef CATEGORY_TREE
# ifdef MULTIPATH_VERSION
/** 
 * <JA>
 * 構築された factoring 情報を multipath 用に調整する．factoring 情報を,
 * モデル全体をスキップする遷移がある場合はその先の音素へコピーする．
 * また，(出力を持たない)文頭文法ノードに単語先頭ノードからコピーする．
 * 
 * @param wchmm [in] 木構造化辞書
 * </JA>
 * <EN>
 * Adjust factoring data in tree lexicon for multipath transition handling.
 * 
 * @param wchmm [in] tree lexicon
 * </EN>
 */
void
adjust_sc_index(WCHMM_INFO *wchmm)
{
  WORD_ID w;
  int i,j,k;
  HMM_Logical *ltmp;
  int ltmp_state_num;
  int ato;
  LOGPROB prob;
  int node, scid;
  A_CELL *ac;
  
  /* duplicate scid for HMMs with more than one arc from initial state */
  for(w=0;w<wchmm->winfo->num;w++) {
    for(k=0;k<wchmm->winfo->wlen[w];k++) {
      node = wchmm->offset[w][k];
      scid = wchmm->state[node].scid;
      if (scid == 0) continue;
      ltmp = wchmm->winfo->wseq[w][k];
      ltmp_state_num = hmm_logical_state_num(ltmp);
      if ((hmm_logical_trans(ltmp))->a[0][ltmp_state_num-1] != LOG_ZERO) {
	j = k + 1;
	if (j == wchmm->winfo->wlen[w]) {
	  if (wchmm->state[wchmm->wordend[w]].scid == 0) {
	    printf("word %d: factoring node copied for skip phone\n", w);
	    wchmm->state[wchmm->wordend[w]].scid = scid;
	  }
	} else {
	  if (wchmm->state[wchmm->offset[w][j]].scid == 0) {
	    printf("word %d: factoring node copied for skip phone\n", w);
	    wchmm->state[wchmm->offset[w][j]].scid = scid;
	  }
	}
      }
      for(ato=1;ato<ltmp_state_num;ato++) {
	prob = (hmm_logical_trans(ltmp))->a[0][ato];
	if (prob != LOG_ZERO) {
	  wchmm->state[node+ato-1].scid = scid;
	}
      }
    }
  }

  /* move scid and fscore on the head state to the head grammar state */
  for(i=0;i<wchmm->startnum;i++) {
    node = wchmm->startnode[i];
    if (wchmm->state[node].out.state != NULL) {
      j_error("Error: outprob exist in word-head node??\n");
    }
    for(ac=wchmm->state[node].ac;ac;ac=ac->next) {
      if (wchmm->state[ac->arc].scid != 0) {
	if (wchmm->state[node].scid != 0 && wchmm->state[node].scid != wchmm->state[ac->arc].scid) {
	  j_error("Error: different successor list within word-head phone?\n");
	}
	wchmm->state[node].scid = wchmm->state[ac->arc].scid;
	wchmm->state[ac->arc].scid = 0;
      }
    }
  }
}

# endif	/* MULTIPATH_VERSION */
#endif /* ~CATEGORY_TREE */


/* -------------------------------------------------------------------- */
/* factoring computation */

#ifdef USE_NGRAM

/* cache should be conditioned by N-gram entry ID */

/* word-internal factoring value cache:
   a branch node (that has successor list) will keep the factoring value
   on search, and re-use it if incoming token in the next frame has the
   same word context.
 */
static LOGPROB *probcache;	///< Word-internal factoring cache indexed by scid, holding last score
static WORD_ID *lastwcache;	///< Word-internal factoring cache indexed by scid, holding last N-gram entry ID
/* cross-word factoring value cache:
   when computing cross-word transition, (1)factoring values on all word
   start nodes should be computed for each word end, and thus (2)each start
   node has more than one transition within a frame.  So factoring value
   is better cached by the word end (last word) than by nodes.
 */
/**
 * @brief  Cross-word factoring cache to hold last-word-dependent factoring
 * score toward word head nodes.
 *
 * Cached values will be stored as [last_nword][n], where n is the number of
 * word-head node on which the last_nword-dependent N-gram factoring value
 * should be computed on cross-word transition.  In 1-gram factoring,
 * n equals to wchmm->isolatenum, the number of isolated (not shared)
 * word-head nodes.
 * In 2-gram factoring, n simply equals to wchmm->startnum, the number of
 * all word-head nodes.
 *
 * The cache area will be allocated per the previous word when they appeared
 * while search.
 * It will retain across the speech stream, so the cache area will grow
 * to an extent as recognition was done for many files.
 */
static LOGPROB **iw_sc_cache;
/**
 * Maximum size of cross-word factoring cache @a iw_sc_cache per last word.
 * The value is set in max_successor_cache_init().
 */
static int iw_cache_num;
#ifdef HASH_CACHE_IW
static WORD_ID *iw_lw_cache; ///< Maps hash cache id [x] to corresponding last word
#endif
/* once initialized on startup, the factoring value caches will not be
   cleared */

/** 
 * <JA>
 * 木構造化辞書用の factoring キャッシュをメモリ割り付けして初期化する．
 * この関数はプログラム開始時に一度だけ呼ばれる．
 * 
 * @param wchmm [i/o] 木構造化辞書
 * </JA>
 * <EN>
 * Initialize factoring cache for a tree lexicon, allocating memory for
 * cache.  This should be called only once on start up.
 * 
 * @param wchmm [i/o] tree lexicon
 * </EN>
 */
void
max_successor_cache_init(WCHMM_INFO *wchmm)
{
  int i;

  /* finally shrink the memory area of successor list here */
  shrink_successor(wchmm);

  /* for word-internal */
  probcache = (LOGPROB *) mymalloc(sizeof(LOGPROB) * wchmm->scnum);
  lastwcache = (WORD_ID *) mymalloc(sizeof(WORD_ID) * wchmm->scnum);
  for (i=0;i<wchmm->scnum;i++) {
    lastwcache[i] = WORD_INVALID;
  }
  /* for cross-word */
#ifdef HASH_CACHE_IW
  iw_cache_num = wchmm->ngram->max_word_num * iw_cache_rate / 100;
  if (iw_cache_num < 10) iw_cache_num = 10;
#else
  iw_cache_num = wchmm->ngram->max_word_num;
#endif /* HASH_CACHE_IW */
  iw_sc_cache = (LOGPROB **)mymalloc(sizeof(LOGPROB *) * iw_cache_num);
  for (i=0;i<iw_cache_num;i++) {
    iw_sc_cache[i] = NULL;
  }
#ifdef HASH_CACHE_IW
  iw_lw_cache = (WORD_ID *)mymalloc(sizeof(WORD_ID) * iw_cache_num);
  for (i=0;i<iw_cache_num;i++) {
    iw_lw_cache[i] = WORD_INVALID;
  }
#endif
}

/** 
 * <JA>
 * 単語間の factoring cache のメモリ領域を解放する．
 * 
 * </JA>
 * <EN>
 * Free cross-word factoring cache.
 * 
 * </EN>
 */
static void
max_successor_prob_iw_free()
{
  int i;
  for (i=0;i<iw_cache_num;i++) {
    if (iw_sc_cache[i] != NULL) free(iw_sc_cache[i]);
    iw_sc_cache[i] = NULL;
  }
}

/** 
 * <JA>
 * factoring 用 cache のメモリ領域を全て解放する．
 * 
 * </JA>
 * <EN>
 * Free all memory for  factoring cache.
 * 
 * </EN>
 */
void
max_successor_cache_free()
{
  free(probcache);
  free(lastwcache);
  max_successor_prob_iw_free();
  free(iw_sc_cache);
#ifdef HASH_CACHE_IW
  free(iw_lw_cache);
#endif
}

#ifdef UNIGRAM_FACTORING

/** 
 * <JA>
 * @brief  単語先頭ノードのうちFactoring においてキャッシュが必要なノードの
 * リストを作成する．
 *
 * 1-gram factoring は，枝ノードにおいて直前単語に依存しない固定値
 * (unigramの最大値)を与える．このため，単語間の factoring 計算において，
 * 木構造化辞書上で複数の単語で共有されている単語先頭ノードについては，
 * その値は直前単語によらず固定値であり，認識時に単語間キャッシュを保持
 * する必要はない．
 * 
 * この関数では，単語先頭ノードのリストからそのような factoring キャッシュが
 * 不要なノードを除外して，1-gram factoring 時に単語間キャッシュが必要な
 * 単語先頭ノード（＝他の単語と共有されていない独立した単語先頭ノード）の
 * リストを作成し，wchmm->start2isolate および wchmm->isolatenum に格納する．
 * 
 * @param wchmm [i/o] 木構造化辞書
 * </JA>
 * <EN>
 * @brief  Make a list of word head nodes on which cross-word factoring cache
 * is needed.
 *
 * On 1-gram factoring, the branch nodes on tree lexicon has a fixed
 * factoring value (maximum 1-gram score of all sub-tree words).  Thus, when
 * computing cross-word factoring at word head nodes on inter-word
 * transition, such 1-gram factoring nodes on word head, shared by several
 * words, need not be cached in inter-word factoring cache.
 *
 * This function make a list of word-head nodes which requires inter-word
 * factoring caching (i.e. isolated word head nodes, does not shared by other
 * words) from the existing list of word head nodes, and set it to
 * wchmm->start2isolate and wchmm->isolatenum.
 * 
 * @param wchmm [i/o] tree lexicon
 * </EN>
 */
void
make_iwcache_index(WCHMM_INFO *wchmm)
{
  int i, node, num;

  wchmm->start2isolate = (int *)mymalloc(sizeof(int) * wchmm->startnum);
  num = 0;
  for(i=0;i<wchmm->startnum;i++) {
    node = wchmm->startnode[i];
    if (wchmm->state[node].scid >= 0) {	/* not a factoring node (isolated node, has no 1-gram factoring value) */
      wchmm->start2isolate[i] = num;
      num++;
    } else {			/* factoring node (shared) */
      wchmm->start2isolate[i] = -1;
    }
  }
  wchmm->isolatenum = num;
}

/** 
 * <JA>
 * @brief  木構造化辞書上の 1-gram factoring 値を計算して格納する．
 *
 * 1-gram factoring では単語間で共有されている枝ノードでは 1-gram の最大値
 * を与える．単語履歴によらないため，その値は認識開始前に
 * 計算しておくことができる．この関数は木構造化辞書
 * 全体について，共有されている（successor list に２つ以上の単語を持つノード）
 * ノードの 1-gram factoring 値を計算して格納する．1-gram factoring値を
 * 計算後は，そのノードの successor list はもはや不要であるため，ここで
 * 削除する．
 *
 * 実際には，factoring 値は wchmm->fscore に順次保存され，ノードの
 * scid にその保存値へのインデックス(1-)の負の値が格納される．不要になった
 * successor list は，実際には compaction_successor 内で，対応するノードの
 * scid が負になっている successor list を削除することで行なわれる．
 * 
 * @param wchmm [i/o] 木構造化辞書
 * </JA>
 * <EN>
 * @brief  Calculate all the 1-gram factoring values on tree lexicon.
 *
 * On 1-gram factoring, the shared nodes on branch has fixed factoring score
 * from 1-gram values, independent of the word context on recognition.  So
 * the values are fixed for all recognition and can be calculated before
 * search.  This function stores all the neede 1-gram factoring value by
 * traversing tree lexicon with successor lists and compute maximum 1-gram
 * for each successor lists that has more than two words (=shared).
 * Since a successor list is no more neede after the 1-gram value is computed,
 * they will be freed.
 *
 * Actually, computed factoring scores will be stored in wchmm->fscore
 * sequencially, and the index value, starting from 1,
 * to the fscore list is stored in scid of each nodes as a negative value.
 * The free will be performed in compaction_successor() by checking if a
 * successor's corresponding scid on tree lexicon has negative value.
 * 
 * @param wchmm [i/o] tree lexicon
 */
void
calc_all_unigram_factoring_values(WCHMM_INFO *wchmm)
{
  S_CELL *sc, *sctmp;
  LOGPROB tmpprob, maxprob;
  int i, n;

  /* count needed number of 1-gram factoring nodes */
  n = 0;
  for (i=1;i<wchmm->scnum;i++) {
    sc = wchmm->sclist[i];
    if (sc == NULL) {
      j_error("InternalError: sclist has no sc?\n");
    }
    if (sc->next != NULL) {
      /* more than two words, so compute maximum 1-gram probability */
      n++;
    }
  }
  wchmm->fsnum = n + 1;
  /* allocate area */
  wchmm->fscore = (LOGPROB *)mymalloc(sizeof(LOGPROB) * wchmm->fsnum);
  /* assign values */
  n = 1;
  for (i=1;i<wchmm->scnum;i++) {
    sc = wchmm->sclist[i];
    if (sc->next != NULL) {
      maxprob = LOG_ZERO;
      for (sctmp = sc; sctmp; sctmp = sctmp->next) {
	tmpprob = uni_prob(wchmm->ngram, wchmm->winfo->wton[sctmp->word])
#ifdef CLASS_NGRAM
	  + wchmm->winfo->cprob[sctmp->word] 
#endif
	  ;
	if (maxprob < tmpprob) maxprob = tmpprob;
      }
      wchmm->fscore[n] = maxprob;
      free_successor(wchmm, i);
      wchmm->state[wchmm->sclist2node[i]].scid = - n;
      n++;
    }
  }
  /* garbage collection of factored sclist */
  compaction_successor(wchmm);
}

#else  /* ~UNIGRAM_FACTORING */

/** 
 * <JA>
 * 木構造化辞書上のあるノードについて，与えられた単語履歴に対する2-gram
 * スコアを計算する．
 * 
 * @param wchmm [in] 木構造化辞書
 * @param last_nword [in] 直前単語のN-gramエントリID
 * @param node [in] @a wchmm 上のノード番号
 * 
 * @return 2-gram 確率．
 * </JA>
 * <EN>
 * Compute 2-gram factoring value for the node and return the probability.
 * 
 * @param wchmm [in] tree lexicon
 * @param last_nword [in] N-gram entry ID of the last context word
 * @param node [in] node ID on @a wchmm
 * 
 * @return the log probability of 2-gram on that node.
 * </EN>
 */
static LOGPROB
calc_successor_prob(WCHMM_INFO *wchmm, WORD_ID last_nword, int node)
{
  S_CELL *sc;
  LOGPROB tmpprob, maxprob;

  maxprob = LOG_ZERO;
  for (sc = wchmm->sclist[wchmm->state[node].scid]; sc; sc = sc->next) {
    tmpprob = bi_prob_lr(wchmm->ngram, last_nword, wchmm->winfo->wton[sc->word])
#ifdef CLASS_NGRAM
      + wchmm->winfo->cprob[sc->word]
#endif
      ;
    if (maxprob < tmpprob) maxprob = tmpprob;
  }
  return(maxprob);
}

#endif  /* ~UNIGRAM_FACTORING */

/** 
 * <JA>
 * @brief  単語内のあるノードについて factoring 値を計算する．
 *
 * 1-gram factoring で固定factoring値がある場合はその値が即座に返される．
 * 他の場合は，そのノードのサブツリー内の単語の 2-gram確率（の最大値）が
 * 計算される．
 *
 * 単語内 factoring キャッシュが考慮される．すなわち各ノードについて
 * 直前単語が前回アクセスされたときと同じであれば，
 * 前回の値が返され，そうでなければ値を計算し，キャッシュが更新される．
 * 
 * @param wchmm [in] 木構造化辞書
 * @param lastword [in] 直前単語のID
 * @param node [in] ノード番号
 * 
 * @return 言語モデルスコア
 * </JA>
 * <EN>
 * @brief  compute factoring LM score for the given word-internal node.
 *
 * If it is a shared branch node and 1-gram factoring is used, the
 * constant factoring value which has already been assigned before search
 * will be returned immediately.  Else, the maximum 2-gram probability
 * of corresponding successor words are computed.
 *
 * The word-internal factoring cache is consulted within this function.
 * If the given last word is the same as the last call on that node,
 * the last computed value will be returned, else the maximum value
 * will be computed update the cache with the last word and value.
 * 
 * @param wchmm [in] tree lexicon
 * @param lastword [in] word ID of last context word
 * @param node [in] node ID
 * 
 * @return the LM factoring score.
 * </EN>
 */
LOGPROB
max_successor_prob(WCHMM_INFO *wchmm, WORD_ID lastword, int node)
{
  LOGPROB maxprob;
  WORD_ID last_nword, w;
  int scid;

  if (lastword != WORD_INVALID) { /* return nothing if no previous word */
    last_nword = wchmm->winfo->wton[lastword];
    scid = wchmm->state[node].scid;
#ifdef UNIGRAM_FACTORING
    if (scid < 0) {
      /* return 1-gram factoring value already calced */
      return(wchmm->fscore[(- scid)]);
    } else {
      /* this node has only one successor */
      /* return precise 2-gram score */
      if (last_nword != lastwcache[scid]) {
	/* calc and cache */
	w = (wchmm->sclist[scid])->word;
	maxprob = bi_prob_lr(wchmm->ngram, last_nword, wchmm->winfo->wton[w])
#ifdef CLASS_NGRAM
	  + wchmm->winfo->cprob[w]
#endif
	  ;
	lastwcache[scid] = last_nword;
	probcache[scid] = maxprob;
	return(maxprob);
      } else {
	/* return cached */
	return (probcache[scid]);
      }
    }
#else  /* UNIGRAM_FACTORING */
    /* 2-gram */
    if (last_nword != lastwcache[scid]) {
      maxprob = calc_successor_prob(wchmm, last_nword, node);
      /* store to cache */
      lastwcache[scid] = last_nword;
      probcache[scid] = maxprob;
      return(maxprob);
    } else {
      return (probcache[scid]);
    }
#endif /* UNIGRAM_FACTORING */
  } else {
    return(0.0);
#if 0
    maxprob = LOG_ZERO;
    for (sc=wchmm->state[node].sc;sc;sc=sc->next) {
      tmpprob = uni_prob(wchmm->ngram, sc->word);
      if (maxprob < tmpprob) maxprob = tmpprob;
    }
    return(maxprob);
#endif
  }

}

#ifdef HASH_CACHE_IW
#define hashid(A) A % iw_cache_limit ///< Simple macro to compute hash id for cross-word factoring cache
#endif
/** 
 * <JA>
 * @brief  単語間の factoring 値のリストを返す．
 *
 * 与えられた直前単語に対して，factoring値を計算すべき全ての単語先頭への
 * factoring 値を計算し，そのリストを返す．このfactoring値は
 * 直前単語ごとにリスト単位でキャッシュされる．すなわち，その直前単語が
 * それまでに一度でも直前単語として出現していた場合，そのリストをそのまま
 * 返す．
 * 
 * @param wchmm [in] 木構造化辞書
 * @param lastword [in] 直前単語
 * 
 * @return 全単語先頭ノードへの factoring スコアのリスト
 * </JA>
 * <EN>
 * @brief  Compute cross-word facgtoring values for word head nodes and return
 * the list.
 *
 * Given a last word, this function compute the factoring LM scores for all
 * the word head node to which the context-dependent (not 1-gram) factoring
 * values should be computed.  The resulting list of factoring values are
 * cached within this function per the last word.
 * 
 * @param wchmm [in] tree lexicon
 * @param lastword [in] last word
 * 
 * @return the list of factoring LM scores for all the needed word-head nodes.
 * </EN>
 */
LOGPROB *
max_successor_prob_iw(WCHMM_INFO *wchmm, WORD_ID lastword)
{
  int i, j, x, node;
  int last_nword;
  WORD_ID w;

  last_nword = wchmm->winfo->wton[lastword];
#ifdef HASH_CACHE_IW
  x = hashid(last_nword);
  if (iw_lw_cache[x] == last_nword) { /* cache hit */
    return(iw_sc_cache[x]);
  }
#else  /* full cache */
  if (iw_sc_cache[last_nword] != NULL) { /* cache hit */
    return(iw_sc_cache[last_nword]);
  }
  x = last_nword;
  /* cache mis-hit, calc probs and cache them as new */
#endif
  /* allocate cache memory */
  if (iw_sc_cache[x] == NULL) {
#ifdef UNIGRAM_FACTORING
    iw_sc_cache[x] = (LOGPROB *)mymalloc(sizeof(LOGPROB)*wchmm->isolatenum);
#else
    iw_sc_cache[x] = (LOGPROB *)mymalloc(sizeof(LOGPROB)*wchmm->startnum);
#endif
    if (iw_sc_cache[x] == NULL) { /* malloc failed */
      /* clear existing cache, and retry */
      max_successor_prob_iw_free();
      j_printf("inter-word LM cache (%dMB) rehashed\n",
	       (iw_cache_num * 
#ifdef UNIGRAM_FACTORING
		wchmm->isolatenum
#else
		wchmm->startnum
#endif
		) / 1000 * sizeof(LOGPROB) / 1000);
#ifdef UNIGRAM_FACTORING
      iw_sc_cache[x] = (LOGPROB *)mymalloc(sizeof(LOGPROB)*wchmm->isolatenum);
#else
      iw_sc_cache[x] = (LOGPROB *)mymalloc(sizeof(LOGPROB)*wchmm->startnum);
#endif
      if (iw_sc_cache[x] == NULL) { /* malloc failed again? */
	j_error("max_successor_prob_iw: cannot malloc\n");
      }
    }
  }

  /* calc prob for all startid */
#ifdef UNIGRAM_FACTORING
  for (j=0;j<wchmm->startnum;j++) {
    i = wchmm->start2isolate[j];
    if (i == -1) continue;
    node = wchmm->startnode[j];
    if (wchmm->state[node].scid <= 0) {
      /* should not happen!!! below is just for debugging */
      j_error("No!!\n");
    } else {
      w = (wchmm->sclist[wchmm->state[node].scid])->word;
      iw_sc_cache[x][i] = bi_prob_lr(ngram, last_nword, wchmm->winfo->wton[w])
#ifdef CLASS_NGRAM
	+ wchmm->winfo->cprob[w]
#endif
	;
    }
  }
#else  /* ~UNIGRAM_FACTORING */
  for (i=0;i<wchmm->startnum;i++) {
    node = wchmm->startnode[i];
    iw_sc_cache[x][i] = calc_successor_prob(wchmm, last_nword, node);
  }
#endif
#ifdef HASH_CACHE_IW
  iw_lw_cache[x] = last_nword;
#endif

  return(iw_sc_cache[x]);
}


#else /* USE_DFA --- カテゴリ木なら不必要 */

/** 
 * <JA>
 * @brief  文法による単語内決定的 factoring
 *
 * Julian において CATEGORY_TREE が定義されているとき（デフォルト），
 * 木構造化辞書はカテゴリ単位（すなわち構文制約の記述単位）で構築されるため，
 * 第1パスでの言語モデルであるカテゴリ対制約は単語の始終端で適用できる．
 * 
 * この CATEGORY_TREE が定義されていない場合，木構造化辞書は
 * 辞書全体で単一の木が作られるため，カテゴリ対制約は N-gram (Julius) と
 * 同様に単語内で factoring と同様の機構で適用される必要がある．
 *
 * この関数は CATEGORY_TREE が定義されていないときに，上記の factoring
 * （決定的 factoring と呼ばれる）を行なうために提供されている．
 * 
 * @param wchmm [in] 木構造化辞書
 * @param lastword [in] 直前単語
 * @param node [in] ノード番号
 *
 * @return カテゴリ制約上その枝への遷移が許されれば TRUE, 不可能であれば FALSE
 * </JA>
 * <EN>
 * @brief  Deterministic factoring for grammar-based recognition (Julian)
 *
 * If CATEGORY_TREE is defined (this is default) on Julian, the tree lexicon
 * will be organized per category and the category-pair constraint used
 * in the 1st pass can be applied statically at cross-word transition.
 *
 * If the CATEGORY_TREE is not defined, a single tree lexicon will be
 * constucted for a whole dictionary.  In this case, the category-pair
 * constraint should be applied dynamically in the word-internal transition,
 * like the factoring scheme with N-gram (Julius).
 *
 * This function provides such word-internal factoring for grammar-based
 * recognition (called deterministic factoring) when CATEGORY_TREE is
 * undefined in Julian.
 * 
 * @param wchmm [in] tree lexicon
 * @param lastword [in] last word
 * @param node [in] node ID to check the constraint
 * 
 * @return TRUE if the transition to the branch is allowed on the category-pair
 * constraint, or FALSE if not allowed.
 * </EN>
 */
boolean
can_succeed(WCHMM_INFO *wchmm, WORD_ID lastword, int node)
{
  int lc;
  S_CELL *sc;

  /* return TRUE if at least one subtree word can connect */

  if (lastword == WORD_INVALID) { /* case at beginning-of-word */
    for (sc=wchmm->sclist[wchmm->state[node].scid];sc;sc=sc->next) {
      if (dfa_cp_begin(wchmm->dfa, sc->word) == TRUE) return(TRUE);
    }
    return(FALSE);
  } else {
    lc = winfo->wton[lastword];
    for (sc=wchmm->sclist[wchmm->state[node].scid];sc;sc=sc->next) {
      if (dfa_cp(wchmm->dfa, lc, sc->word) == TRUE) return(TRUE);
    }
    return(FALSE);
  }
}

#endif /* USE_DFA */


#endif /* CATEGORY_TREE */
