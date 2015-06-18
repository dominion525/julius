/**
 * @file   put_htkdata_info.c
 * @author Akinobu LEE
 * @date   Tue Feb 15 23:36:00 2005
 * 
 * <JA>
 * @brief  %HMM 定義や特徴パラメータの情報をテキスト出力する
 * </JA>
 * 
 * <EN>
 * @brief  Output %HMM and parameter information to standard out
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
#include <sent/htk_param.h>
#include <sent/hmm.h>

static char buf[512];		///< Local work area for text handling


/** 
 * Output transition matrix.
 * 
 * @param t [in] pointer to a transion matrix
 */
void
put_htk_trans(HTK_HMM_Trans *t)
{
  int i,j;

  if (t == NULL) {
    j_printf("no transition\n");
  } else {
    for (i=0;i<t->statenum;i++) {
      for (j=0;j<t->statenum;j++) {
	j_printf(" %e", t->a[i][j]);
      }
      j_printf("\n");
    }
  }
}

/** 
 * Output variance vector (diagonal).
 * 
 * @param v [in] pointer to a variance data
 */
void
put_htk_var(HTK_HMM_Var *v)
{
  int i;

  if (v == NULL) {
    j_printf("no covariance\n");
  } else {
    j_printf("variance(%d): (may be inversed)", v->len);
    for (i=0;i<v->len;i++) {
      j_printf(" %e", v->vec[i]);
    }
    j_printf("\n");
  }
}

/** 
 * Output a density information, mean and variance.
 * 
 * @param d [in] pointer to a density data
 */
void
put_htk_dens(HTK_HMM_Dens *d)
{
  int i;
  
  if (d == NULL) {
    j_printf("no dens\n");
  } else {
    j_printf("mean(%d):", d->meanlen);
    for (i=0;i<d->meanlen;i++) {
      j_printf(" %e", d->mean[i]);
    }
    j_printf("\n");
    put_htk_var(d->var);
    j_printf("gconst: %e\n", d->gconst);
  }
}

/** 
 * Output mixture component of a state.
 * 
 * @param s [in] pointer to %HMM state
 */
void
put_htk_state(HTK_HMM_State *s)
{
  int i;

  if (s == NULL) {
    j_printf("no output state\n");
  } else {
    j_printf("mixture num: %d\n", s->mix_num);
    for (i=0;i<s->mix_num;i++) {
      j_printf("-- d%d (weight=%f)--\n",i+1,pow(10.0, s->bweight[i]));
      put_htk_dens(s->b[i]);
    }
  }
}

/** 
 * Output %HMM model, number of states and information for each state.
 * 
 * @param h [in] pointer to %HMM model
 */
void
put_htk_hmm(HTK_HMM_Data *h)
{
  int i;
  
  j_printf("name: %s\n", h->name);
  j_printf("state num: %d\n", h->state_num);
  for (i=0;i<h->state_num;i++) {
    j_printf("**** state %d ****\n",i+1);
    put_htk_state(h->s[i]);
  }
  put_htk_trans(h->tr);
}

/** 
 * Output logical %HMM data and its mapping status.
 * 
 * @param logical [in] pointer to a logical %HMM
 */
void
put_logical_hmm(HMM_Logical *logical)
{
  j_printf("name: %s\n", logical->name);
  if (logical->is_pseudo) {
    j_printf("mapped to: %s (pseudo)\n", logical->body.pseudo->name);
  } else {
    j_printf("mapped to: %s\n", logical->body.defined->name);
  }
}

/** 
 * Output transition arcs of an HMM instance.
 * 
 * @param d [in] pointer to a HMM instance.
 */
void
put_hmm_arc(HMM *d)
{
  A_CELL *ac;
  int i;

  j_printf("total len: %d\n", d->len);
  for (i=0;i<d->len;i++) {
    j_printf("node-%d\n", i);
    for (ac=d->state[i].ac;ac;ac=ac->next) {
      j_printf(" arc: %d %f (%f)\n",ac->arc, ac->a, pow(10.0, ac->a));
    }
  }
#ifndef MULTIPATH_VERSION
  j_printf("last arc to accept state: %f\n", d->accept_ac_a);
#endif
}

/** 
 * Output output probability information of an HMM instance.
 * 
 * @param d [in] pointer to a HMM instance.
 */
void
put_hmm_outprob(HMM *d)
{
  int i;

  j_printf("total len: %d\n", d->len);
  for (i=0;i<d->len;i++) {
    j_printf("n%d\n", i);
    if (d->state[i].is_pseudo_state) {
      j_printf("[[[pseudo state cluster with %d states]]]\n", d->state[i].out.cdset->num);
    } else {
      put_htk_state(d->state[i].out.state);
    }
  }
}

