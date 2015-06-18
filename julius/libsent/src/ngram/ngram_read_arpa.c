/**
 * @file   ngram_read_arpa.c
 * @author Akinobu LEE
 * @date   Wed Feb 16 16:52:24 2005
 * 
 * <JA>
 * @brief  ARPA形式のN-gramファイルを読み込む
 *
 * ARPA形式のN-gramファイルを用いる場合，2-gram と逆向き 3-gram を
 * それぞれ別々のファイルから読み込みます．
 * </JA>
 * 
 * <EN>
 * @brief  Read ARPA format N-gram files
 *
 * When N-gram data is given in ARPA format, both 2-gram file and
 * reverse 3-gram file should be specified.
 * </EN>
 *
 * @sa ngram2.h
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

/* $Id: ngram_read_arpa.c,v 1.6 2006/11/10 02:27:43 sumomo Exp $ */

/* words should be alphabetically sorted */

#include <sent/stddefs.h>
#include <sent/ngram2.h>

static char buf[800];			///< Local buffer for reading
static char pbuf[800];			///< Local buffer for error string 


/** 
 * Get N-gram word/class id of a string, and terminate program if not found.
 * 
 * @param ndata [in] N-gram data
 * @param str [in] name string of N-gram entry
 * 
 * @return the entry ID.
 */
static WORD_ID
lookup_word(NGRAM_INFO *ndata, char *str)
{
  WORD_ID wid;
  
  if ((wid = ngram_lookup_word(ndata, str)) == WORD_INVALID) {
    j_error("word %s not in N-gram vocabulary.\n",str);
  }
  return wid;
}

/** 
 * @brief  Set unknown word ID to the N-gram data.
 *
 * In CMU-Cam SLM toolkit, OOV words are always mapped to UNK, which
 * always appear at the very beginning of N-gram entry, so we fix the
 * unknown word ID at "0".
 * 
 * @param ndata [out] N-gram data to set unknown word ID.
 */
void
set_unknown_id(NGRAM_INFO *ndata)
{
#if 0
  ndata->unk_id = ngram_lookup_word(ndata, unkword);
  if (ndata->unk_id == WORD_INVALID) {
    j_printerr("word %s not found, so assume this is a closed vocabulary model\n",
	    unkword);
    ndata->isopen = FALSE;
  } else {
    ndata->isopen = TRUE;
  }
#endif
  ndata->isopen = TRUE;
  ndata->unk_id = 0;		/* unknown (OOV) words are always mapped to
				   the number 0 (by CMU-TK)*/
}


/** 
 * Set number of N-gram entries, for reading the first LR 2-gram.
 * 
 * @param fp [in] file pointer
 * @param ndata [out] N-gram data to set it.
 */
static void
set_total_info(FILE *fp, NGRAM_INFO *ndata)
{
  char *p;
  int n;

  while (getl(buf, sizeof(buf), fp) != NULL && buf[0] != '\\') {
    if (strnmatch(buf, "ngram", 5)) { /* n-gram num */
      p = strtok(buf, "=");
      n = p[strlen(p)-1] - '0' - 1;
      p = strtok(NULL, "=");
      ndata->ngram_num[n] = atoi(p);
    }
  }
}

/* read total info and check it with LR data (RL) */
/** 
 * Read number of N-gram entries of the second RL 3-gram, and
 * check if those values are exactly the same as the previous LR values.
 * 
 * @param fp [in] file pointer
 * @param ndata [i/o] N-gram data 
 */
static void
set_and_check_total_info(FILE *fp, NGRAM_INFO *ndata)
{
  char *p;
  int n;

  while (getl(buf, sizeof(buf), fp) != NULL && buf[0] != '\\') {
    if (strnmatch(buf, "ngram", 5)) { /* n-gram num */
      p = strtok(buf, "=");
      n = p[strlen(p)-1] - '0' - 1;
      p = strtok(NULL, "=");
/* 
 *	 if (n <= 2 && ndata->ngram_num[n] != atoi(p)) {
 *	   j_printerr("LR and RL don't match at ngram_num!\n");
 *	   j_error("cut-off value when building LM differ?\n");
 *	 }
 */
      if (n == 2) {		/* 3-gram */
	ndata->ngram_num[n] = atoi(p);
      } else {
	if (n <= 1 && ndata->ngram_num[n] != atoi(p)) {
	  j_printerr("Warning: %d-gram total num differ! may cause read error\n",n+1);
	}
      }
    }
  }
}

