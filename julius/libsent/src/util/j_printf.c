/**
 * @file   j_printf.c
 * @author Akinobu LEE
 * @date   Thu Feb 17 16:02:41 2005
 * 
 * <JA>
 * @brief  メッセージテキスト出力とエラー終了
 *
 * メッセージ出力用の汎用関数の定義です．
 * 認識結果の端末出力やネットワークへの出力といった，
 * Julius の主要な出力はここの関数を使って行われます．
 * また，出力文字コードの変換もここで行います．
 * </JA>
 * 
 * <EN>
 * @brief  Message text output and error exit functions
 *
 * These are generic functions for text message output.
 * Most of text messages such as recognition results, status, informations
 * in Julius/Julian will be output to TTY or via network using these functions.
 * The character set conversion will also be performed here.
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
#include <sent/tcpip.h>
#include <stdarg.h>

/* pointer to functions of general text output */
static void (*j_print_func)(char *) = NULL;///< Alternative output function replacing standard output
static void (*j_printerr_func)(char *) = NULL; ///< Alternative output function replacing stndard error
static void (*j_flush_func)(void) = NULL;///< Alternative function to flush the output

#define MAX_PRINTF_LEN 4096	///< Maximum string length at one printf call
static char inbuf[MAX_PRINTF_LEN]; ///< Local work area for text output
#ifdef CHARACTER_CONVERSION
static char outbuf[MAX_PRINTF_LEN]; ///< Local work area for charset conversion
#endif

/** 
 * Set the alternative output functions instead of normal tty output.
 * 
 * @param print [in] function to output string in stdout
 * @param printerr [in] function to output string in stderr
 * @param flush [in] function to flush the stdout
 */
void
set_print_func(void (*print)(char *), void (*printerr)(char *), void (*flush)(void))
{
  j_print_func = print;
  j_printerr_func = printerr;
  j_flush_func = flush;
  return;
}

/** 
 * @brief  Output a text message to standard out.
 *
 * The output device can be changed by set_print_func().
 * 
 * @param fmt [in] format string, like printf.
 * @param ... [in] variable length argument like printf.
 * 
 * @return the same as printf, i.e. number of characters printed.
 */
int
j_printf(char *fmt, ...)
{
  va_list ap;
  int ret;
  char *buf;
  
  va_start(ap,fmt);
  ret = vsnprintf(inbuf, MAX_PRINTF_LEN, fmt, ap);
  va_end(ap);
#ifdef CHARACTER_CONVERSION
  buf = charconv(inbuf, outbuf, MAX_PRINTF_LEN);
#else
  buf = inbuf;
#endif
  if (j_print_func != NULL) {
    (*j_print_func)(buf);
  } else {
    ret = printf("%s", buf);
  }

  return(ret);
}

/** 
 * @brief  Output a text message to standard error
 *
 * The output device can be changed by set_print_func().
 * 
 * @param fmt [in] format string, like printf.
 * @param ... [in] variable length argument like printf.
 * 
 * @return the same as printf, i.e. number of characters printed.
 */
int
j_printerr(char *fmt, ...)
{
  va_list ap;
  int ret;
  char *buf;
  
  va_start(ap,fmt);
  ret = vsnprintf(inbuf, MAX_PRINTF_LEN, fmt, ap);
  va_end(ap);
#ifdef CHARACTER_CONVERSION
  buf = charconv(inbuf, outbuf, MAX_PRINTF_LEN);
#else
  buf = inbuf;
#endif
  if (j_printerr_func != NULL) {
    (*j_printerr_func)(buf);
  } else {
    ret = fprintf(stderr, "%s", buf);
  }

  return(ret);
}

/** 
 * @brief  Flush text message
 *
 * The output device can be changed by set_print_func().
 * 
 * @return the same as fflush, i.e. 0 on success, other if failed.
 */
int
j_flushprint()
{
  if (j_flush_func != NULL) {
    (*j_flush_func)();
    return 0;
  } else {
    return(fflush(stdout));
  }
  return(fflush(stdout));
}

/** 
 * Generic function to send a formatted message to client module.
 *
 * @param sd [in] socket descriptor
 * @param fmt [in] format string, like printf.
 * @param ... [in] variable length argument like printf.
 * 
 * @return the same as printf, i.e. number of characters printed.
 */
int
module_send(int sd, char *fmt, ...)
{
  va_list ap;
  int ret;
  char *buf;
  
  va_start(ap,fmt);
  ret = vsnprintf(inbuf, MAX_PRINTF_LEN, fmt, ap);
  va_end(ap);
  if (ret > 0) {		/* success */
    
#ifdef CHARACTER_CONVERSION
    buf = charconv(inbuf, outbuf, MAX_PRINTF_LEN);
#else
    buf = inbuf;
#endif
    if (
#ifdef WINSOCK
	send(sd, buf, strlen(buf), 0)
#else
	write(sd, buf, strlen(buf))
#endif
	< 0) {
      perror("Error: module_send:");
    }
  }
  return(ret);
}

static int (*error_func)() = NULL; ///< Alternative function to be executed for application error exit

/** 
 * Register a function that should be executed just before application
 * error exit.
 * 
 * @param f [in] pointer to the function
 */
void
j_error_register_exitfunc(int (*f)())
{
  error_func = f;
}

/** 
 * Output error message and exit the program.
 * 
 * @param fmt [in] format string, like printf.
 * @param ... [in] variable length argument like printf.
 */
void
j_error(char *fmt, ...)
{
  va_list ap;
  int ret;
  va_start(ap,fmt);
  ret = vfprintf(stderr, fmt, ap);
  va_end(ap);
  if (error_func != NULL) ret = (*error_func)(); /* call external error function */
  else ret = 1;

  /* clean up socket if already opened */
  cleanup_socket();

  exit(ret);
}

/* normal exit end  */
static int (*exit_func)() = NULL; ///< Alternative function to be executed at application normal exit

/** 
 * Register a function that should be executed just before application ends.
 * 
 * @param f [in] pointer to the function
 */
void
j_exit_register_exitfunc(int (*f)())
{
  exit_func = f;
}

/** 
 * Exit the program.
 * 
 */
void
j_exit()
{
  int ret;

  if (exit_func != NULL) ret = (*exit_func)(); /* call external error function */
  else ret = 0;

  /* clean up socket if already opened */
  cleanup_socket();

  exit(ret);
}
