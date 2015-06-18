/**
 * @file   voca_load_htkdict.c
 * @author Akinobu LEE
 * @date   Fri Feb 18 19:43:06 2005
 * 
 * <JA>
 * @brief  HTK形式の単語辞書データの読み込み
 *
 * トライフォンモデルを用いる場合，モノフォン表記からトライフォンへの
 * 変換およびモデルの存在チェックはこの辞書読み込み時に行なわれます．
 * </JA>
 * 
 * <EN>
 * @brief  Read word dictionary from a file in HTK format
 *
 * When using triphone model, conversion from monophone expression
 * in dictionary to triphone and the existence check of word-internal
 * triphone will be done here.
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

#include <sent/stddefs.h>
#include <sent/vocabulary.h>
#include <sent/htk_hmm.h>

/* 
 * dictinary format:
 * 
 * 1 words per line.
 * 
 * fields: GrammarEntry [OutputString] phone1 phone2 ....
 * 
 *     GrammarEntry
 *		   (for N-gram)
 *		   word name in N-gram
 *                 (for DFA)
 *                 terminal symbol ID
 *
 *     [OutputString]
 *		   String to output when the word is recognized.
 *
 *     {OutputString}
 *		   String to output when the word is recognized.
 *                 Also specifies that this word is transparent
 * 
 *     phone1 phon2 ....
 *		   sequence of logical HMM name (normally phoneme)
 *                 to express the pronunciation
 */

#define PHONEMELEN_STEP  30	///< Memory allocation step for phoneme sequence
static char buf[MAXLINELEN];	///< Local work area for input text processing
static char bufbak[MAXLINELEN];	///< Local work area for debug message

static char trbuf[3][20];	///< Local buffer for triphone convertion
static char chbuf[30];	     ///< Another local buffer for triphone convertion
static char nophone[1];		///< Local buffer to indicate 'no phone'
static int  trp_l;		///< Triphone cycle index
static int  trp;		///< Triphone cycle index
static int  trp_r;		///< Triphone cycle index

/** 
 * Return string of triphone name composed from last 3 call.
 * 
 * @param p [in] next phone string
 * 
 * @return the composed triphone name, or NULL on end.
 */
char *
cycle_triphone(char *p)
{
  int i;
  
  if (p == NULL) {		/* initialize */
    nophone[0]='\0';
    for(i=0;i<3;i++) trbuf[i][0] = '\0';
    trp_l = 0;
    trp   = 1;
    trp_r = 2;
    return NULL;
  }

  strcpy(trbuf[trp_r],p);

  chbuf[0]='\0';
  if (trbuf[trp_l][0] != '\0') {
    strcat(chbuf,trbuf[trp_l]);
    strcat(chbuf,HMM_LC_DLIM);
  }
  if (trbuf[trp][0] == '\0') {
    i = trp_l;
    trp_l = trp;
    trp = trp_r;
    trp_r = i;
    return NULL;
  }
  strcat(chbuf, trbuf[trp]);
  if (trbuf[trp_r][0] != '\0') {
    strcat(chbuf,HMM_RC_DLIM);
    strcat(chbuf,trbuf[trp_r]);
  }
  i = trp_l;
  trp_l = trp;
  trp = trp_r;
  trp_r = i;

  return(chbuf);
}

/** 
 * Flush the triphone buffer and return the last biphone.
 * 
 * @return the composed last bi-phone name.
 */
char *
cycle_triphone_flush()
{
  return(cycle_triphone(nophone));
}

/** 
 * Add a triphone name to the missing error list in WORD_INFO.
 * 
 * @param winfo [i/o] word dictionary to add the error phone to error list
 * @param name [in] phone name to be added
 */
static void
add_to_error(WORD_INFO *winfo, char *name)
{
  char *buf;
  char *match;

  buf = (char *)mymalloc(strlen(name) + 1);
  strcpy(buf, name);
  if (winfo->errph_root == NULL) {
    winfo->errph_root = aptree_make_root_node(buf);
  } else {
    match = aptree_search_data(buf, winfo->errph_root);
    if (!strmatch(match, buf)) {
      aptree_add_entry(buf, buf, match, &(winfo->errph_root));
    }
  }
}

