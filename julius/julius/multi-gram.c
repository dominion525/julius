/**
 * @file   multi-gram.c
 * @author Akinobu Lee
 * @date   Sat Jun 18 23:45:18 2005
 * 
 * <JA>
 * @brief  ǧ����ʸˡ�δ��� for Julian
 *
 * ���Υե�����ˤϡ�ǧ����ʸˡ���ɤ߹��ߤȴ�����Ԥ��ؿ����ޤޤ�Ƥ��ޤ���
 * �����δؿ��ϡ�ʸˡ�ե�������ɤ߹��ߡ�����ӳƼ�ǡ�����
 * ���åȥ��åפ�Ԥ��ޤ���
 *
 * ʣ��ʸˡ��Ʊ��ǧ�����б����Ƥ��ޤ���ʣ����ʸˡ����٤��ɤ߹���ǡ�
 * �����ǧ����Ԥ��ޤ����ޤ����⥸�塼��⡼�ɤǤϡ����饤����Ȥ���
 * ǧ���¹����ʸˡ��ưŪ���ɲá���������ꡤ����ʬ��ʸˡ��̵������
 * ͭ����������Ǥ��ޤ����ޤ�Ϳ����줿�ġ���ʸˡ���Ȥ�ǧ����̤�
 * �Ф����Ȥ��Ǥ��ޤ���
 *
 * Ϳ����줿��ʣ���Ρ�ʸˡ�ϰ�ĤΥ����Х�ʸˡ�Ȥ��Ʒ�礵��,
 * ʸˡ���ɤ߹��ߤ����ʤɤξ����ѹ���Ԥä��Ȥ�����������ޤ���
 * ��礵�줿��ʸ��§ (DFA) �� global_dfa �ˡ����ü��� global_winfo ��
 * ���줾�������˳�Ǽ����ޤ���������Ŭ�ڤʥ����ߥ󥰤�
 * multigram_setup() ���ƤӽФ��줿�Ȥ��ˡ�global.h �������ѿ� dfa
 * ����� winfo �˥��ԡ����졤ǧ�������ˤ����ƻ��Ѥ����褦�ˤʤ�ޤ���
 * </JA>
 * 
 * <EN>
 * @brief  Management of Recognition grammars for Julian
 *
 * This file contains functions to read and manage recognition grammar.
 * These function read in grammar and dictionary, and setup data for
 * recognition.
 *
 * Recognition with multiple grammars are supported.  Julian can read
 * several grammars specified at startup time, and perform recognition
 * with those grammars simultaneously.  In module mode, you can add /
 * delete / activate / deactivate each grammar while performing recognition,
 * and also can output optimum results for each grammar.
 *
 * Internally, the given grammars are composed to a single Global Grammar.
 * The global grammar will be updated whenever a new grammar has been read
 * or deleted.  The syntax rule (DFA) of the global grammar will be stored
 * at global_dfa, and the corresponding dictionary will be at global_winfo
 * locally, independent of the decoding timing.  After that, multigram_setup()
 * will be called to make the prepared global grammar to be used in the
 * actual recognition process, by copying the grammar and the dictionary
 * to the global variable dfa and winfo.
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

#ifdef USE_DFA

/// For debug: define this if you want grammar update messages to stdout
#define MDEBUG

/**
 * Pointer to hold DFA information of the current global grammar.
 * This is used to build a global grammar.
 * 
 */
static DFA_INFO *global_dfa = NULL;
/**
 * Pointer to hold vocabulary information of the current global grammar.
 * This is used to build a global grammar.
 * 
 */
static WORD_INFO *global_winfo = NULL;
/**
 * Current maximum value of assigned grammar ID.
 * A new grammar ID will be assigned to each new grammar.
 * 
 */
static int gram_maxid = 0;

/** 
 * <JA>
 * @brief  Ϳ����줿ʸˡ��ǧ����Ԥ�����˳ƥǡ����򥻥åȥ��åפ��롥
 *
 * Ϳ����줿ʸˡ���顤�ڹ�¤������򿷤��˺ƹ��ۤ��ޤ����ޤ�
 * ��ư���˥ӡ�����������Ū�˻ؼ�����Ƥ��ʤ�����ե륵�����ξ�硤
 * �ӡ������κ������Ԥ��ޤ���Ϳ����줿ʸˡ����Ӻƹ��ۤ��줿�ڹ�¤��
 * ����ϡ�ǧ�����Ѥ��뤿��ˤ��줾������ѿ� dfa, winfo, wchmm �˥��å�
 * ����ޤ���
 * 
 * @param d [in] ǧ�����Ѥ���DFA��¤�ξ���
 * @param w [in] �������
 * </JA>
 * <EN>
 * @brief  Setup informations for recognition with the given grammar.
 *
 * This function will re-construct the tree lexicon using the given grammar
 * and dictionary to prepare for the recognition.  If explicit value is not
 * specified for the beam width on startup, it also resets the beam width
 * according to the size of the new dictionary.  The given grammars (DFA and
 * dictionary) and the rebuilt tree lexicon will be set to the global
 * variables "dfa", "winfo" and "wchmm" to be accessible from recognition
 * functions.
 * 
 * @param d [in] DFA grammar information
 * @param w [in] dictionary information.
 * </EN>
 */
