/**
 * @file   gmm.c
 * @author Akinobu LEE
 * @date   Tue Mar 15 05:14:10 2005
 * 
 * <JA>
 * @brief  Gaussian Mixture Model による入力発話のスコア計算
 *
 * Gaussian Mixture Model (GMM) が起動時に指定された場合，Julius/Julian は
 * 入力発話に対してフレームごとにスコアを計算し，その累積スコアを算出する．
 * これはGMMに基づく入力音声の発話検証および棄却に用いられる．実際の計算は
 * 第1パスの認識処理と並行してリアルタイムに行なわれ，第1パス終了と同時に
 * 結果が出力される．
 *
 * GMMのスコア計算には Gaussian pruning の safe algorithm が用いられ，
 * 各フレームにおいて上位 N 個だけが正しく得られるように計算される．
 * ただし通常の認識用音響モデルの場合と異なり，直前フレームの順位情報は
 * 用いていない．
 * </JA>
 * 
 * <EN>
 * @brief  Compute scores using Gaussian Mixture Model
 *
 * When a Gaussian Mixture Model (GMM) is specified on startup, Julius/Julian
 * will compute the frame-wise likelihoods of each GMM for given inputs,
 * and produces the accumulated scores for each.  Then the input rejection is
 * determined from the value.  Actually, the recognition will be computed
 * on-line concurrently with the 1st pass, and the result will be got as
 * soon as the 1st pass ends.
 *
 * Gaussian pruning is performed using the safe algorithm in the computation
 * of GMM scores.  In each frame, pruning will be done to fully compute only
 * the top N Gaussians.  The algorithm is slightly simpler than AM computation,
 * i.e. the score order of the previous frame is not used here.
 * </EN>
 * 
 * $Revision: 1.5 $
 * 
 */

/*
 * Copyright (c) 2003-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <julius.h>

#undef MES

static LOGPROB *gmm_score;	///< Current accumurated scores for each GMM
static int framecount;		///< Current frame count


static LOGPROB *OP_calced_score; ///< Work area for Gaussian pruning on GMM: scores
static int *OP_calced_id; ///< Work area for Gaussian pruning on GMM: id
static int OP_calced_num; ///< Work area for Gaussian pruning on GMM: number of above
static int OP_calced_maxnum; ///< Work area for Gaussian pruning on GMM: size of allocated area
static int OP_gprune_num; ///< Number of Gaussians to be computed in Gaussian pruning
static VECT *OP_vec;		///< Local workarea to hold the input vector of current frame
static short OP_veclen;		///< Local workarea to hold the length of above

/** 
 * <JA>
 * Gaussianのスコアを計算済みGaussianリストのどの位置に挿入すべきかを返す．
 * 
 * @param score [in] 挿入したいスコア
 * @param len [in] 現在のリストの長さ
 * 
 * @return リスト内の挿入位置
 * </JA>
 * <EN>
 * Return insertion point where a computed Gaussian score should be
 * inserted in current list of computed Gaussians.
 * 
 * @param score [in] a score to be inserted
 * @param len [in] current length of the list
 * 
 * @return index to insert the value at the list.
 * </EN>
 */
