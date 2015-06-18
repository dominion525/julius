/**
 * @file   init_ngram.c
 * @author Akinobu LEE
 * @date   Wed Feb 16 07:40:53 2005
 * 
 * <JA>
 * @brief  N-gramファイルをメモリに読み込み単語辞書と対応を取る
 * </JA>
 * 
 * <EN>
 * @brief  Load N-gram file into memory and setup with word dictionary
 * </EN>
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

#include <sent/stddefs.h>
#include <sent/ngram2.h>
#include <sent/vocabulary.h>

/** 
 * Read and setup N-gram data from binary format file.
 * 
 * @param ndata [out] pointer to N-gram data structure to store the data
 * @param bin_ngram_file [in] file name of the binary N-gram
 */
void
init_ngram_bin(NGRAM_INFO *ndata, char *bin_ngram_file)
{
  FILE *fp;
  
  j_printerr("Reading in word n-gram...");
  if ((fp = fopen_readfile(bin_ngram_file)) == NULL) {
    j_error("open error for %s\n", bin_ngram_file);
  }
  if (ngram_read_bin(fp, ndata) == FALSE) {
    j_error("read error for %s\n", bin_ngram_file);
  }
  if (fclose_readfile(fp) == -1) {
    j_error("close error\n");
  }
  j_printerr("done\n");
}

/** 
 * Read and setup N-gram data from ARPA format files of 2-gram and 3-gram.
 * 
 * @param ndata [out] pointer to N-gram data structure to store the data
 * @param ngram_lr_file [in] file name of ARPA 2-gram file
 * @param ngram_rl_file [in] file name of ARPA reverse 3-gram file
 */
void
init_ngram_arpa(NGRAM_INFO *ndata, char *ngram_lr_file, char *ngram_rl_file)
{
  FILE *fp;

  ndata->root = NULL;
  j_printerr("Reading in LR 2-gram...\n");
  /* read LR 2-gram */
  if ((fp = fopen_readfile(ngram_lr_file)) == NULL) {
    j_error("open error for %s\n", ngram_lr_file);
  }
  if (ngram_read_arpa(fp, ndata, DIR_LR) == FALSE) {
    j_error("read error for %s\n", ngram_lr_file);
  }
  if (fclose_readfile(fp) == -1) {
    j_error("close error\n");
  }
  if (ngram_rl_file != NULL) {
    j_printerr("done\nReading in RL 3-gram...\n");
    /* read RL 3-gram */
    if ((fp = fopen_readfile(ngram_rl_file)) == NULL) {
      j_error("open error for %s\n", ngram_rl_file);
    }
    if (ngram_read_arpa(fp, ndata, DIR_RL) == FALSE) {
      j_error("read error for %s\n", ngram_rl_file);
    }
    if (fclose_readfile(fp) == -1) {
      j_error("close error\n");
    }
  }

  j_printerr("done\n");
}

/** 
 * Make correspondence between word dictionary and N-gram vocabulary.
 * 
 * @param ndata [i/o] word/class N-gram, the unknown word information will be set.
 * @param winfo [i/o] word dictionary, the word-to-ngram-entry mapping will be done here.
 */
void
make_voca_ref(NGRAM_INFO *ndata, WORD_INFO *winfo)
{
  int i;

  j_printerr("Mapping dictonary words to n-gram entries...");
  ndata->unk_num = 0;
  for (i = 0; i < winfo->num; i++) {
    winfo->wton[i] = make_ngram_ref(ndata, winfo->wname[i]);
    if (winfo->wton[i] == ndata->unk_id) {
      (ndata->unk_num)++;
    }
  }
  if (ndata->unk_num == 0) {
    ndata->unk_num_log = 0.0;	/* for safe */
  } else {
    ndata->unk_num_log = (float)log10(ndata->unk_num);
  }
  j_printerr("done\n");
}

