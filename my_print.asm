global mprint
global change_color
global back_color
mprint:
;sys_write
	mov		edx,[esp+8];len 右边的参数
	mov		ecx,[esp+4];字符串地址	左边的
	mov		ebx,1;function fd 为1代表stdout
	mov		eax,4;sys_write linux中是4
	int		0x80
	ret
;系统调用号在eax 参数存在其他的register中
;如果有返回值都保存在eax中

