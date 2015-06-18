/**
 * @file   hmm_check.c
 * @author Akinobu LEE
 * @date   Thu Mar 17 20:50:07 2005
 * 
 * <JA>
 * @brief  トライフォンモデルのデバッグ用関数
 * </JA>
 * 
 * <EN>
 * @brief  Functions for checking triphone model
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

#include <julius.h>

#define PHSTEP 10		///< Malloc step for phoneme conversion

/** 
 * <JA>
 * @brief  音素列からHMM列への変換を行ない，結果を表示する．
 *
 * このルーチンは，Julius/Julian に与えられた音響モデルと
 * HMMList ファイルにおいて，音素列からHMM列への変換をテストする
 * ための関数である．
 * 
 * 空白で区切られた音素列の文字列に対して，トライフォンモデル使用時には
 * コンテキストが考慮され，最終的に対応する HMM 列へ変換される．
 * その後，変換した結果を，
 *   - 音素列から導かれる本来の適用すべきモデル名
 *   - 上記を HMMList にしたがって変換した論理 HMM 名
 *   - 実際に計算で用いられる物理HMM名または pseudo HMM 名
 * の順に出力する．
 * 
 * なお，文字列中に "|" を含めることで，そこを単語区切りとして扱い，
 * トライフォンにおいて単語間の展開を考慮することができる．
 * 
 * @param str [i/o] 空白で区切られた音素列の文字列
 * @param hmminfo [in] HMM定義構造体
 * @param len_ret [out] 返り値の論理 HMM の要素数
 * 
 * @return 新たにメモリ割り付けられた変換後の論理HMMのポインタ列
 * </JA>
 * <EN>
 * @brief  Convert phoneme sequences to logical HMM sequences, and output the
 * result.
 *
 * This function is for testing procedure to convert words in dictionary
 * to corresponding HMM sequences in Julius/Julian, given an HMMList and
 * HTK HMM definition.
 *
 * Given a space-separated list of phoneme names in a string, each phonemes
 * will be converted to context-dependent style (if using triphone model),
 * and then converted to HMM sequence that will finally be used for
 * recognition.  Then, the following data will be output for all HMM:
 *   - Original phone HMM name,
 *   - Logical HMM name that is converted from above,
 *   - Physical or pseudo HMM name that will actually be used.
 *
 * Additionally, specifying '|' in the string gives a word boundary between
 * phonemes, and triphone conversion will consider the cross-word expansion.
 * 
 * @param str [i/o] string that contains space-saparated phoneme sequence.
 * @param hmminfo [in] HMM definition structure
 * @param len_ret [out] num of elements in the return value
 * 
 * @return the newly allocated pointer array to the converted logical HMMs.
 * </EN>
 */
