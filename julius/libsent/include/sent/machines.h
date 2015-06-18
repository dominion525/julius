/**
 * @file   machines.h
 * @author Akinobu LEE
 * @date   Fri Feb 11 03:38:31 2005
 *
 * <EN>
 * @brief  Machine-dependent definitions
 *
 * This file contains machine-dependent or OS-dependent definitions,
 * although most of such runtime dependencies are now handled by
 * configure script.
 * </EN>
 * <JA>
 * @brief �¹ԴĶ���¸�����
 *
 * ���Υե�����ˤϼ¹ԴĶ��˰�¸����������ޤޤ�Ƥ��ޤ���
 * �����������Τ褦�ʼ¹ԴĶ��˰�¸����������ѹ���ؿ���̵ͭ��Ƚ���
 * ���ߤۤȤ�� configure ������ץȤˤ�äƹԤʤ��Ƥ��ޤ���
 * </JA>
 *
 * @sa config.h.in
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

#ifndef __SENT_MACHINES_H__
#define __SENT_MACHINES_H__

#include <sent/stddefs.h>

#ifndef HAVE_STRCASECMP
int strcasecmp(char *s1, char *s2);
int strncasecmp(char *s1, char *s2, size_t n);
#endif

#endif /* __SENT_MACHINES_H__ */
