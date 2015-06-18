/**
 * @file   rdhmmdef_tiedmix.c
 * @author Akinobu LEE
 * @date   Wed Feb 16 03:25:11 2005
 * 
 * <JA>
 * @brief  HTK %HMM 定義ファイルの読み込み：tied-mixtureモデルの混合分布コードブック
 * </JA>
 * 
 * <EN>
 * @brief  Read HTK %HMM definition file: mixture codebook in tied-mixture model
 * </EN>
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

#include <sent/stddefs.h>
#include <sent/htk_hmm.h>

extern char *rdhmmdef_token;	///< Current token

/** 
 * Look up a data macro by the name.
 * 
 * @param hmm [in] %HMM definition data
 * @param keyname [in] macro name to find
 * 
 * @return pointer to the found data, or NULL if not found.
 */
static GCODEBOOK *
codebook_lookup(HTK_HMM_INFO *hmm, char *keyname)
{
  GCODEBOOK *book;

  if (hmm->codebook_root == NULL) return(NULL);
  book = aptree_search_data(keyname, hmm->codebook_root);
  if (strmatch(book->name, keyname)) {
    return book;
  } else {
    return NULL;
  }
}

/** 
 * Add a new data to the global structure.
 * 
 * @param hmm [i/o] %HMM definition data to store it
 * @param new [in] new data to be added
 */
void
codebook_add(HTK_HMM_INFO *hmm, GCODEBOOK *new)
{
  GCODEBOOK *match;
  if (hmm->codebook_root == NULL) {
    hmm->codebook_root = aptree_make_root_node(new);
  } else {
    match = aptree_search_data(new->name, hmm->codebook_root);
    if (strmatch(match->name, new->name)) {
      j_printerr("Error: ~s \"%s\" is already defined\n", new->name);
      rderr(NULL);
    } else {
      aptree_add_entry(new->name, new, match->name, &(hmm->codebook_root));
    }
  }
}

/** 
 * @brief  Convert codebook ID to the defined %HMM density
 * 
 * This function assigns a list of %HMM density definition to the
 * given codebook.  The densities are searched by the name of
 * codebook name followed by the mixture component ID starting from 1.
 * For example, if you have a codebook whose name is "ny4s2m", The densities
 * of names like "ny4s2m1", "ny4s2m2", ... will be searched through the
 * %HMM definition data.  The resulting list will be stored in the codebook.
 *
 * If some density definitions are not found, they are just skipped.
 * In this case, a warning message will be output to standard error.
 * 
 * @param hmminfo [in] %HMM definition data that has densities
 * @param book [i/o] codebook, name given and density list will be stored.
 */
static void
tmix_create_codebook_index(HTK_HMM_INFO *hmminfo, GCODEBOOK *book)
{
  char *mixname;
  HTK_HMM_Dens *dtmp;
  int i;
  int realbooknum = 0;

  mixname = (char *)mymalloc(strlen(book->name)+30);
  book->d = (HTK_HMM_Dens **) mybmalloc2(sizeof(HTK_HMM_Dens *) * book->num, &(hmminfo->mroot));
  for (i=0;i<book->num;i++) {
    sprintf(mixname, "%s%d", book->name, i + 1);
    if ((dtmp = dens_lookup(hmminfo, mixname)) == NULL) {
/* 
 *	 j_printerr("Error: mixture \"%s\" (%dth mixture in codebook \"%s\") not found\n", mixname, i + 1, book->name);
 *	 rderr(NULL);
 */
      book->d[i] = NULL;
    } else {
      book->d[i] = dtmp;
      realbooknum++;
    }
  }
  if (realbooknum < book->num) {
    j_printerr("Warning: book [%s]: defined=%d < %d\n",
	       book->name, realbooknum, book->num);
  }
  
  free(mixname);
}

/** 
 * @brief  Read a codebook name and weights, build the codebook structure
 * on demand, and assigns them to the current state.
 *
 * The required codebook on the current token will be assigned to this
 * state.  If the corresponding codebook structure is not built yet,
 * it will be constructed here on demand by gathering corresponding mixture
 * density definitions.  Then this state will store the pointer to the
 * codebook, together with its own mixture weights in the following tokens.
 * 
 * @param fp [in] file pointer
 * @param state [i/o] current %HMM state to hold pointer to the codebook and their weights
 * @param hmm [i/o] %HMM definition data, codebook statistics and tied-mixture marker will be modified.
 */
void
tmix_read(FILE *fp, HTK_HMM_State *state, HTK_HMM_INFO *hmm)
{
  char *bookname;
  GCODEBOOK *thebook;
  int mid, i;

  NoTokErr("missing TMIX bookname");
  bookname = rdhmmdef_token;
  /* check whether the specified codebook exist */
  if ((thebook = codebook_lookup(hmm, bookname)) == NULL) {
    /* create GCODEBOOK global index structure from mixture macros */
    thebook = (GCODEBOOK *)mybmalloc2(sizeof(GCODEBOOK), &(hmm->mroot));
    thebook->name = mybstrdup2(bookname, &(hmm->mroot));
    thebook->num = state->mix_num;
    /* map codebook id to HTK_HMM_Dens* */
    tmix_create_codebook_index(hmm, thebook);
    /* register the new codebook */
    codebook_add(hmm, thebook);
    thebook->id = hmm->codebooknum;
    hmm->codebooknum++;
    /* set maximum codebook size */
    if (hmm->maxcodebooksize < thebook->num) hmm->maxcodebooksize = thebook->num;
  } else {
    /* check coherence */
    if (state->mix_num != thebook->num) {
      rderr("tmix_read: TMIX weight num don't match the codebook size");
    }
  }

  /* set pointer to the GCODEBOOK structure  */
  state->b = (HTK_HMM_Dens **)thebook;

  /* store the weights to `state->bweight[]' */
  read_token(fp);
  state->bweight = (PROB *) mybmalloc2(sizeof(PROB) * state->mix_num, &(hmm->mroot));
  {
    int len;
    double w;

    mid = 0;
    while (mid < state->mix_num)
    {
      char *p, q;
      NoTokErr("missing some TMIX weights");
      if ((p = strchr(rdhmmdef_token, '*')) == NULL) {
	len = 1;
	w = atof(rdhmmdef_token);
      } else {
	len = atoi(p+1);
	q = *p;
	*p = '\0';
	w = atof(rdhmmdef_token);
	*p = q;
      }
      read_token(fp);
      for(i=0;i<len;i++) {
	state->bweight[mid] = (PROB)log(w);
	mid++;
      }
    }
  }

  /* mark info as tied mixture */
  hmm->is_tied_mixture = TRUE;
}

