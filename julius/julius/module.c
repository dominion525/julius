/**
 * @file   module.c
 * @author Akinobu Lee
 * @date   Mon May 30 15:59:54 2005
 * 
 * <JA>
 * @brief  �⥸�塼��⡼�ɡʥ����С��⡼�ɡ˻��Υᥤ��롼��
 * </JA>
 * 
 * <EN>
 * @brief  Main loop for module mode (server mode)
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
#include <stdarg.h>

#define MAXBUFLEN 4096 ///< Maximum line length of a message sent from a client
static int listen_sd = -1;	///< Socket to listen to a client
static char mbuf[MAXBUFLEN];	///< Work buffer for message output

/**
 * Status flag indicating whether the recognition is alive or not.  If
 * TRUE, the process is currently activated, either monitoring an
 * audio input or recognizing the current input.  If FALSE, the recognition
 * is now disabled until some activation command has been arrived from
 * client.  While disabled, all the inputs are ignored.
 *
 * If set to FALSE in the program, Julius/Julian will stop after
 * the current recognition ends, and enter the disabled status.
 * 
 * 
 */
static boolean process_active = TRUE;

/**
 * If set to TRUE, Julius/Julian stops recognition immediately, terminating
 * the currenct recognition process, and enter into disabled status.
 * 
 */
static boolean process_want_terminate = FALSE;

/**
 * If set to TRUE, Julius/Julian stops recognition softly.  If it is performing
 * recognition of the 1st pass, it immediately segments the current input,
 * process the 2nd pass, and output the result.  Then it enters the disabled
 * status.
 * 
 */
static boolean process_want_reload = FALSE;

#ifdef USE_DFA
/// Switch to specify the grammar changing timing policy.
enum{SM_TERMINATE, SM_PAUSE, SM_WAIT};
/// Specify when to refresh the global lexicon while recognition.
static short gram_switch_input_method = SM_PAUSE;
#endif

/** 
 * <JA>
 * @brief �⥸�塼��⡼�ɻ��Υ��ץꥱ�������ᥤ��롼�ס�
 *
 * ���饤����ȥץ����������³�׵���Ԥ����׵᤬�褿�饽���åȤ򳫤���
 * ǧ���Υᥤ��롼�פ�Ƥ֡���³���������åȥǥ����ץ꥿���Ӱ��ѿ�
 * module_sd �˵�Ͽ����롥�ʤ� Win32 �Ǥϥե��������ʤ���
 * 
 * </JA>
 * <EN>
 * @brief  Application main loop for module mode.
 *
 * This function wait for a connection request from a client process, and 
 * when connected, it calls the recognition main loop for that.  The
 * established socket descriptor will be stored in the global variable
 * "module_sd".   In Win32, this process will not fork by the connection
 * acceptance.
 * 
 * </EN>
 */
void
main_module_loop()
{
#if !defined(_WIN32) || defined(__CYGWIN32__)
  pid_t cid;
#endif
  
  /* prepare socket to listen */
  if ((listen_sd = ready_as_server(module_port)) < 0) {
    j_error("Error: failed to bind socket\n");
  }

  j_printf  ("///////////////////////////////\n");
  j_printf  ("///  Module mode ready\n");
  j_printf  ("///  waiting client at %5d\n", module_port);
  j_printf  ("///////////////////////////////\n");
  j_printf  ("///  ");


  if (fork_on_module_connection) {

    /* server loop */
    for(;;) {
      /* accept connection from a client */
      if ((module_sd = accept_from(listen_sd)) < 0) {
	j_error("Error: failed to accept connection\n");
      }
      /* fork this process */
      if ((cid = fork()) != 0) {
	/* parent process */
	j_printf("///  forked [%d]\n", cid);
	close_socket(module_sd); /* parent not need socket to the client */
	continue;
      }
      /* child not need listen socket */
      close_socket(listen_sd);
      /* call main recognition loop here */
      main_recognition_loop();
    }

  } else {
    
    /* no fork, just wait for one connection and proceed */
    if ((module_sd = accept_from(listen_sd)) < 0) {
      j_error("Error: failed to accept connection\n");
    }
    /* call main recognition loop here */
    main_recognition_loop();
  }
}

/** 
 * <JA>
 * ������³��Υ��饤����ȤȤ���³�����Ǥ��롥
 * 
 * @return ������ TRUE, ���Ի� FALSE ���֤���
 * </JA>
 * <EN>
 * Disconnect the current client.
 * 
 * @return TRUE on success, or FALSE on failure.
 * </EN>
 */
