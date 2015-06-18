/**
 * @file   search.h
 * @author Akinobu Lee
 * @date   Wed Sep 07 07:40:11 2005
 * 
 * <JA>
 * @brief  ��2�ѥ��ǻ��Ѥ��벾�����򰷤���¤��
 *
 * �����Ǥϡ���2�ѥ��Υ����å��ǥ����ǥ��󥰤��Ѥ����벾�����ι�¤��
 * ���������Ƥ��ޤ���NODE ����ʬʸ������ݻ�������ץ������丽�ߤ�Viterbi
 * �����������쥹�����������٥����������ꤵ�줿��ü�ե졼��ʤɤ��͡��ʲ���
 * ������ݻ����ޤ���WordGraph ��ñ�쥰����������˥�������ñ��򤢤�魯
 * �Τ��Ѥ����ޤ���NEXTWORD ��ñ��Ÿ�����˼�ñ������ɽ�����ޤ���POPNODE
 * ��õ�����ֲĻ벽��ǽ (--enable-visualize) ������ˡ�õ���β�����Ĥ��Ƥ���
 * �Τ˻Ȥ��ޤ���
 * </JA>
 * 
 * <EN>
 * @brief  Strucures for handling hypotheses on the 2nd pass.
 * </EN>
 *
 * This file includes definitions for handling hypothesis used on the 2nd
 * pass stack decoding.  Partial sentence hypotheses are stored in NODE
 * structure, with its various information about total scores, viterbi scores,
 * language scores, confidence scores, estimated end frame, and so on.
 * WordGraph express a word in graph, generated through the 2nd pass.
 * NEXTWORD is used to hold next word information at
 * hypothesis expantion stage. POPNODE will be used when Visualization is
 * enabled to store the search trail.
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

#ifndef __SENT_SEARCH_H__
#define __SENT_SEARCH_H__

/**
 * <JA>
 * ��2�ѥ��μ�ñ����䡥���벾�⤫�鼡����³������ñ��ν���򤢤�魯�Τ�
 * �Ѥ����롥
 * </JA>
 * <EN>
 * Next word candidate in the 2nd pass.  This will be used to hold word
 * candidates that can be connected to a given hypothesis.
 * </EN>
 */
typedef struct __nextword__ {
  WORD_ID id;			///< Word ID
#ifdef USE_NGRAM
  LOGPROB lscore;		///< Language score of this word
#else  /* USE_DFA */
  int next_state;		///< Next DFA grammar state ID
  boolean can_insert_sp;	///< TRUE if a short pause can insert between source hypothesis and this word
#endif
  TRELLIS_ATOM *tre;		///< Pointer to the corresponding word in trellis
} NEXTWORD;

#ifdef VISUALIZE
/**
 * <JA>
 * �Ļ벽��ǽ�Ѥˡ���2�ѥ���pop���줿�ȥ�ꥹñ��ξ�����ݻ����롥
 * </JA>
 * <EN>
 * Store popped trellis words on the 2nd pass for visualization.
 * </EN>
 */
typedef struct __popnode__ {
  TRELLIS_ATOM *tre;		///< Last referred trellis word
  LOGPROB score;		///< Total score when expanded (g(x)+h(x))
  struct __popnode__ *last;	///< Link to previous word context
  struct __popnode__ *next;	///< List pointer to next data
} POPNODE;
#endif /* VISUALIZE */

#ifdef GRAPHOUT
#define FANOUTSTEP 7		///< Memory allocation step for connection words in WordGraph

/**
 * <JA>
 * ñ�쥰��վ��ñ����䡥
 * </JA>
 * <EN>
 * Word arc on the word graph.
 * </EN>
 */
typedef struct __word_graph__ {
  WORD_ID wid;			///< Word ID
  int lefttime;			///< Head frame where this word begins
  int righttime;		///< Tail frame where this word ends
  LOGPROB fscore_head;		///< Partial sentence score 'f' when the next (left) word of this word was expanded at 2nd pass.  f = g(thisword) + h(nextword)
  LOGPROB fscore_tail;		///< Partial sentence score when this word was expanded in 2nd pass.  f = g(rightword) + h(thisword)
  LOGPROB gscore_head;		///< Accumulated viterbi score at the head state of this word on lefttime.  This value includes both accumulated AM score and LM score of this word.
  LOGPROB gscore_tail;		///< Accumultaed viterbi score at the head state of previous (right) word.
#ifdef USE_NGRAM
  LOGPROB lscore;		///< (Scaled) language score + penalty
#endif
#ifdef CM_SEARCH
  LOGPROB cmscore;		///< Confidence score obtained while search
#endif
  HMM_Logical *headphone;	///< Applied phone HMM at the head of the word 
  HMM_Logical *tailphone;	///< Applied phone HMM at the end of the word 
  struct __word_graph__ **leftword; ///< List of left context
  int leftwordnum;		///< Stored num of @a leftword
  int leftwordmaxnum;		///< Allocated size of @a leftword
  struct __word_graph__ **rightword; ///< List of right context
  int rightwordnum;		///< Stored num of @a leftword
  int rightwordmaxnum;		///< Allocated size of @a letfword
  struct __word_graph__ *next;	///< Pointer to next wordgraph for throughout access
  boolean mark;			///< Delete mark for compaction operation
  int id;			///< Unique ID within the graph
  boolean saved;		///< Save mark for graph generation
#ifdef GRAPHOUT_DYNAMIC
  boolean purged;		///< Purged mark for graph generation
#endif
} WordGraph;
#endif /* GRAPHOUT */

