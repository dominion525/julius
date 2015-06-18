/**
 * @file   mfcc-core.c
 * @author Akinobu Lee
 * @date   Mon Aug  7 11:55:45 2006
 * 
 * <JA>
 * @brief  MFCC 特徴量の計算
 *
 * ここでは，窓をかけて取り出された音声波形データから MFCC 特徴量を
 * 算出するコア関数が納められています．
 * </JA>
 * 
 * <EN>
 * @brief  Compute MFCC parameter vectors
 *
 * These are core functions to compute MFCC vectors from windowed speech data.
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
#include <sent/mfcc.h>


#ifdef MFCC_SINCOS_TABLE
/* sin/cos tables */

/* sin/cos table for hamming window */
static double *costbl_hamming; ///< Cos table for hamming window
static int costbl_hamming_len = 0; ///< Length of above
/* cos/-sin table for FFT */
static double *costbl_fft; ///< Cos table for FFT
static double *sintbl_fft; ///< Sin table for FFT
static int tbllen = 0; ///< Length of above
/* cos table for MakeMFCC */
static double *costbl_makemfcc; ///< Cos table for DCT
static int costbl_makemfcc_len = 0; ///< Length of above
/* sin table for WeightCepstrum */
static double *sintbl_wcep; ///< Sin table for cepstrum weighting
static int sintbl_wcep_len = 0; ///< Length of above

#endif /* MFCC_SINCOS_TABLE */

static float sqrt2var;		///< Work area that holds value of sqrt(2.0) / fbank_num

#ifdef MFCC_SINCOS_TABLE
/* prepare tables */

/** 
 * Generate table for hamming window.
 * 
 * @param framesize [in] window size
 */
void
make_costbl_hamming(int framesize)
{
  int i;
  float a;

  if (costbl_hamming_len  == framesize) return;
  if (costbl_hamming_len != 0) {
    j_printerr("WARNING!! frame size changed!! (%d -> %d)\n",
	       costbl_hamming_len, framesize);
    free(costbl_hamming);
  }
  costbl_hamming = (double *)mymalloc(sizeof(double) * framesize);

  a = 2.0 * PI / (framesize - 1);
  for(i=1;i<=framesize;i++) {
    /*costbl_hamming[i-1] = 0.54 - 0.46 * cos(2 * PI * (i - 1) / (float)(framesize - 1));*/
    costbl_hamming[i-1] = 0.54 - 0.46 * cos(a * (i - 1));
  }
  costbl_hamming_len = framesize;
#ifdef MFCC_TABLE_DEBUG
  j_printerr("generated Hamming cos table (%d bytes)\n",
 	     costbl_hamming_len * sizeof(double));
#endif
}

/** 
 * Build tables for FFT.
 * 
 * @param n [in] 2^n = FFT point
 */
void
make_fft_table(int n)
{
  int m;
  int me, me1;
  
  if (tbllen == n) return;	/* need not update */
  
  if (tbllen != 0) {
    j_printerr("WARNING!! fft size changed!! (%d -> %d)\n", tbllen, n);
    free(costbl_fft);
    free(sintbl_fft);
  }    
  costbl_fft = (double *)mymalloc(sizeof(double) * n);
  sintbl_fft = (double *)mymalloc(sizeof(double) * n);
  for (m = 1; m <= n; m++) {
    me = 1 << m;
    me1 = me / 2;
    costbl_fft[m-1] =  cos(PI / me1);
    sintbl_fft[m-1] = -sin(PI / me1);
  }
  tbllen = n;
#ifdef MFCC_TABLE_DEBUG
  j_printerr("generated FFT sin/cos table (%d bytes)\n", tbllen * sizeof(double));
#endif
}

/** 
 * Generate table for DCT operation to make mfcc from fbank.
 * 
 * @param fbank_num [in] number of filer banks
 * @param mfcc_dim [in] number of dimensions in MFCC
 */
void
make_costbl_makemfcc(int fbank_num, int mfcc_dim)
{
  int size;
  int i, j, k;
  float B, C;

  size = fbank_num * mfcc_dim;
  if (costbl_makemfcc_len  == size) return;
  if (costbl_makemfcc_len != 0) {
    j_printerr("WARNING!! fbank num or mfcc dim changed!! (%d -> %d)\n",
	       costbl_makemfcc_len, size);
    free(costbl_makemfcc);
  }
  costbl_makemfcc = (double *)mymalloc(sizeof(double) * size);

  B = PI / fbank_num;
  k = 0;
  for(i=1;i<=mfcc_dim;i++) {
    C = i * B;
    for(j=1;j<=fbank_num;j++) {
      costbl_makemfcc[k] = cos(C * (j - 0.5));
      k++;
    }
  }
  costbl_makemfcc_len = size;
#ifdef MFCC_TABLE_DEBUG
  j_printerr("generated MakeMFCC cos table (%d bytes)\n",
 	     costbl_makemfcc_len * sizeof(double));
#endif
}

