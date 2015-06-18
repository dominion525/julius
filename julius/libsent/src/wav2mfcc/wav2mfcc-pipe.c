/**
 * @file   wav2mfcc-pipe.c
 * @author Akinobu LEE
 * @date   Thu Feb 17 18:12:30 2005
 * 
 * <JA>
 * @brief  音声波形から MFCC 特徴量へ変換する (フレーム単位)
 *
 * ここでは wav2mfcc.c の関数をフレーム同期に処理するために変換した
 * 関数が納められています．認識処理を音声入力と平行して行う場合，こちらの
 * 関数が用いられます．
 * </JA>
 * 
 * <EN>
 * @brief  Convert speech inputs into MFCC parameter vectors (per input frame)
 *
 * There are functions are derived from wav2mfcc.c, to compute
 * MFCC vectors in per-frame basis.  When performing on-line recognition,
 * these functions will be used instead of ones in wav2mfcc.c
 * </EN>
 * 
 * $Revision: 1.9 $
 * 
 */
/*
 * Copyright (c) 1991-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

/* wav2mfcc-pipe.c --- split Wav2MFCC to perform per-frame-basis,
   and also realtime CMN for 1st-pass pipe-lining */

/************************************************************************/
/*    wav2mfcc.c   Convert Speech file to MFCC_E_D_(Z) file             */
/*----------------------------------------------------------------------*/
/*    Author    : Yuichiro Nakano                                       */
/*                                                                      */
/*    Copyright(C) Yuichiro Nakano 1996-1998                            */
/*----------------------------------------------------------------------*/
/************************************************************************/


#include <sent/stddefs.h>
#include <sent/mfcc.h>

/** 
 * initialize and setup buffers for a MFCC computataion.
 * 
 * @param para [in] configuration parameters
 * @param bf [out] pointer to the entry point of workspace for FFT
 * @param ssbuf [in] noise spectrum, or NULL if not using spectral subtraction
 * @param ssbuflen [in] length of above, ignoredwhen @a ssbuf is NULL
 */
void
WMP_init(Value para, float **bf, float *ssbuf, int ssbuflen)
{
  int bflen;

  /* initialize module */
  WMP_calc_init(para, bf, &bflen);

  if (ssbuf != NULL) {
    /* check ssbuf length */
    if (ssbuflen != bflen) {
      j_error("Error: Wav2MFCC: noise spectrum length not match\n");
    }
  }

}

/***********************************************************************/
/** 
 * Allocate a new delta cycle buffer.
 * 
 * @param veclen [in] length of a vector
 * @param windowlen [in] window width for computing delta
 * 
 * @return pointer to newly allocated delta cycle buffer structure.
 */
DeltaBuf *
WMP_deltabuf_new(int veclen, int windowlen)
{
  int i;
  DeltaBuf *db;

  db = (DeltaBuf *)mymalloc(sizeof(DeltaBuf));
  db->veclen = veclen;
  db->win = windowlen;
  db->len = windowlen * 2 + 1;
  db->mfcc = (float **)mymalloc(sizeof(float *) * db->len);
  db->is_on = (boolean *) mymalloc(sizeof(boolean) * db->len);
  for (i=0;i<db->len;i++) {
    db->mfcc[i] = (float *)mymalloc(sizeof(float) * veclen * 2);
  }
  db->B = 0;
  for(i = 1; i <= windowlen; i++) db->B += i * i;
  db->B *= 2;

  return (db);
}

/** 
 * Destroy the delta cycle buffer.
 * 
 * @param db [i/o] delta cycle buffer
 */
void
WMP_deltabuf_free(DeltaBuf *db)
{
  int i;

  for (i=0;i<db->len;i++) {
    free(db->mfcc[i]);
  }
  free(db->is_on);
  free(db->mfcc);
  free(db);
}

/** 
 * Reset and clear the delta cycle buffer.
 * 
 * @param db [i/o] delta cycle buffer
 */
void
WMP_deltabuf_prepare(DeltaBuf *db)
{
  int i;
  db->store = 0;
  for (i=0;i<db->len;i++) {
    db->is_on[i] = FALSE;
  }
}

/** 
 * Calculate delta coefficients of the specified point in the cycle buffer.
 *
 * @param db [i/o] delta cycle buffer
 * @param cur [in] target point to calculate the delta coefficients
 */
