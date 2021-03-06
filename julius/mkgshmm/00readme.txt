MKGSHMM(1)                                                          MKGSHMM(1)



NAME
       mkgshmm - convert monophone HMM to GS HMM for Julius

SYNOPSIS
       mkgshmm monophone_hmmdefs > outputfile

DESCRIPTION
       mkgshmm  converts monophone HMM definition file (hmmdefs) in HTK format
       to a special format for Gaussian Mixture Selection (GMS) in Julius.

       GMS is an algorithm to reduce the amount of acoustic  computation  with
       triphone  HMM,  by  pre-selection  of promising gaussian mixtures using
       likelihoods of corresponding monophone mixtures.

       For more details, please consult related documents and papers

EXAMPLE
       (1) Prepare a monophone model which was trained by the same  corpus  as
       target triphone model.

       (2) Convert the monophone model using mkgshmm.

           % mkgshmm monophone > gshmmfile

       (3) Specify the output file in Julius with option "-gshmm"

           % julius -C foo.jconf -gshmm gshmmfile


SEE ALSO
       julius(1)

VERSION
       This version is provided as part of Julius-3.5.1.

COPYRIGHT
       Copyright (c) 2001-2005 Kawahara Lab., Kyoto University
       Copyright  (c)  2001-2005  Shikano  Lab., Nara Institute of Science and
       Technology
       Copyright (c) 2005      Julius project team, Nagoya Institute of  Tech-
       nology

AUTHORS
       LEE Akinobu (Nagoya Institute of Technology, Japan)
       contact: julius-info at lists.sourceforge.jp

LICENSE
       Same as Julius.



4.3 Berkeley Distribution            LOCAL                          MKGSHMM(1)
