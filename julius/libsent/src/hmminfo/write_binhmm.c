/**
 * @file   write_binhmm.c
 * @author Akinobu LEE
 * @date   Wed Feb 16 06:03:36 2005
 * 
 * <JA>
 * @brief  %HMM 定義をバイナリ形式のファイルへ書き出す
 *
 * Julius は独自のバイナリ形式の %HMM 定義ファイルをサポートしています．
 * HTKのアスキー形式の %HMM 定義ファイルからバイナリ形式への変換は，
 * 附属のツール mkbinhmm で行ないます．このバイナリ形式は，HTK の
 * バイナリ形式とは非互換ですので注意して下さい．
 * </JA>
 * 
 * <EN>
 * @brief  Write a binary %HMM definition to a file
 *
 * Julius supports a binary format of %HMM definition file.
 * The tool "mkbinhmm" can convert the ascii format HTK %HMM definition
 * file to this format.  Please note that this binary format is 
 * not compatible with the HTK binary format.
 * </EN>
 * 
 * $Revision: 1.5 $
 * 
 */
/*
 * Copyright (c) 2003-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

/* $Id: write_binhmm.c,v 1.5 2006/12/14 08:18:36 sumomo Exp $ */

#include <sent/stddefs.h>
#include <sent/htk_param.h>
#include <sent/htk_hmm.h>
#include <sent/mfcc.h>


/** 
 * Binary write function with byte swap (assume file is BIG ENDIAN)
 * 
 * @param fp [in] file pointer
 * @param buf [in] data to write
 * @param unitbyte [in] size of a unit in bytes
 * @param unitnum [in] number of unit to write
 */
static void
wrt(FILE *fp, void *buf, size_t unitbyte, int unitnum)
{
#ifndef WORDS_BIGENDIAN
  if (unitbyte != 1) {
    swap_bytes((char *)buf, unitbyte, unitnum);
  }
#endif
  if (myfwrite(buf, unitbyte, unitnum, fp) < unitnum) {
    perror("write_binhmm: wrt");
    j_error("write failed\n");
  }
#ifndef WORDS_BIGENDIAN
  if (unitbyte != 1) {
    swap_bytes((char *)buf, unitbyte, unitnum);
  }
#endif
}

/** 
 * Write a string, teminating at NULL.
 * 
 * @param fp [in] file pointer
 * @param str [in] string to write
 */
static void
wrt_str(FILE *fp, char *str)
{
  static char noname = '\0';
  
  if (str) {
    wrt(fp, str, sizeof(char), strlen(str)+1);
  } else {
    wrt(fp, &noname, sizeof(char), 1);
  }
}


static char *binhmm_header_v2 = BINHMM_HEADER_V2; ///< Header string for V2

/** 
 * Write header string as binary HMM file (ver. 2)
 * 
 * @param fp [in] file pointer
 */
static void
wt_header(FILE *fp, boolean emp, boolean inv)
{
  char buf[50];
  char *p;

  wrt_str(fp, binhmm_header_v2);
  p = &(buf[0]);
  if (emp) {
    *p++ = '_';
    *p++ = BINHMM_HEADER_V2_EMBEDPARA;
  }
  if (inv) {
    *p++ = '_';
    *p++ = BINHMM_HEADER_V2_VARINV;
  }
  *p = '\0';
  wrt_str(fp, buf);
}


/** 
 * Write acoustic analysis configration parameters into header of binary HMM.
 * 
 * @param fp [in] file pointer
 * @param para [in] acoustic analysis configration parameters
 */
