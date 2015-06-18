/**
 * @file   mybmalloc.c
 * @author Akinobu LEE
 * @date   Thu Feb 17 16:14:59 2005
 * 
 * <JA>
 * @brief  ブロック単位の動的メモリ確保
 *
 * このファイルには，要求されたサイズごとではなく，定められた一定の
 * 大きさ単位でメモリを確保する関数が定義されています．これを用いることで，
 * 細かい単位で大量にメモリ割り付けする際に起こるメモリ管理のオーバヘッドを
 * 改善できます．これらの関数は，主に言語モデルや音響モデルの読み込みに
 * 用いられています．
 * </JA>
 * 
 * <EN>
 * @brief  Dynamic memory allocation per large block
 *
 * This file defines functions that allocate memories per a certain big
 * block instead of allocating for each required sizes.
 * This function will improve the overhead of memory management operation,
 * especially when an application requires huge number of small segments
 * to be allocated.  These functions are mainly used for allocating memory
 * for acoustic models and language models.
 * </EN>
 * 
 * $Revision: 1.5 $
 * 
 */
/*
 * Copyright (c) 1991-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#undef DEBUG			/* output debug message */

#define MYBMALLOC_LINKED 1	///< Use improved version that enables freeing

#include <sent/stddefs.h>

static boolean mybmalloc_initialized = FALSE; ///< TRUE if mybmalloc has already initialized
static char *current = NULL;	///< Pointer to the current block
static char *nowp = NULL;	///< Pointer to the current allocating point in the block
static char *endp = NULL;	///< Pointer at the end of the current block
static unsigned int blocksize;  ///< Block size in bytes
static int align;		///< Allocation alignment size in bytes
static unsigned int align_mask; ///< Bit mask to compute the actual aligned memory size

#ifdef MYBMALLOC_LINKED
/// Linked data structure to hold malloced areas
typedef struct _linked_buffer {
  unsigned long size;		///< Size of the area
  char *buffer;			///< Pointer to the area
  char *nowp;			///< Pointer to the current point
  struct _linked_buffer *next; ///< Link to the next data, NULL if last.
} *LINKED_BUFFER;

static LINKED_BUFFER first_linked_buffer = NULL; ///< Pointer to the first linked buffer
static LINKED_BUFFER last_linked_buffer = NULL; ///< Pointer to the last linked buffer
#endif


/** 
 * Set block size and memory alignment factor.
 * 
 */
void
mybmalloc_set_param()
{
  long pagesize, blockpagenum;

  /* block size should be rounded up by page size */
  pagesize = getpagesize();
  blockpagenum = (MYBMALLOC_BLOCK_SIZE + (pagesize - 1)) / pagesize;
  blocksize = pagesize * blockpagenum;

  /* alignment by a word (= pointer size?) */
#ifdef NO_ALIGN_DOUBLE
  align = sizeof(void *);
#else
  /* better for floating points */
  align = sizeof(double);
#endif
  align_mask = ~(align - 1);	/* assume power or 2 */
#ifdef DEBUG
  j_printerr("pagesize=%d blocksize=%d align=%d (bytes)\n", (int)pagesize, blocksize, align);
#endif
  
  mybmalloc_initialized = TRUE;
}

/** 
 * Malloc specified size and return the pointer.
 * 
 * @param size [in] required size in bytes
 * 
 * @return pointer to the allocated area.
 */