static HMM_Logical **
new_str2phseq(char *str, HTK_HMM_INFO *hmminfo, int *len_ret)
{
  char **tokens;
  boolean *word_end;
  int phnum;
  boolean word_mode = FALSE;
  HMM_Logical **new;
  static char buf[MAX_HMMNAME_LEN];
  
  /* read in string and divide into token unit */
  {
    char *p;
    int tokenmax;
    tokenmax = PHSTEP;
    tokens = (char **)mymalloc(sizeof(char *) * tokenmax);
    word_end = (boolean *)mymalloc(sizeof(boolean) * tokenmax);
    phnum = 0;
    for(p = first_token(str); p; p = next_token_if_any()) {
      if (strmatch(p, "|")) {
	word_mode = TRUE;
	if (phnum > 0) word_end[phnum-1] = TRUE;
	continue;
      }
      if (phnum >= tokenmax) {
	tokenmax += PHSTEP;
	tokens = (char **)myrealloc(tokens, sizeof(char *) * tokenmax);
	word_end = (boolean *)myrealloc(word_end, sizeof(boolean) * tokenmax);
      }
      tokens[phnum] = strcpy((char *)mymalloc(strlen(p)+1), p);
      word_end[phnum] = FALSE;
      phnum++;
    }
    if (phnum == 0) {
      j_printf("failed: no phone specified\n");
      new = NULL;
      goto spend;
    }
    word_end[phnum-1] = TRUE;
  }
  /* check if the phonemes exist in basephone list */
  {
    BASEPHONE *ph;
    int i;
    boolean ok_flag = TRUE;
    for (i=0;i<phnum;i++) {
      ph = aptree_search_data(tokens[i], hmminfo->basephone.root);
      if (! strmatch(ph->name, tokens[i])) {
	j_printf("         %2d: unknown phone \"%s\"\n", i+1, tokens[i]);
	ok_flag = FALSE;
	continue;
      }
    }
    if (! ok_flag) {
      j_printf("failed\n");
      new = NULL;
      goto spend;
    }
  }
  /* token -> original logical name -> logical HMM -> physical/pseudo phone */
  /* cross-word conversion and fallback to bi/mono-phone is also considered */
  {
    int i;
    char *hmmstr;
    HMM_Logical *lg;
    boolean ok_flag = TRUE;

    new = (HMM_Logical **)mymalloc(sizeof(HMM_Logical *) * phnum);

    /* original logical name, applied logical HMM name (defined by HMMList),
       and the actual physical/pseudo HMM name (defined in hmmdefs) */
    j_printf("\n  id     original   logical    physical/pseudo\n");
    j_printf(" -------------------------------------------------\n");


    if (hmminfo->is_triphone) {
      cycle_triphone(NULL);
      cycle_triphone(tokens[0]);
      for (i = 0; i < phnum; i++) {
	if (i < phnum - 1) {
	  hmmstr = cycle_triphone(tokens[i+1]);
	} else {
	  hmmstr = cycle_triphone_flush();
	}
	lg = htk_hmmdata_lookup_logical(hmminfo, hmmstr);
	if (lg == NULL) {
	  if (word_mode) {
	    if (i > 0 && word_end[i-1]) {
	      if (word_end[i]) {
		center_name(hmmstr, buf);
	      } else {
		rightcenter_name(hmmstr, buf);
	      }
	    } else if (word_end[i]) {
	      leftcenter_name(hmmstr, buf);
	    }
	    lg = htk_hmmdata_lookup_logical(hmminfo, buf);
	    if (lg == NULL) {
	      j_printf("InternalError: no defined/pseudo HMM for \"%s\"??\n", buf);
	      ok_flag = FALSE;
	      continue;
	    }
	    if (lg->is_pseudo) {
	      j_printf("  %2d: %11s -> (pseudo) -> {%s}\n", i+1, hmmstr, lg->body.pseudo->name);
	    } else {
	      j_printf("  %2d: %11s -> %8s -> [%s]\n", i+1, hmmstr, lg->name, lg->body.defined->name);
	    }
	  } else {
	    j_printf(" UNKNOWN %2d: (%s)\n", i+1, hmmstr);
	    ok_flag = FALSE;
	    continue;
	  }
	} else {
	  if (lg->is_pseudo) {
	    j_printf("  %2d: %11s -> (pseudo) -> {%s}\n", i+1, hmmstr, lg->body.pseudo->name);
	  } else {
	    j_printf("  %2d: %11s -> %8s -> [%s]\n", i+1, hmmstr, " ", lg->body.defined->name);
	  }
	}
	new[i] = lg;
      }
    } else {
      for (i = 0; i < phnum; i++) {
	lg = htk_hmmdata_lookup_logical(hmminfo, tokens[i]);
	if (lg == NULL) {
	  j_printf("%2d: unknown logical HMM \"%s\"\n", i+1, tokens[i]);
	  ok_flag = FALSE;
	  continue;
	}
	new[i] = lg;
      }
    }
    if (ok_flag) {
      j_printf("succeeded\n");
    } else {
      j_printf("failed\n");
      free(new);
      new = NULL;
      goto spend;
    }
      
  }

 spend:
  {
    int i;
    for(i=0;i<phnum;i++) {
      free(tokens[i]);
    }
    free(tokens);
    free(word_end);
  }

  *len_ret = phnum;

  return new;
}

/** 
 * <JA>
 * 標準入力から1行を音素列表記として読み込み，トライフォンへの変換チェックを
 * 行なう．
 * 
 * @param hmminfo [in] HMM定義構造体
 * </JA>
 * <EN>
 * Read in line from stdin as phoneme sequence and try convertion to
 * triphone for checking.
 * 
 * @param hmminfo [in] HMM definition structure
 * </EN>
 */
