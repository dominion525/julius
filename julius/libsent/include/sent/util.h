/**
 * @file   util.h
 * @author Akinobu LEE
 * @date   Sat Feb 12 12:30:40 2005
 *
 * <JA>
 * @brief  ���ѥ桼�ƥ���ƥ��ؿ����˴ؤ������
 *
 * ���Υե�����ˤϡ��ƥ������ɤ߹��ߤ䰵�̥ե�������
 * �������դ����Х��ȥ����������ѤΥ�å��������ϴؿ��ʤɤ�
 * ���ѤΥ桼�ƥ���ƥ��ؿ��˴ؤ���������ޤޤ�Ƥ��ޤ���
 *
 * </JA>
 * <EN>
 * @brief  Definitions for common utility functions
 *
 * This file contains definitions for common utility functions:
 * text reading and parsing, compressed file input, memory allocation,
 * byte order changing, common printf function (j_printf etc.) and so on.
 *
 * </EN>
 *
 * $Revision: 1.9 $
 * 
 */
/*
 * Copyright (c) 1991-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#ifndef __SENT_UTIL_H__
#define __SENT_UTIL_H__

/*
 *  
 */
/**
 * @brief Memory block size in bytes for mybmalloc()
 *
 * mybmalloc() allocate memory per big block to reduce memory
 * management overhead.  This value sets the block size to be allocated.
 * Smaller value may leads to finer granularity, but overhead may increase.
 * Larger value may result in reduction of overhead, but too much memory
 * can be allocated for a small memory requirement.
 * 
 */
#define MYBMALLOC_BLOCK_SIZE 10000

/// Information of allocated memory block for mybmalloc()
typedef struct _bmalloc_base {
  void *base;			///< Pointer to the actually allocated memory block
  char *now;		    ///< Start Pointer of Currently assigned area
  char *end;		    ///< End Pointer of Currently assigned area
  struct _bmalloc_base *next;	///< Link to next data, NULL if no more
} BMALLOC_BASE;

/* readfile.c */
char *getl(char *, int, FILE *);
char *getl_fp(char *, int, FILE *);
char *getl_fd(char *, int, int);
char *getl_sd(char *, int, int);
char *first_token(char *);
char *next_token(void);
char *next_token_if_any(void);
char *rest_token(void);

/* gzfile.c */
FILE *fopen_readfile(char *);
int fclose_readfile(FILE *);
FILE *fopen_writefile(char *);
int fclose_writefile(FILE *);
size_t myfread(void *ptr, size_t size, size_t n, FILE *fp);
size_t myfwrite(void *ptr, size_t size, size_t n, FILE *fp);
int myfgetc(FILE *fp);
int myfeof(FILE *fp);
int myfrewind(FILE *fp);

/* mybmalloc.c */
void mybmalloc_set_param();
void *mybmalloc(int);
void mybmalloc_free(void);
char *mybstrdup(char *);
void *mybmalloc2(int size, BMALLOC_BASE **list);
char *mybstrdup2(char *, BMALLOC_BASE **list);
void mybfree2(BMALLOC_BASE **list);

/* mymalloc.c */
void *mymalloc(int);
void *myrealloc(void *, int);
void *mycalloc(int, int);

/* endian.c */
void swap_sample_bytes(SP16 *buf, int len);
void swap_bytes(char *buf, size_t unitbyte, int unitnum);

/* j_printf.c */
#include <stdarg.h>
#ifdef CHARACTER_CONVERSION
boolean j_printf_set_charconv(char *fromcode, char *tocode);
char *charconv(char *instr, char *outstr, int maxoutlen);
void j_printf_clear_charconv(void);
#ifdef HAVE_ICONV
boolean charconv_iconv_setup(char *fromcode, char *tocode, boolean *enabled);
char *charconv_iconv(char *instr, char *outstr, int maxoutlen);
void charconv_iconv_clear(void);
#endif
#ifdef USE_WIN32_MULTIBYTE
boolean charconv_win32_setup(char *fromcode, char *tocode, boolean *enabled);
char *charconv_win32(char *instr, char *outstr, int maxoutlen);
#endif
#ifdef USE_LIBJCODE
boolean charconv_libjcode_setup(char *fromcode, char *tocode, boolean *enabled);
char *charconv_libjcode(char *instr, char *outstr, int maxoutlen);
#endif
#endif /* CHARACTER_CONVERSION */
void set_print_func(void (*print)(char *), void (*printerr)(char *), void (*flush)(void));
int j_printf(char *format, ...);
int j_printerr(char *format, ...);
int j_flushprint();
int module_send(int sd, char *format, ...);
void j_error(char *format, ...);
void j_error_register_exitfunc(int (*error_func)());
void j_exit();
void j_exit_register_exitfunc(int (*exit_func)());

/* mystrtok.c */
char  *mystrtok_quotation(char *str, char *delim, int left_paren, int right_paren, int mode); /* mode:0=normal, 1=just move to next token */
char *mystrtok_quote(char *str, char *delim);
char *mystrtok(char *str, char *delim);
char *mystrtok_movetonext(char *str, char *delim);

/* confout.c */
void confout(FILE *strm);
void confout_version(FILE *strm);
void confout_audio(FILE *strm);
void confout_lm(FILE *strm);
void confout_am(FILE *strm);
void confout_lib(FILE *strm);
void confout_process(FILE *strm);

#endif /* __SENT_UTIL_H__ */
