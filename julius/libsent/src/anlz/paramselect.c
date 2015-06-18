/**
 * @file   paramselect.c
 * @author Akinobu LEE
 * @date   Sun Feb 13 20:46:39 2005
 *
 * <JA>
 * @brief  パラメータベクトルの型のチェックと調整
 *
 * %HMMと入力特徴パラメータの型をチェックします．タイプが一致しない場合，
 * 特徴パラメータの一部を削除することで一致するよう調整できるかどうか
 * を試みます．（例：特徴量ファイルが MFCC_E_D_Z (26次元) で与えられた
 * とき，音響モデルが MFCC_E_D_N_Z (25次元) である場合，絶対値パワー項を
 * 差し引くことで調整できます．）
 *
 * 調整アルゴリズムは以下のとおりです．
 *    -# 入力の各ベクトル要素に対応するマークを 0 に初期化
 *    -# %HMMで要求されている型に対応しないベクトル要素に 1 をマークする
 *    -# 新たにパラメータ領域を確保し，必要な要素（マークされていない要素）
 *       のみをコピーする．
 * 
 * </JA>
 * <EN>
 * @brief  Check and adjust parameter vector types
 *
 * This file is to check if %HMM parameter and input parameter are the same.
 * If they are not the same, it then tries to modify the input to match the
 * required format in %HMM.  Available parameter modification is only to
 * delete some part of the parameter (ex. MFCC_E_D_Z (26 dim.) can be
 * modified to MFCC_E_D_N_Z (25 dim.) by just deleting the absolute power).
 * Note that no parameter generation or conversion is implemented currently.
 *
 * The adjustment algorithm is as follows:
 *    -# Initialize mark to 0 for each input vector element.
 *    -# Compare parameter type and mark unnecessary element as EXCLUDE(=1).
 *    -# Allocate a new parameter area and copy needed (=NOT marked) element.
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
#include <sent/htk_param.h>
#include <sent/htk_hmm.h>


/* vector mark & delete */
static int *vmark;		///< Exclusion marks of input vector
static int vlen;		///< Length of above
static int vnewlen;		///< Adjusted new length
static short src_type;		///< Newly set source type

/** 
 * Allocate new area for exclusion marks.
 * 
 * @param param [in] input parameter
 */
static void
init_mark(HTK_Param *param)
{
  int i;
  vmark = (int *)mymalloc(sizeof(int) * param->veclen);
  vlen = param->veclen;
  for (i=0;i<vlen;i++) {
    vmark[i] = 0;
  }
}

/** 
 * Free the exclusion marks.
 * 
 */
static void
free_mark()
{
  free(vmark);
}

/** 
 * Put exlusion marks for vector for @a len elements from @a loc -th dimension.
 * 
 * @param loc [in] beginning dimension to mark
 * @param len [in] number of dimension to mark from @a loc
 */
static void
mark_exclude_vector(int loc, int len)
{
  int i;
#ifdef DEBUG
  j_printf("delmark: %d-%d\n",loc, loc+len-1);
#endif
  for (i=0;i<len;i++) {
    if (loc + i >= vlen) {
      j_error("delmark buffer exceeded!!\n");
    }
    vmark[loc+i] = 1;
  }
#ifdef DEBUG
  j_printf("now :");
  for (i=0;i<vlen;i++) {
    if (vmark[i] == 1) {
      j_printf("-");
    } else {
      j_printf("O");
    }
  }
  j_printf("\n");
#endif
}

/**
 * @brief  Execute exclusion for a parameter data according to the
 * exclusion marks.
 * 
 * Execute vector element exclusion by copying vector from @a src to @a new
 * according to the current exclusion marks.  The parameter vector of @a new
 * will be newly allocated here.
 * 
 * @param new [out] new adjusted parameter
 * @param src [in] source parameter
 */
static void
exec_exclude_vectors(HTK_Param *new, HTK_Param *src)
{
  int i, loc;
  unsigned int t;
  /* src ---(vmark)--> new */

  /* new length */
  new->veclen = vnewlen;
#ifdef DEBUG
  j_printf("new length = %d\n", new->veclen);
#endif

  /* malloc */
  new->samplenum = src->samplenum;
  new->parvec = (VECT **)mymalloc(sizeof(VECT *) * new->samplenum);
  
  for(t = 0; t < src->samplenum; t++) {
    new->parvec[t] = (VECT *)mymalloc(sizeof(VECT) * new->veclen);
    loc = 0;
    for (i=0;i<src->veclen;i++) {
      if (vmark[i] == 0) {	/* not delete == copy */
	new->parvec[t][loc] = src->parvec[t][i];
	loc++;
      }
    }
  }
}

/** 
 * @brief  Execute deletion for one vector according to the exlusion marks.
 *
 * This can be used to frame-synchronous parameter adjustment.
 * 
 * @param vec [I/O] target vector
 * @param len [in]  length of above
 * 
 * @return the new length.
 */