static void
multigram_setup(DFA_INFO *d, WORD_INFO *w)
{
  if (d == NULL || w == NULL) {
    /* no grammar was specified */
    dfa = NULL;			/* clear */
    winfo = NULL;
    return;
  }

  /* set the global grammar and vocabulary pointer */
  dfa = d;
  winfo = w;

  /* re-build wchmm */
  if (wchmm != NULL) {
    wchmm_free(wchmm);
  }
  wchmm = wchmm_new();
  wchmm->dfa = d;
  wchmm->winfo = w;
  wchmm->hmminfo = hmminfo;
#ifdef CATEGORY_TREE
  if (old_tree_function_flag) {
    build_wchmm(wchmm);
  } else {
    build_wchmm2(wchmm);
  }
#else
  build_wchmm2(wchmm);
#endif /* CATEGORY_TREE */
  
  /* guess beam width from models, when not specified */
  trellis_beam_width = set_beam_width(wchmm, specified_trellis_beam_width);
  if (specified_trellis_beam_width == 0) {
    j_printf("now beam width = %d (full)\n", trellis_beam_width);
  } else if (specified_trellis_beam_width == -1) {
    j_printf("now beam width = %d (guess)\n", trellis_beam_width);
  }

#ifdef USE_NGRAM
  /* re-allocate factoring cache for the tree lexicon*/
  max_successor_cache_free();
  max_successor_cache_init(wchmm);
#endif

  /* finished! */
}

/// Grammar status to be processed in the next reload timing.
static char *hookstr[] = {"", "delete", "activate", "deactivate"};
/** 
 * <JA>
 * �����ݻ����Ƥ���ʸˡ�Υꥹ�Ȥ�ɸ����Ϥ˽��Ϥ��롥
 * 
 * </JA>
 * <EN>
 * Output current list of grammars to stdout.
 * 
 * </EN>
 */
static void 
print_all_gram()
{
  MULTIGRAM *m;

  j_printf("[grammars]\n");
  for(m=gramlist;m;m=m->next) {
    j_printf("  #%2d: [%-11s] %4d words, %3d categories, %4d nodes",
	     m->id,
	     m->active ? "active" : "inactive",
	     m->winfo->num, m->dfa->term_num, m->dfa->state_num);
    if (m->newbie) j_printf(" (new)");
    if (m->hook != MULTIGRAM_DEFAULT) {
      j_printf(" (next: %s)", hookstr[m->hook]);
    }
    j_printf(" \"%s\"\n", m->name);
  }
  if (global_dfa != NULL) {
    j_printf("  Global:            %4d words, %3d categories, %4d nodes\n",
	     global_winfo->num, global_dfa->term_num, global_dfa->state_num);
  }
}

/** 
 * <JA>
 * ���ߤ��ݻ����Ƥ���ʸˡ�Υꥹ�Ȥ�⥸�塼����������롥
 * 
 * </JA>
 * <EN>
 * Send current list of grammars to module client.
 * 
 * </EN>
 */
static void
send_gram_info()
{
  MULTIGRAM *m;

  module_send(module_sd, "<GRAMINFO>\n");
  for(m=gramlist;m;m=m->next) {
    module_send(module_sd, "  #%2d: [%-11s] %4d words, %3d categories, %4d nodes",
		m->id,
		m->active ? "active" : "inactive",
		m->winfo->num, m->dfa->term_num, m->dfa->state_num);
    if (m->newbie) module_send(module_sd, " (new)");
    if (m->hook != MULTIGRAM_DEFAULT) {
      module_send(module_sd, " (next: %s)", hookstr[m->hook]);
    }
    module_send(module_sd, " \"%s\"\n", m->name);
  }
  if (global_dfa != NULL) {
    module_send(module_sd, "  Global:            %4d words, %3d categories, %4d nodes\n",
		global_winfo->num, global_dfa->term_num, global_dfa->state_num);
  }
  module_send(module_sd, "</GRAMINFO>\n.\n");
}

