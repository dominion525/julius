/**
 * @file   para.c
 * @author Akinobu Lee
 * @date   Fri Oct 27 14:55:00 2006
 * 
 * <JA>
 * @brief  特徴量抽出条件の扱い
 *
 * 音響分析の設定パラメータを保持する Value 構造体を扱う．
 * </JA>
 * 
 * <EN>
 * @brief  Acoustic analysis condition parameter handling
 * </EN>
 *
 * Value structure holds acoustic analysis configuration parameters.
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

#include <sent/mfcc.h>
#include <sent/speech.h>

/** 
 * Reset configuration parameters for MFCC computation.
 * 
 * @param para [out] feature extraction parameters
 * 
 */
void
undef_para(Value *para)
{
  para->smp_period = -1;
  para->smp_freq   = -1;
  para->framesize  = -1;
  para->frameshift = -1;
  para->preEmph    = -1;
  para->mfcc_dim   = -1;
  para->lifter     = -1;
  para->fbank_num  = -1;
  para->delWin     = -1;
  para->accWin     = -1;
  para->silFloor   = -1;
  para->escale     = -1;
  para->enormal    = -1;
  para->hipass     = -2;	/* undef */
  para->lopass     = -2;	/* undef */
  para->cmn        = -1;
  para->raw_e      = -1;
  para->c0         = -1;
  para->ss_alpha   = -1;
  para->ss_floor   = -1;
  para->zmeanframe = -1;
  para->delta      = -1;
  para->acc        = -1;
  para->energy     = -1;
  para->absesup    = -1;
  para->baselen    = -1;
  para->vecbuflen  = -1;
  para->veclen     = -1;

  para->loaded     = 0;
}

/** 
 * Set Julius default parameters for MFCC computation.
 * 
 * @param para [out] feature extraction parameters
 * 
 */
void
make_default_para(Value *para)
{
  para->smp_period = 625;	/* 16kHz = 625 100ns unit */
  para->smp_freq   = 16000;	/* 16kHz = 625 100ns unit */
  para->framesize  = DEF_FRAMESIZE;
  para->frameshift = DEF_FRAMESHIFT;
  para->preEmph    = DEF_PREENPH;
  para->fbank_num  = DEF_FBANK;
  para->lifter     = DEF_CEPLIF;
  para->delWin     = DEF_DELWIN;
  para->accWin     = DEF_ACCWIN;
  para->raw_e      = FALSE;
  para->enormal    = FALSE;
  para->escale     = DEF_ESCALE;
  para->silFloor   = DEF_SILFLOOR;
  para->hipass     = -1;	/* disabled */
  para->lopass     = -1;	/* disabled */
  para->ss_alpha    = DEF_SSALPHA;
  para->ss_floor    = DEF_SSFLOOR;
  para->zmeanframe = FALSE;
}

/** 
 * Set HTK default configuration parameters for MFCC computation.
 * This will be refered when parameters are given as HTK Config file.
 * 
 * @param para [out] feature extraction parameters
 * 
 */
void
make_default_para_htk(Value *para)
{
  para->framesize  = 256000.0;	/* dummy! */
  para->preEmph    = 0.97;
  para->fbank_num  = 20;
  para->lifter     = 22;
  para->delWin     = 2;
  para->accWin     = 2;
  para->raw_e      = TRUE;
  para->enormal    = TRUE;
  para->escale     = 0.1;
  para->silFloor   = 50.0;
  para->hipass     = -1;	/* disabled */
  para->lopass     = -1;	/* disabled */
  para->zmeanframe = FALSE;
}

/** 
 * Merge two configuration parameters for MFCC computation.
 * 
 * @param dst [out] feature extraction parameters to set to
 * @param src [out] feature extraction parameters to set from
 * 
 */
void
apply_para(Value *dst, Value *src)
{
  if (dst->smp_period == -1) dst->smp_period = src->smp_period;
  if (dst->smp_freq   == -1) dst->smp_freq = src->smp_freq; 
  if (dst->framesize  == -1) dst->framesize = src->framesize; 
  if (dst->frameshift == -1) dst->frameshift = src->frameshift; 
  if (dst->preEmph    == -1) dst->preEmph = src->preEmph; 
  if (dst->mfcc_dim   == -1) dst->mfcc_dim = src->mfcc_dim; 
  if (dst->lifter     == -1) dst->lifter = src->lifter; 
  if (dst->fbank_num  == -1) dst->fbank_num = src->fbank_num; 
  if (dst->delWin     == -1) dst->delWin = src->delWin; 
  if (dst->accWin     == -1) dst->accWin = src->accWin; 
  if (dst->silFloor   == -1) dst->silFloor = src->silFloor; 
  if (dst->escale     == -1) dst->escale = src->escale; 
  if (dst->enormal    == -1) dst->enormal = src->enormal; 
  if (dst->hipass     == -2) dst->hipass = src->hipass;
  if (dst->lopass     == -2) dst->lopass = src->lopass;
  if (dst->cmn        == -1) dst->cmn = src->cmn; 
  if (dst->raw_e      == -1) dst->raw_e = src->raw_e; 
  if (dst->c0         == -1) dst->c0 = src->c0; 
  if (dst->ss_alpha   == -1) dst->ss_alpha = src->ss_alpha; 
  if (dst->ss_floor   == -1) dst->ss_floor = src->ss_floor; 
  if (dst->zmeanframe == -1) dst->zmeanframe = src->zmeanframe; 
  if (dst->delta      == -1) dst->delta = src->delta; 
  if (dst->acc        == -1) dst->acc = src->acc; 
  if (dst->energy     == -1) dst->energy = src->energy; 
  if (dst->absesup    == -1) dst->absesup = src->absesup; 
  if (dst->baselen    == -1) dst->baselen = src->baselen; 
  if (dst->vecbuflen  == -1) dst->vecbuflen = src->vecbuflen; 
  if (dst->veclen     == -1) dst->veclen = src->veclen; 
}


