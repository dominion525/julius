/**
 * @file   mfcc.h
 * @author Akinobu LEE
 * @date   Fri Feb 11 03:40:52 2005
 *
 * <JA>
 * @brief MFCC計算のための定義
 *
 * このファイルには，音声波形データからMFCC形式の特徴量ベクトル系列を
 * 計算するための構造体の定義およびデフォルト値が含まれています．
 * デフォルト値は Julius とともに配布されている音響モデルで使用している
 * 値であり，HTKのデフォルトとは値が異なる部分がありますので注意して下さい．
 * </JA>
 * <EN>
 * @brief Definitions for MFCC computation
 *
 * This file contains structures and default values for extracting speech
 * parameter vectors of Mel-Frequency Cepstral Cefficients (MFCC).
 * The default values here are the ones used in the standard acoustic models
 * distributed together with Julius, and some of them have different value from
 * HTK defaults.  So be careful of the default values.
 * </EN>
 *
 * @sa libsent/src/wav2mfcc/wav2mfcc.c
 * @sa libsent/src/wav2mfcc/wav2mfcc-pipe.c
 * @sa julius/wav2mfcc.c
 * @sa julius/realtime-1stpass.c
 * 
 * $Revision: 1.8 $
 * 
 */


/************************************************************************/
/*    mfcc.h                                                            */
/*                                                                      */
/*    Author    : Yuichiro Nakano                                       */
/************************************************************************/

#ifndef __MFCC_H__
#define __MFCC_H__

/// DEBUG: define if you want to enable debug messages for sin/cos table operation
#undef MFCC_TABLE_DEBUG

#include <sent/stddefs.h>
#include <sent/htk_defs.h>
#include <ctype.h>

#define DEF_SMPPERIOD   625	///< Default sampling period in 100ns (625 = 16kHz)
#define DEF_FRAMESIZE   400	///< Default Window size in samples, similar to WINDOWSIZE in HTK (unit is different)
#define DEF_FFTNUM      512	///< Number of FFT steps
#define DEF_FRAMESHIFT  160	///< Default frame shift length in samples
#define DEF_PREENPH     0.97	///< Default pre-emphasis coefficient, corresponds to PREEMCOEF in HTK
#define DEF_MFCCDIM     12	///< Default number of MFCC dimension, corresponds to NUMCEPS in HTK
#define DEF_CEPLIF      22	///< Default cepstral Liftering coefficient, corresponds to CEPLIFTER in HTK
#define DEF_FBANK       24	///< Default number of filterbank channels, corresponds to NUMCHANS in HTK
#define DEF_DELWIN      2	///< Default delta window size, corresponds to DELTAWINDOW in HTK
#define DEF_ACCWIN      2	///< Default acceleration window size, corresponds to ACCWINDOW in HTK
#define DEF_SILFLOOR    50.0	///< Default energy silence floor in dBs, corresponds to SILFLOOR in HTK
#define DEF_ESCALE      1.0	///< Default scaling coefficient of log energy, corresponds to ESCALE in HTK
#define DEF_SSALPHA     2.0	///< Default alpha coefficient for spectral subtraction
#define DEF_SSFLOOR     0.5	///< Default flooring coefficient for spectral subtraction

#define VALUE_VERSION 1	///< Integer version number of Value, for embedding

/// mfcc configuration parameter values
typedef struct {
  long smp_period;      ///< Sampling period in 100ns units
  long smp_freq;	///< Sampling frequency
  int framesize;        ///< Window size in samples, similar to WINDOWSIZE in HTK (unit is different)
  int frameshift;       ///< Frame shift length in samples
  float preEmph;        ///< Pre-emphasis coefficient, corresponds to PREEMCOEF in HTK
  int lifter;           ///< Cepstral liftering coefficient, corresponds to CEPLIFTER in HTK
  int fbank_num;        ///< Number of filterbank channels, corresponds to NUMCHANS in HTK
  int delWin;           ///< Delta window size, corresponds to DELTAWINDOW in HTK
  int accWin;           ///< Acceleration window size, corresponds to ACCWINDOW in HTK
  float silFloor;       ///< Energy silence floor in dBs, corresponds to SILFLOOR in HTK
  float escale;         ///< Scaling coefficient of log energy, corresponds to ESCALE in HTK
  int hipass;		///< High frequency cut-off in fbank analysis, -1 if disabled, corresponds to HIFREQ in HTK
  int lopass;		///< Low frequency cut-off in fbank analysis, -1 if disabled, corresponds to LOFREQ in HTK
  int enormal;          ///< 1 if normalise raw energy, 0 if disabled, corresponds to ENORMALISE in HTK
  int raw_e;            ///< 1 if using raw energy, 0 if disabled, corresponds to RAWENERGY in HTK
  float ss_alpha;	///< Alpha coefficient for spectral subtraction
  float ss_floor;	///< Flooring coefficient for spectral subtraction
  int zmeanframe;	///< 1 if apply zero mean frame like ZMEANSOURCE in HTK

  /* items below does not need to be embedded, because they can be
     detemined from the acoustic model header, or should be computed
     from run-time variables */
  int delta;            ///< 1 if delta coef. needs to be computed
  int acc;              ///< 1 if acceleration coef. needs to be computed
  int energy;		///< 1 if energy coef. needs to be computed
  int c0;		///< 1 if use 0'th cepstral parameter, 0 if disabled, corresponds to _0 qualifier in HTK
  int absesup;		///< 1 if absolute energy should be suppressed
  int cmn;              ///< 1 if use Cepstrum Mean Normalization, 0 if disabled, corresponds to _Z qualifier in HTK
  int mfcc_dim;         ///< Number of MFCC dimensions
  int baselen;		///< Number of base MFCC dimension with energies
  int vecbuflen;	///< Vector length needed for computation
  int veclen;		///< Resulting length of vector

  int loaded;		///< 1 if these parameters were loaded from HTK config file or binhmm header
}Value;