/** 
 * Read word/class entry names and 1-gram data from LR 2-gram file.
 * 
 * @param fp [in] file pointer
 * @param ndata [out] N-gram to set the read data.
 */
static void
set_unigram(FILE *fp, NGRAM_INFO *ndata)
{
  WORD_ID read_word_num;	/* # of words already read */
  WORD_ID nid, resid;
  LOGPROB prob, bo_wt;
  char *name, *p;

  /* malloc area */
  ndata->wname = (char **)mymalloc(sizeof(char *)*ndata->ngram_num[0]);
  ndata->p = (LOGPROB *)mymalloc(sizeof(LOGPROB)*ndata->ngram_num[0]);
  ndata->bo_wt_lr = (LOGPROB *)mymalloc(sizeof(LOGPROB)*ndata->ngram_num[0]);
  ndata->bo_wt_rl = (LOGPROB *)mymalloc(sizeof(LOGPROB)*ndata->ngram_num[0]);
  ndata->n2_bgn = (NNID *)mymalloc(sizeof(NNID)*ndata->ngram_num[0]);
  ndata->n2_num = (WORD_ID *)mymalloc(sizeof(WORD_ID)*ndata->ngram_num[0]);
  read_word_num = 0;
  
  while (getl(buf, sizeof(buf), fp) != NULL && buf[0] != '\\') {
    prob = (LOGPROB)atof(first_token(buf));
    p = next_token();
    name = strcpy((char *)mymalloc(strlen(p)+1), p);
    bo_wt = (LOGPROB)atof(next_token());

    /* register unigram */
    nid = read_word_num;
    ndata->wname[nid] = name;
    /* add entry name to index tree */
    if (ndata->root == NULL) {
      ndata->root = ptree_make_root_node(nid);
    } else {
      resid = ptree_search_data(name, ndata->root);
      if (strmatch(name, ndata->wname[resid])) { /* already exist */
	j_error("Error: word \"%s\" multiply defined at (#%d and #%d)\n",
		   name, resid, nid);
      } else {
	ptree_add_entry(name, nid, ndata->wname[resid], &(ndata->root));
      }
    }
    ndata->p[nid] = prob;
    ndata->bo_wt_lr[nid] = bo_wt;
    ndata->n2_bgn[nid] = NNID_INVALID;
    ndata->n2_num[nid] = 0;
  
    read_word_num++;
    if (read_word_num > ndata->max_word_num) {
      j_printerr("Error: actual n-gram word num exceeded header value\n");
      j_error("%d > %d\n", read_word_num, ndata->max_word_num);
    }
  }

  if (read_word_num != ndata->ngram_num[0]) {
    j_printerr("Error: actual n-gram word num not match the header value\n");
    j_error("%d != %d ?\n", read_word_num, ndata->ngram_num[0]);
  }
  j_printerr("  1-gram read %d end\n", read_word_num);
}

/* read-in 1-gram (RL) --- only add back-off weight */
/** 
 * Read 1-gram data from RL 3-gram file.  Only the back-off weights are
 * stored.
 * 
 * @param fp [in] file pointer
 * @param ndata [out] N-gram to store the read data.
 */
static void
add_unigram(FILE *fp, NGRAM_INFO *ndata)
{
  WORD_ID read_word_num;
  WORD_ID nid;
  LOGPROB prob, bo_wt;
  char *name, *p;

  read_word_num = 0;
  while (getl(buf, sizeof(buf), fp) != NULL && buf[0] != '\\') {
    prob = atof(first_token(buf));
    p = next_token();
    name = strcpy((char *)mymalloc(strlen(p)+1), p);
    bo_wt = (LOGPROB)atof(next_token());
  
    /* add bo_wt_rl to existing 1-gram entry */
    nid = lookup_word(ndata, name);
    if (nid == WORD_INVALID) {
      j_printerr("Warning: n-gram word \"%s\" in RL not exist in LR (ignored)\n", name);
    } else {
      ndata->bo_wt_rl[nid] = bo_wt;
    }
  
    read_word_num++;
    if (read_word_num > ndata->max_word_num) {
      j_printerr("Error: actual n-gram word num exceeded header value\n");
      j_error("%d > %d\n", read_word_num, ndata->max_word_num);
    }
    free(name);
  }
  j_printerr("  1-gram read %d end\n", read_word_num);
  
}

