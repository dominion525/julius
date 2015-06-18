/**
 * @file   ngram_read_bin.c
 * @author Akinobu LEE
 * @date   Wed Feb 16 17:12:08 2005
 * 
 * <JA>
 * @brief  バイナリ形式のN-gramファイルを読み込む
 *
 * バイナリ形式では 2-gram と逆向き 3-gram が1つのファイルに
 * 収められています．バイナリ形式はJuilus独自形式のみをサポートしており，
 * 他のバイナリ形式と互換性はありませんので注意して下さい．
 *
 * rev.3.5 より，バイナリN-gramのファイル形式の一部が変更されました．
 * バイトオーダーが Big endian 固定からマシン依存に変更され(ヘッダに
 * 変換時の条件を記述), またインデックスの 24bit 化
 * および 2-gram のバックオフデータの圧縮も行われました．
 * これにより，3.5 以降の mkbingram で生成したバイナリN-gramは,
 * 3.4.2以前の Julius では使えませんので注意してください．
 * (ヘッダチェックでエラーとなる)
 *
 * なお 3.5 以降の Julius では従来のモデルも問題なく読める．この場合,
 * インデックスの 24bit 化とバックオフの圧縮はモデル読み込み時に
 * その都度行われる．またバイトオーダーはヘッダを見て適宜変換するので，
 * 異なるバイトオーダーのマシンで生成した
 * バイナリN-gramでも問題なく読める．もちろん従来のモデルもそのまま
 * 読み込める．
 * 
 * </JA>
 * 
 * <EN>
 * @brief  Read binary foramt N-gram file
 *
 * In binary format, both 2-gram and reverse 3-gram are stored
 * together in one file.  This binary format is not
 * compatible with other binary format of language model.
 * 
 * From 3.5, internal format of binary N-gram has changed for using
 * machine-dependent natural byte order (previously fixed to big endian),
 * 24bit index and 2-gram backoff compression.  So, binary N-gram
 * generated by mkbingram of 3.5 and later will not work on 3.4.2 and
 * earlier versions.
 *
 * There is full upward- and cross-machine compatibility in 3.5.  Old
 * binary N-gram files still can be read directly, in which case the conversion
 * to 24bit index will performed just after model has been read.
 * Byte order will also considered by header information, so
 * binary N-gram still can be used among different machines.
 * </EN>
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

#include <sent/stddefs.h>
#include <sent/ngram2.h>

static int file_version;  ///< N-gram format version of the file
static boolean need_swap; ///< TRUE if need byte swap
#ifdef WORDS_INT
static boolean need_conv;	///< TRUE if need conversion of word ID from 2 bytes to 4 bytes
static boolean words_int_retry = FALSE; ///< TRUE if retrying with conversion
#endif

/** 
 * Binary read function with byte swap
 * 
 * @param fp [in] file pointer
 * @param buf [out] data buffer
 * @param unitbyte [in] unit size in bytes
 * @param unitnum [in] number of unit to read.
 */
static void
rdn(FILE *fp, void *buf, size_t unitbyte, int unitnum)
{
  size_t tmp;
  if ((tmp = myfread(buf, unitbyte, unitnum, fp)) < (size_t)unitnum) {
    perror("ngram_read_bin");
    j_error("read failed\n");
  }
  if (need_swap) {
    if (unitbyte != 1) {
      swap_bytes(buf, unitbyte, unitnum);
    }
  }

}

#ifdef WORDS_INT
/** 
 * Binary read function with byte swap and word id conversion
 * 
 * @param fp [in] file pointer
 * @param buf [out] data buffer
 * @param unitnum [in] number of unit to read.
 * @param need_conv [in] TRUE if need conversion from 2byte to 4byte
 */
