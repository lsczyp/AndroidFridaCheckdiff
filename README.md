# AndroidFridaCheckdiff
 一款基于Frida实现的，用于检测Android app内存中lib库是否被篡改。
 
 参照 [r0ysue/AndroidSecurityStudy](https://github.com/r0ysue/AndroidSecurityStudy)学习frida，以一个工具为目标。<br>
 工具使用了python、JavaScript、java、C/C++多种混合方式，是为了更好的学习，并不是一个理想的开发模式。<br>
 
使用方式：<br>
Main.py 修改package，或以命令行方式传入。<br>
Run.js 设置_scanmodule传入除linker、libdvm/libart以外的lib，Japi.dex、libelf.so需要手动拷贝。<br>

未完成：<br>
frida使用execjs compile被将python中参数，有效传入js。<br>
js包含js，create_script载入js脚本问题。<br>

支持：<br>
可检查代码段、导入表、导出表；code段判断跳转支持arm、thumb、arm64。<br>

效果：<br>
code   /system/lib64/libc.so   abort + 0x0   Arm JmpTo addr: 0x7f9f86a300<br>
mem - 0x7fa38c9f9c: 0x50 0x00 0x00 0x58 0x00 0x02 0x1f 0xd6 0x00 0xa3 0x86 0x9f 0x7f 0x00 <br>
file- 0x7f852dcf9c: 0xfd 0x7b 0xba 0xa9 0xfd 0x03 0x00 0x91 0xf3 0x53 0x01 0xa9 0xb3 0xe3 <br>
