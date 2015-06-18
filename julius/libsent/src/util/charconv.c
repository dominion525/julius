/**
 * @file   charconv.c
 * @author Akinobu LEE
 * @date   Thu Feb 17 16:02:41 2005
 * 
 * <JA>
 * @brief  文字コード変換
 *
 * 実際には，環境にあわせて iconv, Win32, libjcode のどれかを呼び出す．
 *
 * </JA>
 * 
 * <EN>
 * @brief  Character set conversion using iconv library
 *
 * The actual functions are defined for iconv, win32 and libjcode, depending
 * on the OS environment.
 *
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

#ifdef CHARACTER_CONVERSION

static boolean convert_enabled = FALSE; ///< TRUE if charset converstion is enabled

/** 
 * Setup charset conversion.
 * 
 * @param fromcode [in] input charset name (only libjcode accepts NULL)
 * @param tocode [in] output charset name, or NULL when disable conversion
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
j_printf_set_charconv(char *fromcode, char *tocode)
{
  boolean enabled;
  boolean ret;

  /* call environment-specific setup function */
#ifdef HAVE_ICONV
  ret = charconv_iconv_setup(fromcode, tocode, &enabled);
#endif
#ifdef USE_LIBJCODE
  ret = charconv_libjcode_setup(fromcode, tocode, &enabled);
#endif
#ifdef USE_WIN32_MULTIBYTE
  ret = charconv_win32_setup(fromcode, tocode, &enabled);
#endif

  /* store whether conversion should be enabled or not to outer variable */
  convert_enabled = enabled;

  /* return the status */
  return(ret);
}

/** 
 * Apply charset conversion to a string.
 * 
 * @param instr [in] source string
 * @param outstr [in] destination buffer
 * @param maxoutlen [in] allocated length of outstr in byte.
 *
 * @return either of instr or outstr, that holds the result string.
 *
 */
char *
charconv(char *instr, char *outstr, int maxoutlen)
{
  char *ret;

  /* if diabled return instr itself */
  if (convert_enabled == FALSE) return(instr); /* no conversion */

  /* call environment-specific conversion function */
#ifdef HAVE_ICONV
  ret = charconv_iconv(instr, outstr, maxoutlen);
#endif
#ifdef USE_LIBJCODE
  ret = charconv_libjcode(instr, outstr, maxoutlen);
#endif
#ifdef USE_WIN32_MULTIBYTE
  ret = charconv_win32(instr, outstr, maxoutlen);
#endif

  /* return pointer to the buffer (either instr or outstr) that have the
     resulting string */
  return(ret);
}

void
j_printf_clear_charconv(void)
{
  if (convert_enabled) {
#ifdef HAVE_ICONV
    charconv_iconv_clear();
#endif
    convert_enabled = FALSE;
  }
}

#endif /* CHARACTER_CONVERSION */
