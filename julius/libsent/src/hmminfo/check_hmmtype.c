/**
 * @file   check_hmmtype.c
 * @author Akinobu LEE
 * @date   Tue Feb 15 19:11:50 2005
 * 
 * <JA>
 * @brief  %HMM と特徴ファイルのパラメータ型野整合性をチェックする
 * </JA>
 * 
 * <EN>
 * @brief  Check the parameter types between %HMM and input
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
#include <sent/htk_defs.h>
#include <sent/htk_hmm.h>
#include <sent/htk_param.h>

/** 
 * Check if the required parameter type in this %HMM can be handled by Julius.
 * 
 * @param hmm [in] HMM definition data
 * 
 * @return TRUE if Julius supports it, otherwise return FALSE.
 */
boolean
check_hmm_options(HTK_HMM_INFO *hmm)
{
  boolean ret_flag = TRUE;
  
  if (hmm->opt.stream_info.num > 1) {
    j_printerr("ERROR: Input stream must be single\n");
    ret_flag = FALSE;
  }
  if (hmm->opt.dur_type != D_NULL) {
    j_printerr("ERROR: Duration types other than NULLD are not supported.\n");
    ret_flag = FALSE;
  }
  if (hmm->opt.cov_type != C_DIAG_C) {
    j_printerr("ERROR: Covariance matrix type must be DIAGC, others not supported.\n");
    ret_flag = FALSE;
  }

  return(ret_flag);
}

/** 
 * Check if an input parameter type exactly matches that of %HMM.
 * 
 * @param hmm [in] HMM definition data
 * @param pinfo [in] input parameter
 * 
 * @return TRUE if matches, FALSE if differs.
 */
boolean
check_param_coherence(HTK_HMM_INFO *hmm, HTK_Param *pinfo)
{
  boolean ret_flag;

  ret_flag = TRUE;

  /* HMM type check */
  if (hmm->opt.param_type
      != (pinfo->header.samptype & ~(F_COMPRESS | F_CHECKSUM))) {
/* 
 *     j_printerr("ERROR: incompatible parameter type\n");
 *     j_printf("HMM trained by %s\n", param_code2str(buf, hmm->opt.param_type, FALSE));
 *     j_printf("input parameter is %s\n", param_code2str(buf, pinfo->header.samptype, FALSE));
 */
    ret_flag = FALSE;
  }

  /* vector length check */
  if (hmm->opt.vec_size != pinfo->veclen) {
/* 
 *     j_printerr("ERROR: vector length differ.\n");
 *     j_printerr("HMM=%d, param=%d\n", hmm->opt.vec_size, pinfo->veclen);
 */
    ret_flag = FALSE;
  }
  
  return(ret_flag);
}

/** 
 * Check if the base type of input parameter matches that of %HMM.
 * 
 * @param hmm [in] HMM definition data
 * @param pinfo [in] input parameter
 * 
 * @return TRUE if matches, FALSE if differs.
 */
boolean
check_param_basetype(HTK_HMM_INFO *hmm, HTK_Param *pinfo)
{
  if ((hmm->opt.param_type & F_BASEMASK)
      != (pinfo->header.samptype & F_BASEMASK)) {
    return FALSE;
  } else {
    return TRUE;
  }
} 
