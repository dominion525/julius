/**
 * @file   ss.c
 * @author Akinobu LEE
 * @date   Thu Feb 17 17:19:54 2005
 * 
 * <JA>
 * @brief  スペクトル減算
 *
 * 実際のスペクトル減算は wav2mfcc-buffer.c および wav2mfcc-pipe.c で
 * 行われます．ここでは平均スペクトルの推定とファイルI/Oのみ定義されています．
 * </JA>
 * 
 * <EN>
 * @brief  Spectral subtraction
 *
 * The actual subtraction will be performed in wav2mfcc-buffer.c and
 * wav2mfcc-pipe.c.  These functions are for estimating average spectrum
 * of audio input, and file I/O for that.
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
#include <sent/mfcc.h>


/** 
 * Binary read function with byte swaping (assume file is BIG ENDIAN)
 * 
 * @param buf [out] read data
 * @param unitbyte [in] size of a unit in bytes
 * @param unitnum [in] number of unit to be read
 * @param fp [in] file pointer
 */
static boolean
myread(void *buf, size_t unitbyte, int unitnum, FILE *fp)
{
  size_t tmp;
  if ((tmp = myfread(buf, unitbyte, unitnum, fp)) < (size_t)unitnum) {
    return(FALSE);
  }
#ifndef WORDS_BIGENDIAN
  swap_bytes(buf, unitbyte, unitnum);
#endif
  return(TRUE);
}

/** 
 * Load a noise spectrum from file.
 * 
 * @param filename [in] path name of noise spectrum file
 * @param slen [out] length of the returned buffer
 * 
 * @return a newly allocated buffer that holds the loaded noise spectrum.
 */
float *
new_SS_load_from_file(char *filename, int *slen)
{
  FILE *fp;
  int num;
  float *sbuf;

  /* open file */
  j_printerr("Reading Noise Spectrum for SS...");
  if ((fp = fopen_readfile(filename)) == NULL) {
    j_printerr("Error: SS_load_from_file: cannot open \"%s\"\n", filename);
    return(NULL);
  }
  /* read length */
  if (myread(&num, sizeof(int), 1, fp) == FALSE) {
    j_printerr("Error: SS_load_from_file: cannot read \"%s\"\n", filename);
    return(NULL);
  }
  /* allocate */
  sbuf = (float *)mymalloc(sizeof(float) * num);
  /* read data */
  if (myread(sbuf, sizeof(float), num, fp) == FALSE) {
    j_printerr("Error: SS_load_from_file: cannot read \"%s\"\n", filename);
    return(NULL);
  }
  /* close file */
  fclose_readfile(fp);

  *slen = num;
  j_printerr("done\n");
  return(sbuf);
}

/** 
 * Compute average spectrum of audio input.
 * This is used to estimate a noise spectrum from input samples.
 * 
 * @param wave [in] input audio data sequence
 * @param wavelen [in] length of above
 * @param para [in] parameter
 * @param slen [out] length of returned buffer
 * 
 * @return a newly allocated buffer that contains the calculated spectrum.
 */
float *
new_SS_calculate(SP16 *wave, int wavelen, Value para, int *slen)
{
  int fftN, n;
  float *bf, *Re, *Im, *spec;
  int t, framenum, start, end, k, i;
  double x, y;
  
  /* Calculate FFT size */
  fftN = 2;  n = 1;
  while(para.framesize > fftN){
    fftN *= 2; n++;
  }

#ifdef MFCC_SINCOS_TABLE
  /* generate tables */
  make_costbl_hamming(para.framesize);
  make_fft_table(n);
#endif
  
  /* allocate work area */
  spec = (float *)mymalloc((fftN + 1) * sizeof(float));
  bf = (float *)mymalloc((fftN + 1) * sizeof(float));
  Re = (float *)mymalloc((fftN + 1) * sizeof(float));
  Im = (float *)mymalloc((fftN + 1) * sizeof(float));
  for(i=0;i<fftN;i++) spec[i] = 0.0;
  
  /* Caluculate Sum of Noise Power Spectral */
  framenum = (int)((wavelen - para.framesize) / para.frameshift) + 1;
  start = 1;
  end = 0;
  for (t = 0; t < framenum; t++) {
    if (end != 0) start = end - (para.framesize - para.frameshift) - 1;
    k = 1;
    for (i = start; i <= start + para.framesize; i++) {
      bf[k] = (float)wave[i-1];
      k++;
    }
    end = i;

    if (para.zmeanframe) {
      ZMeanFrame(bf, para.framesize);
    }

    /* Pre-emphasis */
    PreEmphasise(bf, para);
    /* Hamming Window */
    Hamming(bf, para.framesize);
    /* FFT Spectrum */
    for (i = 1; i <= para.framesize; i++) {
      Re[i-1] = bf[i]; Im[i-1] = 0.0;
    }
    for (i = para.framesize + 1; i <= fftN; i++) {
      Re[i-1] = 0.0;   Im[i-1] = 0.0;
    }
    FFT(Re, Im, n);
    /* Sum noise spectrum */
    for(i = 1; i <= fftN; i++){
      x = Re[i - 1];  y = Im[i - 1];
      spec[i - 1] += sqrt(x * x + y * y);
    }
  }

  /* Calculate average noise spectrum */
  for(t=0;t<fftN;t++) {
    spec[t] /= (float)framenum;
  }

  /* return the new spec[] */
  free(Im);
  free(Re);
  free(bf);
  *slen = fftN;
  return(spec);
}
