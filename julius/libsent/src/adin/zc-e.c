/**
 * @file   zc-e.c
 * @author Akinobu LEE
 * @date   Mon Feb 14 19:11:34 2005
 *
 * <JA>
 * @brief  音声区間検出のための零交差数カウント
 *
 * 与えられたバッファ長内の零交差数をカウントします．
 * 同時に, 呼ばれたバッファを順次バッファ長分だけ古いものに入れ替えます．
 * このため入力はバッファ長分だけ遅延することになります．
 * </JA>
 * <EN>
 * @brief  Count zero cross and level for speech detection
 *
 * Count zero cross number within the given length of cycle buffer.
 * The content of the cycle buffer will be swapped with the newest data,
 * So the input delays for the length of the cycle buffer.
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

/* Sat Feb 19 13:48:00 JST 1994 */
/*  Kawahara 1986 */
/*  Munetsugu 1991 */
/*  shinohara 1993 */
/*  mikik 1993 */
/*  ri 1997 for cycle buffer */

#include <sent/stddefs.h>

#define UNDEF 2			///< Undefined mark used for @a sign
#define POSITIVE 1		///< Positive mark used for @a sign
#define NEGATIVE -1		///< Negative mark used for @a sign

static int trigger;		///< Level threshold
static int length;		///< Cycle buffer size = number of samples to hold
static int offset;		///< Static data DC offset
static int *is_zc;		///< Zero-crossing point flag list
static SP16 *data;		///< Data cycle buffer
static int zero_cross;		///< Total zerocross num in @a data
static int sign;		///< Current sign of sample for zerocross counting
static int is_trig;		///< Triggering sign
static int top;			///< Current pointer of buffer
static int valid_len;		///< Valid samples in buffer (for short input)

/** 
 * Initialize all parameters and buffers for zero-cross counting.
 * 
 * @param c_trigger [in] Tgigger level threshold
 * @param c_length [in] Cycle buffer size = Number of samples to hold
 * @param c_offset [in] Static DC offset of input data
 */
void
init_count_zc_e(int c_trigger, int c_length, int c_offset)
{
  int i;

  trigger = c_trigger;
  length = c_length;
  offset = c_offset;

  zero_cross = 0;
  is_trig = FALSE;
  sign = POSITIVE;
  top = 0;
  valid_len = 0;

  /* data spool for header-margin */
  data = (SP16 *)mymalloc(length * sizeof(SP16));
  /* zero-cross location */
  is_zc = (int *)mymalloc(length * sizeof(int));
  for (i=0; i<length; i++){
    is_zc[i] = UNDEF;
  }
}

/** 
 * End procedure: free all buffers.
 * 
 */
void
end_count_zc_e()
{
  free(is_zc);
  free(data);
}

/** 
 * Adding buf[0..step-1] to the cycle buffer and update the count of
 * zero cross.   Also swap them with the oldest ones in the cycle buffer.
 * 
 * @param buf [I/O] new samples given, and swapped samples will be returned.
 * @param step [in] length of above.
 * 
 * @return zero-cross count of the samples in the cycle buffer.
 */
int
count_zc_e(SP16 *buf,int step)
{
  int i;
  SP16 tmp;

  for (i=0; i<step; i++) {
    if (is_zc[top]==TRUE) {
      --zero_cross;
    }
    is_zc[top] = FALSE;
    /* exchange old data and buf */
    tmp = buf[i] + offset;
    if (is_trig) {
      if (sign==POSITIVE && tmp<0) {
	++zero_cross;
	is_zc[top] = TRUE;
	is_trig = FALSE;
	sign = NEGATIVE;
      } else if (sign==NEGATIVE && tmp>0) {
	++zero_cross;
	is_zc[top] = TRUE;
	is_trig = FALSE;
	sign = POSITIVE;
      }
    }
    if (abs(tmp)>trigger) {
      is_trig = TRUE;
    }
    data[top] = buf[i];
    top++;
    if (valid_len < top) valid_len = top;
    if (top >= length) {
      top = 0;
    }
  }
  return (zero_cross);
}

/** 
 * Adding buf[0..step-1] to the cycle buffer and update the count of
 * zero cross.   Also swap them with the oldest ones in the cycle buffer.
 * Also get the maximum level in the cycle buffer.
 * 
 * @param buf [I/O] new samples, will be swapped by old samples when returned.
 * @param step [in] length of above.
 * @param levelp [out] maximum level in the cycle buffer.
 * 
 * @return zero-cross count of the samples in the cycle buffer.
 */
int
count_zc_e_level(SP16 *buf,int step,int *levelp)
{
  int i;
  SP16 tmp, level;

  level = 0;
  for (i=0; i<step; i++) {
    if (is_zc[top]==TRUE) {
      --zero_cross;
    }
    is_zc[top] = FALSE;
    /* exchange old data and buf */
    tmp = buf[i] + offset;
    if (is_trig) {
      if (sign==POSITIVE && tmp<0) {
	++zero_cross;
	is_zc[top] = TRUE;
	is_trig = FALSE;
	sign = NEGATIVE;
      } else if (sign==NEGATIVE && tmp>0) {
	++zero_cross;
	is_zc[top] = TRUE;
	is_trig = FALSE;
	sign = POSITIVE;
      }
    }
    if (abs(tmp)>trigger) {
      is_trig = TRUE;
    }
    if (abs(tmp)>level) level = abs(tmp);
    data[top] = buf[i];
    top++;
    if (valid_len < top) valid_len = top;
    if (top >= length) {
      top = 0;
    }
  }
  *levelp = (int)level;
  return (zero_cross);
}

/** 
 * Flush samples in the current cycle buffer.
 * 
 * @param newbuf [out] the samples in teh cycle buffer will be written here.
 * @param len [out] length of above.
 */
void
zc_copy_buffer(SP16 *newbuf, int *len)
{
  int i, t;
  if (valid_len < length) {
    t = 0;
  } else {
    t = top;
  }
  for(i=0;i<valid_len;i++) {
    newbuf[i] = data[t];
    if (++t == length) t = 0;
  }
  *len = valid_len;
}