/** 
 * <JA>
 * ʸˡ�˿�����ʸˡ���礹�롥
 * 
 * @param gdfa [i/o] ������ʸˡ��DFA����
 * @param gwinfo [i/o] ������ʸˡ�μ������
 * @param m [i/o] ��礹��ʸˡ���󡥷��Ū�ˤɤΰ��֤˷�礵�줿������Ͽ����롥
 * </JA>
 * <EN>
 * Install a new grammar to the existing one.
 * 
 * @param gdfa [i/o] DFA information of the existing grammar to which the @a m will be installed.
 * @param gwinfo [i/o] Dictionary information of the existing grammar to which the @a m will be installed.
 * @param m [i/o] New grammar information to be installed.  The resulting location of the grammar within @a gdfa and @a gwinfo will be stored in the data.
 * </EN>
 */
static void
multigram_build_append(DFA_INFO *gdfa, WORD_INFO *gwinfo, MULTIGRAM *m)
{
  /* the new grammar 'm' will be appended to the last of gdfa and gwinfo */
  m->state_begin = gdfa->state_num;	/* initial state ID */
  m->cate_begin = gdfa->term_num;	/* initial terminal ID */
  m->word_begin = gwinfo->num;	/* initial word ID */
  
  /* append category ID and node number of src DFA */
  /* Julius allow multiple initial states: connect each initial node
     is not necesarry. */
  dfa_append(gdfa, m->dfa, m->state_begin, m->cate_begin);
  /* append words of src vocabulary to global winfo */
  voca_append(gwinfo, m->winfo, m->cate_begin, m->word_begin);
  /* append category->word mapping table */
  terminfo_append(&(gdfa->term), &(m->dfa->term), m->cate_begin, m->word_begin);
  /* append catergory-pair information */
  /* pause has already been considered on m->dfa, so just append here */
  cpair_append(gdfa, m->dfa, m->cate_begin);
  /* re-set noise entry by merging */
  dfa_pause_word_append(gdfa, m->dfa, m->cate_begin);
#ifdef MDEBUG
  j_printf("- Gram #%d: installed\n", m->id);
#endif
}

/** 
 * <JA>
 * ���߽�����Ƥ���ʸˡ�Υꥹ�Ȥ˿�����ʸˡ���ɲ���Ͽ���롥
 * 
 * @param dfa [in] �ɲ���Ͽ����ʸˡ��DFA����
 * @param winfo [in] �ɲ���Ͽ����ʸˡ�μ������
 * @param name [in] �ɲ���Ͽ����ʸˡ��̾��
 * </JA>
 * <EN>
 * Add a new grammar to the current list of grammars.
 * 
 * @param dfa [in] DFA information of the new grammar.
 * @param winfo [in] dictionary information of the new grammar.
 * @param name [in] name string of the new grammar.
 * </EN>
 */
void
multigram_add(DFA_INFO *dfa, WORD_INFO *winfo, char *name)
{
  MULTIGRAM *new;

  /* allocate new gram */
  new = (MULTIGRAM *)mymalloc(sizeof(MULTIGRAM));
  if (name != NULL) {
    strncpy(new->name, name, MAXGRAMNAMELEN);
  } else {
    strncpy(new->name, "(no name)", MAXGRAMNAMELEN);
  }

  new->id = gram_maxid;
  new->dfa = dfa;
  new->winfo = winfo;
  new->hook = MULTIGRAM_DEFAULT;
  new->newbie = TRUE;		/* need to setup */
  new->active = TRUE;		/* default: active */

  /* the new grammar is now added to gramlist */
  new->next = gramlist;
  gramlist = new;

  j_printf("- Gram #%d: read\n", new->id);
  if (module_mode) {
    send_gram_info();
  }
#ifdef MDEBUG
  print_all_gram();
#endif
  gram_maxid++;
}

/** 
 * <JA>
 * ʸˡ�ꥹ����Τ���ʸˡ�򡤼��󹹿����˺������褦�ޡ������롥
 * 
 * @param delid [in] �������ʸˡ��ʸˡID
 * 
 * @return �̾�� TRUE ���֤������ꤵ�줿ID��ʸˡ��̵������ FALSE ���֤���
 * </JA>
 * <EN>
 * Mark a grammar in the grammar list to be deleted at the next grammar update.
 * 
 * @param delid [in] grammar id to be deleted
 * 
 * @return TRUE on normal exit, or FALSE if the specified grammar is not found
 * in the grammar list.
 * </EN>
 */