static void
rdn_wordid(FILE *fp, void *buf, int unitnum, boolean need_conv)
{
  int i;
  unsigned short *s;
  WORD_ID *t;
  WORD_ID d;

  if (need_conv) {
    /* read unsigned short units */
    rdn(fp, buf, sizeof(unsigned short), unitnum);
    /* convert them to WORD_ID (integer) */
    for(i=unitnum-1;i>=0;i--) {
      s = (unsigned short *)buf + i;
      t = (WORD_ID *)buf + i;
      d = *s;
      *t = d;
    }
  } else {
    /* read as usual */
    rdn(fp, buf, sizeof(WORD_ID), unitnum);
  }
}
#endif

/** 
 * Check header to see whether the version matches.
 * 
 * @param fp [in] file pointer
 */
static void
check_header(FILE *fp)
{
  char buf[BINGRAM_HDSIZE], *p;
  rdn(fp, buf, 1, BINGRAM_HDSIZE);
  
  p = buf;
#ifdef WORDS_INT
  need_conv = FALSE;
#endif

  /* version check */
  if (strnmatch(p, BINGRAM_IDSTR, strlen(BINGRAM_IDSTR))) {
    /* bingram file made by mkbingram before 3.4.2 */
    file_version = 3;
    p += strlen(BINGRAM_IDSTR) + 1;
  } else if (strnmatch(p, BINGRAM_IDSTR_V4, strlen(BINGRAM_IDSTR_V4))) {
    /* bingram file made by mkbingram later than 3.5 */
    file_version = 4;
    p += strlen(BINGRAM_IDSTR_V4) + 1;
  } else {
    /* not a bingram file */
    j_printerr("Error: invalid header, you probably use an old bingram\n");
    j_error("Error: if so, please re-make with newer mkbingram that comes with Julius-2.0 or later\n");
  }
  /* word size check (for bingram build by mkbingram 3.3p5 and later */
  if (strnmatch(p, BINGRAM_SIZESTR_HEAD, strlen(BINGRAM_SIZESTR_HEAD))) {
    p += strlen(BINGRAM_SIZESTR_HEAD);
    if (! strnmatch(p, BINGRAM_SIZESTR_BODY, strlen(BINGRAM_SIZESTR_BODY))) {
      /* word size does not match (int / short) */
#ifdef WORDS_INT
      if (strnmatch(p, BINGRAM_SIZESTR_BODY_2BYTE, strlen(BINGRAM_SIZESTR_BODY_2BYTE))) {
	/* this is 2-byte word ID, will convert while reading */
	j_printerr("\nWarning: 2-bytes bingram, converting to 4 bytes\n");
	need_conv = TRUE;
	p += strlen(BINGRAM_SIZESTR_BODY_2BYTE) + 1;
      } else {
	j_error("\nError: unknown word byte size!\n");
      }
#else
      if (strnmatch(p, BINGRAM_SIZESTR_BODY_4BYTE, strlen(BINGRAM_SIZESTR_BODY_4BYTE))) {
	/*** 4bytes to 2bytes not implemented, just terminate here... ***/
	j_printerr("\nError: cannot handle 4-bytes bingram\n");
	j_error("Error: please use Julius compiled with --enable-words-int\n");
	//p += strlen(BINGRAM_SIZESTR_BODY_4BYTE) + 1;
      } else {
	j_error("\nError: unknown word byte size!\n");
      }
#endif
    } else {
      p += strlen(BINGRAM_SIZESTR_BODY) + 1;
    }

    /* byte order check (v4 (rev.3.5) and later) */
    if (file_version == 4) {
      if (!strnmatch(p, BINGRAM_BYTEORDER_HEAD, strlen(BINGRAM_BYTEORDER_HEAD))) {
	j_error("\nError: no information for byte order??\n");
      }
      p += strlen(BINGRAM_BYTEORDER_HEAD);
      if (! strnmatch(p, BINGRAM_NATURAL_BYTEORDER, strlen(BINGRAM_NATURAL_BYTEORDER))) {
	/* file endian and running endian is different, need swapping */
	need_swap = TRUE;
      } else {
	need_swap = FALSE;
      }
      p += strlen(BINGRAM_NATURAL_BYTEORDER) + 1;
    }
  } /* if no BINGRAM_SIZESTR_HEAD found, just pass it */

  /* in case of V3 bingram file, the unit size of word_id and its byte order
     cannot be determined from the header.  In that case, we assume 
     byteorder to be a BIG ENDIAN.  The word_id unit size (2byte in normal,
     or 4byte if bingram generated with mkbingram with --enable-words-int)
     will be automagically detected.
     */

  if (file_version != 4) {
    /* assume input as big endian */
#ifdef WORDS_BIGENDIAN
    need_swap = FALSE;
#else
    need_swap = TRUE;
#endif
  }
    
  /*j_printf("%s",buf);*/
}

