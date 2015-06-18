/**
 * @file   extern.h
 * @author Akinobu LEE
 * @date   Mon Mar  7 23:19:14 2005
 * 
 * <JA>
 * @brief  外部関数宣言
 * </JA>
 * 
 * <EN>
 * @brief  External function declarations
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

/* should be included after all include files */

/* backtrellis.c */
void bt_init(BACKTRELLIS *bt);
void bt_prepare(BACKTRELLIS *bt);
void bt_store(BACKTRELLIS *bt, TRELLIS_ATOM *aotm);
void bt_relocate_rw(BACKTRELLIS *bt);
#ifdef SP_BREAK_CURRENT_FRAME
void set_terminal_words(BACKTRELLIS *bt);
#endif
void bt_discount_pescore(WCHMM_INFO *wchmm, BACKTRELLIS *bt, HTK_Param *param);
void bt_sort_rw(BACKTRELLIS *bt);
TRELLIS_ATOM *bt_binsearch_atom(BACKTRELLIS *bt, int time, WORD_ID wkey);

/* factoring_sub.c */
void make_iwcache_index(WCHMM_INFO *wchmm);
#ifndef CATEGORY_TREE
# ifdef MULTIPATH_VERSION
void adjust_sc_index(WCHMM_INFO *wchmm);
# endif
#endif
void make_successor_list(WCHMM_INFO *wchmm);
#ifdef USE_NGRAM
void max_successor_cache_init(WCHMM_INFO *wchmm);
void max_successor_cache_free(void);
LOGPROB max_successor_prob(WCHMM_INFO *wchmm, WORD_ID lastword, int node);
LOGPROB *max_successor_prob_iw(WCHMM_INFO *wchmm, WORD_ID lastword);
void  calc_all_unigram_factoring_values(WCHMM_INFO *wchmm);
#else  /* USE_DFA */
boolean can_succeed(WCHMM_INFO *wchmm, WORD_ID lastword, int node);
#endif

/* beam.c */
void get_back_trellis_init(HTK_Param *param, WCHMM_INFO *wchmm, BACKTRELLIS *backtrellis);
boolean get_back_trellis_proceed(int t, HTK_Param *param, WCHMM_INFO *wchmm, BACKTRELLIS *backtrellis
#ifdef MULTIPATH_VERSION
				 , boolean final
#endif
				 );
void get_back_trellis_end(HTK_Param *param, WCHMM_INFO *wchmm, BACKTRELLIS *backtrellis);
void get_back_trellis(HTK_Param *param, WCHMM_INFO *wchmm, BACKTRELLIS *backtrellis, LOGPROB *backmax);
LOGPROB finalize_1st_pass(BACKTRELLIS *backtrellis, WORD_INFO *winfo, int len);
#ifdef SP_BREAK_CURRENT_FRAME
boolean is_sil(WORD_ID w, WORD_INFO *winfo, HTK_HMM_INFO *hmm);
void finalize_segment(BACKTRELLIS *backtrellis, HTK_Param *param, int len);
#endif

/* outprob_style.c */
#ifdef PASS1_IWCD
void outprob_style_cache_init(WCHMM_INFO *wchmm);
#ifdef CATEGORY_TREE
CD_Set *lcdset_lookup_with_category(WCHMM_INFO *wchmm, HMM_Logical *hmm, WORD_ID category);
void lcdset_register_with_category_all(WCHMM_INFO *wchmm, HTK_HMM_INFO *hmminfo, WORD_INFO *winfo, DFA_INFO *dfa);
void lcdset_remove_with_category_all(WCHMM_INFO *wchmm);
#endif
#endif
LOGPROB outprob_style(WCHMM_INFO *wchmm, int node, int last_wid, int t, HTK_Param *param);
void error_missing_right_triphone(HMM_Logical *base, char *rc_name);
void error_missing_left_triphone(HMM_Logical *base, char *lc_name);

/* ngram_decode.c */
#include "search.h"
#ifdef USE_NGRAM
int ngram_firstwords(NEXTWORD **nw, int peseqlen, int maxnw, WORD_INFO *winfo, BACKTRELLIS *bt);
int ngram_nextwords(NODE *hypo, NEXTWORD **nw, int maxnw, NGRAM_INFO *ngram, WORD_INFO *winfo, BACKTRELLIS *bt);
boolean ngram_acceptable(NODE *hypo, WORD_INFO *winfo);
#else  /* USE_DFA */
int dfa_firstwords(NEXTWORD **nw, int peseqlen, int maxnw, DFA_INFO *dfa);
int dfa_nextwords(NODE *hypo, NEXTWORD **nw, int maxnw, DFA_INFO *dfa);
boolean dfa_acceptable(NODE *hypo, DFA_INFO *dfa);
boolean dfa_look_around(NEXTWORD *nword, NODE *hypo, BACKTRELLIS *bt);
#endif

/* search_bestfirst_main.c */
void putsentence(NODE *hypo, int num);
void wchmm_fbs(HTK_Param *param, BACKTRELLIS *backtrellis, LOGPROB backmax, int stacksize, int ncan, int maxhypo, int cate_bgn, int cate_num);

/* search_bestfirst_v?.c */
void clear_stocker();
void free_node(NODE *node);
NODE *cpy_node(NODE *dst, NODE *src);
NODE *newnode();
void malloc_wordtrellis();
void free_wordtrellis();
void scan_word(NODE *now, HTK_Param *param);
void next_word(NODE *now, NODE *new, NEXTWORD *nword, HTK_Param *param, BACKTRELLIS *backtrellis);
void start_word(NODE *new, NEXTWORD *nword, HTK_Param *param, BACKTRELLIS *backtrellis);
void last_next_word(NODE *now, NODE *new, HTK_Param *param);

