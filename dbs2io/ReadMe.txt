/----------------------------------------------------------------------------
// dbs2io.cpp :
// �w�肳�ꂽ�v���Z�X���N�����AOutputDebugString�̏o�͂��t�b�N���ĕW���o�͂֏o�͂���B
//
// Usege: dbs2io.exe [-exec:execFilename] [-work:currentDirectory] [-exit:exitOnText] [-time:timeOutSec]
// -exec: ���s�t�@�C����
// -work: ���s�t�@�C���̃��[�L���O�f�B���N�g��
// -exit: �I��������B���ꂪ�W���o�͂ɏo���dbs2io�͐���I������B
// -time: �^�C���A�E�g���ԁB���̎��Ԉȏ�̕W���o�͂������ꍇ�Adbs2io�̓G���[�I������B
//
// �g�����Ƃ��ẮB
// 1. �f�o�b�O�o�͂œ��삷��A�v���P�[�V�������Adbs2io�o�R�ŋN������B�f�o�b�O�o�͂��W���o�͂ɗ����悤�ɂȂ�B
// 2. �e�X�g�g�ށB����I�����ɃL�[���[�h���o�͂���悤�ɂ��Ƃ��B����"success"�Ƃ���B
// 3. dbs2io��-exit�ŃL�[���[�h"success"���w�肷��ƁAdbs2io�o�R�Ő���I�������o�ł���悤�ɂȂ�B
// 4. dbs2io��-time�Ŏ��Ԏw�肷��ƁA���炩�̗��R�Ńe�X�g���I���Ȃ��ꍇ�A�G���[�����o�ł���悤�ɂȂ�B
// �Ƃ��������B
// ps3ctr, psp2ctr��������Q�l��Win32/Win64�Ō݊����삷��c�[���A�Ƃ����ʒu�Â��B
/----------------------------------------------------------------------------
