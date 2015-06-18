/**
 * @file   readfile.c
 * @author Akinobu LEE
 * @date   Thu Feb 17 16:41:58 2005
 * 
 * <JA>
 * @brief  様々な入力からテキストを行単位で読み込む関数群
 *
 * 入力ストリームやファイルデスクプリタなど，様々なソースから
 * テキスト入力を行単位で読み込むための関数群です．
 * 読み込み時において，空行は無視されます．また行末の改行は削除されます．
 * </JA>
 * 
 * <EN>
 * @brief  Read strings per line from various input source
 *
 * This file provides functions to read text inputs from various
 * input source such as file pointer, file descpritor, socket descriptor.
 * The input will be read per line, and the newline characters are removed.
 * Also, blank lines will be ignored.
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
#include <sent/tcpip.h>

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif


/** 
 * Read one line from file that has been opened by fopen_readfile().
 * Blank line will be skipped.
 * 
 * @param buf [out] data buffer
 * @param maxlen [in] maximum length of above
 * @param fp [in] file pointer or gzFile pointer
 * 
 * @return the buffer @a buf, or NULL on EOF or error.
 */
char *
getl(char *buf, int maxlen, FILE *fp)
{
  int newline;

  while(
#ifdef HAVE_ZLIB
	gzgets((gzFile)fp, buf, maxlen) != Z_NULL
#else
	fgets(buf, maxlen, fp) != NULL
#endif
	) {
    newline = strlen(buf)-1;    /* chop newline */
    if (buf[newline] == '\n') {
      buf[newline] = '\0';
      newline--;
    }
    if (newline >= 0 && buf[newline] == '\r') buf[newline] = '\0';
    if (buf[0] == '\0') continue; /* if blank line, read next */
    return buf;
  }
  return NULL;
}

/** 
 * Read one line from file pointer.
 * Blank line will be skipped.
 * 
 * @param buf [out] data buffer
 * @param maxlen [in] maximum length of above
 * @param fp [in] file pointer
 * 
 * @return the buffer @a buf, or NULL on EOF or error.
 */
char *
getl_fp(char *buf, int maxlen, FILE *fp)
{
  int newline;

  while(fgets(buf, maxlen, fp) != NULL) {
    newline = strlen(buf)-1;    /* chop newline */
    if (buf[newline] == '\n') {
      buf[newline] = '\0';
      newline--;
    }
    if (newline >= 0 && buf[newline] == '\r') buf[newline] = '\0';
    if (buf[0] == '\0') continue; /* if blank line, read next */
    return buf;
  }
  return NULL;
}

/** 
 * Read one line from a file descriptor.
 * Blank line will be skipped.
 * 
 * @param buf [out] data buffer
 * @param maxlen [in] maximum length of above
 * @param fd [in] file descriptor
 * 
 * @return the buffer @a buf, or NULL on EOF or error.
 */
char *
getl_fd(char *buf, int maxlen, int fd)
{
  int cnt;
  char *p;
  p = buf;
  while(1) {
    cnt = read(fd, p, 1);
    if (cnt <= 0) return NULL;		/* eof or error */
    if (*p == '\n') {
      *p = '\0';
      if (p - 1 >= buf && *(p-1) == '\r') *(p-1) = '\0';
      if (buf[0] == '\0') {
	p = buf;
	continue;
      } else {
	break;
      }
    } else {
      if (++p >= buf + maxlen) {
	j_error("Error: getl_fd: line too long (> %d)\n", maxlen);
      }
    }
  }
  return buf;
}

/** 
 * Read one line from a socket descriptor.
 * Blank line will be skipped.
 * 
 * @param buf [out] data buffer
 * @param maxlen [in] maximum length of above
 * @param sd [in] socket descpritor
 * 
 * @return the buffer @a buf, or NULL on EOF or error.
 */
char *
getl_sd(char *buf, int maxlen, int sd)
{
  int cnt;
  char *p;
  p = buf;
  while(1) {
    cnt = recv(sd, p, 1, 0);
    if (cnt <= 0) return NULL;                /* eof or error */
    if (*p == '\n') {
      *p = '\0';
      if (p - 1 >= buf && *(p-1) == '\r') *(p-1) = '\0';
      if (buf[0] == '\0') {
      p = buf;
      continue;
      } else {
      break;
      }
    } else {
      if (++p >= buf + maxlen) {
      j_error("Error: getl_sd: line too long (> %d)\n", maxlen);
      }
    }
  }
  return buf;
}

/** 
 * Return first token of a buffer, delimited by DELM.
 * Program will terminate if any token does not found.
 * 
 * @param buf [i/o] string buffer
 * 
 * @return pointer to the extracted token.
 */
char *
first_token(char *buf)
{
  char *p;
  if ((p=strtok(buf, DELM)) == NULL) {
    j_error("data format error: corrupted data?\n");
  }
  return p;
}
/** 
 * Return next token of a buffer, delimited by DELM.
 * Should be called after first_token().
 * Program will terminate if any token does not found.
 * 
 * @return pointer to the extracted token.
 */
char *
next_token() {
  char *p;
  if ((p=strtok(NULL,DELM)) == NULL) {
    j_error("data format error: corrupted data?\n");
  }
  return p;
}
/** 
 * Return next token of a buffer, delimited by DELM.
 * Should be called after first_token().
 * 
 * @return pointer to the extracted token, or NULL if not found.
 */
char *
next_token_if_any() {
  char *p;

  p=strtok(NULL,DELM);

  return p;
}
/** 
 * Return the rest tokens till newline.
 * Program will terminate if any token does not found.
 * 
 * @return pointer to the extracted token.
 */
char *
rest_token() {
  char *p;
  if ((p=strtok(NULL, "\n")) == NULL) {
    j_error("data format error: corrupted data?\n");
  }
  return p;
}    
