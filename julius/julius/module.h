/**
 * @file   module.h
 * @author Akinobu Lee
 * @date   Sat Jun 18 23:43:15 2005
 * 
 * <JA>
 * @brief  モジュール通信関連の定義
 * </JA>
 * 
 * <EN>
 * @brief  Definitions for module communication
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

#ifndef __MODULE_H__
#define __MODULE_H__

#include <sent/tcpip.h>

/// Default port number for module connection
#define DEFAULT_MODULEPORT 10500

void main_module_loop();
boolean module_disconnect();
boolean module_is_connected();
boolean module_is_active();
boolean module_wants_terminate();
void module_reset_reload();
void msock_exec_command(char *command);
void msock_process_command();
void msock_check_and_process_command();
int msock_check_in_adin();


#endif /* __MODULE_H__ */
