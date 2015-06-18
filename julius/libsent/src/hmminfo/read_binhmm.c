/**
 * @file   read_binhmm.c
 * @author Akinobu LEE
 * @date   Wed Feb 16 05:23:59 2005
 * 
 * <JA>
 * @brief  バイナリ形式の %HMM 定義ファイルを読み込む
 *
 * Julius は独自のバイナリ形式の %HMM 定義ファイルをサポートしています．
 * HTKのアスキー形式の %HMM 定義ファイルからバイナリ形式への変換は，
 * 附属のツール mkbinhmm で行ないます．このバイナリ形式は，HTK の
 * バイナリ形式とは非互換ですので注意して下さい．
 * </JA>
 * 
 * <EN>
 * @brief  Read a binary %HMM definition file
 *
 * Julius supports a binary format of %HMM definition file.
 * The tool "mkbinhmm" can convert the ascii format HTK %HMM definition
 * file to this format.  Please note that this binary format is 
 * not compatible with the HTK binary format.
 * </EN>
 * 
 * $Revision: 1.8 $
 * 
 */
/*
 * Copyright (c) 2003-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/htk_param.h>
#include <sent/htk_hmm.h>

#undef DMES			/* define to enable debug message */

static boolean gzfile;	      ///< TRUE when opened by fopen_readfile

/** 
 * Binary read function with byte swaping (assume file is BIG ENDIAN)
 * 
 * @param fp [in] file pointer
 * @param buf [out] read data
 * @param unitbyte [in] size of a unit in bytes
 * @param unitnum [in] number of unit to be read
 */
static void
rdn(FILE *fp, void *buf, size_t unitbyte, int unitnum)
{
  size_t tmp;
  if (gzfile) {
    tmp = myfread(buf, unitbyte, unitnum, fp);
  } else {
    tmp = fread(buf, unitbyte, unitnum, fp);
  }
  if (tmp < (size_t)unitnum) {
    perror("ngram_read_bin");
    j_error("read failed\n");
  }
#ifndef WORDS_BIGENDIAN
  if (unitbyte != 1) {
    swap_bytes(buf, unitbyte, unitnum);
  }
#endif
}

static char buf[MAXLINELEN];	///< Local work are for text handling
/** 
 * Read a string till NULL.
 * 
 * @param fp [in] file pointer
 * @param hmm [out] pointer to %HMM definition data to store the values.
 * 
 * @return pointer to a newly allocated buffer that contains the read string.
 */
static char *
rdn_str(FILE *fp, HTK_HMM_INFO *hmm)
{
  int c;
  int len;
  char *p;

  len = 0;
  while ((c = gzfile ? myfgetc(fp) : fgetc(fp)) != -1) {
    if (len >= MAXLINELEN) j_error("Error: string len exceeded %d bytes\n", len);
    buf[len++] = c;
    if (c == '\0') break;
  }
  if (len == 1) {
    p = NULL;
  } else {
    p = (char *)mybmalloc2(len, &(hmm->mroot));
    strcpy(p, buf);
  }
  return(p);
}



static char *binhmm_header = BINHMM_HEADER; ///< Header string
static char *binhmm_header_v2 = BINHMM_HEADER_V2; ///< Header string for V2

/** 
 * Read acoustic analysis configration parameters from header of binary HMM.
 * 
 * @param fp [in] file pointer
 * @param para [out] acoustic analysis configration parameters
 */
static void
rd_para(FILE *fp, Value *para)
{
  short version;

  rdn(fp, &version, sizeof(short), 1);
  switch(version) {
  case 1:
    rdn(fp, &(para->smp_period), sizeof(long), 1);      
    rdn(fp, &(para->smp_freq), sizeof(long), 1);	
    rdn(fp, &(para->framesize), sizeof(int), 1);        
    rdn(fp, &(para->frameshift), sizeof(int), 1);       
    rdn(fp, &(para->preEmph), sizeof(float), 1);        
    rdn(fp, &(para->lifter), sizeof(int), 1);           
    rdn(fp, &(para->fbank_num), sizeof(int), 1);        
    rdn(fp, &(para->delWin), sizeof(int), 1);           
    rdn(fp, &(para->accWin), sizeof(int), 1);           
    rdn(fp, &(para->silFloor), sizeof(float), 1);       
    rdn(fp, &(para->escale), sizeof(float), 1);         
    rdn(fp, &(para->hipass), sizeof(int), 1);		
    rdn(fp, &(para->lopass), sizeof(int), 1);		
    rdn(fp, &(para->enormal), sizeof(int), 1);          
    rdn(fp, &(para->raw_e), sizeof(int), 1);            
    rdn(fp, &(para->ss_alpha), sizeof(float), 1);	
    rdn(fp, &(para->ss_floor), sizeof(float), 1);	
    rdn(fp, &(para->zmeanframe), sizeof(int), 1);	
    break;
  default:
    j_error("Error: read_binhmm: unknown embedded parameter format version: %d\n", version);
  }
}

