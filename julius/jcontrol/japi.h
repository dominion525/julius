/**
 * @file   japi.h
 * @author Akinobu LEE
 * @date   Thu Mar 24 07:12:32 2005
 * 
 * <JA>
 * @brief  jcontrol 共通ヘッダファイル
 * </JA>
 * 
 * <EN>
 * @brief  Common header file for jcontrol
 * </EN>
 * 
 * $Revision: 1.3 $
 * 
 */
/*
 * Copyright (c) 2002-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 2002-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#ifndef __JAPI_H__
#define __JAPI_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#if !defined(_WIN32) || defined(__CYGWIN32__)
/* unixen/cygwin */
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#else
/* win32 */
#include <io.h>
#include <winsock.h>
#define WINSOCK
/* use winsock functions */
#define access _access        
#define close _close
#define open _open
#define read _read
#define write _write
#define vsnprintf _vsnprintf
#define snprintf _snprintf
#endif

/* file test operators */
#ifndef R_OK
#define R_OK 4
#endif
#ifndef W_OK
#define W_OK 2
#endif
#ifndef X_OK
# if defined(_WIN32) && !defined(__CYGWIN32__)
# define X_OK 0
# else
# define X_OK 1
# endif
#endif
#ifndef F_OK
#define F_OK 0
#endif


#define DEFAULT_PORT 10500	/* default server port number */

#define MAXLINELEN 4096

/* server.c */
int do_connect(char *hostname, int portnum);
void do_disconnect(int sd);
void do_sendf(int sd, char *fmt, ...);
void do_send(int sd, char *buf);
char *do_receive(int sd, char *buf, int maxlen);

/* japi_misc.c */
void japi_die(int);
void japi_get_version(int);
void japi_get_status(int);
void japi_pause_recog(int);
void japi_terminate_recog(int);
void japi_resume_recog(int);
void japi_set_input_handler_on_change(int, char *);

/* japi_grammar.c */
void japi_change_grammar(int sd, char *prefixpath);
void japi_add_grammar(int sd, char *prefixpath);
void japi_delete_grammar(int sd, char *idlist);
void japi_activate_grammar(int sd, char *idlist);
void japi_deactivate_grammar(int sd, char *idlist);
void japi_sync_grammar(int sd);

#endif /* __JAPI_H__ */