#define ISTOKEN(A) (A == ' ' || A == '\t' || A == '\n') ///< Determine token characters

/** 
 * Read and parse an HTK Config file, and set the specified option values.
 * 
 * @param HTKconffile [in] HTK Config file path name
 * @param para [out] MFCC parameter to set
 *
 * @return TRUE on success, FALSE on failure.
 */
boolean
htk_config_file_parse(char *HTKconffile, Value *para)
{
  FILE *fp;
  char buf[512];
  char *p, *d, *a;
  float srate;
  boolean skipped;

  j_printerr("include HTK Config: %s\n", HTKconffile);
  
  /* convert the content into argument list c_argv[1..c_argc-1] */
  /* c_argv[0] will be the original conffile name */
  if ((fp = fopen(HTKconffile, "r")) == NULL) {
    j_printerr("Error: rdhtkconf: failed to open HTK Config file: %s\n", HTKconffile);
  }

  while (getl_fp(buf, 512, fp) != NULL) {
    p = buf;
    if (*p == 35) { /* skip comment line */
      continue;
    }

    /* parse the input line to get directive and argument */
    while (*p != '\0' && ISTOKEN(*p)) p++;
    if (*p == '\0') continue;
    d = p;
    while (*p != '\0' && (!ISTOKEN(*p)) && *p != '=') p++;
    if (*p == '\0') continue;
    *p = '\0'; p++;
    while (*p != '\0' && ((ISTOKEN(*p)) || *p == '=')) p++;
    if (*p == '\0') continue;
    a = p;
    while (*p != '\0' && (!ISTOKEN(*p))) p++;
    *p = '\0';

    /* process arguments */
    skipped = FALSE;
    if (strmatch(d, "SOURCERATE")) { /* -smpPeriod */
      srate = atof(a);
    } else if (strmatch(d, "TARGETRATE")) { /* -fshift */
      para->frameshift = atof(a);
    } else if (strmatch(d, "WINDOWSIZE")) { /* -fsize */
      para->framesize = atof(a);
    } else if (strmatch(d, "ZMEANSOURCE")) { /* -zmeansource */
      para->zmeanframe = (a[0] == 'T') ? TRUE : FALSE;
    } else if (strmatch(d, "PREEMCOEF")) { /* -preemph */
      para->preEmph = atof(a);
    } else if (strmatch(d, "USEHAMMING")) { /* (fixed to T) */
      if (a[0] != 'T') {
	j_error("\nError: in HTK Config \"%s\": USEHAMMING should be T\n", HTKconffile);
      }
    } else if (strmatch(d, "NUMCHANS")) { /* -fbank */
      para->fbank_num = atoi(a);
    } else if (strmatch(d, "CEPLIFTER")) { /* -ceplif */
      para->lifter = atoi(a);
    } else if (strmatch(d, "DELTAWINDOW")) { /* -delwin */
      para->delWin = atoi(a);
    } else if (strmatch(d, "ACCWINDOW")) { /* -accwin */
      para->accWin = atoi(a);
    } else if (strmatch(d, "LOFREQ")) { /* -lofreq */
      para->lopass = atof(a);
    } else if (strmatch(d, "HIFREQ")) { /* -hifreq */
      para->hipass = atof(a);
    } else if (strmatch(d, "RAWENERGY")) { /* -rawe */
      para->raw_e = (a[0] == 'T') ? TRUE : FALSE;
    } else if (strmatch(d, "ENORMALISE")) { /* -enormal */
      para->enormal = (a[0] == 'T') ? TRUE : FALSE;
    } else if (strmatch(d, "ESCALE")) { /* -escale */
      para->escale = atof(a);
    } else if (strmatch(d, "SILFLOOR")) { /* -silfloor */
      para->silFloor = atof(a);
    } else if (strmatch(d, "TARGETKIND")) {
      j_printerr("TARGETKIND specified but skipped (will be set from AM header)\n");
      skipped = TRUE;
    } else if (strmatch(d, "NUMCEPS")) {
      j_printerr("NUMCEPS specified but skipped (will be set from AM header)\n");
      skipped = TRUE;
    } else {
      skipped = TRUE;
    }
    if (!skipped) {
      j_printerr("%s=%s\n", d, a);
    }
  }

  para->smp_period = srate;
  para->smp_freq = period2freq(para->smp_period);
  para->frameshift /= srate;
  para->framesize /= srate;

  if (fclose(fp) == -1) {
    j_printerr("Error: rdhtkconf: jconf file cannot close\n");
  }

  para->loaded = 1;

  return TRUE;
}