static int
gmm_find_insert_point(LOGPROB score, int len)
{
  /* binary search on score */
  int left = 0;
  int right = len - 1;
  int mid;

  while (left < right) {
    mid = (left + right) / 2;
    if (OP_calced_score[mid] > score) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  return(left);
}

/** 
 * <JA>
 * あるGaussianの計算結果を計算済みGaussianリストに格納する．
 * 
 * @param id [in] Gaussian の GMM 内での番号
 * @param score [in] その Gaussian の計算された音響尤度
 * @param len [in] 現在のリストの長さ（現在格納されている Gaussian の数）
 * 
 * @return 格納後のリストの長さ．
 * </JA>
 * <EN>
 * Store a Gaussian likelihood to the list of computed Gaussians.
 * 
 * @param id [in] id of a Gaussian in the GMM to be stored
 * @param score [in] the likelihood of the Gaussian to be stored
 * @param len [in] current list length (= current number of Gaussians in cache)
 * 
 * @return the current length of list after the storing.
 * </EN>
 */
static int
gmm_cache_push(int id, LOGPROB score, int len)
{
  int insertp;

  if (len == 0) {               /* first one */
    OP_calced_score[0] = score;
    OP_calced_id[0] = id;
    return(1);
  }
  if (OP_calced_score[len-1] >= score) { /* bottom */
    if (len < OP_gprune_num) {          /* append to bottom */
      OP_calced_score[len] = score;
      OP_calced_id[len] = id;
      len++;
    }
    return len;
  }
  if (OP_calced_score[0] < score) {
    insertp = 0;
  } else {
    insertp = gmm_find_insert_point(score, len);
  }
  if (len < OP_gprune_num) {
    memmove(&(OP_calced_score[insertp+1]), &(OP_calced_score[insertp]), sizeof(LOGPROB)*(len - insertp));
    memmove(&(OP_calced_id[insertp+1]), &(OP_calced_id[insertp]), sizeof(int)*(len - insertp));    
  } else if (insertp < len - 1) {
    memmove(&(OP_calced_score[insertp+1]), &(OP_calced_score[insertp]), sizeof(LOGPROB)*(len - insertp - 1));
    memmove(&(OP_calced_id[insertp+1]), &(OP_calced_id[insertp]), sizeof(int)*(len - insertp - 1));
  }
  OP_calced_score[insertp] = score;
  OP_calced_id[insertp] = id;
  if (len < OP_gprune_num) len++;
  return(len);
}

/** 
 * <JA>
 * 現在のフレームの入力ベクトルに対する Gaussian の出力確率を計算する．
 * Gaussian pruning は行なわない．
 * 
 * @param binfo [in] Gaussian
 * 
 * @return 出力確率の対数値
 * </JA>
 * <EN>
 * Compute an output probability of a Gaussian for the input vector of
 * current frame.  No Gaussian pruning is performed in this function.
 * 
 * @param binfo [in] Gaussian
 * 
 * @return the log output probability.
 * </EN>
 */
static LOGPROB
gmm_compute_g_base(HTK_HMM_Dens *binfo)
{
  VECT tmp, x;
  VECT *mean;
  VECT *var;
  VECT *vec = OP_vec;
  short veclen = OP_veclen;

  if (binfo == NULL) return(LOG_ZERO);
  mean = binfo->mean;
  var = binfo->var->vec;
  tmp = 0.0;
  for (; veclen > 0; veclen--) {
    x = *(vec++) - *(mean++);
    tmp += x * x * *(var++);
  }
  return((tmp + binfo->gconst) * -0.5);
}

/** 
 * <JA>
 * 現在のフレームの入力ベクトルに対する Gaussian の出力確率を計算する．
 * 計算時には固定しきい値による safe pruning を行なう．
 * 
 * @param binfo [in] Gaussian
 * @param thres [in] safe pruning のための枝刈りしきい値
 * 
 * @return 出力確率の対数値
 * </JA>
 * <EN>
 * Compute an output probability of a Gaussian for the input vector of
 * current frame.  Safe pruning is performed in this function.
 * 
 * @param binfo [in] Gaussian
 * @param thres [in] pruning threshold for safe pruning
 * 
 * @return the log output probability.
 * </EN>
 */
static LOGPROB
gmm_compute_g_safe(HTK_HMM_Dens *binfo, LOGPROB thres)
{
  VECT tmp, x;
  VECT *mean;
  VECT *var;
  VECT *vec = OP_vec;
  short veclen = OP_veclen;
  VECT fthres = thres * (-2.0);

  if (binfo == NULL) return(LOG_ZERO);
  mean = binfo->mean;
  var = binfo->var->vec;
  tmp = binfo->gconst;
  for (; veclen > 0; veclen--) {
    x = *(vec++) - *(mean++);
    tmp += x * x * *(var++);
    if (tmp > fthres)  return LOG_ZERO;
  }
  return(tmp * -0.5);
}

/** 
 * <JA>
 * GMM計算における Gaussian pruning のためのワークエリアを確保する
 * 
 * @param hmminfo [in] HMM 構造体
 * @param prune_num [in] Gaussian pruning において計算する上位ガウス分布数
 * </JA>
 * <EN>
 * Allocate work area for Gaussian pruning for GMM calculation.
 * 
 * @param hmminfo [in] HMM structure
 * @param prune_num [in] number of top Gaussians to be computed at the pruning
 * </EN>
 */
static void
gmm_gprune_safe_init(HTK_HMM_INFO *hmminfo, int prune_num)
{
  /* store the pruning num to local area */
  OP_gprune_num = prune_num;
  /* maximum Gaussian set size = maximum mixture size */
  OP_calced_maxnum = hmminfo->maxmixturenum;
  /* allocate memory for storing list of currently computed Gaussian in a frame */
  OP_calced_score = (LOGPROB *)mymalloc(sizeof(LOGPROB) * OP_gprune_num);
  OP_calced_id = (int *)mymalloc(sizeof(int) * OP_gprune_num);
}

/** 
 * <JA>
 * @brief  ガウス分布集合内の各ガウス分布の現フレームに対する出力確率を計算する．
 *
 * Gaussian pruning により，実際には上位 N 個のみを保証する枝刈りが行なわれ，
 * スコアの低いガウス分布は計算されない．
 *
 * 計算結果は計算済みGaussianリスト (OP_calced_score, OP_calced_id) に
 * 格納される．
 * 
 * @param g [in] ガウス分布集合
 * @param gnum [in] @a g の長さ
 * </JA>
 * <EN>
 * @brief  Compute scores for a set of Gaussians with Gaussian pruning for
 * the current frame.
 *
 * Gaussian pruning will be performed to guarantee only the top N Gaussians
 * to be fully computed.  The results will be stored in the list of
 * computed Gaussians in OP_calced_score and OP_calced_id.
 * 
 * @param g [in] set of Gaussians
 * @param gnum [in] length of @a g
 * </EN>
 */
static void
gmm_gprune_safe(HTK_HMM_Dens **g, int gnum)
{
  int i, num = 0;
  LOGPROB score, thres;

  thres = LOG_ZERO;
  for (i = 0; i < gnum; i++) {
    if (num < OP_gprune_num) {
      score = gmm_compute_g_base(g[i]);
    } else {
      score = gmm_compute_g_safe(g[i], thres);
      if (score <= thres) continue;
    }
    num = gmm_cache_push(i, score, num);
    thres = OP_calced_score[num-1];
  }
  OP_calced_num = num;
}


/** 
 * <JA>
 * あるGMM状態の現フレームに対する出力確率を計算する．
 * 
 * @param s [in] GMM 状態
 * 
 * @return 出力確率の対数スコア
 * </JA>
 * <EN>
 * Compute the output probability of a GMM state for the current frame.
 * 
 * @param s [in] GMM state
 * 
 * @return the log probability.
 * </EN>
 */
static LOGPROB
gmm_calc_mix(HTK_HMM_State *s)
{
  int i;
  LOGPROB logprob = LOG_ZERO;

  /* compute Gaussian set */
  gmm_gprune_safe(s->b, s->mix_num);
  /* computed Gaussians will be set in:
     score ... OP_calced_score[0..OP_calced_num]
     id    ... OP_calced_id[0..OP_calced_num] */
  
  /* sum */
  for(i=0;i<OP_calced_num;i++) {
    OP_calced_score[i] += s->bweight[OP_calced_id[i]];
  }
  logprob = addlog_array(OP_calced_score, OP_calced_num);
  if (logprob <= LOG_ZERO) return LOG_ZERO;
  return (logprob * INV_LOG_TEN);
}

/** 
 * <JA>
 * 入力の指定フレームにおけるGMM状態のスコアを求めるメイン関数．
 * 
 * @param t [in] 計算するフレーム
 * @param stateinfo [in] GMM状態
 * @param param [in] 入力ベクトル系列
 * 
 * @return 出力確率の対数スコア
 * </JA>
 * <EN>
 * Main function to compute the output probability of a GMM state for
 * the specified input frame.
 * 
 * @param t [in] time frame on which the output probability should be computed
 * @param stateinfo [in] GMM state
 * @param param [in] input vector sequence
 * 
 * @return the log output probability.
 * </EN>
 */
static LOGPROB
outprob_state_nocache(int t, HTK_HMM_State *stateinfo, HTK_Param *param)
{
  /* set global values for outprob functions to access them */
  OP_vec = param->parvec[t];
  OP_veclen = param->veclen;
  return(gmm_calc_mix(stateinfo));
}


/************************************************************************/
/* global functions */

/** 
 * <JA>
 * GMMの計算のための初期化．起動時に一度だけ呼ばれる．
 * 
 * @param gmm [in] GMM定義構造体
 * @param gmm_prune_num [in] Gaussian pruning において計算するガウス分布数
 * </JA>
 * <EN>
 * Initialization for computing GMM likelihoods.  This will be called
 * once on startup.
 * 
 * @param gmm [in] GMM definition structure
 * @param gmm_prune_num [in] number of Gaussians which is guaranteed to be
 * fully computed on Gaussian pruning
 * </EN>
 */
void
gmm_init(HTK_HMM_INFO *gmm, int gmm_prune_num)
{
  HTK_HMM_Data *d;

  /* check GMM format */
  /* tied-mixture GMM is not supported */
  if (gmm->is_tied_mixture) {
    j_exit("Error: mixture-tying GMM is not supported yet.\n");
  }
  /* assume 3 state GMM (only one output state) */
  for(d=gmm->start;d;d=d->next) {
    if (d->state_num > 3) {
      j_exit("Error: GMM has more than 1 output state! [%s]\n", d->name);
    }
  }

  /* check if CMN needed */
  
  /* allocate score buffer */
  gmm_score = (LOGPROB *)mymalloc(sizeof(LOGPROB) * gmm->totalhmmnum);

  /* initialize work area */
  gmm_gprune_safe_init(gmm, gmm_prune_num);

  /* check if variances are inversed */
  if (!gmm->variance_inversed) {
    /* here, inverse all variance values for faster computation */
    htk_hmm_inverse_variances(gmm);
    gmm->variance_inversed = TRUE;
  }

}

/** 
 * <JA>
 * GMM計算のための準備を行なう．１入力開始ごとに呼ばれる．
 * 
 * @param gmm [in] GMM定義構造体
 * </JA>
 * <EN>
 * Prepare for the next GMM computation.  This will be called just before
 * an input begins.
 * 
 * @param gmm [in] GMM definition structure
 * </EN>
 */
void
gmm_prepare(HTK_HMM_INFO *gmm)
{
  HTK_HMM_Data *d;
  int i;

  /* initialize score buffer and frame count */
  i = 0;
  for(d=gmm->start;d;d=d->next) {
    gmm_score[i] = 0.0;
    i++;
  }
  framecount = 0;
}

/** 
 * <JA>
 * 与えられた入力ベクトル列上のあるフレームについて，全GMMのスコアを計算し，
 * 計算結果を gmm_score に積算する．
 * 
 * @param gmm [in] GMM定義構造体
 * @param param [in] 入力ベクトル列
 * @param t [in] 計算する時刻フレーム
 * </JA>
 * <EN>
 * Compute output probabilities of all GMM for a given input vector, and
 * accumulate the results to the gmm_score buffer.
 * 
 * @param gmm [in] GMM definition structure
 * @param param [in] input vectors
 * @param t [in] time frame to compute the probabilities.
 * </EN>
 */
void
gmm_proceed(HTK_HMM_INFO *gmm, HTK_Param *param, int t)
{
  HTK_HMM_Data *d;
  int i;

  framecount++;
  i = 0;
  for(d=gmm->start;d;d=d->next) {
    gmm_score[i] += outprob_state_nocache(t, d->s[1], param);
#ifdef MES
    printf("[%d:total=%f avg=%f]\n", i, gmm_score[i], gmm_score[i] / (float)framecount);
#endif
    i++;
  }
}

static HTK_HMM_Data *max_d;	///< Local workarea to hold the pointer to GMM which resulted in the maximum score
#ifdef CONFIDENCE_MEASURE
static LOGPROB gmm_max_cm;	///< Local workarea to hold the posterior probability based confidence score of the maximum GMM above
#endif
static HTK_HMM_INFO *gmm_local; ///< Local workarea to hold the GMM definition used in the computation, for result output
/** 
 * <JA>
 * @brief  GMMの計算を終了し，結果を出力する．
 *
 * gmm_proceed() によって累積された各フレームごとのスコアから，
 * 最大スコアのGMMを決定する．その事後確率に基づく信頼度を計算し
 * 最終的な結果を result_gmm() によって出力する．
 * 
 * @param gmm [in] GMM定義構造体
 * </JA>
 * <EN>
 * @brief  Finish the GMM computation for an input, and output the result.
 *
 * The GMM of the maximum score is finally determined from the accumulated
 * scores computed by gmm_proceed(), and compute the confidence score of the
 * maximum GMM using posterior probability.  Then the result will be output
 * using result_gmm().
 * 
 * @param gmm [in] GMM definition structure.
 * </EN>
 */
void
gmm_end(HTK_HMM_INFO *gmm)
{
  HTK_HMM_Data *d;
  LOGPROB maxprob, sum;
  int i;

  /* get max score */
  i = 0;
  maxprob = LOG_ZERO;
  for(d=gmm->start;d;d=d->next) {
    if (maxprob < gmm_score[i]) {
      max_d = d;
      maxprob = gmm_score[i];
    }
    i++;
  }
#ifdef CONFIDENCE_MEASURE
  /* compute CM */
  sum = 0.0;
  i = 0;
  for(d=gmm->start;d;d=d->next) {
    sum += pow(10, cm_alpha * (gmm_score[i] - maxprob));
    i++;
  }
  gmm_max_cm = 1.0 / sum;
#endif
  
  /* output result */
  gmm_local = gmm;
  result_gmm();
}

/** 
 * <JA>
 * GMMの識別結果，最後の入力が音声入力として有効であったか
 * 無効であったかを返す．
 * 
 * @return 一位のGMMの名前が gmm_reject_cmn_string 内に無ければ valid として
 * TRUE, あれば invalid として FALSE を返す．
 * </JA>
 * <EN>
 * Return whether the last input was valid or invalid, from the result of
 * GMM computation.
 * 
 * @return TRUE if input is valid, i.e. the name of maximum GMM is not included
 * in gmm_reject_cmn_string, or FALSE if input is invalid, i.e. the name is
 * included in that string.
 * </EN>
 */
boolean
gmm_valid_input()
{
  if (max_d == NULL) return FALSE;
  if (strstr(gmm_reject_cmn_string, max_d->name)) {
    return FALSE;
  }
  return TRUE;
}

/*********************************************************************/
/********************* RESULT OUTPUT FOR GMM *************************/
/*********************************************************************/
/** 
 * <JA>
 * GMMの計算結果を標準出力に出力する．("-result tty" 用)
 * </JA>
 * <EN>
 * Output result of GMM computation to standard out.
 * (for "-result tty" option)
 * </EN>
 */
void
ttyout_gmm(){
  HTK_HMM_Data *d;
  int i;

  if (debug2_flag) {
    j_printf("--- GMM result begin ---\n");
    i = 0;
    for(d=gmm_local->start;d;d=d->next) {
      j_printf("  [%8s: total=%f avg=%f]\n", d->name, gmm_score[i], gmm_score[i] / (float)framecount);
      i++;
    }
    j_printf("  max = \"%s\"", max_d->name);
#ifdef CONFIDENCE_MEASURE
    j_printf(" (CM: %f)", gmm_max_cm);
#endif
    j_printf("\n");
    j_printf("--- GMM result end ---\n");
  } else if (verbose_flag) {
    j_printf("GMM: max = \"%s\"", max_d->name);
#ifdef CONFIDENCE_MEASURE
    j_printf(" (CM: %f)", gmm_max_cm);
#endif
    j_printf("\n");
  } else {
    j_printf("[GMM: %s]\n", max_d->name);
  }
}

/** 
 * <JA>
 * GMMの計算結果をモジュールのクライアントに送信する ("-result msock" 用)
 * </JA>
 * <EN>
 * Send the result of GMM computation to module client.
 * (for "-result msock" option)
 * </EN>
 */
void
msock_gmm()
{
  module_send(module_sd, "<GMM RESULT=\"%s\"", max_d->name);
#ifdef CONFIDENCE_MEASURE
  module_send(module_sd, " CMSCORE=\"%f\"", gmm_max_cm);
#endif
  module_send(module_sd, "/>\n.\n");
}