/** 
 * Read 2-gram data from LR 2-gram file.
 * 
 * @param fp [in] file pointer
 * @param ndata [out] N-gram to set the read data.
 */
static void
set_bigram(FILE *fp, NGRAM_INFO *ndata)
{
  int w_l, w_r;
  int w_last, w_r_last;
  LOGPROB p;
  NNID n2;

  ndata->n2tonid = (WORD_ID *)mymalloc(sizeof(WORD_ID)*ndata->ngram_num[1]);
  ndata->p_lr = (LOGPROB *)mymalloc(sizeof(LOGPROB)*ndata->ngram_num[1]);
  ndata->p_rl = (LOGPROB *)mymalloc(sizeof(LOGPROB)*ndata->ngram_num[1]);
  ndata->bo_wt_rrl = (LOGPROB *)mymalloc(sizeof(LOGPROB)*ndata->ngram_num[1]);

  n2 = 0;
  
  /* read in LR 2-gram */
  w_last = -1; w_r_last = -1;
  for (;;) {
    if (getl(buf, sizeof(buf), fp) == NULL || buf[0] == '\\') break;
    strcpy(pbuf, buf);
    if ( n2 % 100000 == 0) {
      j_printerr("  2-gram read %d (%d%%)\n", n2, n2 * 100 / ndata->ngram_num[1]);
    }

    /* 2-gram probability */
    p = (LOGPROB)atof(first_token(buf));
    /* read in left (context) word and lookup the ID */
    w_l = lookup_word(ndata, next_token());
    /* increment n2_bgn and n2_num if context word changed */
    if (w_l != w_last) {
      if (w_last != -1) ndata->n2_num[w_last] = n2 - ndata->n2_bgn[w_last];
      /* the next context word should be an new entry */
      if (ndata->n2_bgn[w_l] != NNID_INVALID) {
	j_printerr("Error: entry not sorted (same left context not sequenced)\n");
	j_error("at 2-gram #%d: \"%s\"\n", n2+1, pbuf);
      }
      ndata->n2_bgn[w_l] = n2;
      w_r_last = -1;
    }
    /* read in right word and set */
    w_r = lookup_word(ndata, next_token());
    if (w_r == w_r_last) {
      j_printerr("Error: duplicated entry\n");
      j_error("at 2-gram #%d: \"%s\"\n", n2+1, pbuf);
    } else if (w_r < w_r_last) {
      j_printerr("Error: entry not sorted downward\n");
      j_error("at 2-gram #%d: \"%s\"\n", n2+1, pbuf);
    }
    ndata->n2tonid[n2] = w_r;
    ndata->p_lr[n2] = p;

    n2++;
    w_last = w_l;
    w_r_last = w_r;

    /* check total num */
    if (n2 > ndata->ngram_num[1]) {
      j_printerr("Error: actual 2-gram num not match the header value\n");
      j_error("%d != %d ?\n", n2, ndata->ngram_num[1]);
    }
  }
  
  /* set the last entry */
  ndata->n2_num[w_last] = n2 - ndata->n2_bgn[w_last];

  j_printerr("  2-gram read %d end\n", n2);

}

/** 
 * Read reverse 2-gram data from RL 3-gram file, and set RL 2-gram
 * probabilities and back-off values for RL 3-gram to the corresponding
 * LR 2-gram data.
 * 
 * @param fp [in] file pointer
 * @param ndata [i/o] N-gram to set the read data.
 */
static void
add_bigram_rl(FILE *fp, NGRAM_INFO *ndata)
{
  WORD_ID w_l, w_r;
  LOGPROB prob, bo_wt;
  int bi_count = 0;
  NNID n2;

  while (getl(buf, sizeof(buf), fp) != NULL && buf[0] != '\\') {
    /* p(w_l|w_r) w_r w_l bo_wt_rl */
    if ( ++bi_count % 100000 == 0) {
      j_printerr("  2-gram read %d (%d%%)\n", bi_count, bi_count * 100 / ndata->ngram_num[1]);
    }
    prob = (LOGPROB)atof(first_token(buf));
    w_r = lookup_word(ndata, next_token());
    w_l = lookup_word(ndata, next_token());
    bo_wt = (LOGPROB)atof(next_token());
    n2 = search_bigram(ndata, w_l, w_r);
    if (n2 == NNID_INVALID) {
      j_printerr("Warning: (%s,%s) not exist in LR 2-gram (ignored)\n",
	      ndata->wname[w_l], ndata->wname[w_r]);
    } else {
      ndata->p_rl[n2] = prob;
      ndata->bo_wt_rrl[n2] = bo_wt;
    }
  }
  j_printerr("  2-gram read %d end\n", bi_count);
  
}
    