boolean
multigram_delete(int delid)
{
  MULTIGRAM *m;
  for(m=gramlist;m;m=m->next) {
    if (m->id == delid) {
      m->hook = MULTIGRAM_DELETE;
      j_printf("- Gram #%d: marked delete\n", m->id);
      break;
    }
  }
  if (! m) {
    j_printf("- Gram #%d: not found\n", delid);
    if (module_mode) {
      module_send(module_sd, "<ERROR MESSAGE=\"Gram #%d not found\"/>\n.\n", delid);
    }
    return FALSE;
  }
  return TRUE;
}

/** 
 * <JA>
 * ʸˡ�ꥹ����Τ��٤Ƥ�ʸˡ�򼡲󹹿����˺������褦�ޡ������롥
 * </JA>
 * <EN>
 * Mark all grammars in the grammar list to be deleted at the next
 * grammar update.
 * </EN>
 */
void
multigram_delete_all()
{
  MULTIGRAM *m;
  for(m=gramlist;m;m=m->next) {
    m->hook = MULTIGRAM_DELETE;
  }
}

/** 
 * <JA>
 * ����ޡ����ΤĤ���줿ʸˡ��ꥹ�Ȥ��������롥
 * 
 * @return �����Х�ʸˡ�κƹ��ۤ�ɬ�פʤȤ��� TRUE ����ɬ�פʤȤ��� FALSE ���֤���
 * </JA>
 * <EN>
 * Purge grammars that has been marked as delete.
 * 
 * @return TRUE if the global grammar must be re-constructed, or FALSE if not needed.
 * </EN>
 */
static boolean
multigram_exec_delete()
{
  MULTIGRAM *m, *mtmp, *mprev;
  boolean ret_flag = FALSE;
#ifdef MDEBUG
  int n;
#endif

  /* exec delete */
  mprev = NULL;
  m = gramlist;
  while(m) {
    mtmp = m->next;
    if (m->hook == MULTIGRAM_DELETE) {
      /* if any grammar is deleted, we need to rebuild lexicons etc. */
      /* so tell it to the caller */
      if (! m->newbie) ret_flag = TRUE;
      dfa_info_free(m->dfa);
      word_info_free(m->winfo);
      n=m->id;
      free(m);
      j_printf("- Gram #%d: purged\n", n);
      if (mprev != NULL) {
	mprev->next = mtmp;
      } else {
	gramlist = mtmp;
      }
    } else {
      mprev = m;
    }
    m = mtmp;
  }

  return(ret_flag);
}

/** 
 * <JA>
 * ʸˡ�ꥹ����λ��ꤵ�줿ʸˡ��ͭ�������롥�����Ǥϼ��󹹿�����
 * ȿ�Ǥ����褦�˥ޡ�����Ĥ���ΤߤǤ��롥
 * 
 * @param gid [in] ͭ����������ʸˡ�� ID
 * </JA>
 * <EN>
 * Activate specified grammar in the grammar list.  The specified grammar
 * will only be marked as to be activated in the next grammar update timing.
 * 
 * @param gid [in] grammar ID to be activated
 * </EN>
 */
void
multigram_activate(int gid)	/* only mark */
{
  MULTIGRAM *m;
  for(m=gramlist;m;m=m->next) {
    if (m->id == gid) {
      if (m->hook == MULTIGRAM_ACTIVATE) {
	j_printf("- Gram #%d: already active\n", m->id);
	if (module_mode) {
	  module_send(module_sd, "<WARN MESSAGE=\"Gram #%d already active\"/>\n.\n", m->id);
	}
      } else {
	m->hook = MULTIGRAM_ACTIVATE;
	j_printf("- Gram #%d: marked activate\n", m->id);
      }
      break;
    }
  }
  if (! m) {
    j_printf("- Gram #%d: not found, activation ignored\n", gid);
    if (module_mode) {
      module_send(module_sd, "<WARN MESSAGE=\"Gram #%d not found\"/>\n.\n", gid);
    }
  }
}

/** 
 * <JA>
 * ʸˡ�ꥹ����λ��ꤵ�줿ʸˡ��̵�������롥̵�������줿ʸˡ��
 * ǧ���ˤ����Ʋ���Ÿ������ʤ�������ˤ�äơ������Х뼭���
 * �ƹ��ۤ��뤳�Ȥʤ������Ū�˸ġ���ʸˡ��ON/OFF�Ǥ��롥̵��������
 * ʸˡ�� multigram_activate() �ǺƤ�ͭ�����Ǥ��롥�ʤ������Ǥ�
 * �����ʸˡ���������ߥ󥰤�ȿ�Ǥ����褦�˥ޡ�����Ĥ���ΤߤǤ��롥
 * 
 * @param gid [in] ̵����������ʸˡ��ID
 * </JA>
 * <EN>
 * Deactivate a grammar in the grammar list.  The words of the de-activated
 * grammar will not be expanded in the recognition process.  This feature
 * enables rapid switching of grammars without re-building tree lexicon.
 * The de-activated grammar will again be activated by calling
 * multigram_activate().
 * 
 * @param gid [in] grammar ID to be de-activated
 * </EN>
 */
