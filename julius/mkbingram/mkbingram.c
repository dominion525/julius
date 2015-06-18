/**
 * @file   mkbingram.c
 * @author Akinobu LEE
 * @date   Thu Mar 24 12:22:27 2005
 * 
 * <JA>
 * @brief  ARPA標準形式のN-gramから Julius 用のバイナリN-gramに変換する．
 *
 * Julius で使える ARPA 標準形式の (前向き)2-gram と 後ろ向き
 * 3-gram を，単一のバイナリN-gramに変換する．
 *
 * バイナリN-gramの形式はまた古い形式(3.4.2以前)
 * のバイナリN-gramを 3.5 以降の新しい形式に変換することもできる．
 * </JA>
 * 
 * <EN>
 * @brief  
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

/* mkbingram --- make binary n-gram for JULIUS from ARPA standard format */

/* $Id: mkbingram.c,v 1.5 2006/12/26 03:17:26 sumomo Exp $ */

#include <sent/stddefs.h>
#include <sent/ngram2.h>
#include <sys/stat.h>
#include <time.h>

NGRAM_INFO *ngram;

void
usage(char *s)
{
  j_printerr("mkbingram: make/convert binary n-gram from ARPA format for Julius\n");
  j_printerr("usage 1: %s ARPA_2-gram ARPA_rev-3-gram outfile\n", s);
  j_printerr("usage 2: %s -d bingram outfile\n", s);
  fprintf(stderr, "\nLibrary configuration: ");
  confout_version(stderr);
  confout_lm(stderr);
  fprintf(stderr, "\n");
  exit(1);
}


int
main(int argc, char *argv[])
{
  FILE *fp;
  char header[512];
  time_t now;
  char *file1, *file2, *binfile, *outfile;
  int i;

  file1 = file2 = binfile = outfile = NULL;
  for(i=1;i<argc;i++) {
    if (argv[i][0] == '-') {
      if (argv[i][1] == 'd') {
	if (++i >= argc) usage(argv[0]);
	binfile = argv[i];
      } else {
	usage(argv[0]);
      }
    } else {
      if (binfile == NULL) {
	if (file1 == NULL) {
	  file1 = argv[i];
	} else if (file2 == NULL) {
	  file2 = argv[i];
	} else if (outfile == NULL) {
	  outfile = argv[i];
	} else {
	  usage(argv[0]);
	}
      } else {
	if (outfile == NULL) {
	  outfile = argv[i];
	} else {
	  usage(argv[0]);
	}
      }
    }
  }
  if (outfile == NULL) usage(argv[0]);
  if (binfile == NULL) {
    if (file1 == NULL || file2 == NULL) usage(argv[0]);
  }

  if (binfile == NULL) {
    printf("    2-gram: %s\n", file1);
    printf("rev-3-gram: %s\n", file2);
  } else {
    printf("bingram: %s\n", binfile);
  }

  /* make header string */
  now = time(NULL);
  if (binfile == NULL) {
    sprintf(header, "converted at %sfrom     2-gram = %s\n     rev-3-gram = %s\n", ctime(&now),  file1, file2);
  } else {
    sprintf(header, "converted at %sfrom     bingram = %s\n", ctime(&now), binfile);
  }

  ngram = ngram_info_new();
  if (binfile == NULL) {
    /* read in ARPA n-gram */
    init_ngram_arpa(ngram, file1, file2);
  } else {
    /* read in bingram */
    init_ngram_bin(ngram, binfile);
  }
  print_ngram_info(ngram);

  /* write in JULIUS binary format */
  if ((fp = fopen_writefile(outfile)) == NULL) {
    j_printerr("failed to open %s\n", outfile);
    return -1;
  }
  if (ngram->version < 4) {
    printf("***********************************************\n");
    printf("**** number of 3-gram exceeds 24bit limit!! \n");
    printf("**** will be saved in old v3 format (<= v3.4.2)\n");
    printf("***********************************************\n");
  }
  printf("Writing in v%d format to %s...\n", ngram->version, outfile);
  if (ngram_write_bin(fp, ngram, header) == FALSE){/* failed */
    j_printerr("failed to write %s\n",outfile);
    return -1;
  }
  fclose_writefile(fp);
  printf("completed\n");
  
  return 0;
}