/** 
 * Generate table for weighing cepstrum.
 * 
 * @param lifter [in] cepstral liftering coefficient
 * @param mfcc_dim [in] number of dimensions in MFCC
 */
void
make_sintbl_wcep(int lifter, int mfcc_dim)
{
  int i;
  float a, b;

  if (sintbl_wcep_len  == mfcc_dim) return;
  if (sintbl_wcep_len != 0) {
    j_printerr("WARNING!! mfcc dim changed!! (%d -> %d)\n",
	       sintbl_wcep_len, mfcc_dim);
    free(sintbl_wcep);
  }
  sintbl_wcep = (double *)mymalloc(sizeof(double) * mfcc_dim);
  a = PI / lifter;
  b = lifter / 2.0;
  for(i=0;i<mfcc_dim;i++) {
    sintbl_wcep[i] = 1.0 + b * sin((i+1) * a);
  }
  sintbl_wcep_len = mfcc_dim;
#ifdef MFCC_TABLE_DEBUG
  j_printerr("generated WeightCepstrum sin table (%d bytes)\n",
 	     sintbl_wcep_len * sizeof(double));
#endif
}

#endif /* MFCC_SINCOS_TABLE */

/** 
 * Return mel-frequency.
 * 
 * @param k [in] channel number of filter bank
 * @param fres [in] constant value computed by "1.0E7 / (para.smp_period * fb.fftN * 700.0)"
 * 
 * @return the mel frequency.
 */
float Mel(int k, float fres)
{
  return(1127 * log(1 + (k-1) * fres));
}

/** 
 * Build filterbank information and generate tables for MFCC comptutation.
 * 
 * @param para [in] configuration parameters
 * 
 * @return the generated filterbank information. 
 */
FBankInfo InitFBank(Value para)
{
  FBankInfo fb;
  float mlo, mhi, ms, melk;
  int k, chan, maxChan, nv2;

  /* Calculate FFT size */
  fb.fftN = 2;  fb.n = 1;
  while(para.framesize > fb.fftN){
    fb.fftN *= 2; fb.n++;
  }

  nv2 = fb.fftN / 2;
  fb.fres = 1.0E7 / (para.smp_period * fb.fftN * 700.0);
  maxChan = para.fbank_num + 1;
  fb.klo = 2;   fb.khi = nv2;
  mlo = 0;      mhi = Mel(nv2 + 1, fb.fres);

  /* lo pass filter */
  if (para.lopass >= 0) {
    mlo = 1127*log(1+(float)para.lopass/700.0);
    fb.klo = ((float)para.lopass * para.smp_period * 1.0e-7 * fb.fftN) + 2.5;
    if (fb.klo<2) fb.klo = 2;
  }
  /* hi pass filter */
  if (para.hipass >= 0) {
    mhi = 1127*log(1+(float)para.hipass/700.0);
    fb.khi = ((float)para.hipass * para.smp_period * 1.0e-7 * fb.fftN) + 0.5;
    if (fb.khi>nv2) fb.khi = nv2;
  }

  /* Create vector of fbank centre frequencies */
  if((fb.cf = (float *)mymalloc((maxChan + 1) * sizeof(float))) == NULL){
    j_error("InitFBank: failed to malloc\n");
  }
  ms = mhi - mlo;
  for (chan = 1; chan <= maxChan; chan++) 
    fb.cf[chan] = ((float)chan / maxChan)*ms + mlo;

  /* Create loChan map, loChan[fftindex] -> lower channel index */
  if((fb.loChan = (short *)mymalloc((nv2 + 1) * sizeof(short))) == NULL){
    j_error("InitFBank: failed to malloc\n");
  }
  for(k = 1, chan = 1; k <= nv2; k++){
    if (k < fb.klo || k > fb.khi) fb.loChan[k] = -1;
    else {
      melk = Mel(k, fb.fres);
      while (fb.cf[chan] < melk && chan <= maxChan) ++chan;
      fb.loChan[k] = chan - 1;
    }
  }

  /* Create vector of lower channel weights */   
  if((fb.loWt = (float *)mymalloc((nv2 + 1) * sizeof(float))) == NULL){
    j_error("InitFBank: failed to malloc\n");
  }
  for(k = 1; k <= nv2; k++) {
    chan = fb.loChan[k];
    if (k < fb.klo || k > fb.khi) fb.loWt[k] = 0.0;
    else {
      if (chan > 0) 
	fb.loWt[k] = (fb.cf[chan + 1] - Mel(k, fb.fres)) / (fb.cf[chan + 1] - fb.cf[chan]);
      else
	fb.loWt[k] = (fb.cf[1] - Mel(k, fb.fres)) / (fb.cf[1] - mlo);
    }
  }
  
  /* Create workspace for fft */
  if((fb.Re = (float *)mymalloc((fb.fftN + 1) * sizeof(float))) == NULL){
    j_error("InitFBank: failed to malloc\n");
  }
  if((fb.Im = (float *)mymalloc((fb.fftN + 1) * sizeof(float))) == NULL){
    j_error("InitFBank: failed to malloc\n");
  }

#ifdef MFCC_SINCOS_TABLE
  /* generate tables */
  make_costbl_hamming(para.framesize);
  make_fft_table(fb.n);
  make_costbl_makemfcc(para.fbank_num, para.mfcc_dim);
  make_sintbl_wcep(para.lifter, para.mfcc_dim);
#endif

  sqrt2var = sqrt(2.0 / para.fbank_num);

  return(fb);
}

