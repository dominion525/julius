/**
 * @file   rdhmmdef_options.c
 * @author Akinobu LEE
 * @date   Wed Feb 16 01:53:45 2005
 * 
 * <JA>
 * @brief  HTK %HMM 定義ファイルの読み込み：グローバルオプション
 * </JA>
 * 
 * <EN>
 * @brief  Read HTK %HMM definition file: global options
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
#include <sent/htk_param.h>
#include <sent/htk_hmm.h>

/// Strings to make mapping between %HMM covariance option strings and code definitions.
static OptionStr optcov[] = {	/* covariance matrix type */
  {"DIAGC", C_DIAG_C, "Diag", TRUE},
  {"INVDIAGC", C_INV_DIAG, "InvDiag", FALSE},
  {"FULLC", C_FULL, "Full", FALSE},
  {"LLTC", C_LLT, "LLT", FALSE}, /* not used in HTK V2.0 */
  {"XFORMC", C_XFORM, "Xform", FALSE},  /* not used in HTK V2.0 */
  {NULL,0,NULL,FALSE}
};
/// Strings to make mapping between %HMM duration option strings and code definitions.
static OptionStr optdur[] = {	/* duration types */
  {"NULLD", D_NULL, "Null", TRUE},
  {"POISSOND", D_POISSON, "Poisson", FALSE},
  {"GAMMAD", D_GAMMA, "Gamma", FALSE},
  {"GEND", D_GEN, "Gen", FALSE},
  {NULL,0,NULL,FALSE}
};

extern char *rdhmmdef_token;	///< Current token


/** 
 * Read in global options.
 * 
 * @param fp [in] file pointer
 * @param op [out] pointer to store the global options
 */
static void
read_global_opt(FILE *fp, HTK_HMM_Options *op)
{
  int i;
  short tmptype;

  for (;;) {
    if (rdhmmdef_token == NULL) break;
    if (currentis("HMMSETID")) { /* <HMMSETID> */
      read_token(fp);
      NoTokErr("missing HMMSETID argument");
    } else if (currentis("STREAMINFO")) { /* <STREAMINFO> */
      read_token(fp);
      NoTokErr("missing STREAMINFO num");
      op->stream_info.num = atoi(rdhmmdef_token);
      /*DM("%d STREAMs:", op->stream_info.num);*/
      if (op->stream_info.num > 50) {
	j_printerr("stream num exceeded %d\n", 50);
	rderr(NULL);
      }
      for (i=0;i<op->stream_info.num;i++) {
	read_token(fp);
	NoTokErr("missing STREAMINFO vector size");
	op->stream_info.vsize[i] = atoi(rdhmmdef_token);
	/*DM(" %d",op->stream_info.vsize[i]);*/
      }
      /*DM("\n");*/
      
    } else if (currentis("VECSIZE")) {	/* <VECSIZE> */
      read_token(fp);
      NoTokErr("missing VECSIZE value");
      op->vec_size = atoi(rdhmmdef_token);
      /*DM("vector size: %d\n", op->vec_size);*/
      
    } else {
      /* covariance matrix type */
      for (i=0;optcov[i].name!=NULL;i++) {
	if (currentis(optcov[i].name)) {
	  op->cov_type = optcov[i].type;
	  /*DM("covariance matrix type: %s\n", optcov[i].desc);*/
	  goto optloop;
	}
      }
      /* duration type */
      for (i=0;optdur[i].name!=NULL;i++) {
	if (currentis(optdur[i].name)) {
	  op->dur_type = optdur[i].type;
	  /*DM("duration type: %s\n", optdur[i].desc);*/
	  goto optloop;
	}
      }
      /* parameter type */
      tmptype = param_str2code(rdhmmdef_token);
      if (tmptype != F_ERR_INVALID) { /* conv success */
	op->param_type = tmptype;
	/*DM("param type: %s", param_code2str(buf, op->param_type, FALSE));*/
	goto optloop;
      } else {
	/* nothing of above --- not option */
	if(rdhmmdef_token[0] != '~') {
	  j_printerr("Error: unknown option in header: %s\n", rdhmmdef_token);
	  rderr("unknown option in header");
	}
	return;
      }
    }
  optloop:
    read_token(fp);
  }
}

/** 
 * Set global options starting at the current token to %HMM definition data.
 * 
 * @param fp [in] file pointer
 * @param hmm [out] %HMM definition data to store the global options
 */
void
set_global_opt(FILE *fp, HTK_HMM_INFO *hmm)
{
  read_global_opt(fp,&(hmm->opt));
}


/** 
 * Get option name string from its type code.
 * 
 * @param confdata [in] option description data
 * @param type [in] type code to search
 * 
 * @return name string if found, or NULL if not found.
 */
static char *
get_opttype_str(OptionStr *confdata, short type)
{
  int i;
  for (i = 0; confdata[i].name != NULL; i++) {
    if (confdata[i].type == type) {
      return(confdata[i].name);
    }
  }
  rderr("unknown typecode in header!");
  return(NULL);
}

/** 
 * Get covariance option name string from its type code.
 * 
 * @param covtype [in] type code to search
 * 
 * @return the name string if found, or NULL if not found.
 */
char *
get_cov_str(short covtype)
{
  return(get_opttype_str(optcov, covtype));
}

/** 
 * Get duration option name string from its type code.
 * 
 * @param durtype [in] type code to search
 * 
 * @return the name string if found, or NULL if not found.
 */
char *
get_dur_str(short durtype)
{
  return(get_opttype_str(optdur, durtype));
}