/** 
 * Read header string of binary HMM file.
 * 
 * @param fp [in] file pointer
 * @param hmm [out] pointer to %HMM definition data to store the values.
 * @param para [out] store embedded acoustic parameters if any (V2)
 * 
 * @return TRUE if a correct header was read, FALSE if header string does not
 * match the current version.
 */
static boolean
rd_header(FILE *fp, HTK_HMM_INFO *hmm, Value *para)
{
  char *p, *q;
  boolean emp, inv;
  
  p = rdn_str(fp, hmm);
  if (strmatch(p, binhmm_header)) {
    /* version 1 */
    hmm->variance_inversed = FALSE;
  } else if (strmatch(p, binhmm_header_v2)) {
    /* version 2 */
    emp = inv = FALSE;
    q = rdn_str(fp, hmm);
    if (q != NULL) {
      while(*q == '_') {
	q++;
	switch (*q) {
	case BINHMM_HEADER_V2_EMBEDPARA:
	  /* read in embedded acoutic condition parameters */
	  emp = TRUE;
	  break;
	case BINHMM_HEADER_V2_VARINV:
	  inv = TRUE;
	  break;
	}
	q++;
      }
    }
    if (emp) {
      para->loaded = 1;
      rd_para(fp, para);
      j_printerr("(acoutic analysis conf embedded)...");
    }
    if (inv) {
      hmm->variance_inversed = TRUE;
      j_printerr("(varinv)...");
    } else {
      hmm->variance_inversed = FALSE;
    }
  } else {
    /* failed to read header */
    return FALSE;
  }
  return TRUE;
}



/** 
 * Read %HMM option specifications.
 * 
 * @param fp [in] file pointer
 * @param opt [out] pointer to the %HMM option structure to hold the read
 * values.
 */
static void
rd_opt(FILE *fp, HTK_HMM_Options *opt)
{
  rdn(fp, &(opt->stream_info.num), sizeof(short), 1);
  rdn(fp, opt->stream_info.vsize, sizeof(short), 50);
  rdn(fp, &(opt->vec_size), sizeof(short), 1);
  rdn(fp, &(opt->cov_type), sizeof(short), 1);
  rdn(fp, &(opt->dur_type), sizeof(short), 1);
  rdn(fp, &(opt->param_type), sizeof(short), 1);
}

/** 
 * Read %HMM type of mixture tying.
 * 
 * @param fp [in] file pointer
 * @param hmm [out] pointer to %HMM definition data to store the values.
 */
static void
rd_type(FILE *fp, HTK_HMM_INFO *hmm)
{
  rdn(fp, &(hmm->is_tied_mixture), sizeof(boolean), 1);
  rdn(fp, &(hmm->maxmixturenum), sizeof(int), 1);
}


/* read transition data */
static HTK_HMM_Trans **tr_index; ///< Map transition matrix id to its pointer
static unsigned int tr_num;	///< Length of above

/** 
 * @brief  Read a sequence of transition matrix data for @a tr_num.
 *
 * The transition matrixes are stored into @a hmm, and their pointers
 * are also stored in @a tr_index for later data mapping operation
 * from upper structure (state etc.).
 * 
 * @param fp [in] file pointer
 * @param hmm [out] %HMM definition structure to hold the read transitions.
 */
