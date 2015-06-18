/**
 * @file   m_jconf.c
 * @author Akinobu Lee
 * @date   Thu May 12 14:16:18 2005
 * 
 * <JA>
 * @brief  jconf 設定ファイルを読み込んで処理する．
 * </JA>
 * 
 * <EN>
 * @brief  Read in a jconf file and process them.
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

#include <julius.h>

#if defined(_WIN32) && !defined(__CYGWIN32__)
#include <mbstring.h>
#endif

#define ISTOKEN(A) (A == ' ' || A == '\t' || A == '\n') ///< Determine token characters

/** 
 * <JA>
 * @brief  jconf 用の行読み込みルーチン
 *
 * バックスラッシュによるエスケープ処理，および Mac/Win の改行コードに
 * 対応する．空行はスキップされ，改行コードは消される．
 * 
 * @param buf [out] 読み込んだ1行分のテキストを格納するバッファ
 * @param size [in] @a buf の大きさ（バイト数）
 * @param fp [in] ファイルポインタ
 * 
 * @return @a buf を返す．EOF でこれ以上入力がなければ NULL を返す．
 * </JA>
 * <EN>
 * @brief  line reading function for jconf file.
 *
 * This function has capability of character escaping and newline codes
 * on Win/Mac.  Blank line will be skipped and newline characters will be
 * stripped.
 * 
 * @param buf [out] buffer to store the read text per line
 * @param size [in] size of @a buf in bytes
 * @param fp [in] file pointer
 * 
 * @return @a buf on success, or NULL when encountered EOF and no further input.
 * </EN>
 */
/* added by H.Banno for Windows & Mac */
static char *
fgets_jconf(char *buf, int size, FILE *fp)
{
  int c, prev_c;
  int pos;

  if (fp == NULL) return NULL;
    
  pos = 0;
  c = '\0';
  prev_c = '\0';
  while (1) {
    if (pos >= size) {
      pos--;
      break;
    }

    c = fgetc(fp);
    if (c == EOF) {
      buf[pos] = '\0';
      if (pos <= 0) {
	return NULL;
      } else {
	return buf;
      }
    } else if (c == '\n' || c == '\r') {
      if (c == '\r' && (c = fgetc(fp)) != '\n') { /* for Mac */
	ungetc(c, fp);
      }
      if (prev_c == '\\') {
	pos--;
      } else {
	break;
      }
    } else {
      buf[pos] = c;
      pos++;

#if defined(_WIN32) && !defined(__CYGWIN32__)
      if (c == '\\' && (_ismbblead(prev_c) && _ismbbtrail(c))) {
      c = '\0';
      }
#endif
    }
  }
  buf[pos] = '\0';

  return buf;
}

/** 
 * <JA>
 * @brief  ファイルのパス名からディレクトリ名を抜き出す．
 *
 * 最後の '/' は残される．
 * 
 * @param path [i/o] ファイルのパス名（関数内で変更される）
 * </JA>
 * <EN>
 * @brief  Get directory name from a path name of a file.
 *
 * The trailing slash will be left, and the given buffer will be modified.
 * 
 * @param path [i/o] file path name, will be modified to directory name
 * </EN>
 */
void
get_dirname(char *path)
{
  char *p;
  /* /path/file -> /path/ */
  /* path/file  -> path/  */
  /* /file      -> / */
  /* file       ->  */
  /* ../file    -> ../ */
  p = path + strlen(path) - 1;
  while (*p != '/'
#if defined(_WIN32) && !defined(__CYGWIN32__)
	 && *p != '\\'
#endif
	 && p != path) p--;
  if (p == path && *p != '/') *p = '\0';
  else *(p+1) = '\0';
}

/* read-in and parse jconf file and process those using m_options */
/** 
 * <JA>
 * jconf 設定ファイルを読み込んで解析し，対応するオプションを設定する．
 * 
 * @param conffile [in] jconf ファイルのパス名
 * </JA>
 * <EN>
 * Read and parse a jconf file, and set the specified option values.
 * 
 * @param conffile [in] jconf file path name
 * </EN>
 */
void
config_file_parse(char *conffile)
{
  int c_argc;
  char **c_argv;
  FILE *fp;
  int maxnum, step;
  static const int len = 512;
  char buf[len], cpy[len];
  char *p, *dst, *dst_from;
  char *cdir;

  j_printerr("include config: %s\n", conffile);
  
  /* set the content of jconf file into argument list c_argv[1..c_argc-1] */
  /* c_argv[0] will be the original conffile name */
  /* inside jconf file, quoting by ", ' and escape by '\' is supported */
  if ((fp = fopen(conffile, "r")) == NULL) {
    j_error("%s: failed to open jconf file: %s\n",EXECNAME, conffile);
  }
  step = 20;
  maxnum = step;
  c_argv = (char **)mymalloc(sizeof(char *) * maxnum);
  c_argv[0] = strcpy((char *)mymalloc(strlen(conffile)+1), conffile);
  c_argc = 1;
  while (fgets_jconf(buf, len, fp) != NULL) {
    if (buf[0] == '\0') continue;
    p = buf; dst = cpy;
    while (1) {
      while (*p != '\0' && ISTOKEN(*p)) p++;
      if (*p == '\0') break;
      
      dst_from = dst;
      
      while (*p != '\0' && (!ISTOKEN(*p))) {
	if (0 &&/* '\' is removed by fgets_jconf */ *p == '\\') {     /* escape by '\' */
	  if (*(++p) == '\0') break;
	  *(dst++) = *(p++);
	} else {
	  if (*p == '"') { /* quote by "" */
	    p++;
	    while (*p != '\0' && *p != '"') *(dst++) = *(p++);
	    if (*p == '\0') break;
	    p++;
	  } else if (*p == '\'') { /* quote by '' */
	    p++;
	    while (*p != '\0' && *p != '\'') *(dst++) = *(p++);
	    if (*p == '\0') break;
	    p++;
	  } else if (*p == '#') { /* comment out by '#' */
	    *p = '\0';
	    break;
	  } else {		/* other */
	    *(dst++) = *(p++);
	  }
	}
      }
      if (dst != dst_from) {
	*dst = '\0'; dst++;
	if (c_argc >= maxnum) {
	  maxnum += step;
	  c_argv = (char **)myrealloc(c_argv, sizeof(char *) * maxnum);
	}
	c_argv[c_argc++] = strcpy((char*)mymalloc(strlen(dst_from)+1), dst_from);
      }
    }
  }
  if (fclose(fp) == -1) {
    j_error("%s: jconf file cannot close\n", EXECNAME);
  }

  if (debug2_flag) {		/* for debug */
    int i;
    j_printf("\t");
    for (i=1;i<c_argc;i++) j_printf(" %s",c_argv[i]);
    j_printf("\n");
  }

  /* now that options are in c_argv[][], call opt_parse() to process them */
  /* relative paths in jconf file are relative to the jconf file (not current) */
  cdir = strcpy((char *)mymalloc(strlen(conffile)+1), conffile);
  get_dirname(cdir);
  opt_parse(c_argc, c_argv, (cdir[0] == '\0') ? NULL : cdir);
  free(cdir);

  /* free arguments */
  while (c_argc-- > 0) {
    free(c_argv[c_argc]);
  }
  free(c_argv);
}