/** 
 * Read reverse 3-gram data from RL 3-gram file and store them.
 * 
 * @param fp [in] file pointer
 * @param ndata [i/o] N-gram to set the read data.
 */
static void
set_trigram(FILE *fp, NGRAM_INFO *ndata)
{
  int w_l, w_m, w_r;
  LOGPROB p_rl;
  int w_r_last, w_m_last, w_l_last;
  NNID n2, n2_last;
  NNID n3;
  NNID ntmp;

  /* allocate pointer from 2gram to 3gram */
  switch(ndata->version) {
  case 3:
    ndata->n3_bgn = (NNID *)mymalloc(sizeof(NNID)*ndata->ngram_num[1]);
    for(n2=0;n2<ndata->ngram_num[1];n2++) ndata->n3_bgn[n2] = NNID_INVALID;
    break;
  case 4:
    ndata->n3_bgn_upper = (NNID_UPPER *)mymalloc(sizeof(NNID_UPPER)*ndata->ngram_num[1]);
    ndata->n3_bgn_lower = (NNID_LOWER *)mymalloc(sizeof(NNID_LOWER)*ndata->ngram_num[1]);
    for(n2=0;n2<ndata->ngram_num[1];n2++) {    
      ndata->n3_bgn_upper[n2] = NNID_INVALID_UPPER;
      ndata->n3_bgn_lower[n2] = 0;
    }
    break;
  }
  ndata->n3_num = (WORD_ID *)mymalloc(sizeof(WORD_ID)*ndata->ngram_num[1]);
  for(n2=0;n2<ndata->ngram_num[1];n2++) ndata->n3_num[n2] = 0;

  /* allocate data area for 3-gram */
  ndata->n3tonid = (WORD_ID *)mymalloc(sizeof(WORD_ID)*ndata->ngram_num[2]);
  ndata->p_rrl = (LOGPROB *)mymalloc(sizeof(LOGPROB)*ndata->ngram_num[2]);
  n3 = 0;

  n2 = n2_last = NNID_INVALID;
  w_r_last = w_m_last = w_l_last = -1;
  for (;;) {

    if (getl(buf, sizeof(buf), fp) == NULL || buf[0] == '\\') break;
    strcpy(pbuf, buf);
    if (n3 % 100000 == 0) {
      j_printerr("  3-gram read %d (%d%%)\n", n3, n3 * 100 / ndata->ngram_num[2]);
    }

    /* N-gram probability */
    p_rl = (LOGPROB)atof(first_token(buf));
    /* read in right (first) word and lookup its ID */
    w_r = lookup_word(ndata, next_token());
    /* read in middle word and lookup its ID */
    w_m = lookup_word(ndata, next_token());

    /* if context changed, create the next structure */
    if (w_r != w_r_last || w_m != w_m_last) {
      n2 = search_bigram(ndata, (WORD_ID)w_m, (WORD_ID)w_r);
      if (n2 == NNID_INVALID) {	/* no context */
        j_printerr("Warning: context (%s,%s) not exist in LR 2-gram (ignored)\n",
		   ndata->wname[w_m], ndata->wname[w_r]);
        continue;
      }
      switch(ndata->version) {
      case 3:
	ntmp = ndata->n3_bgn[n2_last];
	break;
      case 4:
	ntmp = ((NNID)(ndata->n3_bgn_upper[n2_last]) << 16) + (NNID)(ndata->n3_bgn_lower[n2_last]);
	break;
      }
      if (n2_last != NNID_INVALID) ndata->n3_num[n2_last] = n3 - ntmp;
      /* check: the next 'n2' should be an new entry */
      switch(ndata->version) {
      case 3:
	if (ndata->n3_bgn[n2] != NNID_INVALID) {
	  j_printerr("Error: entry not sorted (same left context not sequenced)\n");
	  j_error("at 3-gram #%d: \"%s\"\n", n3+1, pbuf);
	}
	ndata->n3_bgn[n2] = n3;
	break;
      case 4:
	if (ndata->n3_bgn_upper[n2] != NNID_INVALID_UPPER) {
	  j_printerr("Error: entry not sorted (same left context not sequenced)\n");
	  j_error("at 3-gram #%d: \"%s\"\n", n3+1, pbuf);
	}
	ntmp = n3 & 0xffff;
	ndata->n3_bgn_lower[n2] = ntmp;
	ntmp = n3 >> 16;
	ndata->n3_bgn_upper[n2] = ntmp;
	break;
      }

      n2_last = n2;
      w_l_last = -1;
    } else {
      if (n2 == NNID_INVALID) continue;
    }
    
    /* read in left (last) word and store */
    w_l = lookup_word(ndata, next_token());
    if (w_l == w_l_last) {
      j_printerr("Error: duplicated entry\n");
      j_error("at 3-gram #%d: \"%s\"\n", n3+1, pbuf);
    } else if (w_l < w_l_last) {
      j_printerr("Error: entry not sorted downward\n");
      j_error("at 3-gram #%d: \"%s\"\n", n3+1, pbuf);
    }
    ndata->n3tonid[n3] = w_l;
    ndata->p_rrl[n3] = p_rl;

    n3++;
    w_m_last = w_m;
    w_r_last = w_r;
    w_l_last = w_l;

    /* check the 3-gram num */
    if (n3 > ndata->ngram_num[2]) {
      j_printerr("Error: actual 3-gram num not match the header value\n");
      j_error("%d != %d ?\n", n3, ndata->ngram_num[2]);
    }
  }

  /* store the last n3_num */
  switch(ndata->version) {
  case 3:
    ntmp = ndata->n3_bgn[n2_last];
    break;
  case 4:
    ntmp = ((NNID)(ndata->n3_bgn_upper[n2_last]) << 16) + (NNID)(ndata->n3_bgn_lower[n2_last]);
    break;
  }
  ndata->n3_num[n2_last] = n3 - ntmp;

  j_printerr("  3-gram read %d end\n", n3);
}


