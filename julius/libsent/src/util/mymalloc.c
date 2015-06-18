/**
 * @file   mymalloc.c
 * @author Akinobu LEE
 * @date   Thu Feb 17 16:27:03 2005
 * 
 * <JA>
 * @brief  動的メモリ確保を行う関数
 *
 * エラーが起こった場合，即座にエラー終了します．
 * </JA>
 * 
 * <EN>
 * @brief  Dynamic memory allocation funtions
 *
 * When allocation error occured within these functions, the program will
 * exit immediately.
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


/** 
 * Allocate a memory, as the same as malloc.
 * 
 * @param size [in] required size in bytes.
 * 
 * @return pointer to the the newly allocated area.
 */
void *
mymalloc(int size)
{
  void *p;
  if ( (p = malloc(size)) == NULL) {
    j_error("cannot malloc %d byte\n",size);
  }
  return p;
}

/** 
 * Re-allocate memory area, keeping the existing data, as the same as realloc.
 *
 * @param ptr [in] memory pointer to be re-allocated
 * @param size [in] required new size in bytes
 * 
 * @return pointer to the the newly allocated area with existing data.
 */
void *
myrealloc(void *ptr, int size)
{
  void *p;
  if ( (p = realloc(ptr,size)) == NULL) {
    j_error("cannot realloc %d byte\n",size);
  }
  return p;
}

/** 
 * Allocate memory area and set it to zero, as the same as calloc.
 * 
 * @param nelem [in] size of element in bytes
 * @param elsize [in] number of elements to allocate
 * 
 * @return pointer to the newly allocated area.
 */
void *
mycalloc(int nelem, int elsize)
{
  void *p;
  if ( (p = calloc(nelem,elsize)) == NULL) {
    j_error("cannot calloc %d x %d byte\n",nelem, elsize);
  }
  return p;
}