/** 
 * Traverse callback function to output a error phone.
 * 
 * @param x [in] error phone string of the node
 */
static void
callback_list_error(void *x)
{
  char *name;
  name = x;
  j_printf("%s\n", name);
}
/** 
 * Output all error phones appeared while readin a word dictionary.
 * 
 * @param winfo [in] word dictionary data
 */
static void
list_error(WORD_INFO *winfo)
{
  j_printf("////// Missing phones:\n");
  aptree_traverse_and_do(winfo->errph_root, callback_list_error);
  j_printf("//////////////////////\n");
}

/** 
 * Parse a word dictionary and set the maximum state length per word.
 * 
 * @param winfo [i/o]
 */
static void
set_maxwn(WORD_INFO *winfo)
{
  int w,p,n;
  int maxwn;

  maxwn = 0;
  for (w=0;w<winfo->num;w++) {
    n = 0;
    for (p=0;p<winfo->wlen[w];p++) {
      n += hmm_logical_state_num(winfo->wseq[w][p]) - 2;
    }
    if (maxwn < n) maxwn = n;
  }
  winfo->maxwn = maxwn;
}

/** 
 * Parse the word dictionary to set the maximum word length.
 * 
 * @param winfo [i/o] word dictionary data
 */
static void
set_maxwlen(WORD_INFO *winfo)
{
  WORD_ID w;
  int maxwlen;
  maxwlen = 0;
  for(w=0;w<winfo->num;w++) {
    if (maxwlen < winfo->wlen[w]) maxwlen = winfo->wlen[w];
  }
  winfo->maxwlen = maxwlen;
}

/** 
 * Top function to read word dictionary via file pointer
 * 
 * @param fp [in] file pointer
 * @param winfo [out] pointer to word dictionary to store the read data.
 * @param hmminfo [in] HTK %HMM definition data.  if NULL, phonemes are ignored.
 * @param ignore_tri_conv [in] TRUE if triphone conversion is ignored
 * 
 * @return TRUE on success, FALSE on any error word.
 */
boolean
voca_load_htkdict(FILE *fp, WORD_INFO *winfo, HTK_HMM_INFO *hmminfo, boolean ignore_tri_conv)
{
  boolean ok_flag = TRUE;
  WORD_ID vnum;
  boolean do_conv = FALSE;

  if (hmminfo != NULL && hmminfo->is_triphone && (! ignore_tri_conv))
    do_conv = TRUE;

  winfo_init(winfo);

  vnum = 0;
  while (getl(buf, sizeof(buf), fp) != NULL) {
    if (vnum >= winfo->maxnum) winfo_expand(winfo);
    if (voca_load_htkdict_line(buf, vnum, winfo, hmminfo, do_conv, &ok_flag) == FALSE) break;
    vnum++;
  }
  winfo->num = vnum;

  if (winfo->errph_root != NULL) list_error(winfo);

  /* compute maxwn */
  set_maxwn(winfo);
  set_maxwlen(winfo);

  return(ok_flag);
}


/** 
 * Top function to read word dictionary via file descriptor.
 * 
 * @param fd [in] file descriptor
 * @param winfo [out] pointer to word dictionary to store the read data.
 * @param hmminfo [in] HTK %HMM definition data.  if NULL, phonemes are ignored.
 * @param ignore_tri_conv [in] TRUE if triphone conversion is ignored
 * 
 * @return TRUE on success, FALSE on any error word.
 */
