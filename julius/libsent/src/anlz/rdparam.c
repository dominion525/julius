/**
 * @file   rdparam.c
 * @author Akinobu LEE
 * @date   Tue Feb 15 00:16:44 2005
 *
 * <JA>
 * @brief  HTK 特徴パラメータファイルの読み込み・構造体の新規割り付け
 *
 * 特徴パラメータファイルのバイトオーダーは big endian を仮定しています．
 * ただし little endian の場合もできる限り自動判別して読み込みます．
 *
 * ファイルの特徴パラメータ型に "_C" （圧縮データ），"_K" （CRCチェック
 * サムつき）が含まれている場合，それらはここで処理されます．この場合，
 * 読み込まれた後の特徴パラメータデータの型からこれらは取り除かれます．
 * </JA>
 * <EN>
 * @brief  Read HTK parameter file
 *
 * The byte order of HTK parameter file is assumed as big endian.  If not,
 * however, these functions try to read with forcing byte (re-)swapping.
 *
 * When "_C" (compressed) or "_K" (CRC checksum added) exists in the file,
 * they are processed in these functions.  Then, after reading finished,
 * these qualifiers are removed from its parameter type code.
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

/* assume sizeof: */
/*            float = 4 */
/* (unsigned)   int = 4 */
/* (unsigned) short = 2 */
/* (unsigned)  char = 1 */

#include <sent/stddefs.h>
#include <sent/htk_param.h>
#include <sys/types.h>

static boolean needswap;	///< TRUE if need byte-swapping

/** 
 * Read binary data from a file pointer, with byte swapping.
 * 
 * @param buf [out] buffer to store read data
 * @param unitbyte [in] size of a unit in bytes
 * @param unitnum [in] number of unit to read
 * @param fp [in] file pointer
 * 
 * @return TRUE if specified number of unit was successfully read, FALSE if failed.
 */
static boolean
myread(char *buf, size_t unitbyte, int unitnum, FILE *fp)
{
  size_t tmp;
  if ((tmp = myfread(buf, unitbyte, unitnum, fp)) < (size_t)unitnum) {
    perror("Error: cannot read\n");
    return(FALSE);
  }
  /* swap if necessary */
  if (needswap) swap_bytes(buf, unitbyte, unitnum);
  return(TRUE);
}


/** 
 * Read in a HTK parameter file from @a fp .
 * 
 * @param fp [in] file pointer
 * @param pinfo [in] parameter data to store the read informations
 * 
 * @return TRUE on success, FALSE on failure.
 */