boolean
module_disconnect()
{
  if (close_socket(module_sd) < 0) return FALSE;
  module_sd = -1;
  return TRUE;
}

/** 
 * <JA>
 * ���ߥ��饤����Ȥ���³�椫�ɤ������֤�
 * 
 * @return ��³��ξ�� TRUE, ̤��³�ξ�� FALSE���֤���
 * </JA>
 * <EN>
 * Return whether a client process has been connected.
 * 
 * @return TRUE if currently connected, or FALSE if not.
 * </EN>
 */
boolean
module_is_connected()
{
  if (module_sd < 0) return FALSE;
  return TRUE;
}


/*
  MSOCK_COMMAND_DIE ,	 exit program 
  MSOCK_COMMAND_PAUSE ,	 stop recog. keeping the current input 
  MSOCK_COMMAND_TERMINATE , stop recog. dropping the current input 
  MSOCK_COMMAND_RESUME};	 resume the stopped process 
*/

#ifdef USE_DFA
/** 
 * <JA>
 * @brief  ��³��Υ⥸�塼�뤫�������Ƥ���ʸˡ���ɤ߹��ࡥ
 *
 * ʸˡ�� ��ʸ�ե����� (.dfa) �ȼ���ե����� (.dict) �����Ƥ�����ɥޡ�����
 * "DFAEND", "DICEND" �ȤȤ�˥⥸�塼�륯�饤����Ȥ��������Ƥ��롥
 * ���δؿ��ǤϤ����������Ǽ���롥��Ǽ��Ϥ��δؿ���ǿ����˳��ݤ���롥
 * ������ä�ʸˡ�ϡ�����ι��������ߥ󥰤ǥ����Х�ʸˡ�˷�礵��롥
 * 
 * @param ret_dfa [out] �����˼�����ä���ʸ����(DFA)�ǡ����ؤΥݥ���
 * @param ret_winfo [out] �����˼�����ä�ñ�켭��ǡ����ؤΥݥ���
 *
 * @return ��������� TRUE�����顼������� FALSE ���֤���
 * </JA>
 * <EN>
 * @brief  Read a recognition grammar from current module client.
 *
 * The syntax file (.dfa) and vicabulary file (.dict) from the module client
 * will be received in this function.  The end markers that specify the end of
 * the files are lines with only "DFAEND" and "DICEND".  The received data
 * are stored to the memory area newly allocated within this function.
 *
 * The received grammar will be incorporated into the global grammar when
 * the next updating chance has come.
 * 
 * @param ret_dfa [out] pointer to the newly received DFA data.
 * @param ret_winfo [out] pointer to the newly received dict data.
 *
 * @return TRUE if successfully read with no error, or FALSE if error exists.
 * </EN>
 */
static boolean
msock_read_grammar(DFA_INFO **ret_dfa, WORD_INFO **ret_winfo)
{
  DFA_INFO *dfa;
  WORD_INFO *winfo;

  /* load grammar: dfa and dict in turn */
  dfa = dfa_info_new();
  if (!
#ifdef WINSOCK
      rddfa_sd(module_sd, dfa)
#else
      rddfa_fd(module_sd, dfa)
#endif
      ) {
    return FALSE;
  }
  winfo = word_info_new();
  /* ignore MONOTREE */
  if (!
#ifdef WINSOCK
      voca_load_htkdict_sd(module_sd, winfo, hmminfo, FALSE)
#else
      voca_load_htkdict_fd(module_sd, winfo, hmminfo, FALSE)
#endif
      ) {
    dfa_info_free(dfa);
    return FALSE;
  }
  *ret_dfa = dfa;
  *ret_winfo = winfo;
  return TRUE;
}

/** 
 * <JA>
 * @brief �����Х�ʸˡ�ι�������ꤹ�롥
 *
 * �����Х�ʸˡ�򹹿����륿���ߥ󥰤�����Ǥ��� gram_switch_input_method
 * ���ͤˤ������äơ�����ι��������ߥ󥰤ǥ����Х뼭��򹹿�����褦
 * �ե饰�򥻥åȤ��롥
 * </JA>
 * <EN>
 * @brief Set flags to update global grammar.
 *
 * This function sets flags to make engine update the global grammar at the
 * next updating chance, according to the configuration variable in
 * gram_switch_input_method.
 * </EN>
 */