/** 
 * Free FBankInfo.
 * 
 * @param fb [in] filterbank information
 */
void
FreeFBank(FBankInfo fb)
{
  free(fb.cf);
  free(fb.loChan);
  free(fb.loWt);
  free(fb.Re);
  free(fb.Im);
}

/** 
 * Remove DC offset per frame
 * 
 * @param wave [i/o] waveform data in the current frame
 * @param framesize [in] frame size
 * 
 */
void
ZMeanFrame(float *wave, int framesize)
{		   
  int i;
  float mean;

  mean = 0.0;
  for(i = 1; i <= framesize; i++) mean += wave[i];
  mean /= framesize;
  for(i = 1; i <= framesize; i++) wave[i] -= mean;
}

/** 
 * Calculate Log Raw Energy.
 * 
 * @param wave [in] waveform data in the current frame
 * @param framesize [in] frame size
 * 
 * @return the calculated log raw energy.
 */
float CalcLogRawE(float *wave, int framesize)
{		   
  int i;
  double raw_E = 0.0;
  float energy;

  for(i = 1; i <= framesize; i++)
    raw_E += wave[i] * wave[i];
  energy = (float)log(raw_E);

  return(energy);
}

/** 
 * Apply pre-emphasis filter.
 * 
 * @param wave [i/o] waveform data in the current frame
 * @param para [in] configuration parameters
 */
void PreEmphasise (float *wave, Value para)
{
  int i;
   
  for(i = para.framesize; i >= 2; i--)
    wave[i] -= wave[i - 1] * para.preEmph;
  wave[1] *= 1.0 - para.preEmph;  
}

/** 
 * Apply hamming window.
 * 
 * @param wave [i/o] waveform data in the current frame
 * @param framesize [in] frame size
 */
void Hamming(float *wave, int framesize)
{
  int i;
#ifdef MFCC_SINCOS_TABLE
  for(i = 1; i <= framesize; i++)
    wave[i] *= costbl_hamming[i-1];
#else
  float a;
  a = 2 * PI / (framesize - 1);
  for(i = 1; i <= framesize; i++)
    wave[i] *= 0.54 - 0.46 * cos(a * (i - 1));
#endif
}

/** 
 * Apply FFT
 * 
 * @param xRe [i/o] real part of waveform
 * @param xIm [i/o] imaginal part of waveform
 * @param p [in] 2^p = FFT point
 */
