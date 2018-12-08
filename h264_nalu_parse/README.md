# 雷神的例子,熟悉h264结构用的
 有两个h264码流文件.
 1.tiny_h264_stream.h264, 一个只有6字节的文件, 用来观察到达末尾时. pos-1怎么算的.
 2.animation_example.h264, 一个正常长度的h264码流文件.
 编译选项 clang -g h264_parse.c -o h264_parse
 然后用lldb -- h264_parse tiny_h264_stream.h264, 即可调试
 感谢雷神!