static void
rd_trans(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Trans *t;
  unsigned int idx;
  int i;
  PROB *atmp;

  rdn(fp, &tr_num, sizeof(unsigned int), 1);
  tr_index = (HTK_HMM_Trans **)mymalloc(sizeof(HTK_HMM_Trans *) * tr_num);

  hmm->trstart = NULL;
  hmm->tr_root = NULL;
  for (idx = 0; idx < tr_num; idx++) {
    t = (HTK_HMM_Trans *)mybmalloc2(sizeof(HTK_HMM_Trans), &(hmm->mroot));
    t->name = rdn_str(fp, hmm);
    rdn(fp, &(t->statenum), sizeof(short), 1);
    t->a = (PROB **)mybmalloc2(sizeof(PROB *) * t->statenum, &(hmm->mroot));
    atmp = (PROB *)mybmalloc2(sizeof(PROB) * t->statenum * t->statenum, &(hmm->mroot));
    for (i=0;i<t->statenum;i++) {
      t->a[i] = &(atmp[i*t->statenum]);
      rdn(fp, t->a[i], sizeof(PROB), t->statenum);
    }
    trans_add(hmm, t);
    tr_index[idx] = t;
  }

#ifdef DMES
  j_printf("%d transition maxtix read\n", tr_num);
#endif
}


static HTK_HMM_Var **vr_index;	///< Map variance id to its pointer
static unsigned int vr_num;	///< Length of above

/** 
 * @brief  Read a sequence of variance vector for @a vr_num.
 *
 * The variance vectors are stored into @a hmm, and their pointers
 * are also stored in @a vr_index for later data mapping operation
 * from upper structure (density etc.).
 * 
 * @param fp [in] file pointer
 * @param hmm [out] %HMM definition structure to hold the read variance.
 */
static void
rd_var(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Var *v;
  unsigned int idx;

  rdn(fp, &vr_num, sizeof(unsigned int), 1);
  vr_index = (HTK_HMM_Var **)mymalloc(sizeof(HTK_HMM_Var *) * vr_num);
  
  hmm->vrstart = NULL;
  hmm->vr_root = NULL;
  for (idx = 0; idx < vr_num; idx++) {
    v = (HTK_HMM_Var *)mybmalloc2(sizeof(HTK_HMM_Var), &(hmm->mroot));
    v->name = rdn_str(fp, hmm);
    rdn(fp, &(v->len), sizeof(short), 1);
    v->vec = (VECT *)mybmalloc2(sizeof(VECT) * v->len, &(hmm->mroot));
    rdn(fp, v->vec, sizeof(VECT), v->len);
    vr_index[idx] = v;
    var_add(hmm, v);
  }
#ifdef DMES
  j_printf("%d variance read\n", vr_num);
#endif
}


/* read density data */
static HTK_HMM_Dens **dens_index; ///< Map density id to its pointer
static unsigned int dens_num;	///< Length of above

/** 
 * @brief  Read a sequence of mixture densities for @a dens_num.
 *
 * The mixture densities are stored into @a hmm, and their references
 * to lower structure (variance etc.) are recovered from the id-to-pointer
 * index.  Their pointers are also stored in @a dens_index for
 * later data mapping operation from upper structure (state etc.).
 * 
 * @param fp [in] file pointer
 * @param hmm [out] %HMM definition structure to hold the read densities.
 */
static void
rd_dens(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Dens *d;
  unsigned int idx;
  unsigned int vid;

  rdn(fp, &dens_num, sizeof(unsigned int), 1);
  hmm->totalmixnum = dens_num;
  dens_index = (HTK_HMM_Dens **)mymalloc(sizeof(HTK_HMM_Dens *) * dens_num);

  hmm->dnstart = NULL;
  hmm->dn_root = NULL;
  for (idx = 0; idx < dens_num; idx++) {
    d = (HTK_HMM_Dens *)mybmalloc2(sizeof(HTK_HMM_Dens), &(hmm->mroot));
    d->name = rdn_str(fp, hmm);
    rdn(fp, &(d->meanlen), sizeof(short), 1);
    d->mean = (VECT *)mybmalloc2(sizeof(VECT) * d->meanlen, &(hmm->mroot));
    rdn(fp, d->mean, sizeof(VECT), d->meanlen);
    rdn(fp, &vid, sizeof(unsigned int), 1);
    d->var = vr_index[vid];
    rdn(fp, &(d->gconst), sizeof(LOGPROB), 1);
    dens_index[idx] = d;
    dens_add(hmm, d);
  }
#ifdef DMES
  j_printf("%d gaussian densities read\n", dens_num);
#endif
}


