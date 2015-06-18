/**
 * @file   ngram2.h
 * @author Akinobu LEE
 * @date   Fri Feb 11 15:04:02 2005
 *
 * <EN>
 * @brief Definitions for word N-gram
 *
 * This file defines a structure for word N-gram language model.
 *
 * Julius uses left-to-right word bigram and reversed (right-to-left)
 * trigram.  Two input file format of ARPA standard format and Julius
 * Binary format is supported.  When using the ARPA format for recognition,
 * the bigram file and reverse trigram file should be specified separately,
 * and their coherence will be checked by Julius.  When using the binary
 * format, the two models are gathered in one file, and the data loading
 * will be much faster than ARPA format.  Model in either format will be
 * stored in the same structure NGRAM_INFO.
 *
 * @sa mkbingram
 *
 * For memory efficiency of holding the huge word N-gram on memory, Julius
 * merges the two language model into one structure.  So the forward bigram and
 * reverse trigram should meet the following requirements:
 *
 *     - their vocabularies should be the same.
 *     - their unigram probabilities of each word should be the same.
 *     - the same bigram tuple sets are defined.
 *     - the bigram tuples for context word sequences of existing trigram
 *     tuples should exist in both.
 * 
 *  The first three requirements can be fullfilled easily if you train the
 *  forward bigram and reverse trigram on the same training text.
 *  The last condition can be qualified if you set a cut-off value of trigram
 *  which is larger or equal to that of bigram.  These conditions are checked
 *  when Julius or mkbingram reads in the ARPA models, and output error if
 *  not cleared.
 *
 *  From 3.5, tuple ID on 2-gram changed from 32bit to 24bit, and 2-gram
 *  back-off weights will not be saved if the corresponding 3-gram is empty.
 *  They will be performed when reading N-gram to reduce memory size.
 * </EN>
 * <JA>
 * @brief 単語N-gram言語モデルの定義
 *
 * このファイルには単語N-gram言語モデルを格納するための構造体定義が
 * 含まれています．
 *
 * Julius では，前向き2-gramと後向き3-gram を用います．入力ファイル形式は
 * ARPA形式とJulius独自のバイナリ形式の２つをサポートしています．
 * 前者の場合，前向き2-gram と後向き3-gram をそれぞれ別々のファイルとして
 * 指定します．後者の場合，それらが統合された１つのバイナリファイルを
 * 読み込みます．読み込みは後者のほうが高速です．なお，Julius 内部では，
 * どちらも同じ構造体 NGRAM_INFO に格納されます．
 *
 * @sa mkbingram
 *
 * NGRAM_INFO ではメモリ量節約のため，これらを一つの構造体で表現しています．
 * このことから，Julius は使用するこれら２つの言語モデルが
 * 以下を満たすことを要求します．
 * 
 *    - 語彙が同一であること
 *    - 各語彙の1-gram確率が同一であること
 *    - 同じ 2-gram tuple 集合が定義されていること
 *    - 3-gram のコンテキストである単語組の2-gramが定義されていること
 *
 * 上記の前提のほとんどは，これらの２つのN-gramを同一のコーパスから
 * 学習することで満たされます．最後の条件については，3-gram のカットオフ
 * 値に 2-gram のカットオフ値と同値かそれ以上の値を指定すればOKです．
 * 与えられたN-gramが上記を満たさない場合，Julius はエラーを出します．
 * </JA>
 * 
 * $Revision: 1.6 $
 * 
 */
/*
 * Copyright (c) 1991-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

/***** revision 3 *****/
/* ngram2.h .. n-gram language model for speech recognition */
/* Third revision: sequencial allocation */
/* for disk-LM and rapid read-in */

#ifndef __SENT_NGRAM2_H__
#define __SENT_NGRAM2_H__

#include <sent/stddefs.h>
#include <sent/ptree.h>


/// Maximum number of N (now fixed to trigram)
#define MAX_N 3

typedef unsigned char NNID_UPPER; ///< Type definition for N-gram word ID
typedef unsigned short NNID_LOWER; ///< Type definition for N-gram word ID
typedef int NNID;	       ///< Type definition for N-gram word ID
#define NNID_INVALID -1		///< Value to indicate no id
#define NNID_INVALID_UPPER 255	///< Value to indicate no id at NNID_UPPER
#define NNIDMAX 16711680        ///< Allowed maximum number of NNID (255*65536)

/**
 * @brief Main N-gram structure
 *
 * bigrams and trigrams are stored in the form of sequential lists.
 * They are grouped by the same context, and referred from the
 * context ((N-1)-gram) data by the beginning ID and its number.
 * 
 */
