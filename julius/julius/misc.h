/**
 * @file   misc.h
 * @author Akinobu Lee
 * @date   Mon May 30 15:58:16 2005
 * 
 * <JA>
 * @brief  その他の雑多な定義
 * </JA>
 * 
 * <EN>
 * @brief  Some miscellaneous definitions
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

#ifndef __JULIUS_MISC__
#define __JULIUS_MISC__

/// Defines for selecting message output destination
enum {
  SP_RESULT_TTY,		///< To tty
  SP_RESULT_MSOCK		///< Socket output in XML-like format for module mode
};

/// Define default for module forking
#if defined(_WIN32) && !defined(__CYGWIN32__)
#define DEFAULT_FORK_ON_MODULE FALSE
#else
#define DEFAULT_FORK_ON_MODULE TRUE
#endif

#endif /* __JULIUS_MISC__ */