static boolean
test_expand_triphone(HTK_HMM_INFO *hmminfo)
{
  char *buf;
  int newline;
  HMM_Logical **phseq;
  int phlen;
  boolean flag = FALSE;

  buf = (char *)mymalloc(4096);
  for(;;) {
    /* read in phoneme sequence from stdin */
    j_printf(">>> input phone sequence (word delimiter is `|', blank to return)\n");
    if (fgets(buf, 4096, stdin) == NULL) {
      flag = TRUE;
      break;
    }
    newline = strlen(buf)-1;    /* chop newline */
    if (buf[newline] == '\n') buf[newline] = '\0';
    if (buf[0] == '\0') break;
    /* convert string to phseq and output */
    phseq = new_str2phseq(buf, hmminfo, &phlen);
    free(phseq);
  }
  free(buf);
  return flag;
}

/** 
 * <JA>
 * コマンドライン上でトライフォンのチェックを行なうモード ("-check triphone")．
 * 
 * @param hmminfo [in] HMM定義構造体
 * @param winfo [in] 単語辞書
 * </JA>
 * <EN>
 * Mode to do interactive triphone conversion check ("-check triphone").
 * 
 * @param hmminfo [in] HMM definition structure
 * @param winfo [in] word dictionary
 * </EN>
 */
void
hmm_check(HTK_HMM_INFO *hmminfo, WORD_INFO *winfo)
{
  boolean endflag;
  static char cmd[MAX_HMMNAME_LEN];
  int newline;
  
  j_printf("\n\n");
  j_printf("*************************************************\n");
  j_printf("********  TRIPHONE COHERENCE CHECK MODE  ********\n");
  j_printf("*************************************************\n");
  j_printf("\n");

  j_printf("hmmdefs=%s\n", hmmfilename);
  if (mapfilename != NULL) {
    j_printf("hmmlist=%s\n", mapfilename);
  }
  j_printf("dict=%s\n", dictfilename);
  j_printf("headsil = "); put_voca(winfo, winfo->head_silwid);
  j_printf("tailsil = "); put_voca(winfo, winfo->tail_silwid);

  make_base_phone(hmminfo, winfo);
  print_phone_info(hmminfo);

  for(endflag = FALSE; endflag == FALSE;) {
    j_printf("===== command (\"H\" for help) > ");
    if (fgets(cmd, MAX_HMMNAME_LEN, stdin) == NULL) break;
    newline = strlen(cmd)-1;    /* chop newline */
    if (cmd[newline] == '\n') cmd[newline] = '\0';
    if (cmd[0] == '\0') continue; /* if blank line, read next */

    switch(cmd[0]) {
    case 'a':			/* all */
      /* check if logical HMMs cover all possible variants */
      test_interword_triphone(hmminfo, winfo);
      break;
    case 'c':			/* conv */
      /* try to expand triphone for given phoneme sequence */
      endflag = test_expand_triphone(hmminfo);
      break;
    case 'i':			/* info */
      /* output data source */
      j_printf("hmmdefs=%s\n", hmmfilename);
      if (mapfilename != NULL) {
	j_printf("hmmlist=%s\n", mapfilename);
      }
      j_printf("dict=%s\n", dictfilename);
      j_printf("headsil = "); put_voca(winfo, winfo->head_silwid);
      j_printf("tailsil = "); put_voca(winfo, winfo->tail_silwid);
      print_phone_info(hmminfo);
      break;
    case 'p':			/* phonelist */
      /* output basephone */
      print_all_basephone_name(&(hmminfo->basephone));
      break;
    case 'd':			/* phonelist in detail */
      /* output basephone */
      print_all_basephone_detail(&(hmminfo->basephone));
      break;
    case 'q':			/* quit */
      /* quit this check mode */
      endflag = TRUE;
      break;
    default:
      j_printf("COMMANDS:\n");
      j_printf("info      --- output HMM information\n");
      j_printf("conv      --- try HMM conversion for given phone sequence\n");
      j_printf("phonelist --- print base phone list\n");
      j_printf("all       --- check if all possible IW-triphone is covered\n");
      j_printf("quit      --- quit\n");
      break;
    }
  }
  j_printf("\n");
  j_printf("*************************************************\n");
  j_printf("*****  END OF TRIPHONE COHERENCE CHECK MODE  ****\n");
  j_printf("*************************************************\n");
  j_printf("\n");
}