static void
set_grammar_switch_timing_flag()
{
  if (process_active) {
    /* if recognition is currently running, tell engine how/when to
       re-construct global lexicon. */
    switch(gram_switch_input_method) {
    case SM_TERMINATE:	/* discard input now and change (immediate) */
      process_want_terminate = TRUE;
      process_want_reload = TRUE;
      break;
    case SM_PAUSE:		/* segment input now, recognize it, and then change */
      process_want_terminate = FALSE;
      process_want_reload = TRUE;
      break;
    case SM_WAIT:		/* wait until the current input end and recognition completed */
      process_want_terminate = FALSE;
      process_want_reload = FALSE;
      break;
    }
    /* After the update, recognition will restart without sleeping. */
  } else {
    /* If recognition is currently not running, the received
       grammars are merely stored in memory here.  The re-construction of
       global lexicon will be delayed: it will be re-built just before
       the recognition process starts next time. */
  }
}
#endif

/** 
 * <JA>
 * @brief  �⥸�塼�륳�ޥ�ɤ�������롥
 *
 * ���饤����Ȥ��Ϳ����줿���ޥ�ɤ�������롥���δؿ��ϥ��饤����Ȥ���
 * ���ޥ�ɤ������Ƥ��뤿�Ӥ˲���ǧ�������˳�����ǸƤФ�롥
 * ���ơ��������ˤĤ��ƤϤ����Ǥ����˱�����å����������뤬��
 * ʸˡ���ɲä����ʤɤϡ������Ǥϼ����Τ߹Ԥ����ºݤ��ѹ�����
 * �ʳ�ʸˡ����Υ����Х�ʸˡ�κƹ��ۤʤɡˤ�ǧ�������ι�֤˼¹Ԥ���롥
 * ����ʸˡ�ƹ��۽�����ºݤ˹Ԥ��Τ� multigram_exec() �Ǥ��롥
 * 
 * @param command [in] ���ޥ��ʸ����
 * </JA>
 * <EN>
 * @brief  Process a module command.
 *
 * This function processes command string received from module client.
 * This will be called whenever a command arrives from a client, interrupting
 * the main recognition process.  The status responses will be performed
 * at this function immediately.  On the whole, grammar modification
 * (add/delete/(de)activation) will not be performed here.  The received
 * data are just stored in this function, and they will be processed later
 * by calling multigram_exec() between the recognition process.
 * 
 * @param command [in] command string
 * </EN>
 */