/* read tmix data */
static GCODEBOOK **tm_index;	///< Map codebook id to its pointer
static unsigned int tm_num;	///< Length of above

/** 
 * @brief  Read a sequence of mixture codebook for @a tm_num.
 *
 * The mixture codebook data are stored into @a hmm, and their references
 * to lower structure (mixtures etc.) are recovered from the id-to-pointer
 * index.  Their pointers are also stored in @a tm_index for
 * later data mapping operation from upper structure (state etc.).
 * 
 * @param fp [in] file pointer
 * @param hmm [out] %HMM definition structure to hold the read codebooks.
 */
static void
rd_tmix(FILE *fp, HTK_HMM_INFO *hmm)
{
  GCODEBOOK *tm;
  unsigned int idx;
  unsigned int did;
  int i;

  rdn(fp, &tm_num, sizeof(unsigned int), 1);
  hmm->codebooknum = tm_num;
  tm_index = (GCODEBOOK **)mymalloc(sizeof(GCODEBOOK *) * tm_num);
  hmm->maxcodebooksize = 0;

  hmm->codebook_root = NULL;
  for (idx = 0; idx < tm_num; idx++) {
    tm = (GCODEBOOK *)mybmalloc2(sizeof(GCODEBOOK), &(hmm->mroot));
    tm->name = rdn_str(fp, hmm);
    rdn(fp, &(tm->num), sizeof(int), 1);
    if (hmm->maxcodebooksize < tm->num) hmm->maxcodebooksize = tm->num;
    tm->d = (HTK_HMM_Dens **)mybmalloc2(sizeof(HTK_HMM_Dens *) * tm->num, &(hmm->mroot));
    for(i=0;i<tm->num;i++) {
      rdn(fp, &did, sizeof(unsigned int), 1);
      if (did >= dens_num) {
	tm->d[i] = NULL;
      } else {
	tm->d[i] = dens_index[did];
      }
    }
    tm->id = idx;
    tm_index[idx] = tm;
    codebook_add(hmm, tm);
  }
#ifdef DMES
  j_printf("%d tied-mixture codebooks read\n", tm_num);
#endif  
}

/* read state data */
static HTK_HMM_State **st_index; ///< Map state id to its pointer
static unsigned int st_num;	///< Length of above

/** 
 * @brief  Read a sequence of state data for @a st_num.
 *
 * The state data are stored into @a hmm, and their references
 * to lower structure (mixture, codebook, etc.) are recovered
 * from the id-to-pointer index.  Their pointers are also stored
 * in @a st_index for later data mapping operation from
 * upper structure (models etc.).
 * 
 * @param fp [in] file pointer
 * @param hmm [out] %HMM definition structure to hold the read states.
 */
static void
rd_state(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_State *s;
  unsigned int idx;
  unsigned int did;
  int i;

  rdn(fp, &st_num, sizeof(unsigned int), 1);
  hmm->totalstatenum = st_num;
  st_index = (HTK_HMM_State **)mymalloc(sizeof(HTK_HMM_State *) * st_num);

  hmm->ststart = NULL;
  hmm->st_root = NULL;
  for (idx = 0; idx < st_num; idx++) {
    s = (HTK_HMM_State *)mybmalloc2(sizeof(HTK_HMM_State), &(hmm->mroot));
    s->name = rdn_str(fp, hmm);
    rdn(fp, &(s->mix_num), sizeof(short), 1);
    if (s->mix_num == -1) {
      /* tmix */
      rdn(fp, &did, sizeof(unsigned int), 1);
      s->b = (HTK_HMM_Dens **)tm_index[did];
      s->mix_num = (tm_index[did])->num;
    } else {
      /* mixture */
      s->b = (HTK_HMM_Dens **)mybmalloc2(sizeof(HTK_HMM_Dens *) * s->mix_num, &(hmm->mroot));
      for (i=0;i<s->mix_num;i++) {
	rdn(fp, &did, sizeof(unsigned int), 1);
	if (did >= dens_num) {
	  s->b[i] = NULL;
	} else {
	  s->b[i] = dens_index[did];
	}
      }
    }
    s->bweight = (PROB *)mybmalloc2(sizeof(PROB) * s->mix_num, &(hmm->mroot));
    rdn(fp, s->bweight, sizeof(PROB), s->mix_num);
    s->id = idx;
    st_index[idx] = s;
    state_add(hmm, s);
  }
#ifdef DMES
  j_printf("%d states read\n", st_num);
#endif
}

