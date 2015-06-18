/*
 * Copyright (c) 2003-2006 Kawahara Lab., Kyoto University 
 * Copyright (c) 2003-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

/* mkbinhmm --- read in ascii hmmdefs file and write in binary format */

/* $Id: mkbinhmm.c,v 1.5 2006/12/26 03:17:26 sumomo Exp $ */

#include <sent/stddefs.h>
#include <sent/htk_hmm.h>


HTK_HMM_INFO *hmminfo;
Value para, para_htk;


static void
usage(char *s)
{
  j_printerr("mkbinhmm: convert HMM definition file to binary format for Julius\n");
  j_printerr("usage: %s [-C HTKConfig] hmmdefs binhmm\n", s);
  fprintf(stderr, "\nLibrary configuration: ");
  confout_version(stderr);
  confout_am(stderr);
  fprintf(stderr, "\n");
  exit(1);
}


int
main(int argc, char *argv[])
{
  FILE *fp;
  char *infile;
  char *outfile;
  char *conffile;
  int i;

  infile = outfile = conffile = NULL;
  for(i=1;i<argc;i++) {
    if (strmatch(argv[i], "-C") || strmatch(argv[i], "-htkconf")) {
      if (++i >= argc) usage(argv[0]);
      conffile = argv[i];
    } else {
      if (infile == NULL) {
	infile = argv[i];
      } else if (outfile == NULL) {
	outfile = argv[i];
      } else {
	usage(argv[0]);
      }
    }
  }
  if (infile == NULL || outfile == NULL)  usage(argv[0]);


  hmminfo = hmminfo_new();

  j_printf("---- reading hmmdefs ----\n");
  j_printf("filename: %s\n", infile);

  /* read hmmdef file */
  undef_para(&para);
  init_hmminfo(hmminfo, infile, NULL, &para);

  if (conffile != NULL) {
    /* if input HMMDEFS already has embedded parameter
       they will be overridden by the parameters in the config file */
    j_printf("\n---- reading HTK Config ----\n");
    if (para.loaded == 1) {
      j_printf("Warning: input hmmdefs has acoustic analysis parameter information\n");
      j_printf("Warning: they are overridden by the HTK Config file...\n");
    }
    /* load HTK config file */
    undef_para(&para);
    if (htk_config_file_parse(conffile, &para) == FALSE) {
      j_error("Error: failed to read %s\n", conffile);
    }
    /* set some parameters from HTK HMM header information */
    j_printf("\nsetting TARGETKIND and NUMCEPS from HMM definition header...");
    calc_para_from_header(&para, hmminfo->opt.param_type, hmminfo->opt.vec_size);
    j_printf("done\n");
    /* fulfill unspecified values with HTK defaults */
    j_printf("fulfill unspecified values with HTK defaults...");
    undef_para(&para_htk);
    make_default_para_htk(&para_htk);
    apply_para(&para, &para_htk);
    j_printf("done\n");
  }

  j_printf("\n------------------------------------------------------------\n");
  print_hmmdef_info(hmminfo);
  j_printf("\n");

  if (para.loaded == 1) {
    put_para(&para);
  }
  j_printf("------------------------------------------------------------\n");

  j_printf("---- writing ----\n");
  j_printf("filename: %s\n", outfile);
  
  if ((fp = fopen_writefile(outfile)) == NULL) {
    j_error("failed to open %s for writing\n", outfile);
  }
  if (write_binhmm(fp, hmminfo, (para.loaded == 1) ? &para : NULL) == FALSE) {
    j_error("failed to write to %s\n", outfile);
  }
  if (fclose_writefile(fp) != 0) {
    j_error("failed to close %s\n", outfile);
  }

  j_printf("\n");
  if (para.loaded == 1) {
    j_printf("binary HMM written to \"%s\", with acoustic parameters embedded for Julius.\n", outfile);
  } else {
    j_printf("binary HMM written to \"%s\"\n", outfile);
  }

  return 0;
}
