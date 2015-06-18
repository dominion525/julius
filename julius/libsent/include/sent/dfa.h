/**
 * @file   dfa.h
 * @author Akinobu LEE
 * @date   Thu Feb 10 18:21:27 2005
 *
 * <EN>
 * @brief  Definitions for DFA grammar and category-pair information
 *
 * This file includes definitions for a finite state grammar called DFA.
 *
 * DFA is a deterministic finite state automaton describing grammartical
 * constraint, using the category number of each dictionary word as an input.
 * It also holds lists of words belonging for each categories.
 *
 * Additionaly, the category-pair information will be generated from the given
 * DFA by extracting allowed connections between categories.  It will be used
 * as a degenerated constraint of word connection at the 1st pass.
 * </EN>
 * <JA>
 * @brief 決定性有限状態オートマトン文法(DFA)およびカテゴリ対情報の構造体定義
 *
 * このファイルには, DFAと呼ばれる有限状態文法の構造体が定義されています．
 *
 * DFAは, 単語のカテゴリ番号を入力とする決定性オートマトンで,構文制約を
 * 表現します．カテゴリごとの単語リストも保持します．
 *
 * また，第１パスの認識のために,DFAカテゴリ間の接続関係のみを抜き出した
 * 単語対情報も保持します．これは文法を読みだし後に内部でDFAから抽出されます．
 * </JA>
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

#ifndef __SENT_DFA_H__
#define __SENT_DFA_H__

#include <sent/stddefs.h>

#define DFA_STATESTEP 1000	///< Allocation step of DFA state

#define INITIAL_S 0x10000000	///< Status flag mask specifying an initial state
#define ACCEPT_S  0x00000001	///< Status flag mask specifying an accept state

/// Transition arc of DFA
typedef struct _dfa_arc {
  short label;                  ///< Input(=category ID) corresponding to this arc
  int to_state;			///< Next state to move
  struct _dfa_arc *next;	///< Pointer to the next arc in the same state, NULL if last
} DFA_ARC;

/// State of DFA
typedef struct {
  int      	number;		///< Unique ID
  unsigned int	status;		///< Status flag
  DFA_ARC	*arc;		///< Pointer to its arc list, NULL if has no arc
} DFA_STATE;

/// Information of each terminal symbol (=category)
typedef struct {
  int term_num;			///< Total number of category
  WORD_ID **tw;			///< Word lists in each category as @c [c][0..wnum[c]-1]
  int *wnum;			///< Number of words in each category
} TERM_INFO;

/// Top structure of a DFA
typedef struct {
  DFA_STATE *st;		///< Array of all states
  int maxstatenum;		///< Number of maximum allocated states
  int state_num;		///< Total number of states actually defined
  int arc_num;			///< Total number of arcs
  int term_num;			///< Total number of categories
  /**
   * Category-pair constraint is stored in bit, i.e.,
   * @code
   * cp[c1][c2] -> (c2%8)th bit on cp[c1][c2/8]
   * cp_begin[c2] -> (c2%8)th bit on cp_begin[c2/8]
   * cp_end[c2] -> (c2%8)th bit on cp_end[c2/8]
   * @endcode
   * If bit is 1, the combination is allowed to connect.
   */
  unsigned char **cp;           ///< Store constraint whether @c c2 can follow @c c1
  unsigned char *cp_begin;      ///< Store constraint whether @c c can appear at beginning of sentence
  unsigned char *cp_end;	///< Store constraint whether @c c can appear at end of sentence
  unsigned char *cp_root;	///< Root pointer of @c cp informations
  TERM_INFO term;		///< Information of terminal symbols (category)
  boolean *is_sp;		///< TRUE if the category contains only \a sp word
  WORD_ID sp_id;		///< Word ID of short pause word
} DFA_INFO;


DFA_INFO *dfa_info_new();
void dfa_info_free(DFA_INFO *dfa);
void dfa_state_init(DFA_INFO *dinfo);
void dfa_state_expand(DFA_INFO *dinfo, int needed);
boolean rddfa(FILE *fp, DFA_INFO *dinfo);
boolean rddfa_fd(int fd, DFA_INFO *dinfo);
boolean rddfa_sd(int sd, DFA_INFO *dinfo);
boolean rddfa_line(char *line, DFA_INFO *dinfo, int *state_max, int *arc_num, int *terminal_max);
void dfa_append(DFA_INFO *dst, DFA_INFO *src, int soffset, int coffset);

void init_dfa(DFA_INFO *dinfo, char *filename);
WORD_ID dfa_symbol_lookup(DFA_INFO *dinfo, char *terminalname);
void extract_cpair(DFA_INFO *dinfo);
void cpair_append(DFA_INFO *dst, DFA_INFO *src, int coffset);
void print_dfa_info(DFA_INFO *dinfo);
void print_dfa_cp(DFA_INFO *dinfo);
boolean dfa_cp(DFA_INFO *dfa, int i, int j);
boolean dfa_cp_begin(DFA_INFO *dfa, int i);
boolean dfa_cp_end(DFA_INFO *dfa, int i);
void set_dfa_cp(DFA_INFO *dfa, int i, int j, boolean value);
void set_dfa_cp_begin(DFA_INFO *dfa, int i, boolean value);
void set_dfa_cp_end(DFA_INFO *dfa, int i, boolean value);
void init_dfa_cp(DFA_INFO *dfa);
void malloc_dfa_cp(DFA_INFO *dfa, int term_num);
void realloc_dfa_cp(DFA_INFO *dfa, int old_term_num, int new_term_num);
void free_dfa_cp(DFA_INFO *dfa);



#include <sent/vocabulary.h>
void make_dfa_voca_ref(DFA_INFO *dinfo, WORD_INFO *winfo);
void make_terminfo(TERM_INFO *tinfo, DFA_INFO *dinfo, WORD_INFO *winfo);
void terminfo_append(TERM_INFO *dst, TERM_INFO *src, int coffset, int woffset);
#include <sent/htk_hmm.h>
void dfa_find_pause_word(DFA_INFO *dfa, WORD_INFO *winfo, HTK_HMM_INFO *hmminfo);
void dfa_pause_word_append(DFA_INFO *dst, DFA_INFO *src, int coffset);

#endif /* __SENT_DFA_H__ */