static void
WMP_deltabuf_calc(DeltaBuf *db, int cur)
{
  int n, theta, p;
  float A1, A2, sum;
  int last_valid_left, last_valid_right;
  
  for (n = 0; n < db->veclen; n++) {
    sum = 0.0;
    last_valid_left = last_valid_right = cur;
    for (theta = 1; theta <= db->win; theta++) {
      p = cur - theta;
      if (p < 0) p += db->len;
      if (db->is_on[p]) {
	A1 = db->mfcc[p][n];
	last_valid_left = p;
      } else {
	A1 = db->mfcc[last_valid_left][n];
      }
      p = cur + theta;
      if (p >= db->len) p -= db->len;
      if (db->is_on[p]) {
	A2 = db->mfcc[p][n];
	last_valid_right = p;
      } else {
	A2 = db->mfcc[last_valid_right][n];
      }
      sum += theta * (A2 - A1);
    }
    db->mfcc[cur][db->veclen + n] = sum / db->B;
  }
}

/** 
 * Store the given MFCC vector into the delta cycle buffer, and compute the
 * latest delta coefficients.
 * 
 * @param db [i/o] delta cycle buffer
 * @param new_mfcc [in] MFCC vector
 * 
 * @return TRUE if next delta coeff. computed, in that case it is saved
 * in db->delta[], or FALSE if delta is not yet computed by short of data.
 */
boolean
WMP_deltabuf_proceed(DeltaBuf *db, float *new_mfcc) 
{
  int cur;
  boolean ret;

  /* copy data to store point */
  memcpy(db->mfcc[db->store], new_mfcc, sizeof(float) * db->veclen);
  db->is_on[db->store] = TRUE;

  /* get current calculation point */
  cur = db->store - db->win;
  if (cur < 0) cur += db->len;

  /* if the current point is fulfilled, compute delta  */
  if (db->is_on[cur]) {
    WMP_deltabuf_calc(db, cur);
    db->vec = db->mfcc[cur];
    ret = TRUE;
  } else {
    ret = FALSE;
  }

  /* move store pointer to next */
  db->store++;
  if (db->store >= db->len) db->store -= db->len;

  /* return TRUE if delta computed for current, or -1 if not calculated yet */
  return (ret);
}

/** 
 * Flush the delta cycle buffer the delta coefficients
 * left in the cycle buffer.
 * 
 * @param db [i/o] delta cycle buffer
 * 
 * @return TRUE if next delta coeff. computed, in that case it is saved
 * in db->delta[], or FALSE if all delta computation has been flushed and
 * no data is available.
 *
 */
boolean
WMP_deltabuf_flush(DeltaBuf *db) 
{
  int cur;
  boolean ret;

  /* clear store point */
  db->is_on[db->store] = FALSE;

  /* get current calculation point */
  cur = db->store - db->win;
  if (cur < 0) cur += db->len;

  /* if the current point if fulfilled, compute delta  */
  if (db->is_on[cur]) {
    WMP_deltabuf_calc(db, cur);
    db->vec = db->mfcc[cur];
    ret = TRUE;
  } else {
    ret = FALSE;
  }

  /* move store pointer to next */
  db->store++;
  if (db->store >= db->len) db->store -= db->len;

  /* return TRUE if delta computed for current, or -1 if not calculated yet */
  return (ret);
}

/***********************************************************************/
/* MAP-CMN */
/***********************************************************************/

#define CPMAX 500		///< Maximum number of frames to store ceptral mean for CMN update

/**
 * Hold sentence sum of MFCC
 * 
 */
typedef struct {
  float *mfcc_sum;		///< values of sum of MFCC parameters
  int framenum;			///< summed number of frames
} CMEAN;
#define CPSTEP 5		///< clist allocate step
static CMEAN *clist;		///< List of MFCC sum for previous inputs
static int clist_max;		///< Allocated number of CMEAN in clist
static int clist_num;		///< Currentlly filled CMEAN in clist
static int dim;			///< Local workarea to store the number of MFCC dimension.
static float cweight;		///< Weight of initial cepstral mean
static float *cmean_init;	///< Initial cepstral mean for each input
static boolean cmean_init_set;	///< TRUE if cmean_init was set
static CMEAN now;		///< Work area to hold current cepstral mean