void
msock_exec_command(char *command)
{
#ifdef USE_DFA
  DFA_INFO *new_dfa;
  WORD_INFO *new_winfo;
  static char *buf = NULL, *p, *q;
  int gid;
  char namebuf[MAXGRAMNAMELEN];

  if (buf == NULL) buf = mymalloc(MAXBUFLEN);
#endif

  /* prompt the received command string */
  j_printf("[[%s]]\n",command);

  if (strmatch(command, "STATUS")) {
    /* return status */
    if (process_active) {
      module_send(module_sd, "<SYSINFO PROCESS=\"ACTIVE\"/>\n.\n");
    } else {
      module_send(module_sd, "<SYSINFO PROCESS=\"SLEEP\"/>\n.\n");
    }
  } else if (strmatch(command, "DIE")) {
    module_disconnect();
    if (fork_on_module_connection) {
      /* this is a forked process, so exit here. */
      j_exit();
    }
  } else if (strmatch(command, "VERSION")) {
    /* return version */
    module_send(module_sd, "<ENGINEINFO TYPE=\"%s\" VERSION=\"%s\" CONF=\"%s\"/>\n.\n",
		PRODUCTNAME, VERSION, SETUP);
  } else if (strmatch(command, "PAUSE")) {
    /* pause recognition: will stop when the current input ends */
    if (process_active) {
      process_want_terminate = FALSE;
      process_want_reload = TRUE;
      process_active = FALSE;
    }
    if (speech_input == SP_ADINNET) {
      /* when taking speech from adinnet client,
	 always tell the client to stop recording */
      adin_tcpip_send_pause();
    }
  } else if (strmatch(command, "TERMINATE")) {
    /* terminate recognition: input will terminate immidiately */
    /* set flags to stop adin to terminate immediately, and
       stop process */
    if (process_active) {
      process_want_terminate = TRUE;
      process_want_reload = TRUE;
      process_active = FALSE;
    }
    if (speech_input == SP_ADINNET) {
      /* when taking speech input from adinnet client,
	 always tell the client to stop recording immediately */
      adin_tcpip_send_terminate();
    }
  } else if (strmatch(command, "RESUME")) {
    /* restart the paused/terminated recognition process */
    if (process_active == FALSE) {
      process_want_terminate = FALSE;
      process_active = TRUE;
    }
    if (speech_input == SP_ADINNET) {
      /* when taking speech from adinnet client,
	 tell the client to restart recording */
      adin_tcpip_send_resume();
    }
#ifdef USE_DFA
  } else if (strmatch(command, "INPUTONCHANGE")) {
    /* change grammar switching timing policy */
    if (
#ifdef WINSOCK
	getl_sd(buf, MAXBUFLEN, module_sd)
#else	
	getl_fd(buf, MAXBUFLEN, module_sd)
#endif
	== NULL) {
      j_error("Error: msock(INPUTONCHANGE): no argument\n");
    }
    if (strmatch(buf, "TERMINATE")) {
      gram_switch_input_method = SM_TERMINATE;
    } else if (strmatch(buf, "PAUSE")) {
      gram_switch_input_method = SM_PAUSE;
    } else if (strmatch(buf, "WAIT")) {
      gram_switch_input_method = SM_WAIT;
    } else {
      j_error("Error: msock(INPUTONCHANGE): unknown method [%s]\n", buf);
    }
  } else if (strnmatch(command, "CHANGEGRAM", strlen("CHANGEGRAM"))) {
    /* receive grammar (DFA + DICT) from the socket, and swap the whole grammar  */
    /* read grammar name if any */
    p = &(command[strlen("CHANGEGRAM")]);
    while (*p == ' ' && *p != '\r' && *p != '\n' && *p != '\0') p++;
    if (*p != '\r' && *p != '\n' && *p != '\0') {
      q = buf;
      while (*p != ' ' && *p != '\r' && *p != '\n' && *p != '\0') *q++ = *p++;
      *q = '\0';
      p = buf;
    } else {
      p = NULL;
    }
    /* read a new grammar via socket */
    if (msock_read_grammar(&new_dfa, &new_winfo) == FALSE) {
      module_send(module_sd, "<GRAMMAR STATUS=\"ERROR\" REASON=\"WRONG DATA\"/>\n.\n");
    } else {
      /* delete all existing grammars */
      multigram_delete_all();
      /* register the new grammar to multi-gram tree */
      multigram_add(new_dfa, new_winfo, p);
      /* need to rebuild the global lexicon */
      /* set engine flag to tell how to switch the grammar when active */
      set_grammar_switch_timing_flag();
      module_send(module_sd, "<GRAMMAR STATUS=\"RECEIVED\"/>\n.\n");
    }
  } else if (strnmatch(command, "ADDGRAM", strlen("ADDGRAM"))) {
    /* receive grammar and add it to the current grammars */
    /* read grammar name if any */
    p = &(command[strlen("ADDGRAM")]);
    while (*p == ' ' && *p != '\r' && *p != '\n' && *p != '\0') p++;
    if (*p != '\r' && *p != '\n' && *p != '\0') {
      q = buf;
      while (*p != ' ' && *p != '\r' && *p != '\n' && *p != '\0') *q++ = *p++;
      *q = '\0';
      p = buf;
    } else {
      p = NULL;
    }
    /* read a new grammar via socket */
    if (msock_read_grammar(&new_dfa, &new_winfo) == FALSE) {
      module_send(module_sd, "<GRAMMAR STATUS=\"ERROR\" REASON=\"WRONG DATA\"/>\n.\n");
    } else {
      /* add it to multi-gram tree */
      multigram_add(new_dfa, new_winfo, p);
      /* need to rebuild the global lexicon */
      /* set engine flag to tell how to switch the grammar when active */
      set_grammar_switch_timing_flag();
      module_send(module_sd, "<GRAMMAR STATUS=\"RECEIVED\"/>\n.\n");
    }
  } else if (strmatch(command, "DELGRAM")) {
    /* remove the grammar specified by ID */
    /* read a list of grammar IDs to be deleted */
    if (
#ifdef WINSOCK
	getl_sd(buf, MAXBUFLEN, module_sd)
#else
	getl_fd(buf, MAXBUFLEN, module_sd)
#endif
	== NULL) {
      j_error("Error: msock(DELGRAM): no argument\n");
    }
    /* extract IDs and mark them as delete
       (actual deletion will be performed on the next 
    */
    for(p=strtok(buf," ");p;p=strtok(NULL," ")) {
      gid = atoi(p);
      if (multigram_delete(gid) == FALSE) { /* deletion marking failed */
	j_printerr("Warning: msock(DELGRAM): gram #%d failed to delete, ignored\n", gid);
      }
    }
    /* need to rebuild the global lexicon */
    /* set engine flag to tell how to switch the grammar when active */
    set_grammar_switch_timing_flag();
  } else if (strmatch(command, "ACTIVATEGRAM")) {
    /* activate grammar in this engine */
    /* read a list of grammar IDs to be activated */
    if (
#ifdef WINSOCK
	getl_sd(buf, MAXBUFLEN, module_sd)
#else
	getl_fd(buf, MAXBUFLEN, module_sd)
#endif
	== NULL) {
      j_error("Error: msock(ACTIVATEGRAM): no argument\n");
    }
    /* mark them as active */
    for(p=strtok(buf," ");p;p=strtok(NULL," ")) {
      gid = atoi(p);
      multigram_activate(gid);
    }
    /* tell engine when to change active status */
    set_grammar_switch_timing_flag();
  } else if (strmatch(command, "DEACTIVATEGRAM")) {
    /* deactivate grammar in this engine */
    /* read a list of grammar IDs to be de-activated */
    if (
#ifdef WINSOCK
	getl_sd(buf, MAXBUFLEN, module_sd)
#else
	getl_fd(buf, MAXBUFLEN, module_sd)
#endif
	== NULL) {
      j_error("Error: msock(DEACTIVATEGRAM): no argument\n");
    }
    /* mark them as not active */
    for(p=strtok(buf," ");p;p=strtok(NULL," ")) {
      gid = atoi(p);
      multigram_deactivate(gid);
    }
    /* tell engine when to change active status */
    set_grammar_switch_timing_flag();
  } else if (strmatch(command, "SYNCGRAM")) {
    /* update grammar if necessary */
    multigram_exec();
    module_send(module_sd, "<GRAMMAR STATUS=\"READY\"/>\n.\n");
#endif
  }
}