void
multigram_deactivate(int gid)	/* only mark */
{
  MULTIGRAM *m;
  for(m=gramlist;m;m=m->next) {
    if (m->id == gid) {
      m->hook = MULTIGRAM_DEACTIVATE;
      j_printf("- Gram #%d: marked deactivate\n", m->id);
      break;
    }
  }
  if (! m) {
    j_printf("- Gram #%d: not found, deactivation ignored\n", gid);
    if (module_mode) {
      module_send(module_sd, "<WARN MESSAGE=\"Gram #%d not found\"/>\n.\n", gid);
    }
  }
}

/** 
 * <JA>
 * ͭ������̵�����ޡ����ΤĤ���줿ʸˡ��ºݤ�ͭ������̵�������롥
 * 
 * @return ̵������ͭ���ء����뤤��ͭ������̵���ؾ��֤��Ѳ�����ʸˡ����ĤǤ�
 * �����TRUE, ���֤������Ѳ����ʤ��ä����� FALSE ���֤���
 * </JA>
 * <EN>
 * Execute (de)activation of grammars previously marked as so.
 * 
 * @return TRUE if at least one grammar has been changed, or FALSE if no
 * grammar has changed its status.
 * </EN>
 */
static boolean
multigram_exec_activate()
{
  MULTIGRAM *m;
  boolean modified;
  
  modified = FALSE;
  for(m=gramlist;m;m=m->next) {
    if (m->hook == MULTIGRAM_ACTIVATE) {
      m->hook = MULTIGRAM_DEFAULT;
      if (!m->active) {
	j_printf("- Gram #%d: turn on active\n", m->id);
      }
      m->active = TRUE;
      modified = TRUE;
    } else if (m->hook == MULTIGRAM_DEACTIVATE) {
      m->hook = MULTIGRAM_DEFAULT;
      if (m->active) {
	j_printf("- Gram #%d: turn off inactive\n, m->id");
      }
      m->active = FALSE;
      modified = TRUE;
    }
  }
  return(modified);
}
 
/************************************************************************/
/* update grammar if needed */
/************************************************************************/
/** 
 * <JA>
 * @brief  �����Х�ʸˡ�ι���
 * 
 * ʸˡ�ꥹ�Ȥκ���ޤ����ɲä�����å�����������б����ƥ����Х�ʸˡ
 * �򹹿����롥
 *
 * �ꥹ����˺���ޡ������Ĥ���줿ʸˡ��������ϡ�����ʸˡ��������
 * �����Х뼭���ƹ��ۤ��롥�������ɲä��줿ʸˡ��������ϡ�
 * ����ʸˡ�򸽺ߤΥ����Х뼭����������ɲä��롥
 *
 * �嵭�Υ����å��η�̥����Х뼭����ѹ�������С����ι������줿�����Х�
 * ���񤫤��ڹ�¤������ʤɤβ���ǧ���ѥǡ�����¤��ƹ��ۤ��롥
 * 
 * @return ��� TRUE ���֤���
 * </JA>
 * <EN>
 * @brief  Update and re-construct global grammar if needed.
 *
 * This function checks for any modification in the grammar list from
 * previous call, and update the global grammar if needed.
 *
 * If there are grammars marked to be deleted in the grammar list,
 * they will be actually deleted from memory.  Then the global grammar is
 * built from scratch using the rest grammars.
 * If there are new grammars, they are appended to the current global grammar.
 * 
 * If any modification of the global grammar occured in the process above,
 * the tree lexicons and some other data for recognition will be re-constructed
 * from the updated global grammar.
 * 
 * @return always TRUE.
 * </EN>
 */