/**
 * Initialize MAP-CMN at startup.
 * 
 * @param dimension [in] vector dimension
 * @param weight [in] initial cepstral mean weight
 * 
 */
void
CMN_realtime_init(int dimension, float weight)
{
  int i;

  dim = dimension;
  cweight = weight;

  clist_max = CPSTEP;
  clist_num = 0;
  clist = (CMEAN *)mymalloc(sizeof(CMEAN) * clist_max);
  for(i=0;i<clist_max;i++) {
    clist[i].mfcc_sum = (float *)mymalloc(sizeof(float)*dim);
    clist[i].framenum = 0;
  }

  now.mfcc_sum = (float *)mymalloc(sizeof(float) * dim);

  cmean_init = (float *)mymalloc(sizeof(float) * dim);
  cmean_init_set = FALSE;

}

/**
 * Prepare for MAP-CMN at start of each input
 * 
 */
void
CMN_realtime_prepare()
{
  int d;
  for(d=0;d<dim;d++) now.mfcc_sum[d] = 0.0;
  now.framenum = 0;
}

/**
 * Perform MAP-CMN for incoming MFCC vectors
 * 
 * @param mfcc [in] MFCC vector
 * @param dim [in] dimension
 * 
 */
void
CMN_realtime(float *mfcc, int dim)
{
  int d;
  double x, y;

  now.framenum++;
  if (cmean_init_set) {
    for(d=0;d<dim;d++) {
      /* accumulate value of given MFCC to sum */
      now.mfcc_sum[d] += mfcc[d];
      /* calculate map-cmn and perform subtraction to the given vector */
      x = now.mfcc_sum[d] + cweight * cmean_init[d];
      y = (double)now.framenum + cweight;
      mfcc[d] -= x / y;
    }
  } else {
    for(d=0;d<dim;d++) {
      now.mfcc_sum[d] += mfcc[d];
      mfcc[d] -= now.mfcc_sum[d] / now.framenum;
    }
  }
}

/**
 * Update initial cepstral mean from previous utterances for next input.
 * 
 */
void
CMN_realtime_update()
{
  float *tmp;
  int i, d;
  int frames;

  /* if CMN_realtime was never called before this, return immediately */
  /* this may occur by pausing just after startup */
  if (now.framenum == 0) return;

  /* compute cepstral mean from now and previous sums up to CPMAX frames */
  for(d=0;d<dim;d++) cmean_init[d] = now.mfcc_sum[d];
  frames = now.framenum;
  for(i=0;i<clist_num;i++) {
    for(d=0;d<dim;d++) cmean_init[d] += clist[i].mfcc_sum[d];
    frames += clist[i].framenum;
    if (frames >= CPMAX) break;
  }
  for(d=0;d<dim;d++) cmean_init[d] /= (float) frames;
  cmean_init_set = TRUE;

  /* expand clist if neccessary */
  if (clist_num == clist_max && frames < CPMAX) {
    clist_max += CPSTEP;
    clist = (CMEAN *)myrealloc(clist, sizeof(CMEAN) * clist_max);
    for(i=clist_num;i<clist_max;i++) {
      clist[i].mfcc_sum = (float *)mymalloc(sizeof(float)*dim);
      clist[i].framenum = 0;
    }
  }
  
  /* shift clist */
  tmp = clist[clist_max-1].mfcc_sum;
  memcpy(&(clist[1]), &(clist[0]), sizeof(CMEAN) * (clist_max - 1));
  clist[0].mfcc_sum = tmp;
  /* copy now to clist[0] */
  memcpy(clist[0].mfcc_sum, now.mfcc_sum, sizeof(float) * dim);
  clist[0].framenum = now.framenum;

  if (clist_num < clist_max) clist_num++;

}

/** 
 * Read binary with byte swap (assume file is Big Endian)
 * 
 * @param buf [out] data buffer
 * @param unitbyte [in] size of unit in bytes
 * @param unitnum [in] number of units to be read
 * @param fp [in] file pointer
 * 
 * @return TRUE if required number of units are fully read, FALSE if failed.
 */
static boolean
myread(void *buf, size_t unitbyte, int unitnum, FILE *fp)
{
  if (myfread(buf, unitbyte, unitnum, fp) < (size_t)unitnum) {
    return(FALSE);
  }
#ifndef WORDS_BIGENDIAN
  swap_bytes(buf, unitbyte, unitnum);
#endif
  return(TRUE);
}