/** 
 * Read a N-gram binary file and store to data.
 * 
 * @param fp [in] file pointer
 * @param ndata [out] N-gram data to store the read data
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
ngram_read_bin(FILE *fp, NGRAM_INFO *ndata)
{
  int i,n,len;
  char *w, *p;
  NNID *n3_bgn;
  NNID d, ntmp;
#ifdef WORDS_INT
  unsigned short *buf;
#endif

#ifdef WORDS_INT
  /* reset retry flag */
  words_int_retry = FALSE;

  /* when retrying, it restarts from here with words_int_retry = TRUE */
 ngram_read_bin_start:

#endif
  
  ndata->from_bin = TRUE;

  /* check initial header */
  check_header(fp);

#ifdef WORDS_INT
  /* in retry mode, force word_id conversion  */
  if (words_int_retry) need_conv = TRUE;
#endif
  
#ifdef WORDS_INT
  if (need_conv) j_printerr("(wordid conv)..");
#endif

  /* read total info and set max_word_num */
  for(n=0;n<MAX_N;n++) {
    rdn(fp, &(ndata->ngram_num[n]), sizeof(NNID), 1);
    if (file_version == 4 && ndata->ngram_num[n] >= NNIDMAX) {
      j_error("Error: too big %d-gram (%d, should be less than %d)\n", n+1, ndata->ngram_num[n], NNIDMAX);
    }
  }
  ndata->max_word_num = ndata->ngram_num[0];
  if (file_version == 4) rdn(fp, &(ndata->bigram_bo_num), sizeof(NNID), 1);

  /* version requirement check */
  switch(file_version) {
  case 4:
    ndata->version = 4;
    break;
  case 3:
    if (ndata->ngram_num[2] >= NNIDMAX) {
      j_printerr("Warning: more than %d 3-gram tuples, use old structure\n", NNIDMAX);
      ndata->version = 3;
    } else {
      ndata->version = 4;	/* will be converted to v4 later */
    }
    break;
  }

  /* read wname */
  rdn(fp, &len, sizeof(int), 1);
  w = mymalloc(len);
  rdn(fp, w, 1, len);
  /* assign... */
  ndata->wname = (char **)mymalloc(sizeof(char *)*ndata->ngram_num[0]);
  p = w; i = 0;
  while (p < w + len) {
    ndata->wname[i++] = p;
    while(*p != '\0') p++;
    p++;
  }
  if (i != ndata->ngram_num[0]) {
    j_error("wname error??\n");
  }
  /* malloc 1-gram */
  ndata->p = (LOGPROB *)mymalloc(sizeof(LOGPROB) * ndata->ngram_num[0]);
  ndata->bo_wt_lr = (LOGPROB *)mymalloc(sizeof(LOGPROB) * ndata->ngram_num[0]);
  ndata->bo_wt_rl = (LOGPROB *)mymalloc(sizeof(LOGPROB) * ndata->ngram_num[0]);
  ndata->n2_bgn = (NNID *)mymalloc(sizeof(NNID) * ndata->ngram_num[0]);
  ndata->n2_num = (WORD_ID *)mymalloc(sizeof(WORD_ID) * ndata->ngram_num[0]);

  /* read 1-gram */
  j_printerr("1-gram.");
  rdn(fp, ndata->p, sizeof(LOGPROB), ndata->ngram_num[0]);
  j_printerr(".");
  rdn(fp, ndata->bo_wt_lr, sizeof(LOGPROB), ndata->ngram_num[0]);
  j_printerr(".");
  rdn(fp, ndata->bo_wt_rl, sizeof(LOGPROB), ndata->ngram_num[0]);
  j_printerr(".");
  rdn(fp, ndata->n2_bgn, sizeof(NNID), ndata->ngram_num[0]);
  j_printerr(".");