boolean
voca_load_htkdict_fd(int fd, WORD_INFO *winfo, HTK_HMM_INFO *hmminfo, boolean ignore_tri_conv)
{
  boolean ok_flag = TRUE;
  WORD_ID vnum;
  boolean do_conv = FALSE;

  if (hmminfo != NULL && hmminfo->is_triphone && (! ignore_tri_conv))
    do_conv = TRUE;

  winfo_init(winfo);

  vnum = 0;
  while(getl_fd(buf, MAXLINELEN, fd) != NULL) {
    if (vnum >= winfo->maxnum) winfo_expand(winfo);
    if (voca_load_htkdict_line(buf, vnum, winfo, hmminfo, do_conv, &ok_flag) == FALSE) break;
    vnum++;
  }
  winfo->num = vnum;

  if (winfo->errph_root != NULL) list_error(winfo);

  /* compute maxwn */
  set_maxwn(winfo);
  set_maxwlen(winfo);

  return(ok_flag);
}

/** 
 * Top function to read word dictionary via socket descriptor.
 * 
 * @param sd [in] socket descriptor
 * @param winfo [out] pointer to word dictionary to store the read data.
 * @param hmminfo [in] HTK %HMM definition data.  if NULL, phonemes are ignored.
 * @param ignore_tri_conv [in] TRUE if triphone conversion is ignored
 * 
 * @return TRUE on success, FALSE on any error word.
 */
boolean
voca_load_htkdict_sd(int sd, WORD_INFO *winfo, HTK_HMM_INFO *hmminfo, boolean ignore_tri_conv)
{
  boolean ok_flag = TRUE;
  WORD_ID vnum;
  boolean do_conv = FALSE;
  
  if (hmminfo != NULL && hmminfo->is_triphone && (! ignore_tri_conv))
    do_conv = TRUE;
  
  winfo_init(winfo);
  
  vnum = 0;
  while(getl_sd(buf, MAXLINELEN, sd) != NULL) {
    if (vnum >= winfo->maxnum) winfo_expand(winfo);
    if (voca_load_htkdict_line(buf, vnum, winfo, hmminfo, do_conv, &ok_flag) == FALSE) break;
    vnum++;
  }
  winfo->num = vnum;
  
  if (winfo->errph_root != NULL) list_error(winfo);

  /* compute maxwn */
  set_maxwn(winfo);
  set_maxwlen(winfo);
  
  return(ok_flag);
}

/** 
 * Append a single entry to the existing word dictionary.
 * 
 * @param entry [in] dictionary entry string to be appended.
 * @param winfo [out] pointer to word dictionary to append the data.
 * @param hmminfo [in] HTK %HMM definition data.  if NULL, phonemes are ignored.
 * @param ignore_tri_conv [in] TRUE if triphone conversion is ignored
 * 
 * @return TRUE on success, FALSE on any error word.
 */
boolean
voca_append_htkdict(char *entry, WORD_INFO *winfo, HTK_HMM_INFO *hmminfo, boolean ignore_tri_conv)
{
  boolean ok_flag = TRUE;
  boolean do_conv = FALSE;

  if (hmminfo != NULL && hmminfo->is_triphone && (! ignore_tri_conv))
    do_conv = TRUE;

  if (winfo->num >= winfo->maxnum) winfo_expand(winfo);
  strcpy(buf, entry);		/* const buffer not allowed in voca_load_htkdict_line() */
  voca_load_htkdict_line(buf, winfo->num, winfo, hmminfo, do_conv, &ok_flag);

  if (ok_flag == TRUE) {
    winfo->num++;
    /* re-compute maxwn */
    set_maxwn(winfo);
    set_maxwlen(winfo);
  } else {
    if (winfo->errph_root != NULL) list_error(winfo);
  }

  return(ok_flag);
}

/** 
 * Sub function to Add a dictionary entry line to the word dictionary.
 * 
 * @param buf [i/o] buffer to hold the input string, will be modified in this function
 * @param vnum [in] current number of words in @a winfo
 * @param winfo [out] pointer to word dictionary to append the data.
 * @param hmminfo [in] HTK %HMM definition data.  if NULL, phonemes are ignored.
 * @param do_conv [in] TRUE if performing triphone conversion
 * @param ok_flag [out] will be set to FALSE if an error occured for this input.
 * 
 * @return FALSE if buf == "DICEND", else TRUE will be returned.
 */