boolean				/* return FALSE if no gram */
multigram_exec()
{
  MULTIGRAM *m;
  boolean global_modified = FALSE;
  boolean active_changed = FALSE;

#ifdef MDEBUG
  j_printf("- Grammar update check\n");
#endif

  /* setup additional grammar info of new ones */
  for(m=gramlist;m;m=m->next) {
    if (m->newbie) {
      /* map dict item to dfa terminal symbols */
      make_dfa_voca_ref(m->dfa, m->winfo);
      /* set dfa->sp_id and dfa->is_sp */
      dfa_find_pause_word(m->dfa, m->winfo, hmminfo);
      /* build catergory-pair information */
      extract_cpair(m->dfa);
    }
  }

  /* delete grammars marked as "delete" */
  if (multigram_exec_delete()) { /* some built grammars deleted */
    /* rebuild global grammar from scratch (including new) */
    /* active status not changed here (inactive grammar will also included) */
    /* activate/deactivate hook will be handled later, so just keep it here */
#ifdef MDEBUG
    j_printf("- Re-build whole global grammar...\n");
#endif
    if (global_dfa != NULL) {    /* free old global */
      dfa_info_free(global_dfa);
      word_info_free(global_winfo);
      global_dfa = NULL;
    }
    for(m=gramlist;m;m=m->next) {
      if (global_dfa == NULL) {
	global_dfa = dfa_info_new();
	dfa_state_init(global_dfa);
	global_winfo = word_info_new();
	winfo_init(global_winfo);
      }
      if (m->newbie) m->newbie = FALSE;
      multigram_build_append(global_dfa, global_winfo, m);
    }
    global_modified = TRUE;
  } else {			/* global not need changed by the deletion */
    /* append only new grammars */
    for(m=gramlist;m;m=m->next) {
      if (m->newbie) {
	if (global_dfa == NULL) {
	  global_dfa = dfa_info_new();
	  dfa_state_init(global_dfa);
	  global_winfo = word_info_new();
	  winfo_init(global_winfo);
	}
	if (m->newbie) m->newbie = FALSE;
	multigram_build_append(global_dfa, global_winfo, m);
	global_modified = TRUE;
      }
    }
  }

  /* process activate/deactivate hook */
  active_changed = multigram_exec_activate();

  if (global_modified) {		/* if global lexicon has changed */
    /* now global grammar info has been updated, */
    /* build up tree lexicon for recognition process */
    multigram_setup(global_dfa, global_winfo);
#ifdef MDEBUG
    j_printf("- update completed\n");
#endif
  }
  
  /* output grammar info when any change has been made */
  if (global_modified || active_changed) {
    print_all_gram();
    if (module_mode) {
      send_gram_info();
    }
  }

  return(TRUE);
}

/***********************************************************************/
/** 
 * <JA>
 * dfa�ե������dict�ե�������ɤ߹����ʸˡ�ꥹ�Ȥ��ɲä��롥
 * 
 * @param dfa_file [in] dfa �ե�����̾
 * @param dict_file [in] dict �ե�����̾
 * </JA>
 * <EN>
 * Read in dfa file and dict file, and add them to the grammar list.
 * 
 * @param dfa_file [in] dfa file name
 * @param dict_file [in] dict file name
 * </EN>
 */
void
multigram_read_file(char *dfa_file, char *dict_file)
{
  WORD_INFO *new_winfo;
  DFA_INFO *new_dfa;
  char buf[MAXGRAMNAMELEN], *p, *q;

  j_printf("reading [%s] and [%s]...\n", dfa_file, dict_file);
  
  /* read dict*/
  new_winfo = word_info_new();
  if ( ! 
#ifdef MONOTREE
      /* leave winfo monophone for 1st pass lexicon tree */
       init_voca(new_winfo, dict_file, hmminfo, TRUE, forcedict_flag)
#else 
       init_voca(new_winfo, dict_file, hmminfo, FALSE, forcedict_flag)
#endif
       ) {
    j_error("ERROR: failed to read dictionary, terminated\n");
  }
#ifdef PASS1_IWCD
  if (triphone_check_flag && hmminfo->is_triphone) {
    /* go into interactive triphone HMM check mode */
    hmm_check(hmminfo, new_winfo);
  }
#endif
  
  /* read dfa */
  new_dfa = dfa_info_new();
  init_dfa(new_dfa, dfa_file);

  /* extract name */
  p = &(dfa_file[0]);
  q = p;
  while(*p != '\0') {
    if (*p == '/') q = p + 1;
    p++;
  }
  p = q;
  while(*p != '\0' && *p != '.') {
    buf[p-q] = *p;
    p++;
  }
  buf[p-q] = '\0';
  
  /* register the new grammar to multi-gram tree */
  multigram_add(new_dfa, new_winfo, buf);

  j_printf("gram \"%s\" registered\n", buf);

}