#ifdef WORDS_INT
  rdn_wordid(fp, ndata->n2_num, ndata->ngram_num[0], need_conv);
#else
  rdn(fp, ndata->n2_num, sizeof(WORD_ID), ndata->ngram_num[0]);
#endif

#ifdef WORDS_INT
  {
    /* check if we are wrongly reading word_id=2byte bingram
       (if bingram version >= 4, this should not be happen because
        header correctly tells the word_id byte size.  This will 
	occur only if matches all the conditions below:
	- you run Julius with --enable-words-int,
	- you use old bingram of version <= 3, and
	- you use bingram file converted without --enable-words-int
     */
    WORD_ID w;
    for(w=0;w<ndata->ngram_num[0];w++) {
      if (ndata->n2_num[w] > ndata->ngram_num[0]) {
	if (words_int_retry) {
	  j_error("\nError: retry failed, wrong bingram format\n");
	}
	j_printerr("\nWarning: incorrect data, may be a 2-byte v3 bingram, retry with converion\n");
	free(ndata->wname[0]);
	free(ndata->wname);
	free(ndata->p);
	free(ndata->bo_wt_lr);
	free(ndata->bo_wt_rl);
	free(ndata->n2_bgn);
	free(ndata->n2_num);
	myfrewind(fp);
	words_int_retry = TRUE;
	goto ngram_read_bin_start;
      }
    }
  }
#endif

  /* malloc the rest */
  ndata->n2tonid = (WORD_ID *)mymalloc(sizeof(WORD_ID) * ndata->ngram_num[1]);
  ndata->p_lr = (LOGPROB *)mymalloc(sizeof(LOGPROB) * ndata->ngram_num[1]);
  ndata->p_rl = (LOGPROB *)mymalloc(sizeof(LOGPROB) * ndata->ngram_num[1]);
  if (file_version == 4) {
    ndata->n2bo_upper = (NNID_UPPER *)mymalloc(sizeof(NNID_UPPER) * ndata->ngram_num[1]);
    ndata->n2bo_lower = (NNID_LOWER *)mymalloc(sizeof(NNID_LOWER) * ndata->ngram_num[1]);
    ndata->bo_wt_rrl = (LOGPROB *)mymalloc(sizeof(LOGPROB) * ndata->bigram_bo_num);
    ndata->n3_bgn_upper = (NNID_UPPER *)mymalloc(sizeof(NNID_UPPER) * ndata->bigram_bo_num);
    ndata->n3_bgn_lower = (NNID_LOWER *)mymalloc(sizeof(NNID_LOWER) * ndata->bigram_bo_num);
    ndata->n3_num = (WORD_ID *)mymalloc(sizeof(WORD_ID) * ndata->bigram_bo_num);
  } else {
    ndata->bo_wt_rrl = (LOGPROB *)mymalloc(sizeof(LOGPROB) * ndata->ngram_num[1]);
    ndata->n3_num = (WORD_ID *)mymalloc(sizeof(WORD_ID) * ndata->ngram_num[1]);
    if (ndata->version == 4) {
      ndata->n3_bgn_upper = (NNID_UPPER *)mymalloc(sizeof(NNID_UPPER) * ndata->ngram_num[1]);
      ndata->n3_bgn_lower = (NNID_LOWER *)mymalloc(sizeof(NNID_LOWER) * ndata->ngram_num[1]);
      n3_bgn = (NNID *)mymalloc(sizeof(NNID) * ndata->ngram_num[1]);
    } else {
      ndata->n3_bgn = (NNID *)mymalloc(sizeof(NNID) * ndata->ngram_num[1]);
    }
  }
      
  ndata->n3tonid = (WORD_ID *)mymalloc(sizeof(WORD_ID) * ndata->ngram_num[2]);
  ndata->p_rrl = (LOGPROB *)mymalloc(sizeof(LOGPROB) * ndata->ngram_num[2]);
  
  /* read 2-gram*/
  j_printerr("2-gram.");
