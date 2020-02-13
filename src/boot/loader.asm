org 0x100
    jmp START

BaseOfStack		equ	0x100	; 调试状态下堆栈基地址(栈底, 从这个位置向低地址生长)

;============================================================================
;   16位实模式代码段
;----------------------------------------------------------------------------
START:
    ; 寄存器复位
    mov	ax, cs
    mov	ds, ax
    mov	es, ax
    mov	ss, ax
    mov	sp, BaseOfStack

    ; 显示字符串 "Hello Loader!"
    mov	dh, 0
    call DispStr		; 显示字符串

    ; 检查并得到内存信息
    mov ebx, 0          ; ebx = 得到后续的内存信息的值，第一次必须为0，将获取第一个ADRS
    mov di, _MemChkBuf  ; es:di -> 指向准备写入ADRS的缓冲区地址
MemChkLoop:
    mov eax, 0x0000e820 ; eax = 0x0000e820
    mov ecx, 20         ; ecx = ADRS的大小
    mov edx, 0x0534d4150; "SMAP"
    int 0x15            ; 得到ADRS
    jc MemChkFail       ; 产生了一个进位，CF=1，检查得到ADRS错误！
    ; CF = 0，检查并获取成功
    add di, 20          ; di += 20，es:di指向缓冲区准备放入下一个ADRS的地址
    inc dword [_ddMCRCount] ; ADRS数量++
    cmp ebx, 0
    je MemChkFinish         ; ebx == 0，表示已经拿到最后一个ADRS了，完成检查并跳出循环
    ; ebx != 0，表示还没拿到最后一个，继续
    jmp MemChkLoop
MemChkFail:
    mov dword [_ddMCRCount], 0  ; 检查失败，ADRS数量设置为0

    mov dh, 1
    call DispStr    ; 打印"Mem Chk Fail!"
    jmp $           ; 死循环

MemChkFinish:

    mov dh, 2
    call DispStr    ; 打印"Mem Check OK"
    jmp $           ; 死循环


;============================================================================
; 要显示的字符串
;----------------------------------------------------------------------------
LoaderFileName		db	"LOADER  BIN", 0	; LOADER.BIN 之文件名
; 为简化代码, 下面每个字符串的长度均为 MessageLength
MessageLength		equ	13
Message:		    db	"Hello Loader!"  ; 12字节, 不够则用空格补齐. 序号 0
                    db  "Mem Chk Fail!"
                    db  "Mem Check OK!"
;============================================================================
;----------------------------------------------------------------------------
; 函数名: DispStr
;----------------------------------------------------------------------------
; 作用:
;	显示一个字符串, 函数开始时 dh 中应该是字符串序号(0-based)
DispStr:
	mov	ax, MessageLength
	mul	dh
	add	ax, Message
	mov	bp, ax			; ┓
	mov	ax, ds			; ┣ ES:BP = 串地址
	mov	es, ax			; ┛
	mov	cx, MessageLength	; CX = 串长度
	mov	ax, 01301h		; AH = 13,  AL = 01h
	mov	bx, 0007h		; 页号为0(BH = 0) 黑底白字(BL = 07h)
	mov	dl, 0
	int	10h			; int 10h
	ret
;============================================================================
;   32位数据段
;----------------------------------------------------------------------------
[section .data32]
align 32
DATA32:
;----------------------------------------------------------------------------
;   16位实模式下的数据地址符号
;----------------------------------------------------------------------------
_ddMCRCount:        dd 0        ; 检查完成的ADRS的数量，为0则代表检查失败
_ddMemSize:         dd 0        ; 内存大小
; 地址范围描述符结构(Address Range Descriptor Structure)
_ADRS:
    _ddBaseAddrLow:  dd 0        ; 基地址低32位
    _ddBaseAddrHigh: dd 0        ; 基地址高32位
    _ddLengthLow:    dd 0        ; 内存长度（字节）低32位
    _ddLengthHigh:   dd 0        ; 内存长度（字节）高32位
    _ddType:         dd 0        ; ADRS的类型，用于判断是否可以被OS使用
; 内存检查结果缓冲区，用于存放没存检查的ADRS结构，256字节是为了对齐32位，256/20=12.8
; ，所以这个缓冲区可以存放12个ADRS。
_MemChkBuf:          times 256 db 0
;----------------------------------------------------------------------------
;   32位模式下的数据地址符号
;----------------------------------------------------------------------------


;============================================================================