/* wav2mfcc.c */
HTK_Param *new_wav2mfcc(SP16 speech[], int speechlen);

/* version.c */
void put_header(FILE *stream);
void put_version(FILE *stream);
void put_compile_defs(FILE *stream);
void put_library_defs(FILE *stream);

/* wchmm.c */
WCHMM_INFO *wchmm_new();
void wchmm_free(WCHMM_INFO *w);
void print_wchmm_info(WCHMM_INFO *wchmm);
#ifdef CATEGORY_TREE
void build_wchmm(WCHMM_INFO *wchmm);
#endif
void build_wchmm2(WCHMM_INFO *wchmm);

/* wchmm_check.c */
void wchmm_check_interactive(WCHMM_INFO *wchmm);
void check_wchmm(WCHMM_INFO *wchmm);

/* realtime.c --- callback for adin_cut() */
void RealTimeInit();
void RealTimePipeLinePrepare();
int RealTimePipeLine(SP16 *Speech, int len);
int RealTimeResume();
HTK_Param *RealTimeParam(LOGPROB *backmax);
void RealTimeCMNUpdate(HTK_Param *param);
void RealTimeTerminate();

/* word_align.c */
void word_align(WORD_ID *words, short wnum, HTK_Param *param);
void phoneme_align(WORD_ID *words, short wnum, HTK_Param *param);
void state_align(WORD_ID *words, short wnum, HTK_Param *param);
void word_rev_align(WORD_ID *revwords, short wnum, HTK_Param *param);
void phoneme_rev_align(WORD_ID *revwords, short wnum, HTK_Param *param);
void state_rev_align(WORD_ID *revwords, short wnum, HTK_Param *param);

/* main.c */
void check_specs();	/* check needed parameter */
void print_setting();		/* output current setting */
void print_info();		/* output current gammar info */
int adin_cut_callback_store_buffer(SP16 *now, int len);
/* m_usage.c */
void opt_terminate();
void usage();
void detailed_help();
/* m_options.c */
char *filepath(char *filename, char *dirname);
void opt_parse(int argc, char *argv[], char *cwd);
void opt_release();
/* m_jconf.c */
void get_dirname(char *path);
void config_file_parse(char *conffile);
/* m_chkparam.c */
void checkpath(char *filename);
void check_specs();
int set_beam_width(WCHMM_INFO *wchmm, int specified);
void set_lm_weight();
void set_lm_weight2();
/* m_info.c */
void print_setting();
void print_info();
/* m_bootup.c */
void system_bootup();
/* m_adin.c */
void adin_initialize();
/* m_fusion.c */
void final_fusion();
/* result_tty.c */
void setup_result_tty();
void ttyout_status_recready();
void ttyout_status_recstart();
void ttyout_status_recend();
void ttyout_status_param(HTK_Param *p);
/* result_msock.c */
void setup_result_msock();
void decode_output_selection(char *str);
void msock_status_recready();
void msock_status_recstart();
void msock_status_recend();
void msock_status_param(HTK_Param *p);

/* hmm_check.c */
void hmm_check(HTK_HMM_INFO *hmminfo, WORD_INFO *winfo);

/* record.c */
void record_sample_open();
void record_sample_write(SP16 *speech, int samplenum);
void record_sample_close();

/* visual.c */
void visual_init();
void visual_show(BACKTRELLIS *bt);
void visual2_init(int maxhypo);
void visual2_popped(NODE *n, int popctr);
void visual2_next_word(NODE *next, NODE *prev, int popctr);
void visual2_best(NODE *now, WORD_INFO *winfo);

/* gmm.c */
void gmm_init(HTK_HMM_INFO *gmm, int prune_num);
void gmm_prepare(HTK_HMM_INFO *gmm);
void gmm_proceed(HTK_HMM_INFO *gmm, HTK_Param *param, int t);
void gmm_end(HTK_HMM_INFO *gmm);
void msock_gmm();
void ttyout_gmm();
boolean gmm_valid_input();

/* graphout.c */
#ifdef GRAPHOUT
void wordgraph_free(WordGraph *wg);
void put_wordgraph(WordGraph *wg);
void wordgraph_dump(WordGraph *root);
WordGraph *wordgraph_assign(WORD_ID wid, WORD_ID wid_left, WORD_ID wid_right, int leftframe, int rightframe, LOGPROB fscore_head, LOGPROB fscore_tail, LOGPROB gscore_head, LOGPROB gscore_tail, LOGPROB lscore, LOGPROB cmscore);
boolean wordgraph_check_and_add_rightword(WordGraph *wg, WordGraph *right);
boolean wordgraph_check_and_add_leftword(WordGraph *wg, WordGraph *left);
void wordgraph_save(WordGraph *wg, WordGraph *right, WordGraph **root);
WordGraph *wordgraph_check_merge(WordGraph *now, WordGraph **root, WORD_ID next_wid, boolean *merged_p);
WordGraph *wordgraph_dup(WordGraph *wg, WordGraph **root);
void wordgraph_purge_leaf_nodes(WordGraph **rootp);
void wordgraph_depth_cut(WordGraph **rootp);
void wordgraph_adjust_boundary(WordGraph **rootp);
void wordgraph_clean(WordGraph **rootp);
void wordgraph_compaction_thesame(WordGraph **rootp);
void wordgraph_compaction_exacttime(WordGraph **rootp);
void wordgraph_compaction_neighbor(WordGraph **rootp);
void wordgraph_sort_and_annotate_id(WordGraph **rootp);
void wordgraph_check_coherence(WordGraph *rootp);
#endif /* GRAPHOUT */

/* main.c */
void main_recognition_loop();