int
exec_exclude_one_vector(VECT *vec, int len)
{
  int i,loc;
  
  loc = 0;
  for (i=0;i<len;i++) {
    if (vmark[i] == 0) {	/* not delete == copy */
      vec[loc] = vec[i];
      loc++;
    }
  }
  return(loc);
}




/** 
 * Guess the length of the base coefficient according to the total vector
 * length and parameter type.
 * 
 * @param p [in] parameter data
 * @param qualtype [in] parameter type
 * 
 * @return the guessed size of the base coefficient.
 */
int
guess_basenum(HTK_Param *p, short qualtype)
{
  int size;
  int compnum;
  
  compnum = 1 + ((qualtype & F_DELTA) ? 1 : 0) + ((qualtype & F_ACCL) ? 1 : 0);
  
  size = p->veclen;
  if (p->header.samptype & F_ENERGY_SUP) size += 1;
  if ((size % compnum) != 0) {
    j_printerr("ERROR: illegal vector length (should not happen)!\n");
    return -1;
  }
  size /= compnum;
  if (p->header.samptype & F_ENERGY) size -= 1;
  if (p->header.samptype & F_ZEROTH) size -= 1;

  return(size);
}

/* can add: _N */
/* can sub: _E_D_A_0 */

/** 
 * Compare source parameter type and required type in HTK %HMM, and set mark.
 * 
 * @param src [in] input parameter
 * @param dst_type_arg [in] required parameter type
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
select_param_vmark(HTK_Param *src, short dst_type_arg)
{
  short dst_type;
  short del_type, add_type;
  int basenum, pb[3],pe[3],p0[3]; /* location */
  int i, len;
  char srcstr[80], dststr[80], buf[80];

  src_type = src->header.samptype & ~(F_COMPRESS | F_CHECKSUM);
  src_type &= ~(F_BASEMASK);	/* only qualifier code needed */
  srcstr[0] = '\0';
  param_qualcode2str(srcstr, src_type, FALSE);
  dst_type = dst_type_arg & ~(F_COMPRESS | F_CHECKSUM);
  dst_type &= ~(F_BASEMASK);	/* only qualifier code needed */
  dststr[0] = '\0';
  param_qualcode2str(dststr, dst_type, FALSE);

#ifdef DEBUG
  j_printf("try to select qualifiers: %s -> %s\n", srcstr, dststr);
#endif

  if (dst_type == F_ERR_INVALID) {
    j_printerr("ERROR: unknown parameter kind for selection: %s\n", dststr);
    return(FALSE);
  }
  
  /* guess base coefficient num */
  basenum = guess_basenum(src, src_type);
  if (basenum < 0) {		/* error */
    return(FALSE);
  }
#ifdef DEBUG
  j_printf("base num = %d\n", basenum);
#endif

  /* determine which component to use */
  del_type = src_type & (~(dst_type));
  add_type = (~(src_type)) & dst_type;

  init_mark(src);

  /* vector layout for exclusion*/
  pb[0] = 0;
  if ((src_type & F_ENERGY) && (src_type & F_ZEROTH)){
    p0[0] = basenum;
    pe[0] = basenum + 1;
    len = basenum + 2;
  } else if ((src_type & F_ENERGY) || (src_type & F_ZEROTH)){
    p0[0] = pe[0] = basenum;
    len = basenum + 1;
  } else {
    p0[0] = pe[0] = 0;
    len = basenum;
  }
  for (i=1;i<3;i++) {
    pb[i] = pb[i-1] + len;
    pe[i] = pe[i-1] + len;
    p0[i] = p0[i-1] + len;
  }
  if (src_type & F_ENERGY_SUP) {
    pe[0] = 0;
    for (i=1;i<3;i++) {
      pb[i]--;
      pe[i]--;
      p0[i]--;
    }
  }
  
  /* modification begin */
  /* qualifier addition: "_N" */
#ifdef DEBUG
  buf[0] = '\0';
  j_printf("try to add: %s\n", param_qualcode2str(buf, add_type, FALSE));
#endif
  
  if (add_type & F_ENERGY_SUP) {
    if (src_type & F_ENERGY) {
      mark_exclude_vector(pe[0], 1);
      src_type = src_type | F_ENERGY_SUP;
    } else if (src_type & F_ZEROTH) {
      mark_exclude_vector(p0[0], 1);
      src_type = src_type | F_ENERGY_SUP;
    } else {
      j_printerr("WARNING: \"_N\" needs \"_E\" or \"_0\". ignored\n");
    }
    add_type = add_type & (~(F_ENERGY_SUP)); /* set to 0 */
  }
  if (add_type != 0) {		/* others left */
    buf[0] = '\0';
    j_printerr("WARNING: can do only parameter exclusion. qualifiers %s ignored\n", param_qualcode2str(buf, add_type, FALSE));
  }
  
  /* qualifier excludeion: "_D","_A","_0","_E" */
#ifdef DEBUG
  buf[0] = '\0';
  j_printf("try to del: %s\n", param_qualcode2str(buf, del_type, FALSE));
