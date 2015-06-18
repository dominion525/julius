/**
 * @file   confout.c
 * @author Akinobu LEE
 * @date   Thu Feb 17 15:34:39 2005
 * 
 * <JA>
 * @brief  コンパイル時の設定をテキスト出力する
 * </JA>
 * 
 * <EN>
 * @brief  Output compilation time configurations strings.
 * </EN>
 * 
 * $Revision: 1.2 $
 * 
 */
/*
 * Copyright (c) 1991-2006 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2006 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/util.h>

/** 
 * Output version of this libsent library.
 * 
 * @param strm [in] file pointer to output
 */
void
confout_version(FILE *strm)
{
  fprintf(strm, "Rev.%s\n", LIBSENT_VERSION);
}

/** 
 * Output audio configuration of this libsent library.
 * 
 * @param strm [in] file pointer to output
 */
void
confout_audio(FILE *strm)
{
  fprintf(strm, "- Audio input\n");
#ifdef USE_MIC
  fprintf(strm, "    mic device API          : %s (%s)\n", AUDIO_API_NAME, AUDIO_API_DESC);
#else
  fprintf(strm, "    mic device API          : N/A\n");
#endif
  fprintf(strm, "    file format supported   : %s\n", AUDIO_FORMAT_DESC);
#ifdef USE_NETAUDIO
  fprintf(strm, "    NetAudio                : yes\n");
#else
  fprintf(strm, "    NetAudio                : no\n");
#endif
}

/** 
 * Output language model configuration of this libsent library.
 * 
 * @param strm [in] file pointer to output
 */
void
confout_lm(FILE *strm)
{
  fprintf(strm, "- Language Model\n");
#ifdef CLASS_NGRAM
  fprintf(strm, "    class N-gram            : yes\n");
#else
  fprintf(strm, "    class N-gram            : no\n");
#endif
#ifdef WORDS_INT
  fprintf(strm, "    word id unit            : integer (%d bytes)\n", sizeof(WORD_ID));
#else
  fprintf(strm, "    word id unit            : short (%d bytes)\n", sizeof(WORD_ID));
#endif
}

/** 
 * Output acoustic model configuration of this libsent library.
 * 
 * @param strm [in] file pointer to output
 */
void
confout_am(FILE *strm)
{
  fprintf(strm, "- Acoustic Model\n");
#ifdef MULTIPATH_VERSION
  fprintf(strm, "    multi-path HMM handling : yes\n");
#else
  fprintf(strm, "    multi-path HMM handling : no\n");
#endif
}

/** 
 * Output about linked libraries of this libsent library.
 * 
 * @param strm [in] file pointer to output
 */
void
confout_lib(FILE *strm)
{
  fprintf(strm, "- Libraries\n");
  fprintf(strm, "    file decompression by   : %s\n", GZIP_READING_DESC);
  fprintf(strm, "    charset conversion by   : %s\n", CHARSET_CONVERSION_DESC);
}

/** 
 * Output about process handling of this libsent library.
 * 
 * @param strm [in] file pointer to output
 */
void
confout_process(FILE *strm)
{
  fprintf(strm, "- Process\n");
#ifdef HAVE_PTHREAD
  fprintf(strm, "    use POSIX thread        : yes\n");
#else
  fprintf(strm, "    use POSIX thread        : no\n");
#endif
#ifdef FORK_ADINNET
  fprintf(strm, "    fork on adinnet input   : yes\n");
#else
  fprintf(strm, "    fork on adinnet input   : no\n");
#endif
}

/** 
 * Output all information of this libsent library.
 * 
 * @param strm [in] file pointer to output
 */
void
confout(FILE *strm)
{
  confout_version(strm);
  confout_audio(strm);
  confout_lm(strm);
  confout_am(strm);
  confout_lib(strm);
  confout_process(strm);
}