boolean
voca_load_htkdict_line(char *buf, int vnum, WORD_INFO *winfo, HTK_HMM_INFO *hmminfo, boolean do_conv, boolean *ok_flag)
{
  char *ptmp, *lp = NULL, *p;
  static char cbuf[MAX_HMMNAME_LEN];
  static HMM_Logical **tmpwseq = NULL;
  static int tmpmaxlen;
  int len;
  HMM_Logical *tmplg;
  boolean pok;

  if (strmatch(buf, "DICEND")) return FALSE;

  /* allocate temporal work area for the first call */
  if (tmpwseq == NULL) {
    tmpmaxlen = PHONEMELEN_STEP;
    tmpwseq = (HMM_Logical **)mymalloc(sizeof(HMM_Logical *) * tmpmaxlen);
  }

  /* backup whole line for debug output */
  strcpy(bufbak, buf);
  
  /* GrammarEntry */
  if ((ptmp = mystrtok(buf, " \t\n")) == NULL) {
    j_printerr("line %d: corrupted data:\n> %s\n", vnum+1, bufbak);
    winfo->errnum++;
    *ok_flag = FALSE;
    return TRUE;
  }
  winfo->wname[vnum] = strcpy((char *)mybmalloc2(strlen(ptmp)+1, &(winfo->mroot)), ptmp);

  /* just move pointer to next token */
  if ((ptmp = mystrtok_movetonext(NULL, " \t\n")) == NULL) {
    j_printerr("line %d: corrupted data:\n> %s\n", vnum+1, bufbak);
    winfo->errnum++;
    *ok_flag = FALSE;
    return TRUE;
  }
#ifdef CLASS_NGRAM
  winfo->cprob[vnum] = 0.0;	/* prob = 1.0, logprob = 0.0 */
#endif
  
  if (ptmp[0] == '@') {		/* class N-gram prob */
#ifdef CLASS_NGRAM
    /* word probability within the class (for class N-gram) */
    /* format: classname @classprob wordname [output] phoneseq */
    /* classname equals to wname, and wordname will be omitted */
    /* format: @%f (log scale) */
    /* if "@" not found or "@0", it means class == word */
    if ((ptmp = mystrtok(NULL, " \t\n")) == NULL) {
      j_printerr("line %d: corrupted data:\n> %s\n", vnum+1, bufbak);
      winfo->errnum++;
      *ok_flag = FALSE;
      return TRUE;
    }
    if (ptmp[1] == '\0') {	/* space between '@' and figures */
      j_printerr("line %d: value after '@' missing, maybe wrong space?\n> %s\n", vnum+1, bufbak);
      winfo->errnum++;
      *ok_flag = FALSE;
      return TRUE;
    }
    winfo->cprob[vnum] = atof(&(ptmp[1]));
    if (winfo->cprob[vnum] != 0.0) winfo->cwnum++;
    /* read next word entry (just skip them) */
    if ((ptmp = mystrtok(NULL, " \t\n")) == NULL) {
      j_printerr("line %d: corrupted data:\n> %s\n", vnum+1,bufbak);
      winfo->errnum++;
      *ok_flag = FALSE;
      return TRUE;
    }
    /* move to the next word entry */
    if ((ptmp = mystrtok_movetonext(NULL, " \t\n")) == NULL) {
      j_printerr("line %d: corrupted data:\n> %s\n", vnum+1, bufbak);
      winfo->errnum++;
      *ok_flag = FALSE;
      return TRUE;
    }
#else  /* ~CLASS_NGRAM */
    j_printerr("line %d: cannot handle in-class word probability\n> %s\n", vnum+1, ptmp, bufbak);
    winfo->errnum++;
    *ok_flag = FALSE;
    return TRUE;
#endif /* CLASS_NGRAM */
  }

  /* OutputString */
  switch(ptmp[0]) {
  case '[':			/* not transparent word */
    winfo->is_transparent[vnum] = FALSE;
    ptmp = mystrtok_quotation(NULL, " \t\n", '[', ']', 0);
    break;
  case '{':			/* transparent word */
    winfo->is_transparent[vnum] = TRUE;
    ptmp = mystrtok_quotation(NULL, " \t\n", '{', '}', 0);
    break;
  default:
    j_printerr("line %d: missing output string??\n> %s\n", vnum+1, bufbak);
    winfo->errnum++;
    *ok_flag = FALSE;
    return TRUE;
  }
  if (ptmp == NULL) {
    j_printerr("line %d: corrupted data:\n> %s\n", vnum+1, bufbak);
    winfo->errnum++;
    *ok_flag = FALSE;
    return TRUE;
  }
  winfo->woutput[vnum] = strcpy((char *)mybmalloc2(strlen(ptmp)+1, &(winfo->mroot)), ptmp);
    
  /* phoneme sequence */
  if (hmminfo == NULL) {
    /* don't read */
    winfo->wseq[vnum] = NULL;
    winfo->wlen[vnum] = 0;
  } else {

    /* store converted phone sequence to temporal bufffer */
    len = 0;
      
    if (do_conv) {
      /* convert phoneme to triphone expression (word-internal) */
      cycle_triphone(NULL);
      if ((lp = mystrtok(NULL, " \t\n")) == NULL) {
	j_printerr("line %d: word %s has no phoneme:\n> %s\n", vnum+1, winfo->wname[vnum], bufbak);
	winfo->errnum++;
	*ok_flag = FALSE;
	return TRUE;
      }
      cycle_triphone(lp);
    }

    pok = TRUE;
    for (;;) {
      if (do_conv) {
/*	if (lp != NULL) j_printf(" %d%s",len,lp);*/
	if (lp != NULL) lp = mystrtok(NULL, " \t\n");
	if (lp != NULL) p = cycle_triphone(lp);
	else p = cycle_triphone_flush();
      } else {
	p = mystrtok(NULL, " \t\n");
      }
      if (p == NULL) break;

      /* both defined/pseudo phone is allowed */
      tmplg = htk_hmmdata_lookup_logical(hmminfo, p);
      if (tmplg == NULL) {
	/* not found */
	if (do_conv) {
	  /* both defined or pseudo phone are not found */
	  if (len == 0 && lp == NULL) {
	    j_printerr("line %d: triphone \"*-%s+*\" or monophone \"%s\" not found\n", vnum+1, p, p);
	    snprintf(cbuf,MAX_HMMNAME_LEN,"*-%s+* or monophone %s", p, p);
	  } else if (len == 0) {
	    j_printerr("line %d: triphone \"*-%s\" or biphone \"%s\" not found\n", vnum+1, p, p);
	    snprintf(cbuf,MAX_HMMNAME_LEN,"*-%s or biphone %s", p, p);
	  } else if (lp == NULL) {
	    j_printerr("line %d: triphone \"%s+*\" or biphone \"%s\" not found\n", vnum+1, p, p);
	    snprintf(cbuf,MAX_HMMNAME_LEN,"%s+* or biphone %s", p, p);
	  } else {
	    j_printerr("line %d: triphone \"%s\" not found\n", vnum+1, p);
	    snprintf(cbuf,MAX_HMMNAME_LEN,"%s", p);
	  }
	} else {
	  j_printerr("line %d: phone \"%s\" not found\n", vnum+1, p);
	  snprintf(cbuf, MAX_HMMNAME_LEN, "%s", p);
	}
	add_to_error(winfo, cbuf);
	pok = FALSE;
      } else {
	/* found */
	if (len >= tmpmaxlen) {
	  /* expand wseq area by PHONEMELEN_STEP */
	  tmpmaxlen += PHONEMELEN_STEP;
	  tmpwseq = (HMM_Logical **)myrealloc(tmpwseq, sizeof(HMM_Logical *) * tmpmaxlen);
	}
	/* store to temporal buffer */
	tmpwseq[len] = tmplg;
      }
      len++;
    }
    if (!pok) {			/* error in phoneme */
      j_printerr("> %s\n", bufbak);
      winfo->errnum++;
      *ok_flag = FALSE;
      return TRUE;
    }
    if (len == 0) {
      j_printerr("line %d: no phone specified:\n> %s\n", vnum+1, bufbak);
      winfo->errnum++;
      *ok_flag = FALSE;
      return TRUE;
    }
    /* store to winfo */
    winfo->wseq[vnum] = (HMM_Logical **)mybmalloc2(sizeof(HMM_Logical *) * len, &(winfo->mroot));
    memcpy(winfo->wseq[vnum], tmpwseq, sizeof(HMM_Logical *) * len);
    winfo->wlen[vnum] = len;
  }
  
  return(TRUE);
}