#endif

  if (del_type & F_DELTA) del_type |= F_ACCL;
  /* mark delete vector */
  if (del_type & F_ACCL) {
    mark_exclude_vector(pb[2], len);
    src_type &= ~(F_ACCL);
    del_type &= ~(F_ACCL);
  }
  if (del_type & F_DELTA) {
    mark_exclude_vector(pb[1], len);
    src_type &= ~(F_DELTA);
    del_type &= ~(F_DELTA);
  }
  
  if (del_type & F_ENERGY) {
    mark_exclude_vector(pe[2], 1);
    mark_exclude_vector(pe[1], 1);
    if (!(src_type & F_ENERGY_SUP)) {
      mark_exclude_vector(pe[0], 1);
    }
    src_type &= ~(F_ENERGY | F_ENERGY_SUP);
    del_type &= ~(F_ENERGY | F_ENERGY_SUP);
  }
  if (del_type & F_ZEROTH) {
    mark_exclude_vector(p0[2], 1);
    mark_exclude_vector(p0[1], 1);
    if (!(src_type & F_ENERGY_SUP)) {
      mark_exclude_vector(p0[0], 1);
    }
    src_type &= ~(F_ZEROTH | F_ENERGY_SUP);
    del_type &= ~(F_ZEROTH | F_ENERGY_SUP);
  }
  
  if (del_type != 0) {		/* left */
    buf[0] = '\0';
    j_printerr("WARNING: cannot exclude qualifiers %s. selection ignored\n", param_qualcode2str(buf, del_type, FALSE));
  }

  vnewlen = 0;
  for (i=0;i<vlen;i++) {
    if (vmark[i] == 0) vnewlen++;
  }

  return(TRUE);
  
}


/** 
 * Extracts needed parameter vector specified in dst_type_arg from src,
 * and returns newly allocated parameter structure.
 * 
 * @param src [in] input parameter
 * @param dst_type_arg [in] required parameter type
 * 
 * @return newly allocated adjusted parameter, NULL on failure.
 */
HTK_Param *
new_select_param_kind(HTK_Param *src, short dst_type_arg)
{
  HTK_Param *new;

  /* prepare new parameter */
  new = new_param();
  /* mark to determine operation */
  if (select_param_vmark(src, dst_type_arg) == FALSE) return(NULL);
  /* execute deletion (copy needed to new param)*/
  exec_exclude_vectors(new, src);
  
  /* copy & set header info */
  new->header.samplenum = src->header.samplenum;
  new->header.wshift = src->header.wshift;
  new->header.sampsize = new->veclen * sizeof(VECT);
  new->header.samptype = src_type | (src->header.samptype & F_BASEMASK);
  
#ifdef DEBUG
  j_printf("new param made: %s\n", param_code2str(buf, new->header.samptype, FALSE));
#endif
  
  free_mark();

  return(new);
}

/** 
 * @brief  Top function to adjust parameter.
 *
 * It compares the types for the given parameter @a param and
 * %HMM definition @a hmminfo.  If type is not the same, adjustment will be
 * tried.
 *
 * If adjustment is performed, the input @a param will be freed and
 * the newly allocated adjusted parameter will be returned.  Otherwise, 
 * the original @a param will be returned.
 * 
 * @param hmminfo [in] HTK %HMM definition
 * @param param [i/o] input parameter, will be freed if adjustment was
 * performed in this function
 * @param vflag [in] if TRUE, output verbose messages
 * 
 * @return the original @a param if no adjustment needed, newly allocated param
 * is adjustment was needed and successfully done, NULL on failure (in case
 * parameter type does not match even by the adjustment).
 */
HTK_Param *
new_param_check_and_adjust(HTK_HMM_INFO *hmminfo, HTK_Param *param, boolean vflag)
{
  HTK_Param *selected_param;
  char pbuf[80],hbuf[80];
  
  param_code2str(pbuf, (short)(param->header.samptype & ~(F_COMPRESS | F_CHECKSUM)), FALSE);
  param_code2str(hbuf, hmminfo->opt.param_type, FALSE);  
  if (!check_param_basetype(hmminfo, param)) {
    /* error if base type not match */
    j_printerr("Error: incompatible parameter type\n");
    j_printerr("  HMM  trained  by %s(%d)\n", hbuf, hmminfo->opt.vec_size);
    j_printerr("input parameter is %s(%d)\n", pbuf, param->veclen);
    return NULL;
  }
  if (!check_param_coherence(hmminfo, param)) {
    /* try to select needed parameter vector */
    if (vflag) j_printerr("attach %s", pbuf);
    selected_param = new_select_param_kind(param, hmminfo->opt.param_type);
    if (selected_param == NULL) {
      if (vflag) j_printerr("->%s failed\n", hbuf);
      j_printerr("Error: incompatible parameter type\n");
      j_printerr("  HMM  trained  by %s(%d)\n", hbuf, hmminfo->opt.vec_size);
      j_printerr("input parameter is %s(%d)\n", pbuf, param->veclen);
      return NULL;
    }
    param_code2str(pbuf, selected_param->header.samptype, FALSE);
    if (vflag) j_printerr("->%s\n", pbuf);
    free_param(param);
    return(selected_param);
  }
  return(param);
}