/** 
 * <JA>
 * ��ư���ɤ߹��ߥꥹ�Ȥ�ʸˡ���ɲä��롥
 * 
 * @param dfafile [in] DFA�ե�����
 * @param dictfile [in] ñ�켭��
 * </JA>
 * <EN>
 * Add a grammar to the grammar list to be read at startup.
 * 
 * @param dfafile [in] DFA file
 * @param dictfile [in] dictionary file
 * </EN>
 */
void
multigram_add_gramlist(char *dfafile, char *dictfile)
{
  GRAMLIST *new;

  new = (GRAMLIST *)mymalloc(sizeof(GRAMLIST));
  new->dfafile = strcpy((char *)mymalloc(strlen(dfafile)+1), dfafile);
  new->dictfile = strcpy((char *)mymalloc(strlen(dictfile)+1), dictfile);
  new->next = gramlist_root;
  gramlist_root = new;
}

/** 
 * <JA>
 * ��ư���ɤ߹��ߥꥹ�Ȥ�ä���
 * 
 * </JA>
 * <EN>
 * Remove the grammar list to be read at startup.
 * 
 * </EN>
 */
void
multigram_remove_gramlist()
{
  GRAMLIST *g;
  GRAMLIST *tmp;

  g = gramlist_root;
  while (g) {
    tmp = g->next;
    free(g->dfafile);
    free(g->dictfile);
    free(g);
    g = tmp;
  }
  gramlist_root = NULL;
}

/** 
 * <JA>
 * ��ư���˻��ꤵ�줿���٤Ƥ�ʸˡ�����Ƥ��ɤ߹��ࡥ
 * 
 * </JA>
 * <EN>
 * Read in all the grammars specified at startup.
 * 
 * </EN>
 */
void
multigram_read_all_gramlist()
{
  GRAMLIST *g;

  for(g = gramlist_root; g; g = g->next) {
    multigram_read_file(g->dfafile, g->dictfile);
  }
}

/** 
 * <JA>
 * @brief  �ץ�ե��å�������ʣ����ʸˡ��ư���ɤ߹��ߥꥹ�Ȥ��ɲä��롥
 *
 * �ץ�ե��å����� "foo", ���뤤�� "foo,bar" �Τ褦�˥���޶��ڤ��
 * ʣ��Ϳ���뤳�Ȥ��Ǥ��ޤ�����ʸ����θ��� ".dfa", ".dict" ��Ĥ���
 * �ե�����򡤤��줾��ʸˡ�ե����롦����ե�����Ȥ��ƽ缡�ɤ߹��ߤޤ���
 * �ɤ߹��ޤ줿ʸˡ�Ͻ缡��ʸˡ�ꥹ�Ȥ��ɲä���ޤ���
 * 
 * @param prefix_list [in]  �ץ�ե��å����Υꥹ��
 * @param cwd [in] �����ȥǥ��쥯�ȥ��ʸ����
 * </JA>
 * <EN>
 * @brief  Add multiple grammars given by their prefixs to the grammar list.
 *
 * This function read in several grammars, given a prefix string that
 * contains a list of file prefixes separated by comma: "foo" or "foo,bar".
 * For each prefix, string ".dfa" and ".dict" will be appended to read
 * dfa file and dict file.  The read grammars will be added to the grammar
 * list.
 * 
 * @param prefix_list [in] string that contains comma-separated list of grammar path prefixes
 * @param cwd [in] string of current working directory
 * </EN>
 */
void
multigram_add_prefix_list(char *prefix_list, char *cwd)
{
  char buf[MAXGRAMNAMELEN], *p, *q;
  char buf2_d[MAXGRAMNAMELEN], *buf_d;
  char buf2_v[MAXGRAMNAMELEN], *buf_v;

  if (prefix_list == NULL) return;
  
  p = &(prefix_list[0]);
  
  while(*p != '\0') {
    /* extract one prefix to buf[] */
    q = p;
    while(*p != '\0' && *p != ',') {
      buf[p-q] = *p;
      p++;
    }
    buf[p-q] = '\0';

    /* register the new grammar to the grammar list to be read later */
    strcpy(buf2_d, buf);
    strcat(buf2_d, ".dfa");
    buf_d = filepath(buf2_d, cwd);
    checkpath(buf_d);
    strcpy(buf2_v, buf);
    strcat(buf2_v, ".dict");
    buf_v = filepath(buf2_v, cwd);
    checkpath(buf_v);
    multigram_add_gramlist(buf_d, buf_v);

    /* move to next */
    if (*p == ',') p++;
  }
}