typedef struct {
  int version;			///< version number
  boolean from_bin;		///< TRUE if source is bingram, otherwise ARPA
  WORD_ID max_word_num;		///< N-gram vocabulary size
  NNID ngram_num[MAX_N];	///< Total number of tuples for each N
  NNID bigram_bo_num;		///< Total number of bigram tuples that has back-off weight (i.e. context of upper 3-gram) (v4)
  /**
   * @brief Unknown word ID
   *
   * This value is always fixed to 0, since the CMU-Cambridge SLM Toolkit
   * always define the unknown word "<UNK>" at the first word in vocabulary.
   * @sa set_unknown_id
   */
  WORD_ID unk_id;
  int unk_num;			///< Number of dictionary words that are not in this N-gram vocabulary
  LOGPROB unk_num_log;		///< Log10 value of @a unk_num, used for calculating probability of unknown words
  boolean isopen;		///< TRUE if dictionary has unknown words, which does not appear in this N-gram

  /* basic data (nid: 0 - max_word_num-1) */
  char **wname;			///< List of word string [nid]
  PATNODE *root;		///< Root of index tree to search n-gram word ID from its name
  
  /* 1-gram ( nid: 0 - ngram_num[0]-1 ) */
  LOGPROB *p;			///< 1-gram log probabilities [nid]
  LOGPROB *bo_wt_lr;		///< Back-off weights for LR 2-gram [nid]
  LOGPROB *bo_wt_rl;		///< Back-off weights for RL 2-gram [nid]
  NNID *n2_bgn; ///< 2-gram IDs (n2) representing beginning point of 2-gram entries that have the left context
  WORD_ID *n2_num;		///< Number of 2-gram that have the left context of above
  
  /* 2-gram (n2: 0 - ngram_num[1] - 1) */
  WORD_ID *n2tonid;		///< Mapping each 2-gram index ID (n2) to its last word ID (nid)
  LOGPROB *p_lr;		///< LR 2-gram log probabilities [n2]
  LOGPROB *p_rl;		///< RL 2-gram log probabilities [n2]
  NNID_UPPER *n2bo_upper; ///< Mapping each 2-gram index ID (n2) to bigram back-off index (n2-bo) (v4)
  NNID_LOWER *n2bo_lower; ///< Mapping each 2-gram index ID (n2) to bigram back-off index (n2-bo) (v4)
  
  /* 2-gram back-off values (separated from  rev.3.5) */
  LOGPROB *bo_wt_rrl;		///< Back-off weights for RL 3-gram [n2-bo]
  NNID *n3_bgn;			///< 3-gram IDs (n3) representing beginning point of 3-gram entries that have the left context (v3)
  NNID_UPPER *n3_bgn_upper; ///< upper 8-bit 3-gram IDs (n3) representing beginning point of 3-gram entries that have the left context (v4)
  NNID_LOWER *n3_bgn_lower; ///< lower 16-bit 3-gram IDs (n3) representing beginning point of 3-gram entries that have the left context (v4)
  WORD_ID *n3_num;		///< Number of 3-gram that have the left context of above
  
  /* 3-gram (n3: 0 - ngram_num[2] - 1) */
  WORD_ID *n3tonid;		///< Mapping each 3-gram index ID (n3) to its last word ID (nid)
  LOGPROB *p_rrl;		///< RL 3-gram log probabilities [n3]
} NGRAM_INFO;


/* Definitions for binary N-gram */

/// Header string to identify version of bingram (v3: <= rev.3.4.2)
#define BINGRAM_IDSTR "julius_bingram_v3"
/// Header string to identify version of bingram (v4: >= rev.3.5)
#define BINGRAM_IDSTR_V4 "julius_bingram_v4"
/// Bingram header size in bytes
#define BINGRAM_HDSIZE 512
/// Bingram header info string to identify the unit byte (head)
#define BINGRAM_SIZESTR_HEAD "word="
/// Bingram header string that indicates 4 bytes unit
#define BINGRAM_SIZESTR_BODY_4BYTE "4byte(int)"
/// Bingram header string that indicates 2 bytes unit
#define BINGRAM_SIZESTR_BODY_2BYTE "2byte(unsigned short)"
#ifdef WORDS_INT
#define BINGRAM_SIZESTR_BODY BINGRAM_SIZESTR_BODY_4BYTE
#else
#define BINGRAM_SIZESTR_BODY BINGRAM_SIZESTR_BODY_2BYTE
#endif
/// Bingram header info string to identify the byte order (head) (v4)
#define BINGRAM_BYTEORDER_HEAD "byteorder="
/// Bingram header info string to identify the byte order (body) (v4)
#ifdef WORDS_BIGENDIAN
#define BINGRAM_NATURAL_BYTEORDER "BE"
#else
#define BINGRAM_NATURAL_BYTEORDER "LE"
#endif


/* function declaration */
NNID search_bigram(NGRAM_INFO *ndata, WORD_ID w_l, WORD_ID w_r);
// NNID search_trigram(NGRAM_INFO *ndata,  NNID n2, WORD_ID wkey);
LOGPROB uni_prob(NGRAM_INFO *ndata, WORD_ID w);
LOGPROB bi_prob_lr(NGRAM_INFO *ndata, WORD_ID w1, WORD_ID w2);
LOGPROB bi_prob_rl(NGRAM_INFO *ndata, WORD_ID w1, WORD_ID w2);
LOGPROB tri_prob_rl(NGRAM_INFO *ndata, WORD_ID w1, WORD_ID w2, WORD_ID w3);

boolean ngram_read_arpa(FILE *fp, NGRAM_INFO *ndata, int direction);
void set_unknown_id(NGRAM_INFO *ndata);
boolean ngram_read_bin(FILE *fp, NGRAM_INFO *ndata);
boolean ngram_write_bin(FILE *fp, NGRAM_INFO *ndata, char *header_str);

void ngram_make_lookup_tree(NGRAM_INFO *ndata);
WORD_ID ngram_lookup_word(NGRAM_INFO *ndata, char *wordstr);
WORD_ID make_ngram_ref(NGRAM_INFO *, char *);

NGRAM_INFO *ngram_info_new();
void ngram_info_free(NGRAM_INFO *ngram);
void init_ngram_bin(NGRAM_INFO *ndata, char *ngram_file);
void init_ngram_arpa(NGRAM_INFO *ndata, char *lrfile, char *rlfile);

void ngram_compact_bigram_context(NGRAM_INFO *ndata);

void print_ngram_info(NGRAM_INFO *ndata);

#include <sent/vocabulary.h>
void make_voca_ref(NGRAM_INFO *ndata, WORD_INFO *winfo);

#endif /* __SENT_NGRAM2_H__ */
