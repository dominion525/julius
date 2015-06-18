/**
 * @file   wav2mfcc.c
 * @author Akinobu Lee
 * @date   Sun Sep 18 19:40:34 2005
 * 
 * <JA>
 * @brief  音声波形を特徴ベクトル系列(MFCC)に変換する．
 *
 * 入力された音声波形から，特徴ベクトル系列を抽出します．
 * Julius/Julianで抽出できる特徴ベクトルは，MFCC の任意次元数のもので，
 * _0, _E, _D, _A, _Z, _N の任意の組合わせをサポートします．
 * そのほか，窓長やフレームシフト，帯域カットなどのパラメータを指定できます．
 * 認識時には，音響モデルのヘッダとチェックが行われ，CMNの有無など
 * が決定されます．
 * 
 * ここの関数は，バッファ上に蓄積された音声波形データを一度に
 * 特徴ベクトル系列に変換するもので，ファイル入力などに用いられます．
 * マイク入力などで，入力と平行に認識を行う場合は，ここの関数ではなく，
 * realtime-1stpass.c 内で行われます．
 * </JA>
 * 
 * <EN>
 * @brief  Convert speech waveform to MFCC parameter vector sequence.
 *
 * Parameter vector sequence extraction of input speech is done
 * here.  The supported parameter is MFCC, with any combination of
 * all the qualifiers in HTK: _0, _E, _D, _A, _Z, _N.  Acoustic model
 * for recognition should be trained with the same parameter type.
 * You can specify other parameters such as window size, frame shift,
 * high/low frequency cut-off via runtime options.  At startup, Julius
 * will check for the parameter types of acoustic model if it conforms
 * the limitation, and determine whether other additional processing
 * is needed such as Cepstral Mean Normalization.
 *
 * Functions below are used to convert fully buffered whole sentence
 * utterance, and typically used for audio file input.  When input
 * is concurrently processed with recognition process at 1st pass, 
 * in case of microphone input, the MFCC computation will be done
 * within functions in realtime-1stpass.c instead of these.
 * </EN>
 * 
 * $Revision: 1.7 $
 * 
 */
/*
 * Copyright (c) 1991-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <julius.h>

#include <sys/stat.h>

/* make MFCC_E_D_N_Z from speech[0..speechlen-1] and returns
   newly malloced param */

/** 
 * <JA>
 * 音声波形データから MFCC パラメータを抽出する．
 * 
 * @param speech [in] 音声波形データ
 * @param speechlen [in] @a speech の長さ（単位：サンプル数）
 * 
 * @return 新たに割り付けられ抽出パラメータベクトルが格納されている
 * パラメータ構造体へのポインタを返す．
 * </JA>
 * <EN>
 * Extract MFCC parameters with sentence CMN from given waveform.
 * 
 * @param speech [in] buffer of speech waveform
 * @param speechlen [in] length of @a speech in samples
 * 
 * @return pointer to newly allocated parameter structure data with extracted
 * MFCC vector sequence.
 * </EN>
 */
HTK_Param *new_wav2mfcc(SP16 speech[], int speechlen)
{
  HTK_Param *param;
  int framenum;
  int i;
  int len;

  if (ssload_filename && ssbuf == NULL) {
    /* load noise spectrum for spectral subtraction from file (once) */
    if ((ssbuf = new_SS_load_from_file(ssload_filename, &sslen)) == NULL) {
      j_error("Error: failed to read \"%s\"\n", ssload_filename);
    }
  }

  if (sscalc) {
    /* compute noise spectrum from head silence for each input */
    len = sscalc_len * para.smp_freq / 1000;
    if (len > speechlen) len = speechlen;
#ifdef SSDEBUG
    printf("[%d]\n", len);
#endif
    ssbuf = new_SS_calculate(speech, len, para, &sslen);
  }
#ifdef SSDEBUG
  {
    int i;
    for(i=0;i<sslen;i++) {
      printf("%d: %f\n", i, ssbuf[i]);
    }
  }
#endif
  
  /* calculate frame length from speech length, frame size and frame shift */
  framenum = (int)((speechlen - para.framesize) / para.frameshift) + 1;
  if (framenum < 1) {
    j_printerr("input too short (%d samples), ignored\n", speechlen);
    return NULL;
  }
  
  /* malloc new param */
  param = new_param();
  param->parvec = (VECT **)mymalloc(sizeof(VECT *) * framenum);
  for(i=0;i<framenum;i++) {
    param->parvec[i] = (VECT *)mymalloc(sizeof(VECT) * para.veclen);
  }

  /* make MFCC from speech data */
  Wav2MFCC(speech, param->parvec, para, speechlen, ssbuf, sslen);

  /* set miscellaneous parameters */
  param->header.samplenum = framenum;
  param->header.wshift = para.smp_period * para.frameshift;
  param->header.sampsize = para.veclen * sizeof(VECT); /* not compressed */
  param->header.samptype = F_MFCC;
  if (para.delta) param->header.samptype |= F_DELTA;
  if (para.acc) param->header.samptype |= F_ACCL;
  if (para.energy) param->header.samptype |= F_ENERGY;
  if (para.c0) param->header.samptype |= F_ZEROTH;
  if (para.absesup) param->header.samptype |= F_ENERGY_SUP;
  if (para.cmn) param->header.samptype |= F_CEPNORM;
  param->veclen = para.veclen;
  param->samplenum = framenum;

  return param;
}

