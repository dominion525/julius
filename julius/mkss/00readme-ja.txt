MKSS(1)                                                                MKSS(1)



NAME
       mkss - compute average spectrum from mic input for SS

SYNOPSIS
       mkss [options..] filename

DESCRIPTION
       mkss �͎w�莞�ԕ��̉����}�C�N���͂���^�����C���̕��σX�y�N�g�������t�@
       �C���ɏo�͂���c�[���ł��D�o�͂��ꂽ�t�@�C���� Julius �ŃX�y�N�g���T �u
       �g ���N�V�����̂��߂̃m�C�Y�X�y�N�g���t�@�C���i�I�v�V���� "-ssload"�j��
       ���ė��p�ł��܂��D

       �����̐؂�o���͍s�킸�C�N���Ɠ����ɘ^�����n�߂܂��D

       �T���v�����O������16bit signed short (big endian), monoral �ŌŒ�ł��D
       �f�[�^�`����RAW(�w�b�_����)�Cbig endian�`���ł��D���ɓ������O�̃t�@�C��
       �����݂���ꍇ�͏㏑�����܂��D

       �Ȃ��t�@�C������ "-" ���w�肷�邱�ƂŕW���o�͂֏o�͂��邱�Ƃ��ł��܂��D

OPTIONS
       -freq threshold
              �T���v�����O���g����Hz�Ŏw�肷��D(default: 16000)

       -len msec
              �^�����Ԓ����~���b�P�ʂŎw�肷�� (default: 3000)�D

       -fsize samplenum
              ���͂̃t���[���T�C�Y���T���v�����Ŏw�肷�� (default: 400)�D

       -fshift samplenum
              ���͂̃t���[���V�t�g���T���v�����Ŏw�肷�� (default: 160)�D

SEE ALSO
       julius(1)

BUGS
       �o�O�񍐁E�₢���킹�E�R�����g�Ȃǂ�
       julius-info at lists.sourceforge.jp �܂ł��肢���܂��D

VERSION
       This version is provided as part of Julius-3.5.1.

COPYRIGHT
       Copyright (c) 2002-2006 ���s��w �͌�������
       Copyright (c) 2002-2005 �ޗǐ�[�Ȋw�Z�p��w�@��w ���쌤����
       Copyright (c) 2005-2006 ���É��H�Ƒ�w Julius�J���`�[��

AUTHORS
       �� �W�L (���É��H�Ƒ�w) ���������܂����D

LICENSE
       Julius �̎g�p�����ɏ����܂��D



4.3 Berkeley Distribution            LOCAL                             MKSS(1)