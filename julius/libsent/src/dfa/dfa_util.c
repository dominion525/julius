/**
 * @file   dfa_util.c
 * @author Akinobu LEE
 * @date   Tue Feb 15 14:18:36 2005
 * 
 * <JA>
 * @brief  文法の情報をテキストで出力
 * </JA>
 * 
 * <EN>
 * @brief  Output text informations about the grammar
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
#include <sent/dfa.h>

/** 
 * Output overall grammar information to stdout.
 * 
 * @param dinfo [in] DFA grammar
 */
void
print_dfa_info(DFA_INFO *dinfo)
{
  j_printf("DFA grammar info:\n");
  j_printf("      %d nodes, %d arcs, %d terminal(category) symbols\n",
	 dinfo->state_num, dinfo->arc_num, dinfo->term_num);
  j_printf("      category-pair matrix size is %d bytes\n",
	 sizeof(unsigned char) * dinfo->term_num * dinfo->term_num / 8);
}

/** 
 * Output the category-pair matrix in text format to stdout
 * 
 * @param dinfo [in] DFA grammar that holds category pair matrix
 */
void
print_dfa_cp(DFA_INFO *dinfo)
{
  int i,j;
  int t;

  j_printf("---------- terminal(category)-pair matrix ----------\n");
  /* horizontal ruler */
  j_printf("    ");
  for (j=0;j<dinfo->term_num;j++) {
    if (j > 0 && (j % 10) == 0) {
      t = j / 10;
      j_printf("%1d", t);
    } else {
      j_printf(" ");
    }
  }
  j_printf("\n    ");
  for (j=0;j<dinfo->term_num;j++) {
    j_printf("%1d", j % 10);
  }
  j_printf("\n");
  
  j_printf("bgn ");
  for (j=0;j<dinfo->term_num;j++) {
    j_printf((dfa_cp_begin(dinfo, j) == TRUE) ? "o" : " ");
  }
  j_printf("\n");
  j_printf("end ");
  for (j=0;j<dinfo->term_num;j++) {
    j_printf((dfa_cp_end(dinfo, j) == TRUE) ? "o" : " ");
  }
  j_printf("\n");
  for (i=0;i<dinfo->term_num;i++) {
    j_printf("%3d ",i);
    for (j=0;j<dinfo->term_num;j++) {
      j_printf((dfa_cp(dinfo, i, j) == TRUE) ? "o" : " ");
    }
    j_printf("\n");
  }
}