static boolean
read_param(FILE *fp, HTK_Param *pinfo)
{
  unsigned int i;
  int v;
  float *a = NULL, *b = NULL;
  char *buf = NULL; /* for uncompressing */
  char *p;
  float d;
  unsigned short c;
  HTK_Param_Header *hd;

  hd = &(pinfo->header);

  /* endian check once */
  /* assume input as BIG ENDIAN */
#ifdef WORDS_BIGENDIAN
  needswap = FALSE;
#else  /* LITTLE ENDIAN */
  needswap = TRUE;
#endif
  
  /* read in headers */
  if(!myread((char *)&(hd->samplenum), sizeof(unsigned int), 1, fp)) return(FALSE);
  /* try to detect and read little-endian parameters from wav2mfcc... */
  if (hd->samplenum >= 60000) {	/* more than 10 minutes! */
    j_printerr("Warning: data corrupted?: %d frames (more than 10 minutes)\n", hd->samplenum);
    j_printerr("Warning: maybe MFCC made by wav2mfcc on a little-endian machine.\n");
    j_printerr("retrying reading with endian conversion...\n");
    swap_bytes((char *)&(hd->samplenum), sizeof(unsigned int), 1);
    needswap = ! needswap;
  }
    
  myread((char *)&(hd->wshift), sizeof(unsigned int), 1, fp);
  myread((char *)&(hd->sampsize), sizeof(unsigned short), 1, fp);
  myread((char *)&(hd->samptype), sizeof(short), 1, fp);
  if (hd->samptype & F_COMPRESS) {
    pinfo->veclen = hd->sampsize / sizeof(short);
  } else {
    pinfo->veclen = hd->sampsize / sizeof(float);
  }

  if (hd->samptype & F_COMPRESS) {
    hd->samplenum -= sizeof(float); /* (-_-) */
    /* read in compression coefficient arrays */
    a = (float *)mymalloc(sizeof(float) * pinfo->veclen);
    b = (float *)mymalloc(sizeof(float) * pinfo->veclen);
    myread((char *)a, sizeof(float), pinfo->veclen, fp);
    myread((char *)b, sizeof(float), pinfo->veclen, fp);
  }
  pinfo->samplenum = hd->samplenum;

  buf = (char *)mymalloc(hd->sampsize);

  /* read in parameter vector */
  pinfo->parvec = (VECT **)mymalloc(sizeof(VECT *) * hd->samplenum);
/* 
 *   {
 *     int size;
 *     float *ftmp;
 *     size = hd->samplenum * pinfo->veclen;
 *     if (size != 0) {
 *	 ftmp = (float *)mymalloc(sizeof(float) * size);
 *	 for(i=0;i<hd->samplenum;i++) {
 *	   pinfo->parvec[i] = &(ftmp[i * pinfo->veclen]);
 *	 }
 *     }
 *   }
 */
  /* needs conversion of integerized */
  for (i=0;i<hd->samplenum;i++) {
    pinfo->parvec[i] = (VECT *)mymalloc(sizeof(VECT) * pinfo->veclen);
    if (hd->samptype & F_COMPRESS) {
      myread(buf, sizeof(short), hd->sampsize / sizeof(short), fp);
      p = buf;
      /* uncompress: (short(2byte) -> float(4byte)) * veclen*/
      for (v=0;v<pinfo->veclen;v++) {
        d = *(short *)p;
        pinfo->parvec[i][v] = (d + b[v]) / a[v];
        p += sizeof(short);
      }
    } else {
      myread(buf, sizeof(float), hd->sampsize / sizeof(float), fp);
      p = buf;
      for (v=0;v<pinfo->veclen;v++) {
        d = *(float *)p;
        pinfo->parvec[i][v] = d;
        p += sizeof(float);
      }
    }
  }

  if (hd->samptype & F_CHECKSUM) {
    /* CRC check (2byte) */
    /* skip this */
    myread((char *)&c, sizeof(unsigned short), 1, fp);
  }

  /*put_param(pinfo);*/

  if (hd->samptype & F_COMPRESS) {
    free(a);
    free(b);
  }
  free(buf);

  return(TRUE);

}

/** 
 * Allocate a new parameter.
 * 
 * @return pointer to the newly allocated area.
 */
HTK_Param *
new_param()
{
  HTK_Param *ret;
  ret = (HTK_Param *)mymalloc(sizeof(HTK_Param));
  ret->parvec = NULL;
  ret->samplenum = 0;
  return(ret);
}

/** 
 * Free the HTK parameter structure.
 * 
 * @param pinfo [in] parameter data to be destroyed.
 */
void
free_param(HTK_Param *pinfo)
{
  unsigned int i;
  if (pinfo->samplenum > 0) {
    for (i=0;i<pinfo->samplenum;i++) {
      free(pinfo->parvec[i]);
    }
  }
  if (pinfo->parvec != NULL) free(pinfo->parvec);
  free(pinfo);
}

/** 
 * Top function to read a HTK parameter file.
 * 
 * @param filename [in] HTK parameter file name 
 * @param pinfo [in] parameter data (already allocated by new_param())
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
rdparam(char *filename, HTK_Param *pinfo)
{
  FILE *fp;
  boolean retflag;
  
  if ((fp = fopen_readfile(filename)) == NULL) return(FALSE);
  retflag = read_param(fp, pinfo);
  if (fclose_readfile(fp) < 0) return (FALSE);
  return (retflag);
}
