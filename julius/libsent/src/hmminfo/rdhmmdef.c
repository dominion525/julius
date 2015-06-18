/**
 * @file   rdhmmdef.c
 * @author Akinobu LEE
 * @date   Wed Feb 16 00:17:18 2005
 * 
 * <JA>
 * @brief  HTK %HMM 定義ファイルの読み込み：メイン
 *
 * ここには HTK 形式の %HMM 定義ファイルを読み込むための関数群を呼び出す
 * メイン関数が収められています．
 *
 * このファイルはまた，読み込み関数群で共通して用いられるトークン単位の
 * ファイル読み込み関数を提供します．
 * %HMM 定義ファイルは read_token() によってトークン単位で順次読み込まれ，
 * グローバル変数 rdhmmdef_token に格納されます．各関数群はこの
 * rdhmmdef_token を参照して現在のトークンを得ます．
 * </JA>
 * 
 * <EN>
 * @brief  Read HTK %HMM definition file: the main
 *
 * This file includes the main routine to read the %HMM definition file in
 * HTK format.
 *
 * This file also contains functions and global variables for per-token
 * reading tailored for reading HTK %HMM definition file.  The read_token()
 * will read the file per token, and the read token is stored in a global
 * variable rdhmmdef_token.  The other reading function will refer to this
 * variable to read the current token.
 * </EN>
 * 
 * $Revision: 1.5 $
 * 
 */
/*
 * Copyright (c) 1991-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/htk_param.h>
#include <sent/htk_hmm.h>

#define MAXBUFLEN  4096		///< Maximum length of a line in the input

char *rdhmmdef_token;		///< Current token string (GLOBAL)
static char *buf = NULL;	///< Local work area for token reading
static int line;		///< Input Line count

/* global functions for rdhmmdef_*.c */

/** 
 * Output error message, with current reading status.
 * 
 * @param str [in] error string
 */
void
rderr(char *str)
{
  if (rdhmmdef_token == NULL) {	/* end of file */
    j_error("\nError: %s on end of file\n", str);
  } else {
    j_error("\nError at line %d: %s\n", line, (str) ? str : "parse error");
  }
}

/** 
 * Read next token and ste it to rdhmmdef_token.
 * 
 * @param fp [in] file pointer
 * 
 * @return the pointer to the read token, or NULL on end of file or error.
 */
char *
read_token(FILE *fp)
{
  if (buf != NULL) {
    /* already have buffer */
    if ((rdhmmdef_token = mystrtok_quote(NULL, HMMDEF_DELM)) != NULL) {
      /* return next token */
      return rdhmmdef_token;
    }
  } else {
    /* init: allocate buffer for the first time */
    buf = (char *)mymalloc(MAXBUFLEN);
    line = 1;
  }
  /* read new 1 line */
  if (getl(buf, MAXBUFLEN, fp) == NULL) {
    rdhmmdef_token = NULL;
  } else {
    rdhmmdef_token = mystrtok_quote(buf, HMMDEF_DELM);
    line++;
  }
  return rdhmmdef_token;
}

/** 
 * Convert all the transition probabilities to log10 scale.
 * 
 * @param hmm [i/o] %HMM definition data to modify.
 */
static void
conv_log_arc(HTK_HMM_INFO *hmm)
{
  HTK_HMM_Trans *tr;
  int i,j;
  LOGPROB l;

  for (tr = hmm->trstart; tr; tr = tr->next) {
    for(i=0;i<tr->statenum;i++) {
      for(j=0;j<tr->statenum;j++) {
	l = tr->a[i][j];
	tr->a[i][j] = (l != 0.0) ? (float)log10(l) : LOG_ZERO;
      }
    }
  }
}
/** 
 * Invert all the variance values.
 * 
 * @param hmm [i/o] %HMM definition data to modify.
 */
void
htk_hmm_inverse_variances(HTK_HMM_INFO *hmm)
{
  HTK_HMM_Var *v;
  int i,j;
  LOGPROB l;

  for (v = hmm->vrstart; v; v = v->next) {
    for(i=0;i<v->len;i++) {
      v->vec[i] = 1 / v->vec[i];
    }
  }
}


