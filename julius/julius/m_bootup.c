/**
 * @file   m_bootup.c
 * @author Akinobu LEE
 * @date   Fri Mar 18 16:21:28 2005
 * 
 * <JA>
 * @brief  システム起動最初期の設定処理および終了時実行関数の定義
 * </JA>
 * 
 * <EN>
 * @brief  Functions to be called on system bootup and exit.
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

/** 
 * <JA>
 * エラー終了時の処理関数．j_error() 実行時に呼ばれるよう system_bootup() 内
 * で処理される．
 * 
 * </JA>
 * <EN>
 * Procedure when called on error exit.  This function will be executed
 * when j_error() is called for error exit.  This function will be registered
 * in system_bootup().
 * 
 * </EN>
 */
static int
m_errexit()
{
  if (module_mode) {
    /* disconnect control module */
    if (module_is_connected()) {
      module_send(module_sd, "<SYSINFO PROCESS=\"ERREXIT\"/>\n.\n");
      module_disconnect();
    }
  } else {
    j_printerr("Terminated\n");
  }

#ifdef CHARACTER_CONVERSION
  /* release character conversion instance */
  j_printf_clear_charconv();
#endif /* CHARACTER_CONVERSION */

  /* release global variables allocated when parsing options */
  opt_release();

  return(1);			/* program exit with status 1 */
}

/** 
 * <JA>
 * 通常終了時の処理関数．j_exit() 実行時に呼ばれるよう system_bootup() 内
 * で処理される．
 * 
 * </JA>
 * <EN>
 * Procedure when called on normal exit.  This function will be executed
 * when j_exit() is called.  This function will be registered
 * in system_bootup().
 * 
 * </EN>
 */
static int
m_exit()
{
  if (module_mode) {
    /* disconnect control module */
    if (module_is_connected()) {
      module_send(module_sd, "<SYSINFO PROCESS=\"EXIT\"/>\n.\n");
      module_disconnect();
    }
  }

#ifdef CHARACTER_CONVERSION
  /* release character conversion instance */
  j_printf_clear_charconv();
#endif /* CHARACTER_CONVERSION */

  /* release global variables allocated when parsing options */
  opt_release();

  return(0);			/* program exit with status 0 */
}

/* system bootup */
/** 
 * <JA>
 * システム起動時に最初に呼ばれる初期化処理．メモリ割り付け関数の初期化
 * やエラー処理関数の登録などを行なう．
 * 
 * </JA>
 * <EN>
 * Function to be called at the first stage on system bootup.
 * It will setup memory allocation functions and register error handling
 * functions.
 * 
 * </EN>
 */
void
system_bootup()
{
  /* set mybmalloc (block memory allocation) parameters first (optional) */
  mybmalloc_set_param();

  /* set error exit function */
  j_error_register_exitfunc(m_errexit);
  j_exit_register_exitfunc(m_exit);

  /* set default param according to the compile flag */
  if (strmatch(SETUP, "fast")) {
    nbest = 1;
    enveloped_bestfirst_width = 30;
  } else {
    nbest = 10;
    enveloped_bestfirst_width = 100;
  }

  /* clear MFCC paramters */
  /* parameter setting priority:
     user-specified > specified HTK Config > model-embedded > default  */
  /* when HTK Config is read, default values for HTK will be chosen */
  undef_para(&para);
  undef_para(&para_hmm);
  undef_para(&para_default);
  undef_para(&para_htk);
  make_default_para(&para_default);
  make_default_para_htk(&para_htk);

}