/** 
 * Write binary with byte swap (assume data is Big Endian)
 * 
 * @param buf [in] data buffer
 * @param unitbyte [in] size of unit in bytes
 * @param unitnum [in] number of units to write
 * @param fd [in] file descriptor
 * 
 * @return TRUE if required number of units are fully written, FALSE if failed.
 */
static boolean
mywrite(void *buf, size_t unitbyte, int unitnum, int fd)
{
#ifndef WORDS_BIGENDIAN
  swap_bytes(buf, unitbyte, unitnum);
#endif
  if (write(fd, buf, unitbyte * unitnum) < unitbyte * unitnum) {
    return(FALSE);
  }
#ifndef WORDS_BIGENDIAN
  swap_bytes(buf, unitbyte, unitnum);
#endif
  return(TRUE);
}

/** 
 * Load CMN parameter from file.  If the number of MFCC dimension in the
 * file does not match the specified one, an error will occur.
 * 
 * @param filename [in] file name
 * @param dim [in] required number of MFCC dimensions
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
CMN_load_from_file(char *filename, int dim)
{
  FILE *fp;
  int veclen;
  if ((fp = fopen_readfile(filename)) == NULL) {
    j_printerr("Error: CMN_load_from_file: failed to open\n");
    return(FALSE);
  }
  /* read header */
  if (myread(&veclen, sizeof(int), 1, fp) == FALSE) {
    j_printerr("Error: CMN_load_from_file: failed to read header\n");
    fclose_readfile(fp);
    return(FALSE);
  }
  /* check length */
  if (veclen != dim) {
    j_printerr("Error: CMN_load_from_file: vector dimension mismatch\n");
    fclose_readfile(fp);
    return(FALSE);
  }
  /* read body */
  if (myread(cmean_init, sizeof(float), dim, fp) == FALSE) {
    j_printerr("Error: CMN_load_from_file: failed to read\n");
    fclose_readfile(fp);
    return(FALSE);
  }
  if (fclose_readfile(fp) == -1) {
    j_printerr("Error: CMN_load_from_file: failed to close\n");
    return(FALSE);
  }

  cmean_init_set = TRUE;

  return(TRUE);
}

/** 
 * Save the current CMN vector to a file.
 * 
 * @param filename [in] filename to save the data.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
CMN_save_to_file(char *filename)
{
  int fd;

  if ((fd = creat(filename, 0644)) == -1) {
    j_printerr("Error: CMN_save_to_file: failed to open\n");
    return(FALSE);
  }
  /* write header */
  if (mywrite(&dim, sizeof(int), 1, fd) == FALSE) {
    j_printerr("Error: CMN_save_to_file: failed to write header\n");
    close(fd);
    return(FALSE);
  }
  /* write body */
  if (mywrite(cmean_init, sizeof(float), dim, fd) == FALSE) {
    j_printerr("Error: CMN_save_to_file: failed to write header\n");
    close(fd);
    return(FALSE);
  }
  close(fd);
  
  return(TRUE);
}


/***********************************************************************/
/* energy normalization and scaling on live input */
/***********************************************************************/
static LOGPROB energy_max_last;	///< Maximum energy value of last input
static LOGPROB energy_min_last;	///< Minimum floored energy value of last input
static LOGPROB energy_max;	///< Maximum energy value of current input

/** 
 * Initialize work area for energy normalization on live input.
 * This should be called once on startup.
 * 
 */
void
energy_max_init()
{
  energy_max = 5.0;
}

/**
 * Prepare values for energy normalization on live input.
 * This should be called before each input segment.
 * 
 * @param para [in] MFCC computation configuration parameter
 */
void
energy_max_prepare(Value *para)
{
  energy_max_last = energy_max;
  energy_min_last = energy_max - (para->silFloor * LOG_TEN) / 10.0;
  energy_max = 0.0;
}

/** 
 * Peform energy normalization using maximum of last input.
 * 
 * @param f [in] raw energy
 * @param para [in] MFCC computation configuration parameter
 * 
 * @return value of the normalized log energy.
 */
LOGPROB
energy_max_normalize(LOGPROB f, Value *para)
{
  if (energy_max < f) energy_max = f;
  if (f < energy_min_last) f = energy_min_last;
  return(1.0 - (energy_max_last - f) * para->escale);
}

  