/** 
 * @brief  Main top routine to read in HTK %HMM definition file.
 *
 * A HTK %HMM definition file will be read from @a fp.  After reading,
 * the parameter type is checked and calculate some statistics.
 * 
 * @param fp [in] file pointer
 * @param hmm [out] pointer to a %HMM definition structure to store data.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
rdhmmdef(FILE *fp, HTK_HMM_INFO *hmm)
{
  char macrosw;
  char *name;

  /* variances in htkdefs are not inversed yet */
  hmm->variance_inversed = FALSE;

  /* read the first token */
  read_token(fp);
  
  /* the toplevel loop */
  while (rdhmmdef_token != NULL) {/* break on EOF */
    if (rdhmmdef_token[0] != '~') { /* toplevel commands are always macro */
      return FALSE;
    }
    macrosw = rdhmmdef_token[1];
    read_token(fp);		/* read next token after the "~.."  */
    switch(macrosw) {
    case 'o':			/* global option */
      set_global_opt(fp,hmm);
      break;
    case 't':			/* transition macro */
      name = mybstrdup2(rdhmmdef_token, &(hmm->mroot));
      if (strlen(name) >= MAX_HMMNAME_LEN) rderr("Macro name too long");
      read_token(fp);
      def_trans_macro(name, fp, hmm);
      break;
    case 's':			/* state macro */
      name = mybstrdup2(rdhmmdef_token, &(hmm->mroot));
      if (strlen(name) >= MAX_HMMNAME_LEN) rderr("Macro name too long");
      read_token(fp);
      def_state_macro(name, fp, hmm);
      break;
    case 'm':			/* density (mixture) macro */
      name = mybstrdup2(rdhmmdef_token, &(hmm->mroot));
      if (strlen(name) >= MAX_HMMNAME_LEN) rderr("Macro name too long");
      read_token(fp);
      def_dens_macro(name, fp, hmm);
      break;
    case 'h':			/* HMM define */
      name = mybstrdup2(rdhmmdef_token, &(hmm->mroot));
      if (strlen(name) >= MAX_HMMNAME_LEN) rderr("Macro name too long");
      read_token(fp);
      def_HMM(name, fp, hmm);
      break;
    case 'v':			/* Variance macro */
      name = mybstrdup2(rdhmmdef_token, &(hmm->mroot));
      if (strlen(name) >= MAX_HMMNAME_LEN) rderr("Macro name too long");
      read_token(fp);
      def_var_macro(name, fp, hmm);
      break;
    case 'r':			/* Regression class macro (ignore) */
      name = mybstrdup2(rdhmmdef_token, &(hmm->mroot));
      if (strlen(name) >= MAX_HMMNAME_LEN) rderr("Macro name too long");
      read_token(fp);
      def_regtree_macro(name, fp, hmm);
      break;
    }
  }

  j_printerr("(ascii)...");
  
  /* check limitation */
  if (check_all_hmm_limit(hmm)) {
    j_printerr("limit check passed\n");
  } else {
    j_error("Error: cannot use this HMM for system limitation.\n");
  }
  /* convert transition prob to log scale */
  conv_log_arc(hmm);

  /* inverse all variance values for faster computation */
  if (! hmm->variance_inversed) {
    htk_hmm_inverse_variances(hmm);
    hmm->variance_inversed = TRUE;
  }
  
  /* check HMM parameter option type */
  if (!check_hmm_options(hmm)) {
    j_error("hmm options check failed\n");
  }

  /* add ID number for all HTK_HMM_State */
  /* also calculate the maximum number of mixture */
  {
    HTK_HMM_State *stmp;
    int n, max;
    n = 0;
    max = 0;
    for (stmp = hmm->ststart; stmp; stmp = stmp->next) {
      if (max < stmp->mix_num) max = stmp->mix_num;
      stmp->id = n++;
      if (n >= MAX_STATE_NUM) {
	j_error("Error: too much states > %d\n", MAX_STATE_NUM);
      }
    }
    hmm->totalstatenum = n;
    hmm->maxmixturenum = max;
  }
  /* compute total number of HMM models and maximum length */
  {
    HTK_HMM_Data *dtmp;
    int n, maxlen;
    n = 0;
    maxlen = 0;
    for (dtmp = hmm->start; dtmp; dtmp = dtmp->next) {
      if (maxlen < dtmp->state_num) maxlen = dtmp->state_num;
      n++;
    }
    hmm->maxstatenum = maxlen;
    hmm->totalhmmnum = n;
  }
  /* compute total number of mixtures */
  {
    HTK_HMM_Dens *dtmp;
    int n = 0;
    for (dtmp = hmm->dnstart; dtmp; dtmp = dtmp->next) {
      n++;
    }
    hmm->totalmixnum = n;
  }
  /* check of HMM name length exceed the maximum */
  {
    HTK_HMM_Dens *dtmp;
    int n = 0;
    for (dtmp = hmm->dnstart; dtmp; dtmp = dtmp->next) {
      n++;
    }
    hmm->totalmixnum = n;
  }

  return(TRUE);			/* success */
}
