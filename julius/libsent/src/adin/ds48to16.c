/**
 * @file   adin_mic_linux_oss_48khz.c
 * @author Akinobu LEE
 * @date   Sun Feb 13 16:18:26 2005
 *
 * <JA>
 * @brief  48kHz -> 16kHz ダウンサンプリング
 *
 * </JA>
 * <EN>
 * @brief  Down sampling from 48kHz to 16kHz
 *
 * </EN>
 *
 * $Revision: 1.1 $
 * 
 */
/*
 * Copyright (c) 1991-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/adin.h>

/// TRUE if use embedded values on header
#define USE_HEADER_COEF

/// USB device sampling rate

#ifdef USE_HEADER_COEF
/// filter parameters in header
#include "lpfcoef_3to4.h"
#include "lpfcoef_2to1.h"
#endif

/* work area for down sampling */
#define	RBSIZE	512		///< Filter size
#define BUFSIZE 256		///< Work area buffer size for x[]
#define BUFSIZE_Y 512		///< Work area buffer size for y[]
#define	mod(x)	((x) & (RBSIZE -1)) ///< Buffer index cycler
typedef struct {
  int decrate;			///< Sample step rate from
  int intrate;			///< Sample step rate to
  double hdn[RBSIZE+1];	///< Filter coefficients
  int hdn_len;		///< Filter length
  int delay;		///< Filter start point delay length
  double x[BUFSIZE];	///< Work area for down sampling
  double y[BUFSIZE_Y];	///< Work area for down sampling
  double rb[RBSIZE];	///< Temporal buffer for firin() and firout()
  int indx;		///< Current index of rb[]
  int bp;		///< Pointer of current input samples
  int count;		///< Current output counter
} FILTER;
static FILTER fir34;			///< Up sampler from 3 to 4
static FILTER fir21_1;			///< First Down sampler from 2 to 1
static FILTER fir21_2;			///< Second Down sampler from 2 to 1
static int recbuf_rate;

#ifdef USE_HEADER_COEF
/** 
 * Set 1/2 filter coefficients from header values.
 * 
 * @param f [out] filter info
 */
static void
load_filter_from_header_2to1(FILTER *f)
{
  int i;
 
  /* read the filter coefficients from header file */
  for(i=0;i<RBSIZE + 1; i++) {
    if (i >= lpfcoef_2to1_num) break;
    f->hdn[i] = lpfcoef_2to1[i];
  }
  f->hdn_len = i - 1;
}

/** 
 * Set 4/2 filter coefficients from header values.
 * 
 * @param f [out] filter info
 */
static void
load_filter_from_header_3to4(FILTER *f)
{
  int i;
 
  /* read the filter coefficients from header file */
  for(i=0;i<RBSIZE + 1; i++) {
    if (i >= lpfcoef_3to4_num) break;
    f->hdn[i] = lpfcoef_3to4[i];
  }
  f->hdn_len = i - 1;
}

#else  /* ~USE_HEADER_COEF */

/** 
 * Read filter coefficients from file.
 * 
 * @param f [out] filter info
 * @param coeffile [in] filename
 */
static boolean
load_filter(FILTER *f, char *coeffile)
{
  FILE *fp;
  static char buf[512];
  int i;
 
  /* read the filter coefficients */
  if ((fp = fopen(coeffile, "r")) == NULL) {
    j_printerr("Error: cannot open filter coefficient file \"%s\"\n", coeffile);
    return FALSE;
  }
  for(i=0;i<RBSIZE + 1; i++) {
    if (fgets(buf, 512, fp) == NULL) break;
    f->hdn[i] = atof(buf);
  }
  fclose(fp);
  if (i <= 0) {
    j_printerr("Error: failed to read filter coefficient from \"%s\"\n", coeffile);
    return FALSE;
  }
  f->hdn_len = i - 1;

  return TRUE;
}

#endif

/** 
 * Initialize filter values
 * 
 * @param f [i/o] filter info
 * @param u [in] up sampling rate
 * @param d [in] down sampling rate
 */
static void
init_filter(FILTER *f, int d, int u)
{
 
  f->decrate = d;
  f->intrate = u;

  /* set filter starting point */
  f->delay = f->hdn_len / (2 * f->decrate);

  /* reset index */
  f->indx = 0;

  /* reset pointer */
  f->bp = 0;

  /* reset output counter */
  f->count = 1;
}

/** 
 * Store input for FIR filter
 * 
 * @param f [i/o] filter info
 * @param in [in] an input sample
 * </EN>
 */
static void
firin(FILTER *f, double in)
{
  f->indx = mod(f->indx - 1);
  f->rb[f->indx] = in;
}

/** 
 * Get filtered output from FIR filter
 * 
 * @param f [i/o] filter info
 * @param os [in] point
 * 
 * @return output value
 */
static double
firout(FILTER *f, int os)
{
  double out;
  int k, l;
  
  out = 0.0;
  for(k = os, l = f->indx ; k <= f->hdn_len; k += f->intrate, l = mod(l + 1)) {
    out += f->rb[l] * f->hdn[k];
  }
  return(out);
}

/** 
 * Perform down sampling of input samples.
 * 
 * @param f [i/o] filter info
 * @param dst [out] store the resulting samples
 * @param src [in] input samples
 * @param len [in] number of input samples
 * @param maxlen [in] maximum length of dst
 * 
 * @return the number of samples written to dst, or -1 on errror.
 * </EN>
 */