#ifdef WORDS_INT
  rdn_wordid(fp, ndata->n2tonid, ndata->ngram_num[1], need_conv);
#else
  rdn(fp, ndata->n2tonid, sizeof(WORD_ID), ndata->ngram_num[1]);
#endif
  j_printerr(".");
  rdn(fp, ndata->p_lr, sizeof(LOGPROB), ndata->ngram_num[1]);
  j_printerr(".");
  rdn(fp, ndata->p_rl, sizeof(LOGPROB), ndata->ngram_num[1]);
  j_printerr(".");
  if (file_version == 4) {
    rdn(fp, ndata->n2bo_upper, sizeof(NNID_UPPER), ndata->ngram_num[1]);
    rdn(fp, ndata->n2bo_lower, sizeof(NNID_LOWER), ndata->ngram_num[1]);
    j_printerr(".");
    rdn(fp, ndata->bo_wt_rrl, sizeof(LOGPROB), ndata->bigram_bo_num);
    j_printerr(".");
    rdn(fp, ndata->n3_bgn_upper, sizeof(NNID_UPPER), ndata->bigram_bo_num);
    rdn(fp, ndata->n3_bgn_lower, sizeof(NNID_LOWER), ndata->bigram_bo_num);
    j_printerr(".");
#ifdef WORDS_INT
    rdn_wordid(fp, ndata->n3_num, ndata->bigram_bo_num, need_conv);
#else
    rdn(fp, ndata->n3_num, sizeof(WORD_ID), ndata->bigram_bo_num);
#endif
  } else {
    rdn(fp, ndata->bo_wt_rrl, sizeof(LOGPROB), ndata->ngram_num[1]);
    j_printerr(".");
    if (ndata->version == 4) {
      rdn(fp, n3_bgn, sizeof(NNID), ndata->ngram_num[1]);
      for(d=0;d<ndata->ngram_num[1];d++) {
	if (n3_bgn[d] == NNID_INVALID) {
	  ndata->n3_bgn_lower[d] = 0;
	  ndata->n3_bgn_upper[d] = NNID_INVALID_UPPER;
	} else {
	  ntmp = n3_bgn[d] & 0xffff;
	  ndata->n3_bgn_lower[d] = ntmp;
	  ntmp = n3_bgn[d] >> 16;
	  ndata->n3_bgn_upper[d] = ntmp;
	}
      }
    } else {
      rdn(fp, ndata->n3_bgn, sizeof(NNID), ndata->ngram_num[1]);
    }
    j_printerr(".");
#ifdef WORDS_INT
    rdn_wordid(fp, ndata->n3_num, ndata->ngram_num[1], need_conv);
#else
    rdn(fp, ndata->n3_num, sizeof(WORD_ID), ndata->ngram_num[1]);
#endif
  }

  /* read 3-gram*/
  j_printerr("3-gram.");
#ifdef WORDS_INT
  rdn_wordid(fp, ndata->n3tonid, ndata->ngram_num[2], need_conv);
#else
  rdn(fp, ndata->n3tonid, sizeof(WORD_ID), ndata->ngram_num[2]);
#endif
  j_printerr(".");
  rdn(fp, ndata->p_rrl, sizeof(LOGPROB), ndata->ngram_num[2]);

  /* make word search tree for later lookup */
  j_printerr("indexing...");
  ngram_make_lookup_tree(ndata);

  /* compact the 2-gram back-off and 3-gram links */
  if (file_version != 4 && ndata->version == 4) {
    free(n3_bgn);
    ngram_compact_bigram_context(ndata);
  }
  
  /* set unknown id */
  set_unknown_id(ndata);
  
  return TRUE;
}