static void
wt_para(FILE *fp, Value *para)
{
  short version;

  version = VALUE_VERSION;
  wrt(fp, &version, sizeof(short), 1);

  wrt(fp, &(para->smp_period), sizeof(long), 1);      
  wrt(fp, &(para->smp_freq), sizeof(long), 1);	
  wrt(fp, &(para->framesize), sizeof(int), 1);        
  wrt(fp, &(para->frameshift), sizeof(int), 1);       
  wrt(fp, &(para->preEmph), sizeof(float), 1);        
  wrt(fp, &(para->lifter), sizeof(int), 1);           
  wrt(fp, &(para->fbank_num), sizeof(int), 1);        
  wrt(fp, &(para->delWin), sizeof(int), 1);           
  wrt(fp, &(para->accWin), sizeof(int), 1);           
  wrt(fp, &(para->silFloor), sizeof(float), 1);       
  wrt(fp, &(para->escale), sizeof(float), 1);         
  wrt(fp, &(para->hipass), sizeof(int), 1);		
  wrt(fp, &(para->lopass), sizeof(int), 1);		
  wrt(fp, &(para->enormal), sizeof(int), 1);          
  wrt(fp, &(para->raw_e), sizeof(int), 1);            
  wrt(fp, &(para->ss_alpha), sizeof(float), 1);	
  wrt(fp, &(para->ss_floor), sizeof(float), 1);	
  wrt(fp, &(para->zmeanframe), sizeof(int), 1);	
}


/** 
 * Write %HMM option specifications
 * 
 * @param fp [in] file pointer
 * @param opt [out] pointer to the %HMM option structure that holds the values.
 */
static void
wt_opt(FILE *fp, HTK_HMM_Options *opt)
{
  wrt(fp, &(opt->stream_info.num), sizeof(short), 1);
  wrt(fp, opt->stream_info.vsize, sizeof(short), 50);
  wrt(fp, &(opt->vec_size), sizeof(short), 1);
  wrt(fp, &(opt->cov_type), sizeof(short), 1);
  wrt(fp, &(opt->dur_type), sizeof(short), 1);
  wrt(fp, &(opt->param_type), sizeof(short), 1);
}

/** 
 * Write %HMM type of mixture tying.
 * 
 * @param fp [in] file pointer
 * @param hmm [out] pointer to the writing %HMM definition data
 */
static void
wt_type(FILE *fp, HTK_HMM_INFO *hmm)
{
  wrt(fp, &(hmm->is_tied_mixture), sizeof(boolean), 1);
  wrt(fp, &(hmm->maxmixturenum), sizeof(int), 1);
}


/* write transition data */
static HTK_HMM_Trans **tr_index; ///< Sorted data pointers for mapping from pointer to id
static unsigned int tr_num;	///< Length of above

/** 
 * qsort callback function to sort transition pointers by their
 * address for indexing.
 * 
 * @param t1 [in] data 1
 * @param t2 [in] data 2
 * 
 * @return value required for qsort.
 */
static int
qsort_tr_index(HTK_HMM_Trans **t1, HTK_HMM_Trans **t2)
{
  if (*t1 > *t2) return 1;
  else if (*t1 < *t2) return -1;
  else return 0;
}

/** 
 * @brief  Write all transition matrix data.
 *
 * The pointers of all transition matrixes are first gathered,
 * sorted by the address.  Then the transition matrix data are written
 * by the sorted order.  The index will be used later to convert any pointer
 * reference to a transition matrix into scholar id.
 * 
 * @param fp [in] file pointer
 * @param hmm [in] writing %HMM definition data 
 */
static void
wt_trans(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Trans *t;
  unsigned int idx;
  int i;

  tr_num = 0;
  for(t = hmm->trstart; t; t = t->next) tr_num++;
  tr_index = (HTK_HMM_Trans **)mymalloc(sizeof(HTK_HMM_Trans *) * tr_num);
  idx = 0;
  for(t = hmm->trstart; t; t = t->next) tr_index[idx++] = t;
  qsort(tr_index, tr_num, sizeof(HTK_HMM_Trans *), (int (*)(const void *, const void *))qsort_tr_index);
  
  wrt(fp, &tr_num, sizeof(unsigned int), 1);
  for (idx = 0; idx < tr_num; idx++) {
    t = tr_index[idx];
    wrt_str(fp, t->name);
    wrt(fp, &(t->statenum), sizeof(short), 1);
    for(i=0;i<t->statenum;i++) {
      wrt(fp, t->a[i], sizeof(PROB), t->statenum);
    }
  }

  j_printf("%d transition maxtix written\n", tr_num);
}