/** 
 * <JA>
 * @brief �ꥹ�ȥե�������ɤ߹���ʣ��ʸˡ��ư���ɤ߹��ߥꥹ�Ȥ��ɲä��롥
 *
 * �ե��������1�Ԥˣ��Ĥ��ĵ��Ҥ��줿ʸˡ�Υץ�ե��å�������,
 * �б�����ʸˡ�ե������缡�ɤ߹��ߤޤ���
 * 
 * �ƹԤ�ʸ����θ��� ".dfa", ".dict" ��Ĥ����ե������
 * ���줾��ʸˡ�ե����롦����ե�����Ȥ��ƽ缡�ɤ߹��ߤޤ���
 * �ɤ߹��ޤ줿ʸˡ�Ͻ缡��ʸˡ�ꥹ�Ȥ��ɲä���ޤ���
 * 
 * @param listfile [in] �ץ�ե��å����ꥹ�ȤΥե�����̾
 * </JA>
 * <EN>
 * @brief  Add multiple grammars from prefix list file to the grammar list.
 *
 * This function read in multiple grammars at once, given a file that
 * contains a list of grammar prefixes, each per line.
 *
 * For each prefix, string ".dfa" and ".dict" will be appended to read the
 * corresponding dfa and dict file.  The read grammars will be added to the
 * grammar list.
 * 
 * @param listfile [in] path of the prefix list file
 * </EN>
 */
void
multigram_add_prefix_filelist(char *listfile)
{
  FILE *fp;
  char buf[MAXGRAMNAMELEN], *p, *src_bgn, *src_end, *dst;
  char *cdir;
  char buf2_d[MAXGRAMNAMELEN], *buf_d;
  char buf2_v[MAXGRAMNAMELEN], *buf_v;

  if (listfile == NULL) return;
  if ((fp = fopen(listfile, "r")) == NULL) {
    j_printerr("failed to open %s\n", listfile);
    return;
  }
  while(getl_fp(buf, MAXGRAMNAMELEN, fp) != NULL) {
    /* remove comment */
    p = &(buf[0]);
    while(*p != '\0') {
      if (*p == '#') {
	*p = '\0';
	break;
      }
      p++;
    }
    if (buf[0] == '\0') continue;
    
    /* trim head/tail blanks */
    p = (&buf[0]);
    while(*p == ' ' || *p == '\t' || *p == '\r') p++;
    if (*p == '\0') continue;
    src_bgn = p;
    p = (&buf[strlen(buf) - 1]);
    while((*p == ' ' || *p == '\t' || *p == '\r') && p > src_bgn) p--;
    src_end = p;
    dst = (&buf[0]);
    p = src_bgn;
    while(p <= src_end) *dst++ = *p++;
    *dst = '\0';
    if (buf[0] == '\0') continue;
    
    /* register the new grammar to the grammar list to be read later */
    /* converting relative paths as relative to this list file */
    cdir = strcpy((char *)mymalloc(strlen(listfile)+1), listfile);
    get_dirname(cdir);
    strcpy(buf2_d, buf);
    strcat(buf2_d, ".dfa");
    buf_d = filepath(buf2_d, cdir);
    checkpath(buf_d);
    strcpy(buf2_v, buf);
    strcat(buf2_v, ".dict");
    buf_v = filepath(buf2_v, cdir);
    checkpath(buf_v);
    multigram_add_gramlist(buf_d, buf_v);
    free(cdir);
  }
  fclose(fp);
}

/** 
 * <JA>
 * ���ߤ���ʸˡ�ο�������(active/inactive�Ȥ�)��
 * 
 * @return ʸˡ�ο����֤���
 * </JA>
 * <EN>
 * Get the number of current grammars (both active and inactive).
 * 
 * @return the number of grammars.
 * </EN>
 */
int
multigram_get_all_num()
{
  MULTIGRAM *m;
  int cnt;
  
  cnt = 0;
  for(m=gramlist;m;m=m->next) cnt++;
  return(cnt);
}

/** 
 * <JA>
 * ñ�쥫�ƥ����°����ʸˡ�����롥
 * 
 * @param category ñ�쥫�ƥ���ID
 * 
 * @return ñ�쥫�ƥ����°����ʸˡ��ID���֤���
 * </JA>
 * <EN>
 * Get which grammar the given category belongs to.
 * 
 * @param category word category ID
 * 
 * @return the id of the belonging grammar.
 * </EN>
 */
int
multigram_get_gram_from_category(int category)
{
  MULTIGRAM *m;
  int tb, te;
  for(m = gramlist; m; m = m->next) {
    if (m->newbie) continue;
    tb = m->cate_begin;
    te = tb + m->dfa->term_num;
    if (tb <= category && category < te) { /* found */
      return(m->id);
    }
  }
  return(-1);
}

#endif /* USE_DFA */