static int
do_filter(FILTER *f, double *dst, double *src, int len, int maxlen)
{
  int dstlen;
  int s;
  int d;
  int i, k;

  s = 0;
  dstlen = 0;

  while(1) {
    /* fulfill temporal buffer */
    /* at this point, x[0..bp-1] may contain the left samples of last call */
    while (f->bp < BUFSIZE) {
      if (s >= len) break;
      f->x[f->bp++] = src[s++];
    }
    if (f->bp < BUFSIZE) {	
      /* when reached last of sample, leave the rest in x[] and exit */
      break;
    }
    /* do conversion from x[0..bp-1] to y[] */
    d = 0;
    for(k=0;k<f->bp;k++) {
      firin(f, f->x[k]);
      for(i=0;i<f->intrate;i++) {
	f->count--;
	if(f->count == 0) {
	  f->y[d++] = firout(f, i);
	  f->count = f->decrate;
	}
      }
    }
    /* store the result to dst[] */
    if(f->delay) {
      if(d > f->delay) {
	/* input samples > delay, store the overed samples and enter no-delay state */
	d -= f->delay;
	for(i=0;i<d;i++) {
	  if (dstlen >= maxlen) break;
	  dst[dstlen++] = f->y[f->delay + i];
	}
	f->delay = 0;
	if (dstlen >= maxlen) {
	  j_printerr("Error: buffer overflow in down sampling, inputs may be lost!\n");
	  return -1;
	}
      } else {
	/* input samples < delay, decrease delay and wait */
	f->delay -= d;
      }
    } else {
      /* no-delay state: store immediately */
      for(i=0;i<d;i++) {
	if (dstlen >= maxlen) break;
	dst[dstlen++] = f->y[i];
      }
      if (dstlen >= maxlen) {
	j_printerr("Error: buffer overflow in down sampling, inputs may be lost!\n");
	return -1;
      }
    }

    /* reset pointer */
    f->bp -= BUFSIZE;
  }

  return dstlen;
}


/** 
 * Setup for down sampling
 * 
 * @param target_freq [in] required sampling rate in kHz
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
ds48to16_setup()
{
  /* define 3 filters: 
     48kHz --f1(3/4)-> 64kHz --f2(1/2)-> 32kHz --f3(1/2)-> 16kHz */
#ifdef USE_HEADER_COEF
  /* set from embedded header */
  j_printf("loading FIR filters for down sampling...");
  load_filter_from_header_3to4(&fir34);
  load_filter_from_header_2to1(&fir21_1);
  load_filter_from_header_2to1(&fir21_2);
#else
  /* read from file */
  j_printf("initializing FIR filters for down sampling...");
  if (load_filter(&fir34, "lpfcoef.3to4") == FALSE) return FALSE;
  if (load_filter(&fir21_1, "lpfcoef.2to1") == FALSE) return FALSE;
  if (load_filter(&fir21_2, "lpfcoef.2to1") == FALSE) return FALSE;
#endif
  init_filter(&fir34, 3, 4);
  init_filter(&fir21_1, 2, 1);
  init_filter(&fir21_2, 2, 1);
  j_printf("done\n");
  
  return TRUE;
}

/** 
 * Perform down sampling of input samples to 1/3.
 * 
 * @param dst [out] store the resulting samples
 * @param src [in] input samples
 * @param srclen [in] number of input samples
 * @param maxdstlen [in] maximum length of dst
 * 
 * @return the number of samples written to dst, or -1 on errror.
 * </EN>
 */
int
ds48to16(SP16 *dst, SP16 *src, int srclen, int maxdstlen)
{
  static double *buf = NULL;
  static double *buf1;
  static double *buf2;
  static double *buf3;
  static int buflen;
  int i, len1, len2, len3;

  if (buf == NULL) {
    buflen = srclen * 2;		/* longer buffer required for 3/4 upsamling */
    buf  = (double *)mymalloc(sizeof(double) * buflen);
    buf1 = (double *)mymalloc(sizeof(double) * buflen);
    buf2 = (double *)mymalloc(sizeof(double) * buflen);
    buf3 = (double *)mymalloc(sizeof(double) * buflen);
  } else if (buflen < srclen * 2) {
    buflen = srclen * 2;
    buf  = (double *)myrealloc(buf,  sizeof(double) * buflen);
    buf1 = (double *)myrealloc(buf1, sizeof(double) * buflen);
    buf2 = (double *)myrealloc(buf2, sizeof(double) * buflen);
    buf3 = (double *)myrealloc(buf3, sizeof(double) * buflen);
  }

  for(i=0;i<srclen;i++) buf[i] = src[i];

  len1 = do_filter(&fir34,   buf1, buf,  srclen,  buflen);
  len2 = do_filter(&fir21_1, buf2, buf1, len1, buflen);
  len3 = do_filter(&fir21_2, buf3, buf2, len2, buflen);
  
  //printf("%d -> %d -> %d -> %d\n", srclen, len1, len2, len3);
  //printf("%d -> %d\n", srclen, len3);

  if (maxdstlen < len3) {
    j_printerr("Error: down sampled num > required!\n");
    return -1;
  }
  for(i=0;i<len3;i++) dst[i] = buf3[i];

  return(len3);
}
