2006.08.05
successfully compiled under:
Visual Studio 6 Interprise Edition(vc6) 
Visual C++ 2005 professional(vc8)
Intel C++ Compiler 8.0

corresponding x264 version: Rev.551
Every modification is remarked with //lsp...
Any problem about my x264 for vc, please send me mail or submit the question on my blog: http://blog.csdn.net/sunshine1314

ps: x264 changelog
https://trac.videolan.org/x264/log/trunk/

��x264 cli����ѡ����͡�
1.��򵥵ķ�ʽ
x264 -o test.264 infile.yuv widthxheight
example:
x264 -o test.264 foreman.cif 352x288
note: 352x288�м����Ӣ����ĸx��������*��������һЩ���ѻ����ˣ��������ڴ˶�һ������һ�¡�

2.���������̶�
--qp ??
example:
x264 --qp 26 -o test.264 foreman.cif 352x288

3.����cavlc
--no-cabac 
note: Ĭ���ǲ���cabac

4.���ñ���֡��
--frames ?? 
example:
x264 --frames 10 --qp 26 -o test.264 foreman.cif 352x288

5. ���òο�֡��Ŀ
--ref ??
example:
x264 -- ref 2 --frames 10 --qp 26 -o test.264 foreman.cif 352x288

6. ����dvdrip�ľ���ѡ����ʿ��ƣ�����B֡���������
example:
x264.exe --frames 10 --pass 1 --bitrate 1500 --ref 2 --bframes 2 --b-pyramid --b-rdo --bime --weightb --filter -2,-2 --trellis 1 --analyse all --8x8dct --progress -o test1.264 foreman.cif 352x288

x264.exe --frames 10 --pass 2 --bitrate 1500 --ref 2 --bframes 2 --b-pyramid --b-rdo --bime --weightb --filter -2,-2 --trellis 1 --analyse all --8x8dct --progress -o test2.264 foreman.cif 352x288

7. ��ʾ�������
--progress



                �q�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�r
                �U                              �U
  �q�T�T�T�T�T�T��           Peter Lee          ���T�T�T�T�T�T�r
  �U            �U                              �U            �U
  �U            �t�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�s            �U
���U                                                          �U
���U                                                          �U
���U�� I'am happy of talking with you about video coding,     �U
���U             transmission, vod and so on.                 �U
  �U       Welcom to MY homepage:  videosky.9126.com          �U
  �U                                                          �U
���U                                                          �U
  �U    �q�����������������������������������������������r    �U
  �t�T�T��               lspbeyond@126.com              ���T�T�s
        �t�����������������������������������������������s

