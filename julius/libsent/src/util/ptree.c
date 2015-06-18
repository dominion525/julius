/**
 * @file   ptree.c
 * @author Akinobu LEE
 * @date   Thu Feb 17 15:34:39 2005
 * 
 * <JA>
 * @brief  �ѥȥꥷ�������ڤ��Ѥ���̾���������ǡ������� int �ξ��
 * </JA>
 * 
 * <EN>
 * @brief  Patricia index tree for name lookup: data type = int
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
#include <sent/ptree.h>

/** 
 * String bit test function.
 * 
 * @param str [in] key string
 * @param bitplace [in] bit location to test
 * 
 * @return the content of tested bit in @a tmp_str, either 0 or 1.
 */
int
testbit(char *str, int bitplace)
{
  int maskptr;
  int bitshift;
  unsigned char maskbit;
  
  maskptr = bitplace / 8;
  if ((bitshift = bitplace % 8) != 0) {
    maskbit = 0x80 >> bitshift;
  } else {
    maskbit = 0x80;
  }
  if (strlen(str)<maskptr) return 0;
  return(str[maskptr] & maskbit);
}

/** 
 * Find in which bit the two strings differ, starting from the head.
 * 
 * @param str1 [in] string 1
 * @param str2 [in] string 2
 * 
 * @return the bit location in which the string differs.
 */
int
where_the_bit_differ(char *str1, char *str2)
{
  int p = 0;
  int bitloc;

  /* step: char, bit */
  while(str1[p] == str2[p]) p++;
  bitloc = p * 8;
  while(testbit(str1, bitloc) == testbit(str2, bitloc)) bitloc++;

  return(bitloc);
}


/** 
 * Allocate a new node.
 * 
 * 
 * @return pointer to the new node.
 */
static PATNODE *
new_node()
{
  PATNODE *tmp;

  tmp = (PATNODE *)mymalloc(sizeof(PATNODE));
  tmp->left0 = NULL;
  tmp->right1 = NULL;

  return(tmp);
}

/** 
 * Make a patricia tree for given string arrays.
 * Recursively called by descending the scan bit.
 * 
 * @param words [in] list of word strings
 * @param data [in] integer value corresponding to each string in @a words
 * @param wordsnum [in] number of above
 * @param bitplace [in] current scan bit.
 * 
 * @return pointer to the root node index.
 */
PATNODE *
make_ptree(char **words, int *data, int wordsnum, int bitplace)
{
  int i,j, tmp;
  char *p;
  int newnum;
  PATNODE *ntmp;

#if 0
  j_printf("%d:", wordsnum);
  for (i=0;i<wordsnum;i++) {
    j_printf(" %s",words[i]);
  }
  j_printf("\n");
  j_printf("test bit = %d\n", bitplace);
#endif

  if (wordsnum == 1) {
    /* word identified: this is leaf node */
    ntmp = new_node();
    ntmp->value.data = data[0];
    return(ntmp);
  }

  newnum = 0;
  for (i=0;i<wordsnum;i++) {
    if (testbit(words[i], bitplace) != 0) {
      newnum++;
    }
  }
  if (newnum == 0 || newnum == wordsnum) {
    /* all words has same bit, continue to descend */
    return(make_ptree(words, data, wordsnum, bitplace + 1));
  } else {
    /* sort word pointers by tested bit */
    j = wordsnum-1;
    for (i=0; i<newnum; i++) {
      if (testbit(words[i], bitplace) == 0) {
	for (; j>=newnum; j--) {
	  if (testbit(words[j], bitplace) != 0) {
	    p = words[i]; words[i] = words[j]; words[j] = p;
	    tmp = data[i]; data[i] = data[j]; data[j] = tmp;
	    break;
	  }
	}
      }
    }
    /* create node and descend for each node */
    ntmp = new_node();
    ntmp->value.thres_bit = bitplace;
    ntmp->right1 = make_ptree(words, data, newnum, bitplace+1);
    ntmp->left0  = make_ptree(&(words[newnum]), &(data[newnum]), wordsnum-newnum, bitplace+1);
    return(ntmp);
  }
}


/** 
 * Output a tree structure in text for debug, traversing pre-order
 *
 * @param node [in] root index node
 * @param level [in] current tree depth
 */
void
disp_ptree(PATNODE *node, int level)
{
  int i;

  for (i=0;i<level;i++) {
    j_printf("-");
  }
  if (node->left0 == NULL && node->right1 == NULL) {
    j_printf("LEAF:%d\n", node->value.data);
  } else {
    j_printf("%d\n", node->value.thres_bit);
    if (node->left0 != NULL) {
      disp_ptree(node->left0, level+1);
    }
    if (node->right1 != NULL) {
      disp_ptree(node->right1, level+1);
    }
  }
}

