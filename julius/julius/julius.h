/**
 * @file   julius.h
 * @author Akinobu LEE
 * @date   Thu Mar 17 21:08:21 2005
 * 
 * <JA>
 * @brief  Julius/Julian 用のトップのヘッダファイル
 * </JA>
 * 
 * <EN>
 * @brief  Top common header for Julius/Julian
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

#ifndef __JULIUS_COMMON_HEADER__
#define __JULIUS_COMMON_HEADER__

/* read configurable definitions */
#if defined(_WIN32) && !defined(__CYGWIN32__) && !defined(__MINGW32__)
/*
 *
 *  You should define -DBUILD_JULIAN to compile Julian in WIN32(VC++)
 *
 */
# ifdef BUILD_JULIAN
# include <config-win-julian.h>
# else
# include <config-win-julius.h>
# endif
#else
#include <config.h>
#endif
#include <sent/config.h>
/* read built-in definitions */
#include <define.h>

/* read libsent includes */
#include <sent/stddefs.h>
#include <sent/tcpip.h>
#include <sent/speech.h>
#include <sent/mfcc.h>
#include <sent/htk_param.h>
#include <sent/hmm.h>
#include <sent/gprune.h>
#include <sent/vocabulary.h>
#ifdef USE_NGRAM
#include <sent/ngram2.h>
#else /* USE_DFA */
#include <sent/dfa.h>
#endif

/* read Julius/Julian includes */
#ifdef USE_DFA
#include <multi-gram.h>
#endif
#include <wchmm.h>
#include <beam.h>
#include <search.h>
#include <module.h>
#include <misc.h>
#include <global.h>
#include <extern.h>

#endif /* __JULIUS_COMMON_HEADER__ */
