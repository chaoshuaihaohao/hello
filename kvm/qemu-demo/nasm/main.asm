[section .data]  

[section .code]  
global _start        ; 导出 _start 标记，供链接器识别程序入口  

_start:  
mov al,0x48
out byte 0xf1,al
mov al,0x65
out 0xf1,al
mov al,0x6c
out 0xf1,al
mov al,0x6c
out 0xf1,al
mov al,0x6f
out 0xf1,al
mov al,0x0a
out 0xf1,al

hlt