/** 
 * <JA>
 * @brief  ǧ�������μ¹Բ�ǽ/�¹���߾��֤��֤���
 *
 * �ץ������¹Բ�ǽ���֡����ϲ����Υȥꥬ���˥��桤���뤤�ϸ���
 * ����ǧ��������¹���ˤǤ���� TRUE �򡤼¹���߾���
 * �ʥ⥸�塼�뤫��Υ��ޥ�ɽ����ʳ��ν�������ߤ��Ƥ��ꡤ���Ϥ�
 * ���äƤ�̵�뤹��ˤǤ���� FALSE ���֤���
 * 
 * @return �¹Բ�ǽ���֤Ǥ���� TRUE �򡤼¹���߾��֤Ǥ���� FALSE ���֤���
 * </JA>
 * <EN>
 * @brief  Return the process activity status.
 *
 * If TRUE, the process is currently activated, either monitoring an
 * audio input or recognizing the current input.  If FALSE,
 * recognition is currently disabled, ignoring all the speech inputs.
 * 
 * @return TRUE if process is currently activated, or FALSE if not.
 * </EN>
 */
boolean
module_is_active()
{
  return process_active;
}

/** 
 * <JA>
 * @brief  ���߲���ǧ����¨������׵᤬���뤫�ɤ������֤���
 *
 * �⥸�塼�륳�ޥ�� "TERMINATE" ��ʸˡ�ѹ��ʤɤǡ�¨����ǧ��������
 * ��ߤ���ɬ�פ����뤫�ɤ������֤������δؿ��ϲ���ǧ�������γƥ롼��
 * ��ǡ�¨������ߤ���ɬ�פ����뤫�ɤ��������å����뤿��˸ƤФ�롥
 * 
 * @return ¨������׵᤬����Ȥ��� TRUE���ʤ��Ȥ��� FALSE ���֤���
 * </JA>
 * <EN>
 * @brief  Return whether we currently has a request of immediate termination.
 *
 * This function returns whether we now has an order for immediate termination.
 * This status will be set by receiving "TERMINATE" command
 * from module client, or when a grammar has been received with immediate
 * update option.  This function is typically called from several recognition
 * functions and ad-in processing function to check if we should stop the
 * current recognition process immediately.
 * 
 * @return TRUE if we have a request of immediate termination, or FALSE if not.
 * </EN>
 */