/** 
 * Output an HMM instance.
 * 
 * @param d [in] pointer to a HMM instance.
 */
void
put_hmm(HMM *d)
{
  put_hmm_arc(d);
  put_hmm_outprob(d);
}

/** 
 * Output parameter header.
 * 
 * @param h [in] pointer to a parameter header information
 */
void
put_param_head(HTK_Param_Header *h)
{
  j_printf("num of samples: %d\n", h->samplenum);
  j_printf("window shift: %d ms\n", h->wshift / 10000);
  j_printf("bytes per sample: %d\n", h->sampsize);
  j_printf("parameter type: %s\n", param_code2str(buf, h->samptype, FALSE));
}

/** 
 * Output array of vectors.
 * 
 * @param p [in] pointer to vector array represented as [0..num-1][0...veclen-1]
 * @param num [in] number of vectors in @a p
 * @param veclen [in] length of each vector
 */
void
put_vec(VECT **p, int num, short veclen)
{
  int t,v;

  for (t=0;t<num;t++) {
    j_printf("%d:\t%8.3f",t,p[t][0]);
    for (v=1;v<veclen;v++) {
      if ((v % 10) ==0) j_printf("\n\t");
      j_printf("%8.3f", p[t][v]);
    }
    j_printf("\n");
  }
}

/** 
 * Output the whole parameter information, including header and all vectors.
 * 
 * @param pinfo [in] pointer to parameter structure.
 */
void
put_param(HTK_Param *pinfo)
{
  put_param_head(&(pinfo->header));
  put_vec(pinfo->parvec, pinfo->samplenum, pinfo->veclen);
}

/** 
 * Output the length of an input parameter by number of frames and seconds.
 * 
 * @param pinfo [in] pointer to parameter structure.
 */
void
put_param_info(HTK_Param *pinfo)
{
  HTK_Param_Header *h;
  float sec;

  h = &(pinfo->header);
  sec = (float)h->samplenum * (float)h->wshift / 10000000;
  j_printf("length: %d frames (%.2f sec.)\n", h->samplenum, sec);
}

/** 
 * Get the maximum number of mixtures per state in a %HMM definition.
 * 
 * @param hmminfo [in] %HMM definition data.
 * 
 * @return the maximum number of mixtures per state.
 */
static int
get_max_mixture_num(HTK_HMM_INFO *hmminfo)
{
  HTK_HMM_State *st;
  int maxmixnum;

  maxmixnum = 0;
  for (st = hmminfo->ststart; st; st = st->next) {
    if (maxmixnum < st->mix_num) maxmixnum = st->mix_num;
  }
  return(maxmixnum);
}

/** 
 * Output total statistic informations of the %HMM definition data.
 * 
 * @param hmminfo [in] %HMM definition data.
 */
void
print_hmmdef_info(HTK_HMM_INFO *hmminfo)
{
  j_printf("HMM Info:\n");
  j_printf("    %d models, %d states, %d mixtures are defined\n",
	   hmminfo->totalhmmnum, hmminfo->totalstatenum, hmminfo->totalmixnum);
  j_printf("\t      model type = ");
  if (hmminfo->is_tied_mixture) j_printf("tied-mixture, ");
  j_printf("context dependency handling %s\n",
	     (hmminfo->is_triphone) ? "ON" : "OFF");
  if (hmminfo->is_tied_mixture) {
    j_printf("\t    codebook num = %d\n", hmminfo->codebooknum);
    j_printf("\tmax codebook size= %d\n", hmminfo->maxcodebooksize);
  }
  j_printf("      training parameter = %s\n",param_code2str(buf, hmminfo->opt.param_type, FALSE));
  j_printf("\t   vector length = %d\n", hmminfo->opt.vec_size);
  j_printf("\tcov. matrix type = %s\n", get_cov_str(hmminfo->opt.cov_type));
  j_printf("\t   duration type = %s\n", get_dur_str(hmminfo->opt.dur_type));
  j_printf("\t     mixture num = %d\n", get_max_mixture_num(hmminfo));
  j_printf("\t   max state num = %d\n", hmminfo->maxstatenum);
#ifdef MULTIPATH_VERSION
  j_printf("      skippable models =");
  {
    HTK_HMM_Data *dtmp;
    int n = 0;
    for (dtmp = hmminfo->start; dtmp; dtmp = dtmp->next) {
      if (is_skippable_model(dtmp)) {
	j_printf(" %s", dtmp->name);
 	n++;
      }
    }
    if (n == 0) {
      j_printf(" none\n");
    } else {
      j_printf(" (%d model(s))\n", n);
    }
  }
#endif
}
