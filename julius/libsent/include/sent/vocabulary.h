/**
 * @file   vocabulary.h
 * @author Akinobu LEE
 * @date   Sat Feb 12 12:38:13 2005
 *
 * <JA>
 * @brief  単語辞書の構造体定義
 *
 * このファイルは認識で用いられる単語辞書を定義します．単語辞書は単語の読み，
 * 出力文字列，音素列の他に，文の開始・終了単語や透過単語情報も保持します．
 * 
 * N-gramに出現する語彙の辞書は NGRAM_INFO に格納され，この認識用単語辞書とは
 * 区別されることに注意して下さい．単語辞書からN-gramの語彙へのマッピングは
 * WORD_INFO 内の wton[] によって行なわれます．またDFAの場合，wton は
 * その単語が属するDFA_INFO 内のカテゴリ番号を含みます．
 * </JA>
 * <EN>
 * @brief  Word dictionary for recognition
 *
 * This file defines data structure for word dictionary used in recognition.
 * It stores word's string, output string, phoneme sequence, transparency.
 * Beginning-of-sentence word and End-of-sentence word guessed from runtime
 * environment is also stored here.
 *
 * Please note that the N-gram vocabulary is stored in NGRAM_INFO and it
 * can differ from this word dictionary.  The reference from the word
 * dictionary to a N-gram vocabulary is done by wton[] member in WORD_INFO.
 * When used with DFA, the wton[] holds a category number to which each word
 * belongs.
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

#ifndef __SENT_VOCA_H__
#define __SENT_VOCA_H__

#include <sent/stddefs.h>
#include <sent/htk_hmm.h>

/// Default word string of beginning-of-sentence word
#define BEGIN_WORD_DEFAULT "<s>"
/// Default word string of end-of-sentence word
#define END_WORD_DEFAULT "</s>"

/// Memory allocation step in number of words when loading a word dictionary
#define	MAXWSTEP 4000

/// Word dictionary structure to hold vocabulary
typedef struct {
  WORD_ID	maxnum;		///< Allocated number of word space
  WORD_ID	num;		///< Number of words
  WORD_ID	errnum;		///< Number of error words that were skipped when reading dictionary
  unsigned char	*wlen;		///< Number of phonemes for each word [wid]
  
  char		**wname;	///< Word name string for each word [wid].  With DFA, it's category ID.  With N-gram, it's N-gram entry name.
  char		**woutput;	///< Word output string that will be output as recognition result for each word [wid]
  HMM_Logical   ***wseq;	///< Phone sequence of each word [wid][0..wlen[wid]-1]
  WORD_ID	*wton;		///< Reference to N-gram/category ID of each word ID [wid]
#ifdef CLASS_NGRAM
  LOGPROB	*cprob;		///< Class probability of each word [wid]
  WORD_ID	cwnum;		///< Number of words whose class prob is specified (just for a statistic)
#endif
  WORD_ID	head_silwid;	///< Word ID of beginning-of-sentence silence
  WORD_ID	tail_silwid;	///< Word ID of end-of-sentence silence
  short		maxwn;		///< Maximum number of %HMM states per word (statistic)
  short         maxwlen;	///< Maximum number of phones in a word (statistic)
  boolean	*is_transparent; ///< TRUE if the word can be treated as transparent [wid]
  APATNODE	*errph_root; ///< Root node of index tree for gathering error %HMM name appeared when reading the dictionary 
  BMALLOC_BASE *mroot;		///< Pointer for block memory allocation
} WORD_INFO;


WORD_INFO *word_info_new();
void word_info_free(WORD_INFO *winfo);
void winfo_init(WORD_INFO *winfo);
void winfo_expand(WORD_INFO *winfo);
boolean init_voca(WORD_INFO *winfo, char *filename, HTK_HMM_INFO *hmminfo, boolean, boolean);
boolean voca_load_htkdict(FILE *, WORD_INFO *, HTK_HMM_INFO *, boolean);
boolean	voca_load_htkdict_fd(int, WORD_INFO *, HTK_HMM_INFO *, boolean);
boolean	voca_load_htkdict_sd(int, WORD_INFO *, HTK_HMM_INFO *, boolean);
boolean voca_append_htkdict(char *entry, WORD_INFO *winfo, HTK_HMM_INFO *hmminfo, boolean ignore_tri_conv);
void voca_append(WORD_INFO *dstinfo, WORD_INFO *srcinfo, int coffset, int woffset);

boolean voca_load_htkdict_line(char *buf, int vnum, WORD_INFO *winfo, HTK_HMM_INFO *hmminfo, boolean do_conv, boolean *ok_flag);
boolean voca_mono2tri(WORD_INFO *winfo, HTK_HMM_INFO *hmminfo);
WORD_ID voca_lookup_wid(char *, WORD_INFO *);
WORD_ID *new_str2wordseq(WORD_INFO *, char *, int *);
char *cycle_triphone(char *p);
char *cycle_triphone_flush();
void print_voca_info(WORD_INFO *);
void put_voca(WORD_INFO *winfo, WORD_ID wid);
void put_voca_err(WORD_INFO *winfo, WORD_ID wid);

/* HMMList check */
void make_base_phone(HTK_HMM_INFO *hmminfo, WORD_INFO *winfo);
void print_phone_info(HTK_HMM_INFO *hmminfo);
void print_all_basephone_detail(HMM_basephone *base);
void print_all_basephone_name(HMM_basephone *base);
void test_interword_triphone(HTK_HMM_INFO *hmminfo, WORD_INFO *winfo);

#endif /* __SENT_VOCA_H__ */