/** 
 * Binary search function to convert transition matrix pointer to a scholar ID.
 * 
 * @param t [in] pointer to a transition matrix
 * 
 * @return the corresponding scholar ID.
 */
static unsigned int
search_trid(HTK_HMM_Trans *t)
{
  unsigned int left = 0;
  unsigned int right = tr_num - 1;
  unsigned int mid;

  while (left < right) {
    mid = (left + right) / 2;
    if (tr_index[mid] < t) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  return(left);
}


/* write variance data */
static HTK_HMM_Var **vr_index;	///< Sorted data pointers for mapping from pointer to id
static unsigned int vr_num;	///< Length of above

/** 
 * qsort callback function to sort variance pointers by their
 * address for indexing.
 * 
 * @param v1 [in] data 1
 * @param v2 [in] data 2
 * 
 * @return value required for qsort.
 */
static int
qsort_vr_index(HTK_HMM_Var **v1, HTK_HMM_Var **v2)
{
  if (*v1 > *v2) return 1;
  else if (*v1 < *v2) return -1;
  else return 0;
}

/** 
 * @brief  Write all variance data.
 *
 * The pointers of all variance vectors are first gathered,
 * sorted by the address.  Then the variance vectors are written
 * by the sorted order.  The index will be used later to convert any pointer
 * reference to a variance vector into scholar id.
 * 
 * @param fp [in] file pointer
 * @param hmm [in] writing %HMM definition data 
 */
static void
wt_var(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Var *v;
  unsigned int idx;

  vr_num = 0;
  for(v = hmm->vrstart; v; v = v->next) vr_num++;
  vr_index = (HTK_HMM_Var **)mymalloc(sizeof(HTK_HMM_Var *) * vr_num);
  idx = 0;
  for(v = hmm->vrstart; v; v = v->next) vr_index[idx++] = v;
  qsort(vr_index, vr_num, sizeof(HTK_HMM_Var *), (int (*)(const void *, const void *))qsort_vr_index);  

  wrt(fp, &vr_num, sizeof(unsigned int), 1);
  for (idx = 0; idx < vr_num; idx++) {
    v = vr_index[idx];
    wrt_str(fp, v->name);
    wrt(fp, &(v->len), sizeof(short), 1);
    wrt(fp, v->vec, sizeof(VECT), v->len);
  }
  j_printf("%d variance written\n", vr_num);
}

/** 
 * Binary search function to convert variance pointer to a scholar ID.
 * 
 * @param v [in] pointer to a variance data
 * 
 * @return the corresponding scholar ID.
 */
static unsigned int
search_vid(HTK_HMM_Var *v)
{
  unsigned int left = 0;
  unsigned int right = vr_num - 1;
  unsigned int mid;

  while (left < right) {
    mid = (left + right) / 2;
    if (vr_index[mid] < v) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  return(left);
}


/* write density data */
static HTK_HMM_Dens **dens_index; ///< Sorted data pointers for mapping from pointer to id
static unsigned int dens_num;	///< Length of above

/** 
 * qsort callback function to sort density pointers by their
 * address for indexing.
 * 
 * @param d1 [in] data 1
 * @param d2 [in] data 2
 * 
 * @return value required for qsort.
 */
static int
qsort_dens_index(HTK_HMM_Dens **d1, HTK_HMM_Dens **d2)
{
  if (*d1 > *d2) return 1;
  else if (*d1 < *d2) return -1;
  else return 0;
}

/** 
 * @brief  Write all mixture density data.
 *
 * The pointers of all mixture densities are first gathered,
 * sorted by the address.  Then the densities are written
 * by the sorted order.  The pointers to the lower structure (variance etc.)
 * in the data are written in a corresponding scholar id.
 * The pointer index of this data will be used later to convert any pointer
 * reference to a density data into scholar id.
 * 
 * @param fp [in] file pointer
 * @param hmm [in] writing %HMM definition data 
 */
static void
wt_dens(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Dens *d;
  unsigned int idx;
  unsigned int vid;

  dens_num = hmm->totalmixnum;
  dens_index = (HTK_HMM_Dens **)mymalloc(sizeof(HTK_HMM_Dens *) * dens_num);
  idx = 0;
  for(d = hmm->dnstart; d; d = d->next) dens_index[idx++] = d;
  qsort(dens_index, dens_num, sizeof(HTK_HMM_Dens *), (int (*)(const void *, const void *))qsort_dens_index);
  
  wrt(fp, &dens_num, sizeof(unsigned int), 1);
  for (idx = 0; idx < dens_num; idx++) {
    d = dens_index[idx];
    wrt_str(fp, d->name);
    wrt(fp, &(d->meanlen), sizeof(short), 1);
    wrt(fp, d->mean, sizeof(VECT), d->meanlen);
    vid = search_vid(d->var);
    /* for debug */
    if (d->var != vr_index[vid]) j_error("index not match!!! dens\n");
    wrt(fp, &vid, sizeof(unsigned int), 1);
    wrt(fp, &(d->gconst), sizeof(LOGPROB), 1);
  }
  j_printf("%d gaussian densities written\n", dens_num);
}

/** 
 * Binary search function to convert density pointer to a scholar ID.
 * 
 * @param d [in] pointer to a mixture density
 * 
 * @return the corresponding scholar ID.
 */
static unsigned int
search_did(HTK_HMM_Dens *d)
{
  unsigned int left = 0;
  unsigned int right = dens_num - 1;
  unsigned int mid;

  while (left < right) {
    mid = (left + right) / 2;
    if (dens_index[mid] < d) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  return(left);
}


/* write tmix data */
static GCODEBOOK **tm_index; ///< Sorted data pointers for mapping from pointer to id
static unsigned int tm_num;	///< Length of above
static unsigned int tm_idx;	///< Current index

/** 
 * Traverse callback function to store pointers in @a tm_index.
 * 
 * @param p [in] pointer to the codebook data
 */
static void
tmix_list_callback(void *p)
{
  GCODEBOOK *tm;
  tm = p;
  tm_index[tm_idx++] = tm;
}

/** 
 * qsort callback function to sort density pointers by their
 * address for indexing.
 * 
 * @param tm1 [in] data 1
 * @param tm2 [in] data 2
 * 
 * @return value required for qsort.
 */
static int
qsort_tm_index(GCODEBOOK **tm1, GCODEBOOK **tm2)
{
  if (*tm1 > *tm2) return 1;
  else if (*tm1 < *tm2) return -1;
  else return 0;
}

/** 
 * @brief  Write all codebook data.
 *
 * The pointers of all codebook densities are first gathered,
 * sorted by the address.  Then the densities are written
 * by the sorted order.  The pointers to the lower structure (mixture etc.)
 * in the data are written by the corresponding scholar id.
 * The pointer index of this data will be used later to convert any pointer
 * reference to a codebook into scholar id.
 * 
 * @param fp [in] file pointer
 * @param hmm [in] writing %HMM definition data 
 */
static void
wt_tmix(FILE *fp, HTK_HMM_INFO *hmm)
{
  GCODEBOOK *tm;
  unsigned int idx;
  unsigned int did;
  int i;

  tm_num = hmm->codebooknum;
  tm_index = (GCODEBOOK **)mymalloc(sizeof(GCODEBOOK *) * tm_num);
  tm_idx = 0;
  aptree_traverse_and_do(hmm->codebook_root, tmix_list_callback);
  qsort(tm_index, tm_num, sizeof(GCODEBOOK *), (int (*)(const void *, const void *))qsort_tm_index);  

  wrt(fp, &tm_num, sizeof(unsigned int), 1);
  for (idx = 0; idx < tm_num; idx++) {
    tm = tm_index[idx];
    wrt_str(fp, tm->name);
    wrt(fp, &(tm->num), sizeof(int), 1);
    for(i=0;i<tm->num;i++) {
      if (tm->d[i] == NULL) {
	did = dens_num;
      } else {
	did = search_did(tm->d[i]);
	/* for debug */
	if (tm->d[i] != dens_index[did]) j_error("index not match!!! dens\n");
      }
      wrt(fp, &did, sizeof(unsigned int), 1);
    }
  }
  j_printf("%d tied-mixture codebooks written\n", tm_num);
}

/** 
 * Binary search function to convert codebook pointer to a scholar ID.
 * 
 * @param tm [in] pointer to a codebook
 * 
 * @return the corresponding scholar ID.
 */
static unsigned int
search_tmid(GCODEBOOK *tm)
{
  unsigned int left = 0;
  unsigned int right = tm_num - 1;
  unsigned int mid;

  while (left < right) {
    mid = (left + right) / 2;
    if (tm_index[mid] < tm) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  return(left);
}


/* write state data */
static HTK_HMM_State **st_index; ///< Sorted data pointers for mapping from pointer to id
static unsigned int st_num;	///< Length of above

/** 
 * qsort callback function to sort state pointers by their
 * address for indexing.
 * 
 * @param s1 [in] data 1
 * @param s2 [in] data 2
 * 
 * @return value required for qsort.
 */
static int
qsort_st_index(HTK_HMM_State **s1, HTK_HMM_State **s2)
{
  if (*s1 > *s2) return 1;
  else if (*s1 < *s2) return -1;
  else return 0;
}

/** 
 * @brief  Write all state data.
 *
 * The pointers of all states are first gathered,
 * sorted by the address.  Then the state informations are written
 * by the sorted order.  The pointers to the lower structure (mixture etc.)
 * in the data are written in a corresponding scholar id.
 * The pointer index of this data will be used later to convert any pointer
 * reference to a state data into scholar id.
 * 
 * @param fp [in] file pointer
 * @param hmm [in] writing %HMM definition data 
 */
static void
wt_state(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_State *s;
  unsigned int idx;
  unsigned int did;
  int i;
  short dummy;

  st_num = hmm->totalstatenum;
  st_index = (HTK_HMM_State **)mymalloc(sizeof(HTK_HMM_State *) * st_num);
  idx = 0;
  for(s = hmm->ststart; s; s = s->next) st_index[idx++] = s;
  qsort(st_index, st_num, sizeof(HTK_HMM_State *), (int (*)(const void *, const void *))qsort_st_index);
  
  wrt(fp, &st_num, sizeof(unsigned int), 1);
  for (idx = 0; idx < st_num; idx++) {
    s = st_index[idx];
    wrt_str(fp, s->name);
    if (hmm->is_tied_mixture) {
      /* try tmix */
      did = search_tmid((GCODEBOOK *)(s->b));
      if ((GCODEBOOK *)s->b == tm_index[did]) {
	/* tmix */
	dummy = -1;
	wrt(fp, &dummy, sizeof(short), 1);
	wrt(fp, &did, sizeof(unsigned int), 1);
      } else {
	/* tmix failed -> normal mixture */
	wrt(fp, &(s->mix_num), sizeof(short), 1);
	for (i=0;i<s->mix_num;i++) {
	  if (s->b[i] == NULL) {
	    did = dens_num;
	  } else {
	    did = search_did(s->b[i]);
	    if (s->b[i] != dens_index[did]) {
	      j_error("index not match!!!");
	    }
	  }
	  wrt(fp, &did, sizeof(unsigned int), 1);
	}
      }
    } else {			/* not tied mixture */
      wrt(fp, &(s->mix_num), sizeof(short), 1);
      for (i=0;i<s->mix_num;i++) {
	if (s->b[i] == NULL) {
	  did = dens_num;
	} else {
	  did = search_did(s->b[i]);
	  if (s->b[i] != dens_index[did]) {
	    j_error("index not match!!!");
	  }
	}
	wrt(fp, &did, sizeof(unsigned int), 1);
      }
    }
    wrt(fp, s->bweight, sizeof(PROB), s->mix_num);
  }
  j_printf("%d states written\n", st_num);
}

/** 
 * Binary search function to convert state pointer to a scholar ID.
 * 
 * @param s [in] pointer to a state
 * 
 * @return the corresponding scholar ID.
 */
static unsigned int
search_stid(HTK_HMM_State *s)
{
  unsigned int left = 0;
  unsigned int right = st_num - 1;
  unsigned int mid;

  while (left < right) {
    mid = (left + right) / 2;
    if (st_index[mid] < s) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  return(left);
}


/** 
 * @brief  Write all model data.
 *
 * The data of all models are written.  The order is not important
 * at this top level, since there are no reference to this data.
 * The pointers to the lower structure (states, transitions, etc.)
 * in the data are written by the corresponding scholar id.
 * 
 * @param fp [in] file pointer
 * @param hmm [in] writing %HMM definition data 
 */
static void
wt_data(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Data *d;
  unsigned int md_num;
  unsigned int sid, tid;
  int i;

  md_num = hmm->totalhmmnum;

  wrt(fp, &(md_num), sizeof(unsigned int), 1);
  for(d = hmm->start; d; d = d->next) {
    wrt_str(fp, d->name);
    wrt(fp, &(d->state_num), sizeof(short), 1);
    for (i=0;i<d->state_num;i++) {
      if (d->s[i] != NULL) {
	sid = search_stid(d->s[i]);
	/* for debug */
	if (d->s[i] != st_index[sid]) j_error("index not match!!! data state\n");
      } else {
	sid = hmm->totalstatenum + 1; /* error value */
      }
      wrt(fp, &sid, sizeof(unsigned int), 1);
    }
    tid = search_trid(d->tr);
    /* for debug */
    if (d->tr != tr_index[tid]) j_error("index not match!!! data trans\n");
    wrt(fp, &tid, sizeof(unsigned int), 1);
  }
  j_printf("%d HMM model definition written\n", md_num);
}


/** 
 * Top function to write %HMM definition data to a binary file.
 * 
 * @param fp [in] file pointer
 * @param hmm [in] %HMM definition structure to be written
 * @param para [in] acoustic analysis parameter, or NULL if not available
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
write_binhmm(FILE *fp, HTK_HMM_INFO *hmm, Value *para)
{

  /* write header */
  wt_header(fp, (para ? TRUE : FALSE), hmm->variance_inversed);

  if (para) {
    /* write acoustic analysis parameter info */
    wt_para(fp, para);
  }
  
  /* write option data */
  wt_opt(fp, &(hmm->opt));

  /* write type data */
  wt_type(fp, hmm);

  /* write transition data */
  wt_trans(fp, hmm);

  /* write variance data */
  wt_var(fp, hmm);

  /* write density data */
  wt_dens(fp, hmm);

  /* write tmix data */
  if (hmm->is_tied_mixture) {
    wt_tmix(fp, hmm);
  }

  /* write state data */
  wt_state(fp, hmm);

  /* write model data */
  wt_data(fp, hmm);

  /* free pointer->index work area */
  free(tr_index);
  free(vr_index);
  free(dens_index);
  if (hmm->is_tied_mixture) free(tm_index);
  free(st_index);

  return (TRUE);
}
