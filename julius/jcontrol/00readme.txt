JCONTROL(1)                                                        JCONTROL(1)



NAME
       jcontrol - simple program to control Julius / Julian module via API

SYNOPSIS
       jcontrol hostname [portnum]

DESCRIPTION
       jcontrol is a simple console program to control julius running on other
       host via network API.  It can send command to Julius, and receive  mes-
       sages from Julius.

       When  invoked, jcontrol tries to connect to Julius or Julian running in
       "module mode" on specified  hostname.   After  connection  established,
       jcontrol waits for user commands from standard input.

       When  user types a command to jcontrol, it will be interpreted and cor-
       responding API command will be sent  to  Julius.   When  a  message  is
       received from Julius, its content will be output to standard output.

       For details about the API, see the related documents.

OPTIONS
       hostname
              Host name where Julius is runnning in module mode.

       portnum
              (optional) port number. (default=10500)

COMMANDS (COMMON)
       After startup, the command string below can be input from stdin.

       pause  Stop recognition, cutting speech input at that point if any.

       terminate
              Stop recognition, discarding the current speech input if any.

       resume (re)start recognition.

       inputparam arg
              Tell  Julius  how  to  deal with speech input in case grammar is
              changed just when recognition is running.  Specify one:  "TERMI-
              NATE", "PAUSE", "WAIT"

       version
              Return version number.

       status Return trigger status (active/sleep).

GRAMMAR COMMANDS (Julian)
       Below are Grammar-related command strings for Julian:

       changegram prefix
              Change  recognition  grammar  to "prefix.dfa" and "prefix.dict".
              All the current grammars used in Julian are deleted and replaced
              to the specifed grammar.

       addgram prefix
              tell  Julian  to  use  additional grammar "prefix.dfa" and "pre-
              fix.dict" for recognition.  The specified grammars are added  to
              the list of recognition grammars, and then activated.

       deletegram ID
              tell  Julian  to  delete  grammar  of  the  specified "ID".  The
              deleted grammar will be erased from Julian.  The grammar "ID" is
              sent from Julian at each time grammar information has changed.

       deactivategram ID
              tell  Julian  to  de-activate  a grammar.  The specified grammar
              will become temporary OFF, and skipped from recognition process.
              These de-activated grammars are kept in Julian, and can be acti-
              vated by "activategram" command.

       activategram ID
              tell Julian to activate previously de-activated grammar.

EXAMPLE
       The dump messages from Julius / Julian are output to tty with prefix ">
       " appended to each line.

       See related documents for more details.

       (1) start Julius / Julian in module mode at host 'host'.
           % julian -C xxx.jconf ... -input mic -module

       (2) (on other tty) start jcontrol, and start communication.
           % jcontrol host
           connecting to host:10500...done
           > <GRAMINFO>
           >  # 0: [active] 99words, 42categories, 135nodes (new)
           > </GRAMINFO>
           > <GRAMINFO>
           >  # 0: [active] 99words, 42categories, 135 nodes
           >   Grobal:      99words, 42categories, 135nodes
           > </GRAMINFO>
           > <INPUT STATUS="LISTEN" TIME="1031583083"/>
        -> pause
        -> resume
           > <INPUT STATUS="LISTEN" TIME="1031583386"/>
        -> addgram test
           ....


SEE ALSO
       julius(1), julian(1)

VERSION
       This version is provided as part of Julius-3.5.1.

COPYRIGHT
       Copyright (c) 2002-2006 Kawahara Lab., Kyoto University
       Copyright  (c)  2002-2005  Shikano  Lab., Nara Institute of Science and
       Technology
       Copyright (c) 2005-2006 Julius project team, Nagoya Institute of  Tech-
       nology

AUTHORS
       LEE Akinobu (Nagoya Institute of Technology, Japan)
       contact: julius-info at lists.sourceforge.jp

LICENSE
       Same as Julius.



4.3 Berkeley Distribution            LOCAL                         JCONTROL(1)