void FFT(float *xRe, float *xIm, int p)
{
  int i, ip, j, k, m, me, me1, n, nv2;
  double uRe, uIm, vRe, vIm, wRe, wIm, tRe, tIm;
  
  n = 1<<p;
  nv2 = n / 2;
  
  j = 0;
  for(i = 0; i < n-1; i++){
    if(j > i){
      tRe = xRe[j];      tIm = xIm[j];
      xRe[j] = xRe[i];   xIm[j] = xIm[i];
      xRe[i] = tRe;      xIm[i] = tIm;
    }
    k = nv2;
    while(j >= k){
      j -= k;      k /= 2;
    }
    j += k;
  }

  for(m = 1; m <= p; m++){
    me = 1<<m;                me1 = me / 2;
    uRe = 1.0;                uIm = 0.0;
#ifdef MFCC_SINCOS_TABLE
    wRe = costbl_fft[m-1];    wIm = sintbl_fft[m-1];
#else
    wRe = cos(PI / me1);      wIm = -sin(PI / me1);
#endif
    for(j = 0; j < me1; j++){
      for(i = j; i < n; i += me){
	ip = i + me1;
	tRe = xRe[ip] * uRe - xIm[ip] * uIm;
	tIm = xRe[ip] * uIm + xIm[ip] * uRe;
	xRe[ip] = xRe[i] - tRe;   xIm[ip] = xIm[i] - tIm;
	xRe[i] += tRe;            xIm[i] += tIm;
      }
      vRe = uRe * wRe - uIm * wIm;   vIm = uRe * wIm + uIm * wRe;
      uRe = vRe;                     uIm = vIm;
    }
  }
}


/** 
 * Convert wave -> (spectral subtraction) -> mel-frequency filterbank
 * 
 * @param wave [in] waveform data in the current frame
 * @param fbank [out] the resulting mel-frequency filterbank
 * @param fb [in] filterbank information
 * @param para [in] configuration parameters
 * @param ssbuf [in] noise spectrum, or NULL if not apply subtraction
 */
void MakeFBank(float *wave, double *fbank, FBankInfo fb, Value para, float *ssbuf)
{
  int k, bin, i;
  double Re, Im, A, P, NP, H, temp;

  for(k = 1; k <= para.framesize; k++){
    fb.Re[k - 1] = wave[k];  fb.Im[k - 1] = 0.0;  /* copy to workspace */
  }
  for(k = para.framesize + 1; k <= fb.fftN; k++){
    fb.Re[k - 1] = 0.0;      fb.Im[k - 1] = 0.0;  /* pad with zeroes */
  }
  
  /* Take FFT */
  FFT(fb.Re, fb.Im, fb.n);

  if (ssbuf != NULL) {
    /* Spectral Subtraction */
    for(k = 1; k <= fb.fftN; k++){
      Re = fb.Re[k - 1];  Im = fb.Im[k - 1];
      P = sqrt(Re * Re + Im * Im);
      NP = ssbuf[k - 1];
      if((P * P -  para.ss_alpha * NP * NP) < 0){
	H = para.ss_floor;
      }else{
	H = sqrt(P * P - para.ss_alpha * NP * NP) / P;
      }
      fb.Re[k - 1] = H * Re;
      fb.Im[k - 1] = H * Im;
    }
  }

  /* Fill filterbank channels */ 
  for(i = 1; i <= para.fbank_num; i++)
    fbank[i] = 0.0;
  
  for(k = fb.klo; k <= fb.khi; k++){
    Re = fb.Re[k-1]; Im = fb.Im[k-1];
    A = sqrt(Re * Re + Im * Im);
    bin = fb.loChan[k];
    Re = fb.loWt[k] * A;
    if(bin > 0) fbank[bin] += Re;
    if(bin < para.fbank_num) fbank[bin + 1] += A - Re;
  }

  /* Take logs */
  for(bin = 1; bin <= para.fbank_num; bin++){ 
    temp = fbank[bin];
    if(temp < 1.0) temp = 1.0;
    fbank[bin] = log(temp);  
  }
}

/** 
 * Calculate 0'th cepstral coefficient.
 * 
 * @param fbank [in] filterbank
 * @param para [in] configuration parameters
 * 
 * @return 
 */
float CalcC0(double *fbank, Value para)
{
  int i; 
  float S;
  
  S = 0.0;
  for(i = 1; i <= para.fbank_num; i++)
    S += fbank[i];
  return S * sqrt2var;
}

/** 
 * Apply DCT to filterbank to make MFCC.
 * 
 * @param fbank [in] filterbank
 * @param mfcc [out] output MFCC vector
 * @param para [in] configuration parameters
 */