boolean
module_wants_terminate()
{
  return process_want_terminate;
}

/** 
 * <JA>
 * process_want_reload �ե饰�򥯥ꥢ���롥
 * 
 * </JA>
 * <EN>
 * Clear the process_want_reload flag.
 * 
 * </EN>
 */
void
module_reset_reload()
{
  process_want_reload = FALSE;
}

/** 
 * <JA>
 * ���饤����ȥ⥸�塼�뤫���̿����ɤ߹���ǽ������롥
 * ̿�᤬̵����硤���Υ��ޥ�ɤ����ޤ��Ԥġ�
 * 
 * </JA>
 * <EN>
 * Process one commands from client module.  If no command is in the buffer,
 * it will block until next command comes.
 * 
 * </EN>
 */
void
msock_process_command()
{
  if (
#ifdef WINSOCK
      getl_sd(mbuf, MAXBUFLEN, module_sd)
#else
      getl_fd(mbuf, MAXBUFLEN, module_sd)
#endif
      != NULL) {
    msock_exec_command(mbuf);
  }
}

/** 
 * <JA>
 * ���ߥ��饤����ȥ⥸�塼�뤫���̿�᤬�Хåե��ˤ��뤫Ĵ�١�
 * �⤷����н������롥�ʤ���Ф��Τޤ޽�λ���롥
 * 
 * </JA>
 * <EN>
 * Process one commands from client module.  If no command is in the buffer,
 * it will return without blocking.
 * 
 * </EN>
 */
void
msock_check_and_process_command()
{
  fd_set rfds;
  int ret;
  struct timeval tv;

  /* check if some commands are waiting in queue */
  FD_ZERO(&rfds);
  FD_SET(module_sd, &rfds);
  tv.tv_sec = 0;
  tv.tv_usec = 0;	      /* 0 msec timeout: return immediately */
  ret = select(module_sd+1, &rfds, NULL, NULL, &tv);
  if (ret < 0) {
    perror("msock_check_and_process_command: cannot poll\n");
  }
  if (ret > 0) {
    /* there is data to read */
    /* process command and change status if necessaty */
    while(select(module_sd+1, &rfds, NULL, NULL, &tv) > 0 &&
#ifdef WINSOCK
	  getl_sd(mbuf, MAXBUFLEN, module_sd)
#else
	  getl_fd(mbuf, MAXBUFLEN, module_sd)
#endif
	  != NULL) {
      msock_exec_command(mbuf);
    }
  }
}

/** 
 * <JA>
 * @brief  ���������Ԥ���Υ⥸�塼�륳�ޥ�ɽ����Τ���Υ�����Хå��ؿ���
 * ���������Ԥ���˥��饤����ȥ⥸�塼�뤫������줿���ޥ�ɤ�
 * �������뤿��Υ�����Хå��ؿ����������Ͻ���������Ū�˸ƤФ�롥
 * �⤷���ޥ�ɤ������礽���������롥�ޤ���¨����ǧ�����ǡ������˴��ˤ�
 * ���Ϥ���ߤ�����Ƥ�����Ϥ��Τ褦�˥��ơ������������Ͻ����ؿ���
 * �֤���
 * 
 * @return �̾�� 0, ¨�������׵᤬������� -2, ǧ������׵᤬����Ȥ���
 * -1 ���֤���
 * </JA>
 * <EN>
 * @brief  callback function to process module command while input detection.
 *
 * This function will be called periodically from A/D-in function to check
 * and process commands from module client.  If some commands are found,
 * it will be processed here.  Also, if some termination or stop of recognition
 * process is requested, this function returns to the caller as so.
 * 
 * @return 0 normally, -2 when there is immediate termination request, and -1
 * if there is recognition stop command.
 * </EN>
 */
int
msock_check_in_adin()
{
  if (module_is_connected()) {
    /* module: check command and terminate recording when requested */
    msock_check_and_process_command();
    /* With audio input via adinnet, TERMINATE command will issue terminate
       command to the adinnet client.  The client then stops recording
       immediately and return end-of-segment ack.  Then it will cause this
       process to stop recognition as normal.  So we need not to
       perform immediate termination at this callback, but just ignore the
       results in the main.c.  */
    if (module_wants_terminate() /* TERMINATE ... force termination */
	&& speech_input != SP_ADINNET) {
      return(-2);
    }
    if (process_want_reload) {
      return(-1);
    }
  }
  return(0);
}