static boolean LR_2gram_read = FALSE; ///< TRUE if LR 2gram has already been read

/** 
 * Read in one ARPA N-gram file, either LR 2-gram or RL 3-gram.
 * 
 * @param fp [in] file pointer
 * @param ndata [out] N-gram data to store the read data
 * @param direction [in] specify whether this is LR 2-gram or RL 3-gram
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
ngram_read_arpa(FILE *fp, NGRAM_INFO *ndata, int direction)
{
  int n;

  ndata->from_bin = FALSE;

  if (!LR_2gram_read && direction == DIR_RL) {
    j_printerr("you should first read LR 2-gram\n");
    return FALSE;
  }

  if (direction == DIR_LR) {
    n = 2;
  } else {
    n = 3;
  }

  /* read until `\data\' found */
  while (getl(buf, sizeof(buf), fp) != NULL && strncmp(buf,"\\data\\",6) != 0);
    
  /* read n-gram total info */
  if (direction == DIR_LR) {
    set_total_info(fp, ndata);
  } else {
    set_and_check_total_info(fp, ndata);
  }
  if (ndata->ngram_num[0] > MAX_WORD_NUM) {
    j_error("Error: vocabulary size exceeded limit (%d)\n", MAX_WORD_NUM);
  }
  ndata->max_word_num = ndata->ngram_num[0];

  /* version requirement check (determined by 3-gram entry limit) */
  if (n >= 3) {
    if (ndata->ngram_num[2] >= NNIDMAX) {
      j_printerr("Warning: more than %d 3-gram tuples, use old structure\n", NNIDMAX);
      ndata->version = 3;
    } else {
      ndata->version = 4;
    }
  }
  
  /* read 1-gram data */
  if (!strnmatch(buf,"\\1-grams",8)) {
    j_error("data format error: 1-gram not found\n");
  }
  j_printerr("  reading 1-gram part...\n");
  if (direction == DIR_LR) {
    set_unigram(fp, ndata);
  } else {
    add_unigram(fp, ndata);
  }
  
  if (n >= 2) {
    /* read 2-gram data */
    if (!strnmatch(buf,"\\2-grams", 8)) {
      j_error("data format error: 2-gram not found\n");
    }
    j_printerr("  reading 2-gram part...\n");
    if (direction == DIR_LR) {
      set_bigram(fp, ndata);
    } else {
      add_bigram_rl(fp, ndata);
    }
  }

  if (n >= 3) {
      /* read 3-gram data */
    if (!strnmatch(buf,"\\3-grams", 8)) {
      j_error("data format error: 3-gram not found\n");
    }
    if ( direction == DIR_LR) {
      j_error("should not happen..\n");
    } else {
      j_printerr("  reading 3-gram part...\n");
      set_trigram(fp, ndata);
    }
  }

  /* finished */
  if (!strnmatch(buf, "\\end", 4)) {
    j_error("data format error: data end marker \"\\end\" not found\n");
  }