void MakeMFCC(double *fbank, float *mfcc, Value para)
{
#ifdef MFCC_SINCOS_TABLE
  int i, j, k;
  k = 0;
  /* Take DCT */
  for(i = 0; i < para.mfcc_dim; i++){
    mfcc[i] = 0.0;
    for(j = 1; j <= para.fbank_num; j++)
      mfcc[i] += fbank[j] * costbl_makemfcc[k++];
    mfcc[i] *= sqrt2var;
  }
#else
  int i, j;
  float B, C;
  
  B = PI / para.fbank_num;
  /* Take DCT */
  for(i = 1; i <= para.mfcc_dim; i++){
    mfcc[i - 1] = 0.0;
    C = i * B;
    for(j = 1; j <= para.fbank_num; j++)
      mfcc[i - 1] += fbank[j] * cos(C * (j - 0.5));
    mfcc[i - 1] *= sqrt2var;     
  }
#endif
}

/** 
 * Re-scale cepstral coefficients.
 * 
 * @param mfcc [i/o] a MFCC vector
 * @param para [in] configuration parameters
 */
void WeightCepstrum (float *mfcc, Value para)
{
#ifdef MFCC_SINCOS_TABLE
  int i;
  for(i=0;i<para.mfcc_dim;i++) {
    mfcc[i] *= sintbl_wcep[i];
  }
#else
  int i;
  float a, b, *cepWin;
  
  if((cepWin = (float *)mymalloc(para.mfcc_dim * sizeof(float))) == NULL){
    j_error("WeightCepstrum: failed to malloc\n");
  }
  a = PI / para.lifter;
  b = para.lifter / 2.0;
  
  for(i = 0; i < para.mfcc_dim; i++){
    cepWin[i] = 1.0 + b * sin((i + 1) * a);
    mfcc[i] *= cepWin[i];
  }
  
  free(cepWin);
#endif
}

/************************************************************************/
/************************************************************************/
/************************************************************************/
/************************************************************************/
/************************************************************************/

static double *fbank;   ///< Local buffer to hold filterbank
static FBankInfo fb;	///< Local buffer to hold filterbank information

/** 
 * Initialize calculation functions and work areas.
 * 
 * @param para [in] configuration parameters
 * @param bf [out] returns pointer to newly allocated window buffer
 * @param bflen [out] length of @a bf
 */
void
WMP_calc_init(Value para, float **bf, int *bflen)
{
  /* Get filterbank information and initialize tables */
  fb = InitFBank(para);
  
  if((fbank = (double *)mymalloc((para.fbank_num+1)*sizeof(double))) == NULL){
    j_error("Error: WMP_calc_init failed\n");
  }

  if((*bf = (float *)mymalloc(fb.fftN * sizeof(float))) == NULL){
    j_error("Error: WMP_calc_init failed\n");
  }

  *bflen = fb.fftN;

}

/** 
 * Calculate MFCC and log energy for one frame.  Perform spectral subtraction
 * if @a ssbuf is specified.
 * 
 * @param mfcc [out] buffer to hold the resulting MFCC vector
 * @param bf [i/o] work area for FFT
 * @param para [in] configuration parameters
 * @param ssbuf [in] noise spectrum, or NULL if not using spectral subtraction
 */
void
WMP_calc(float *mfcc, float *bf, Value para, float *ssbuf)
{
  float energy = 0.0;
  float c0 = 0.0;
  int p;

  if (para.zmeanframe) {
    ZMeanFrame(bf, para.framesize);
  }

  if (para.energy && para.raw_e) {
    /* calculate log raw energy */
    energy = CalcLogRawE(bf, para.framesize);
  }
  /* pre-emphasize */
  PreEmphasise(bf, para);
  /* hamming window */
  Hamming(bf, para.framesize);
  if (para.energy && ! para.raw_e) {
    /* calculate log energy */
    energy = CalcLogRawE(bf, para.framesize);
  }
  /* filterbank */
  MakeFBank(bf, fbank, fb, para, ssbuf);
  /* 0'th cepstral parameter */
  if (para.c0) c0 = CalcC0(fbank, para);
  /* MFCC */
  MakeMFCC(fbank, mfcc, para);
  /* weight cepstrum */
  WeightCepstrum(mfcc, para);
  /* set energy to mfcc */
  p = para.mfcc_dim;
  if (para.c0) mfcc[p++] = c0;
  if (para.energy) mfcc[p++] = energy;
}

/** 
 * Free work area for MFCC computation
 * 
 * @param bf [in] window buffer previously allocated by WMP_calc_init()
 */
void
WMP_calc_fin(float *bf)
{
  FreeFBank(fb);
  free(fbank);
  free(bf);
}
