ADINTOOL(1)                                                        ADINTOOL(1)



NAME
       adintool  -  audio  tool  to  record/split/send/receive speech data for
       Julius

SYNOPSIS
       adintool -in inputdev -out outputdev [options...]

DESCRIPTION
       adintool �́C�����g�`�f�[�^���̉�����Ԃ̌��o����ыL�^��A���I�� �s ��
       �c�[ ���ł��D���͉����ɑ΂��ė�������ƐU�����x���Ɋ�Â�������Ԍ��o��
       �����s���C������ԕ�����A���o�͂��܂��D

       adintool �� adinrec �̍��@�\�łł��D�����f�[�^�̓��͌��Ƃ��āC�}�C�N ��
       �́E �� ���g�`�t�@�C���E�W�����́E�l�b�g���[�N����(adinnet �T�[�o�[���[
       �h)���I���ł��܂��D�܂��C�o�͐�Ƃ��āC�����g�`�t�@�C���E�W���o�́E�l�b
       �g ���[�N�o��(adinnet �N���C�A���g���[�h)���I���ł��܂��D���Ƀl�b�g���[
       �N�o�́iadinnet �N���C�A���g���[�h�j�ł́C julius �փl�b�g���[�N�o�R ��
       �����𑗐M���ĉ����F�������邱�Ƃ��ł��܂��D

       �� �͉����͉�����Ԃ��ƂɎ�����������C�����o�͂���܂��D������Ԃ̐؂�
       �o���ɂ� adinrec �Ɠ����C��莞�ԓ��̗�������ƃp���[�i�U�����x���j�̂�
       �� ���l��p���܂��D������ԊJ�n�Ɠ����ɉ����o�͂��J�n����܂��D�o�͂Ƃ�
       �ăt�@�C���o�͂�I�񂾏ꍇ�́C�A�ԃt�@�C�����Ō��o���ꂽ��Ԃ��Ƃɕ� ��
       ���܂��D

       �T���v�����O���g���͔C�ӂɐݒ�\�ł��D�`���� 16bit monoral �ł��D����
       �o�����f�[�^�`���� WAV, 16bit, ���m�����ł��D���ɓ������O�̃t�@�C����
       ���݂���ꍇ�͏㏑�����܂��D

INPUT
       ������ǂݍ��ޓ��̓f�o�C�X�͈ȉ��̂����ǂꂩ���w�肵�܂��D

       -in mic
              �}�C�N���́i�f�t�H���g�j�D

       -in file
              �����g�`�t�@�C���D�`���� RAW (16bit big endian)�CWAV(�����k)�Ȃ�
              �i�R���p�C�����̐ݒ�ɂ��j�D
              �Ȃ��C���̓t�@�C�����͋N����ɁC�v�����v�g�ɑ΂��ē��͂���D

       -in adinnet
              adinnet �T�[�o�[�ƂȂ��ăl�b�g���[�N�o�R�� adinnet �N���C�A�� �g
              �� �特���f�[�^���󂯎��D adinnet �N���C�A���g�����TCP/IP�ڑ�
              ��҂��C�ڑ����m��������� adinnet �N���C�A���g���特���f�[�^ ��
              �󂯎��D
              �|�[ �g �ԍ��̃f�t�H���g�� 5530 �ł���D����̓I�v�V���� "-port"
              �ŕύX�\�D

       -in netaudio
              (�T�|�[�g����Ă���΁j�����f�[�^��Netaudio/DatLink�T�[�o�[�� ��
              �� �����D�T�[�o�̃z�X�g���ƃ��j�b�g���� "-NA host:unit" �Ŏw��
              ����K�v������D

       -in stdin
              �W�����́D�����f�[�^�`���� RAW, WAV �̂݁D

OUTPUT
       ���o����������Ԃ̉����f�[�^�������o���o�̓f�o�C�X�Ƃ��āC�ȉ��̂��� ��
       �ꂩ���w�肵�܂��D

       -out file
              �t�@ �C �� �� �o�͂���D�o�̓t�@�C�����͕ʂ̃I�v�V����"-filename
              foobar" �̌`�ŗ^����D���ۂɂ� "foobar.0000" , "foobar.0001" ...
              �� �悤�ɋ�Ԃ��ƂɁC�w�肵�����O�̖�����4����ID���������O�ŋL
              �^����Ȃ�D ID �� 0 �������l�Ƃ��āC������Ԍ��o���ƂɂP���� ��
              ��D �� ���l�̓I�v�V���� "-startid" �ŕύX�\�ł���D�܂��C�o��
              �t�@�C���`���� 16bit WAV �`���ł���D RAW �`���ŏo�� �� �� �� ��
              "-raw" �I�v�V�������w�肷��D

       -out adinnet
              adinnet  �N���C�A���g�ƂȂ��āC�l�b�g���[�N�o�R�� adinnet �T�[�o
              �։����f�[�^�𑗂�D���͂̎��Ƃ͋t�ɁC adintool �� adinnet �N ��
              �C �A �� �g �ƂȂ�Cadinnet �T�[�o�[�֐ڑ���C�����f�[�^�𑗐M��
              ��Dadinnet �T�[�o�[�Ƃ��ẮC adintool ����� Julius  ��adinnet
              ���͂���������D
              "-server"  �ő��M���adinnet�T�[�o�̃z�X�g�����w�肷��D�܂��|�[
              �g�ԍ��̃f�t�H���g�� 5530 �ł���D����̓I�v�V���� "-port" �� ��
              �X�\�D

       -out stdout
              �W �� �o�͂֏o�͂���D�`���� RAW, 16bit signed (big endian) �ł�
              ��D


OPTIONS
       -nosegment
              ���͉����ɑ΂��ĉ�����Ԃ̌��o���s�킸�C���̂܂܏o�͂փ��_�C���N
              �g ����D�t�@�C���o�͂̏ꍇ�C�t�@�C�����̖�����4����ID�͕t�^����
              �Ȃ��Ȃ�D

       -oneshot
              ���͊J�n��C��ԍŏ��̂P������Ԃ݂̂𑗐M��C�I������D

       -freq threshold
              �T���v�����O���g���D�P�ʂ� Hz (default: 16000)

       -lv threslevel
              �g�`�̐U�����x���̂������l (0 - 32767)�D(default: 2000)�D

       -zc zerocrossnum
              �P�b������̗�������̂������l (default: 60)

       -headmargin msec
              ������ԊJ�n���̒��O�̃}�[�W���D�P�ʂ̓~���b (default: 400)

       -tailmargin msec
              ������ԏI�����̒���̃}�[�W���D�P�ʂ̓~���b (default: 400)

       -nostrip
              ������ 0 �T���v���̎����������s��Ȃ��悤�ɂ���D�f�t�H���g�� ��
              ���������s���D

       -zmean DC�����������s���D

       -raw   �t�@�C���o�͌`���� RAW, 16bit signed (big endian) �ɂ���D�f�t�H
              ���g�� WAV �`���ł���D

EXAMPLE
       �}�C�N����̉������͂��C���b���Ƃ� "data.0000.wav" ���珇�ɋL�^����F

           % adintool -in mic -out file -filename data

       ����Ȏ��^�����t�@�C�� "foobar.raw"��������Ԃ��� ��  "foobar.1500.wav"
       "foobar.1501.wav" ... �ɕ�������F

           % adintool -in file -out file -filename foobar
             -startid 1500
             (�N����v�����v�g�ɑ΂��ăt�@�C���������)
             enter filename->foobar.raw

       �l�b�g���[�N�o�R�ŉ����t�@�C����]������(��Ԍ��o�Ȃ�)�F

         [��M��]
           % adintool -in adinnet -out file -nosegment
         [���M��]
           % adintool -in file -out adinnet -server hostname
             -nosegment

       �}�C�N����̓��͉�����ʃT�[�o�[�� Julius �ɑ���F

       (1) ���̓f�[�^��S�đ��M���CJulius���ŋ�Ԍ��o�E�F���F

         [Julius]
           % julius -C xxx.jconf ... -input adinnet
         [adintool]
           % adintool -in mic -out adinnet -server hostname
             -nosegment

       (2)  ���̓f�[�^�̓N���C�A���g(adintool)���ŋ�Ԍ��o���C���o������Ԃ���
       ������ Julius �֑��M�E�F���F

         [Julius]
           % julius -C xxx.jconf ... -input adinnet
         [adintool]
           % adintool -in mic -out adinnet -server hostname

SEE ALSO
       julius(1), adinrec(1)

BUGS
       �o�O�񍐁E�₢���킹�E�R�����g�Ȃǂ�
       julius-info at lists.sourceforge.jp �܂ł��肢���܂��D

VERSION
       This version is provided as part of Julius-3.5.1.

COPYRIGHT
       Copyright (c)   1991-2006 ���s��w �͌�������
       Copyright (c)   2000-2005 �ޗǐ�[�Ȋw�Z�p��w�@��w ���쌤����
       Copyright (c)   2005-2006 ���É��H�Ƒ�w Julius�J���`�[��

AUTHORS
       �� �W�L (���É��H�Ƒ�w) ���������܂����D

LICENSE
       Julius �̎g�p�����ɏ����܂��D



4.3 Berkeley Distribution            LOCAL                         ADINTOOL(1)