#ifdef CLASS_NGRAM
  /* skip in-class word entries (they should be in word dictionary) */
  if (getl(buf, sizeof(buf), fp) != NULL) {
    if (strnmatch(buf, "\\class", 6)) {
      j_printerr("  skipping in-class word entries...\n");
    }
  }
#endif

  if (n >= 3 && ndata->version == 4) {
    /* compact the 2-gram back-off and 3-gram links */
    ngram_compact_bigram_context(ndata);
  }
  
  /* set unknown (=OOV) word id */
  set_unknown_id(ndata);

  if (direction == DIR_LR) {
    LR_2gram_read = TRUE;
  }

  return TRUE;
}

/** 
 * Compact the 2-gram context information.
 * 
 * @param ndata [i/o] N-gram data
 */
void
ngram_compact_bigram_context(NGRAM_INFO *ndata)
{
  NNID i;
  int c;
  int dst;
  NNID ntmp;

  /* version check */
  if (ndata->version != 4) {
    j_error("InternalError: bigram context compaction called for version != 4\n");
  }

  /* count number of valid bigram context */
  c = 0;
  for(i=0;i<ndata->ngram_num[1];i++) {
    if (ndata->n3_bgn_upper[i] != NNID_INVALID_UPPER) {
      c++;
    } else {
      if (ndata->n3_num[i] != 0) {
	printf("bgn=%d|%d, num=%d, bo_wt_rrl=%f\n",
	       ndata->n3_bgn_upper[i], 
	       ndata->n3_bgn_lower[i], 
	       ndata->n3_num[i],
	       ndata->bo_wt_rrl[i]);
	j_error("Error: ngram_compact_bigram_context: internal error\n");
      }
      if (ndata->bo_wt_rrl[i] != 0.0) {
	j_error("Error: 2-gram has no upper 3-gram, but not 0.0 back-off weight\n");
      }
    }
  }
  ndata->bigram_bo_num = c;
  j_printerr("num: %d -> %d\n", ndata->ngram_num[1], ndata->bigram_bo_num);
  
  /* allocate index buffer */
  ndata->n2bo_upper = (NNID_UPPER *)mymalloc(sizeof(NNID_UPPER) * ndata->ngram_num[1]);
  ndata->n2bo_lower = (NNID_LOWER *)mymalloc(sizeof(NNID_LOWER) * ndata->ngram_num[1]);
  /* make index and do compaction of context informations */
  dst = 0;
  for(i=0;i<ndata->ngram_num[1];i++) {
    if (ndata->n3_bgn_upper[i] != NNID_INVALID_UPPER) {
      ndata->bo_wt_rrl[dst] = ndata->bo_wt_rrl[i];
      ndata->n3_bgn_upper[dst] = ndata->n3_bgn_upper[i];
      ndata->n3_bgn_lower[dst] = ndata->n3_bgn_lower[i];
      ndata->n3_num[dst] = ndata->n3_num[i];
      ntmp = dst & 0xffff;
      ndata->n2bo_lower[i] = ntmp;
      ntmp = dst >> 16;
      ndata->n2bo_upper[i] = ntmp;
      dst++;
    } else {
      ndata->n2bo_upper[i] = NNID_INVALID_UPPER;
      ndata->n2bo_lower[i] = 0;
    }
  }
  /* really shrink the memory area */
  ndata->bo_wt_rrl = (LOGPROB *)myrealloc(ndata->bo_wt_rrl, sizeof(LOGPROB) * ndata->bigram_bo_num);
  ndata->n3_bgn_upper = (NNID_UPPER *)myrealloc(ndata->n3_bgn_upper, sizeof(NNID_UPPER) * ndata->bigram_bo_num);
  ndata->n3_bgn_lower = (NNID_LOWER *)myrealloc(ndata->n3_bgn_lower, sizeof(NNID_LOWER) * ndata->bigram_bo_num);
  ndata->n3_num = (WORD_ID *)myrealloc(ndata->n3_num, sizeof(WORD_ID) * ndata->bigram_bo_num);
}