/** 
 * @brief Convert whole words in word dictionary to word-internal triphone.
 *
 * Normally triphone conversion will be performed directly when reading
 * dictionary file.  This function is for post conversion only.
 * 
 * @param winfo [i/o] word dictionary information
 * @param hmminfo [in] HTK %HMM definition
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
voca_mono2tri(WORD_INFO *winfo, HTK_HMM_INFO *hmminfo)
{
  WORD_ID w;
  int ph;
  char *p;
  HMM_Logical *tmplg;
  boolean ok_flag = TRUE;
  
  for (w=0;w<winfo->num;w++) {
    cycle_triphone(NULL);
    cycle_triphone(winfo->wseq[w][0]->name);

    for (ph = 0; ph < winfo->wlen[w] ; ph++) {
      if (ph == winfo->wlen[w] - 1) {
	p = cycle_triphone_flush();
      } else {
	p = cycle_triphone(winfo->wseq[w][ph + 1]->name);
      }
      if ((tmplg = htk_hmmdata_lookup_logical(hmminfo, p)) == NULL) {
	j_printerr("voca_mono2tri: word \"%s[%s]\"(id=%d): HMM \"%s\" not found\n", winfo->wname[w], winfo->woutput[w], w, p);
	ok_flag = FALSE;
	continue;
      }
      winfo->wseq[w][ph] = tmplg;
    }
  }
  return (ok_flag);
}

/** 
 * Append one word dictionary to other, for multiple grammar handling.
 * Assumes that the same %HMM definition is used on both word dictionary.
 * 
 * @param dstinfo [i/o] word dictionary
 * @param srcinfo [in] word dictionary to be appended to @a dst
 * @param coffset [in] category id offset in @a dst where the new data
 * should be stored
 * @param woffset [in] word id offset in @a dst where the new data
 * should be stored
 */