/**
 * <JA>
 * ��2�ѥ���ʸ����
 * </JA>
 * <EN>
 * Sentence hypothesis at 2nd pass
 * </EN>
 */
typedef struct __node__ {
  struct __node__    *next;	///< Link to next hypothesis, used in stack
  struct __node__    *prev;	///< Link to previous hypothesis, used in stack
  boolean endflag;              ///< TRUE if this is a final sentence result
  WORD_ID seq[MAXSEQNUM];	///< Word sequence
  short seqnum;                 ///< Length of @a seq
  LOGPROB score;		///< Total score (forward+backward, LM+AM)
  short bestt;                  ///< Best connection frame of last word in word trellis
  short estimated_next_t;	///< Estimated next connection time frame (= beginning of last word on word trellis): next word hypothesis will be looked up near this frame on word trellis
  LOGPROB *g;			///< Current forward viterbi score in each frame
#ifdef MULTIPATH_VERSION
  LOGPROB final_g;		///< Extra forward score on end of frame
#endif
#ifdef USE_DFA
  int state;			///< Current DFA state ID
#endif
  TRELLIS_ATOM *tre;		///< Trellis word of last word
  
#ifndef PASS2_STRICT_IWCD
  /* for inter-word context dependency, the last phone on previous word
     need to be calculated later */
  LOGPROB *g_prev;		///< Viterbi score back to last 1 phoneme
#endif
  HMM_Logical *last_ph;		///< Last applied triphone
#ifdef MULTIPATH_VERSION
  boolean last_ph_sp_attached;  ///< Last phone which the inter-word sp has been attached
#endif
#ifdef USE_NGRAM
  LOGPROB lscore;		///< N-gram score of last word (will be used for 1-phoneme backscan and graph output
  LOGPROB totallscore;		///< Accumulated language score (LM only)
#endif /* USE_NGRAM */
#ifdef CONFIDENCE_MEASURE
#ifdef CM_MULTIPLE_ALPHA
  LOGPROB cmscore[MAXSEQNUM][100]; ///< Confidence score of each word (multiple)
#else
  LOGPROB cmscore[MAXSEQNUM];	///< Confidence score of each word
#endif /* CM_MULTIPLE_ALPHA */
#endif /* CONFIDENCE_MEASURE */
#ifdef VISUALIZE
  POPNODE *popnode;		///< Pointer to last popped node 
#endif
#ifdef GRAPHOUT
#ifdef GRAPHOUT_PRECISE_BOUNDARY
  short *wordend_frame;		///< Buffer to store propagated word end score for word boundary adjustment
  LOGPROB *wordend_gscore;	///< Buffer to store propagated scores at word end for word boundary adjustment
#endif
  WordGraph *prevgraph;		///< Graph word corresponding to the last word
  WordGraph *lastcontext;	///< Graph word of next previous word
#ifndef GRAPHOUT_PRECISE_BOUNDARY
  LOGPROB tail_g_score;		///< forward g score for later score adjustment
#endif
#endif /* GRAPHOUT */
} NODE;

/*
  HOW SCORES ARE CALCULATED:
  
              0         bestt                            T-1
              |-h(n)---->|<------------g(n)--------------|
==============================================================
              |\                                         |
	      .....                                  .....  
                                                            <word trellis>
              |    \estimated_next_t                     |  =backward trellis
--------------------\------------------------------------|  (1st pass)
              |      \                                   |   
seq[seqnum-1] |       \_                                 |   
              |         \bestt                           |   
=========================+====================================================
              |           \                              |<-g[0..T-1]
              |            \                             |   
seq[seqnum-2] |             \__                          |
              |                \                         |
--------------------------------\------------------------|
     (last_ph)|                  \__                     |
              |_ _ _ _ _ _ _ _ _ _ _\ _ _ _ _ _ _ _ _ _ _|
seq[seqnum-3] |                      \______             |<--g_prev[0..T-1]
              |                             \___         |  
              |                                 \        |  
-------------------------------------------------\-------|  <forward trellis>
              ......                                ......  (2nd pass)

              |                                        \_|
===============================================================	      
*/

#endif /* __SENT_SEARCH_H__ */