/* temporary work area for search */
static char *tmp_str;		///< Local work area: current string we are searching for
static int maxbitplace;		///< Local work area: Maximum bit length of @a tmp_str

/** 
 * Local bit test function for search.
 * 
 * @param bitplace [in] bit place to test.
 * 
 * @return the content of tested bit in @a tmp_str, either 0 or 1.
 */
static int
testbit_local(int bitplace)
{
  int maskptr;
  int bitshift;
  unsigned char maskbit;

  if (bitplace >= maxbitplace) return 0;
  maskptr = bitplace / 8;
  if ((bitshift = bitplace % 8) != 0) {
    maskbit = 0x80 >> bitshift;
  } else {
    maskbit = 0x80;
  }
  return(tmp_str[maskptr] & maskbit);
}

/** 
 * Recursive function to search the data in the tree
 * 
 * @param node [in] current node.
 * 
 * @return the found integer value.
 */
static int
ptree_search_data_r(PATNODE *node)
{
  if (node->left0 == NULL && node->right1 == NULL) {
    return(node->value.data);
  } else {
    if (testbit_local(node->value.thres_bit) != 0) {
      return(ptree_search_data_r(node->right1));
    } else {
      return(ptree_search_data_r(node->left0));
    }
  }
}

/** 
 * Search for the data whose key string matches the given string.
 * 
 * @param str [in] search key string
 * @param node [in] root node of index tree
 * 
 * @return the exactly found integer value, or the nearest one.
 */
int
ptree_search_data(char *str, PATNODE *node)
{
  if (node == NULL) {
    j_error("Error: ptree_search_data: no node, search for \"%s\" failed\n", str);
  }
  tmp_str = str;
  maxbitplace = strlen(str) * 8 + 8;
  return(ptree_search_data_r(node));
}


/*******************************************************************/
/* add 1 node to given ptree */

/** 
 * Make a root node of a index tree.
 * 
 * @param data [in] the first data
 * 
 * @return the newly allocated root node.
 */
PATNODE *
ptree_make_root_node(int data)
{
  PATNODE *nnew;
  /* make new leaf node for newstr */
  nnew = new_node();
  nnew->value.data = data;
  return(nnew);
}

/** 
 * Insert a new node to the existing index tree.
 * 
 * @param str [in] new key string
 * @param bitloc [in] bit branch to which this node will be added
 * @param data [in] new data integer value
 * @param parentlink [i/o] the parent node to which this node will be added
 */
static void
ptree_add_entry_at(char *str, int bitloc, int data, PATNODE **parentlink)
{
  PATNODE *node;
  node = *parentlink;
  if (node->value.thres_bit > bitloc ||
      (node->left0 == NULL && node->right1 == NULL)) {
    PATNODE *newleaf, *newbranch;
    /* insert between [parent] and [node] */
    newleaf = new_node();
    newleaf->value.data = data;
    newbranch = new_node();
    newbranch->value.thres_bit = bitloc;
    *parentlink = newbranch;
    if (testbit(str, bitloc) ==0) {
      newbranch->left0  = newleaf;
      newbranch->right1 = node;
    } else {
      newbranch->left0  = node;
      newbranch->right1 = newleaf;
    }
    return;
  } else {
    if (testbit(str, node->value.thres_bit) != 0) {
      ptree_add_entry_at(str, bitloc, data, &(node->right1));
    } else {
      ptree_add_entry_at(str, bitloc, data, &(node->left0));
    }
  }
}

/** 
 * Insert a new node to the index tree.
 * 
 * @param str [in] new key string
 * @param data [in] new data integer value
 * @param matchstr [in] the most matching data already exist in the index tree,
 * as obtained by aptree_search_data()
 * @param rootnode [i/o] pointer to root index node
 */
void
ptree_add_entry(char *str, int data, char *matchstr, PATNODE **rootnode)
{
  int bitloc;

  bitloc = where_the_bit_differ(str, matchstr);
  if (*rootnode == NULL) {
    *rootnode = ptree_make_root_node(data);
  } else {
    ptree_add_entry_at(str, bitloc, data, rootnode);
  }

}

/** 
 * Free all the sub nodes from specified node.
 * 
 * @param node [in] current node.
 */
void
free_ptree(PATNODE *node)
{
  if (node->left0 != NULL) free_ptree(node->left0);
  if (node->right1 != NULL) free_ptree(node->right1);
  free(node);
}