void
voca_append(WORD_INFO *dstinfo, WORD_INFO *srcinfo, int coffset, int woffset)
{
  WORD_ID n, w;
  int i;

  n = woffset;
  for(w=0;w<srcinfo->num;w++) {
    /* copy data */
    dstinfo->wlen[n] = srcinfo->wlen[w];
    dstinfo->wname[n] = strcpy((char *)mymalloc(strlen(srcinfo->wname[w])+1), srcinfo->wname[w]);
    dstinfo->woutput[n] = strcpy((char *)mymalloc(strlen(srcinfo->woutput[w])+1), srcinfo->woutput[w]);
    dstinfo->wseq[n] = (HMM_Logical **)mymalloc(sizeof(HMM_Logical *) * srcinfo->wlen[w]);
    for(i=0;i<srcinfo->wlen[w];i++) {
      dstinfo->wseq[n][i] = srcinfo->wseq[w][i];
    }
    dstinfo->is_transparent[n] = srcinfo->is_transparent[w];
    /* offset category ID by coffset */
    dstinfo->wton[n] = srcinfo->wton[w] + coffset;
    
    n++;
    if (n >= dstinfo->maxnum) winfo_expand(dstinfo);
  }
  dstinfo->num = n;

  /* compute maxwn */
  set_maxwn(dstinfo);
  set_maxwlen(dstinfo);
}