/** 
 * @brief  Read a sequence of %HMM models.
 *
 * The models are stored into @a hmm.  Their references
 * to lower structures (state, transition, etc.) are stored in schalar
 * ID, and are recovered from the previously built id-to-pointer index.
 * when reading the sub structures.
 * 
 * @param fp [in] file pointer
 * @param hmm [out] %HMM definition structure to hold the read models.
 */
static void
rd_data(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Data *d;
  unsigned int md_num;
  unsigned int sid, tid;
  unsigned int idx;
  int i;

  rdn(fp, &(md_num), sizeof(unsigned int), 1);
  hmm->totalhmmnum = md_num;

  hmm->start = NULL;
  hmm->physical_root = NULL;
  for (idx = 0; idx < md_num; idx++) {
    d = (HTK_HMM_Data *)mybmalloc2(sizeof(HTK_HMM_Data), &(hmm->mroot));
    d->name = rdn_str(fp, hmm);
    rdn(fp, &(d->state_num), sizeof(short), 1);
    d->s = (HTK_HMM_State **)mybmalloc2(sizeof(HTK_HMM_State *) * d->state_num, &(hmm->mroot));
    for (i=0;i<d->state_num;i++) {
      rdn(fp, &sid, sizeof(unsigned int), 1);
      if (sid > hmm->totalstatenum) {
	d->s[i] = NULL;
      } else {
	d->s[i] = st_index[sid];
      }
    }
    rdn(fp, &tid, sizeof(unsigned int), 1);
    d->tr = tr_index[tid];
    htk_hmmdata_add(hmm, d);
  }
#ifdef DMES
  j_printf("%d HMM model definition read\n", md_num);
#endif
}



/** 
 * Top function to read a binary %HMM file from @a fp.
 * 
 * @param fp [in] file pointer
 * @param hmm [out] %HMM definition structure to hold the read models.
 * @param gzfile_p [in] TRUE if the file pointer points to a gzip file
 * @param para [out] store acoustic parameters if embedded in binhmm (V2)
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
read_binhmm(FILE *fp, HTK_HMM_INFO *hmm, boolean gzfile_p, Value *para)
{

  gzfile = gzfile_p;

  /* read header */
  if (rd_header(fp, hmm, para) == FALSE) {
    return FALSE;
  }

  j_printerr("(binary)...");
  
  /* read option data */
  rd_opt(fp, &(hmm->opt));

  /* read type data */
  rd_type(fp, hmm);

  /* read transition data */
  rd_trans(fp, hmm);

  /* read variance data */
  rd_var(fp, hmm);

  /* read density data */
  rd_dens(fp, hmm);

  /* read tmix data */
  if (hmm->is_tied_mixture) {
    rd_tmix(fp, hmm);
  }

  /* read state data */
  rd_state(fp, hmm);

  /* read model data */
  rd_data(fp, hmm);

  /* free pointer->index work area */
  free(tr_index);
  free(vr_index);
  free(dens_index);
  if (hmm->is_tied_mixture) free(tm_index);
  free(st_index);

  /* count maximum state num (it is not stored in binhmm... */
  {
    HTK_HMM_Data *dtmp;
    int maxlen = 0;
    for (dtmp = hmm->start; dtmp; dtmp = dtmp->next) {
      if (maxlen < dtmp->state_num) maxlen = dtmp->state_num;
    }
    hmm->maxstatenum = maxlen;
  }

  if (! hmm->variance_inversed) {
    /* inverse all variance values for faster computation */
    htk_hmm_inverse_variances(hmm);
    hmm->variance_inversed = TRUE;
  }

  j_printerr("finished\n");

  return (TRUE);
}