/** 
 * Set acoustic analysis parameters from HTK HMM definition header information.
 * 
 * @param para [out] acoustic analysis parameters
 * @param param_type [in] parameter type specified at HMM header
 * @param vec_size [in] vector size type specified at HMM header
 */
void
calc_para_from_header(Value *para, short param_type, short vec_size)
{
  int dim;

  /* decode required parameter extraction types */
  para->delta = (param_type & F_DELTA) ? TRUE : FALSE;
  para->acc = (param_type & F_ACCL) ? TRUE : FALSE;
  para->energy = (param_type & F_ENERGY) ? TRUE : FALSE;
  para->c0 = (param_type & F_ZEROTH) ? TRUE : FALSE;
  para->absesup = (param_type & F_ENERGY_SUP) ? TRUE : FALSE;
  para->cmn = (param_type & F_CEPNORM) ? TRUE : FALSE;
  /* guess MFCC dimension from the vector size and parameter type in the
     acoustic HMM */
  dim = vec_size;
  if (para->absesup) dim++;
  dim /= 1 + (para->delta ? 1 : 0) + (para->acc ? 1 : 0);
  if (para->energy) dim--;
  if (para->c0) dim--;
  para->mfcc_dim = dim;
    
  /* determine base size */
  para->baselen = para->mfcc_dim + (para->c0 ? 1 : 0) + (para->energy ? 1 : 0);
  /* set required size of parameter vector for MFCC computation */
  para->vecbuflen = para->baselen * (1 + (para->delta ? 1 : 0) + (para->acc ? 1 : 0));
  /* set size of final parameter vector */
  para->veclen = para->vecbuflen - (para->absesup ? 1 : 0);
}


/** 
 * Output acoustic analysis configuration parameters to stdout.
 * 
 * @param para [in] configuration parameter
 * 
 */
void
put_para(Value *para)
{
  j_printf("Acoustic analysis condition:\n");
  j_printf("\t       parameter = MFCC");
  if (para->c0) j_printf("_0");
  if (para->energy) j_printf("_E");
  if (para->delta) j_printf("_D");
  if (para->acc) j_printf("_A");
  if (para->absesup) j_printf("_N");
  if (para->cmn) j_printf("_Z");
  j_printf(" (%d dimension from %d cepstrum)\n", para->veclen, para->mfcc_dim);
  j_printf("\tsample frequency = %5d Hz\n", para->smp_freq);
  j_printf("\t   sample period = %4d  (100ns unit)\n", para->smp_period);
  j_printf("\t     window size = %4d samples (%.1f ms)\n", para->framesize,
           (float)para->smp_period * (float)para->framesize / 10000.0);
  j_printf("\t     frame shift = %4d samples (%.1f ms)\n", para->frameshift,
           (float)para->smp_period * (float)para->frameshift / 10000.0);
  j_printf("\t    pre-emphasis = %.2f\n", para->preEmph);
  j_printf("\t    # filterbank = %d\n", para->fbank_num);
  j_printf("\t   cepst. lifter = %d\n", para->lifter);
  j_printf("\t      raw energy = %s\n", para->raw_e ? "True" : "False");
  if (para->enormal) {
    j_printf("\tenergy normalize = True (scale = %.1f, silence floor = %.1f dB)\n", para->escale, para->silFloor);
  } else {
    j_printf("\tenergy normalize = False\n");
  }
  if (para->delta) {
    j_printf("\t    delta window = %d frames (%.1f ms) around\n", para->delWin,  (float)para->delWin * (float)para->smp_period * (float)para->frameshift / 10000.0);
  }
  if (para->acc) {
    j_printf("\t      acc window = %d frames (%.1f ms) around\n", para->accWin, (float)para->accWin * (float)para->smp_period * (float)para->frameshift / 10000.0);
  }
  j_printf("\t        hi freq. = ");
  if (para->hipass < 0) j_printf("OFF\n"); 
  else j_printf("%5d Hz\n", para->hipass);
  j_printf("\t        lo freq. = ");
  if (para->lopass < 0) j_printf("OFF\n"); 
  else j_printf("%5d Hz\n", para->lopass);
  j_printf("\t zero mean frame = ");
  if (para->zmeanframe) j_printf("ON\n");
  else j_printf("OFF\n");
}