/// Workspace for filterbank analysis
typedef struct {
   int fftN;            ///< Number of FFT point
   int n;               ///< log2(fftN)
   int klo;             ///< FFT indices of lopass cut-off
   int khi;             ///< FFT indices of hipass cut-off
   float fres;          ///< Scaled FFT resolution
   float *cf;           ///< Array[1..pOrder+1] of centre freqs
   short *loChan;       ///< Array[1..fftN/2] of loChan index
   float *loWt;         ///< Array[1..fftN/2] of loChan weighting
   float *Re;           ///< Array[1..fftN] of fftchans (real part)
   float *Im;           ///< Array[1..fftN] of fftchans (imag part)
} FBankInfo;

/// Cycle buffer for delta computation
typedef struct {
  float **mfcc;			///< MFCC buffer
  int veclen;			///< Vector length of above
  float *vec;			///< Points to the current MFCC
  int win;			///< Delta window length
  int len;			///< Length of the buffer (= win*2+1)
  int store;			///< Current next storing point
  boolean *is_on;		///< TRUE if data filled
  int B;			///< B coef. for delta computation
} DeltaBuf;

/**** mfcc-core.c ****/
void WMP_calc_init(Value para, float **bf, int *bflen);
void WMP_calc(float *mfcc, float *bf, Value para, float *ssbuf);
void WMP_calc_fin(float *bf);
#ifdef MFCC_SINCOS_TABLE
/* functions for making tables */
void make_costbl_hamming(int framesize);
void make_fft_table(int n);
void make_costbl_makemfcc(int fbank_num, int mfcc_dim);
void make_sintbl_wcep(int lifter, int mfcc_dim);
#endif
/* Get filterbank information */
FBankInfo InitFBank(Value para);
void FreeFBank(FBankInfo fb);
/* Apply hamming window */
void Hamming (float *wave, int framesize);
/* Apply pre-emphasis filter */
void PreEmphasise (float *wave, Value para);
/* Return mel-frequency */
float Mel(int k, float fres);
/* Apply FFT */
void FFT(float *xRe, float *xIm, int p);
/* Convert wave -> mel-frequency filterbank */
void MakeFBank(float *wave, double *fbank, FBankInfo info, Value para, float *ssbuf);
/* Apply the DCT to filterbank */ 
void MakeMFCC(double *fbank, float *mfcc, Value para);
/* Calculate 0'th Cepstral parameter*/
float CalcC0(double *fbank, Value para);
/* Calculate Log Raw Energy */
float CalcLogRawE(float *wave, int framesize);
/* Zero Mean Souce by frame */
void ZMeanFrame(float *wave, int framesize);
/* Re-scale cepstral coefficients */
void WeightCepstrum (float *mfcc, Value para);

/**** wav2mfcc-buffer.c ****/
/* Convert wave -> MFCC_E_D_(Z) (batch) */
int Wav2MFCC(SP16 *wave, float **mfcc, Value para, int nSamples, float *ssbuf, int ssbuflen);
/* Calculate delta coefficients (batch) */
void Delta(float **c, int frame, Value para);
/* Calculate acceleration coefficients (batch) */
void Accel(float **c, int frame, Value para);
/* Normalise log energy (batch) */
void NormaliseLogE(float **c, int frame_num, Value para);
/* Cepstrum Mean Normalization (batch) */
void CMN(float **mfcc, int frame_num, int dim);

/**** wav2mfcc-pipe.c ****/
void WMP_init(Value para, float **bf, float *ssbuf, int ssbuflen);
DeltaBuf *WMP_deltabuf_new(int veclen, int windowlen);
void WMP_deltabuf_free(DeltaBuf *db);
void WMP_deltabuf_prepare(DeltaBuf *db);
boolean WMP_deltabuf_proceed(DeltaBuf *db, float *new_mfcc);
boolean WMP_deltabuf_flush(DeltaBuf *db);
void CMN_realtime_init(int dimension, float weight);
void CMN_realtime_prepare();
void CMN_realtime(float *mfcc, int dim);
void CMN_realtime_update();
boolean CMN_load_from_file(char *filename, int dim);
boolean CMN_save_to_file(char *filename);
void energy_max_init();
void energy_max_prepare(Value *para);
LOGPROB energy_max_normalize(LOGPROB f, Value *para);

/**** ss.c ****/
/* spectral subtraction */
float *new_SS_load_from_file(char *filename, int *slen);
float *new_SS_calculate(SP16 *wave, int wavelen, Value para, int *slen);

/**** para.c *****/
void undef_para(Value *para);
void make_default_para(Value *para);
void make_default_para_htk(Value *para);
void apply_para(Value *dst, Value *src);
boolean htk_config_file_parse(char *HTKconffile, Value *para);
void calc_para_from_header(Value *para, short param_type, short vec_size);
void put_para(Value *para);


#endif /* __MFCC_H__ */
