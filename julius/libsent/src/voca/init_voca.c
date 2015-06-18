/**
 * @file   init_voca.c
 * @author Akinobu LEE
 * @date   Fri Feb 18 19:41:12 2005
 * 
 * <JA>
 * @brief  単語辞書ファイルをメモリに読み込む
 * </JA>
 * 
 * <EN>
 * @brief  Load a word dictionary into memory
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
#include <sent/htk_hmm.h>
#include <sent/vocabulary.h>

/** 
 * Load and initialize a word dictionary.
 * 
 * @param winfo [out] pointer to a word dictionary data to store the read data
 * @param filename [in] file name of the word dictionary to read
 * @param hmminfo [in] %HMM definition data, needed for triphone conversion.
 * @param not_conv_tri [in] TRUE if not converting monophone to triphone.
 * @param force_dict [in] TRUE if want to ignore the error words in the dictionary
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
init_voca(WORD_INFO *winfo, char *filename, HTK_HMM_INFO *hmminfo, boolean not_conv_tri, boolean force_dict)
{
  FILE *fd;

  j_printerr("Reading in dictionary...\n");
  if ((fd = fopen_readfile(filename)) == NULL) {
    j_printerr("failed to open %s\n",filename);
    return(FALSE);
  }
  if (!voca_load_htkdict(fd, winfo, hmminfo, not_conv_tri)) {
    if (force_dict) {
      j_printerr("errors are ignored\n");
    } else {
      j_printerr("error in reading %s: %d words failed out of %d words\n",filename, winfo->errnum, winfo->num);
      fclose_readfile(fd);
      return(FALSE);
    }
  }
  if (fclose_readfile(fd) == -1) {
    j_printerr("close error\n");
    return(FALSE);
  }

  j_printerr("%d words...done\n", winfo->num);
  return(TRUE);
}