void *
mybmalloc(int size)
{
  void *allocated;
  if (!mybmalloc_initialized) mybmalloc_set_param();  /* initialize if not yet */
  /* malloc segment should be aligned to a word boundary */
  size = (size + align - 1) & align_mask;
  if (!current || nowp + size >= endp) {
#ifndef MYBMALLOC_LINKED
    if (size > blocksize) {
      /* large block, fall to normal malloc */
      return(mymalloc(size));
    }
    /* allocate next block */
    current = (char *)mymalloc(blocksize);
    endp = current + blocksize;
    nowp = current;
#else
    unsigned long current_size;
    LINKED_BUFFER linked_buffer = NULL;
    static LINKED_BUFFER prev_linked_buffer = NULL;
    
    if (first_linked_buffer != NULL) {
      LINKED_BUFFER next_lb;
      
      if (prev_linked_buffer != NULL) {
	prev_linked_buffer->nowp = nowp;
      }
      
      /* search the buffer having left space */
      next_lb = first_linked_buffer;
      while (next_lb != NULL) {
	if (next_lb->nowp + size <= next_lb->buffer + next_lb->size) {
	  linked_buffer = next_lb;
	  break;
	}
	next_lb = next_lb->next;
      }
    }
    
    if (linked_buffer != NULL) {
      current = linked_buffer->nowp;
      endp = linked_buffer->buffer + linked_buffer->size;
    } else {
      current_size = (unsigned long)blocksize;
      while (current_size < (unsigned long)size) {
	current_size += (unsigned long)blocksize;
      }
      
      linked_buffer = (LINKED_BUFFER)mymalloc(sizeof(struct _linked_buffer));
      linked_buffer->size = current_size;
      linked_buffer->buffer = (char *)mymalloc(linked_buffer->size);
      linked_buffer->nowp = linked_buffer->buffer;
      linked_buffer->next = NULL;
      
      if (first_linked_buffer == NULL) {
	first_linked_buffer = linked_buffer;
      } else if (last_linked_buffer != NULL) {
	last_linked_buffer->next = linked_buffer;
      }
      last_linked_buffer = linked_buffer;
      current = linked_buffer->buffer;
      endp = current + current_size;
    }
    prev_linked_buffer = linked_buffer;
   
    nowp = current;
#endif
  }
  /* return current pointer */
  allocated = nowp;
  nowp += size;
  return(allocated);
}

/** 
 * Free all the allocated area.
 * 
 */
void mybmalloc_free(void)
{
#ifdef MYBMALLOC_LINKED
  if (first_linked_buffer != NULL) {
    LINKED_BUFFER curr_lb, next_lb;
    
    next_lb = first_linked_buffer;
    while (next_lb != NULL) {
      curr_lb = next_lb;
      next_lb = curr_lb->next;
      free(curr_lb->buffer);
      free(curr_lb);
    }
  }
  first_linked_buffer = NULL;
  last_linked_buffer = NULL;
  current = nowp = endp = NULL;
#endif
  
  return;
}
     
/** 
 * String duplication using mybmalloc().
 * 
 * @param s [in] string to be duplicated
 * 
 * @return pointer to the newly allocated string.
 */
char *
mybstrdup(char *s)
{
  char *allocated;
  int size = strlen(s) + 1;
  allocated = mybmalloc(size);
  memcpy(allocated, s, size);
  return(allocated);
}

/** 
 * Another version of memory block allocation, used for tree lexicon.
 * 
 * @param size [in] memory size to be allocated
 * @param list [i/o] total memory management information (will be updated here)
 * 
 * @return pointer to the newly allocated area.
 */
void *
mybmalloc2(int size, BMALLOC_BASE **list)
{
  void *allocated;
  BMALLOC_BASE *new;
  if (!mybmalloc_initialized) mybmalloc_set_param();  /* initialize if not yet */
  /* malloc segment should be aligned to a word boundary */
  size = (size + align - 1) & align_mask;
  if (*list == NULL || (*list)->now + size >= (*list)->end) {
    new = (BMALLOC_BASE *)mymalloc(sizeof(BMALLOC_BASE));
    if (size > blocksize) {
      /* large block, allocate a whole block */
      new->base = mymalloc(size);
      new->end = (char *)new->base + size;
    } else {
      /* allocate per blocksize */
      new->base = mymalloc(blocksize);
      new->end = (char *)new->base + blocksize;
    }
    new->now = (char *)new->base;
    new->next = (*list);
    *list = new;
  }
  /* return current pointer */
  allocated = (*list)->now;
  (*list)->now += size;
  return(allocated);
}

/** 
 * String duplication using mybmalloc2().
 * 
 * @param s [in] string to be duplicated
 * @param list [i/o] total memory management information pointer
 * 
 * @return pointer to the newly allocated string.
 */
char *
mybstrdup2(char *s, BMALLOC_BASE **list)
{
  char *allocated;
  int size = strlen(s) + 1;
  allocated = mybmalloc2(size, list);
  memcpy(allocated, s, size);
  return(allocated);
}

/** 
 * Free all memories allocated by mybmalloc2()
 * 
 * @param list [i/o] total memory management information (will be cleaned here)
 */
void
mybfree2(BMALLOC_BASE **list)
{
  BMALLOC_BASE *b, *btmp;
  b = *list;
  while (b) {
    btmp = b->next;
    free(b->base);
    free(b);
    b = btmp;
  }
  *list = NULL;
}
