; 引导程序，加载FlyanxOS的第一步: boot　其做一些系统的初始化工作,然后寻找启动盘中的Loader,并加载它,使命完成
;================================================================================================
;%define	_BOOT_DEBUG_	; 做 Boot Sector 时一定将此行注释掉!将此行打开后用 nasm Boot.asm -o Boot.com 做成一个.COM文件易于调试
%ifdef	_BOOT_DEBUG_
	org  0100h			; 调试状态, 做成 .COM 文件, 可调试
%else
	org  07c00h			; Boot 状态, Bios 将把 Boot Sector 加载到 0:7C00 处并开始执行
%endif

; LOADER加载到的段地址
LOADER_SEG      equ 0x9000
; LOADER加载到的偏移地址
LOADER_OFFSET   equ 0x100
;================================================================================================
%ifdef	_BOOT_DEBUG_
    BaseOfStack		equ	0100h	; 调试状态下堆栈基地址(栈底, 从这个位置向低地址生长)
%else
    BaseOfStack		equ	07c00h	; Boot状态下堆栈基地址(栈底, 从这个位置向低地址生长)
%endif
;================================================================================================
	jmp short LABEL_START		; 跳转到程序开始处
	nop							; 这个 nop 不可少
    ; 导入FAT12头以及相关常量信息
    %include "fat12hdr.inc"
;================================================================================================
; 程序入口
;----------------------------------------------------------------------------
LABEL_START:
    ; 寄存器复位
    mov	ax, cs
	mov	ds, ax
	mov	es, ax
	mov	ss, ax
	mov	sp, BaseOfStack

    ; 清屏,清理BIOS的输出
    mov	ax, 0x0600		; AH = 6,  AL = 0h
    mov	bx, 0x0700		; 黑底白字(BL = 07h)
    mov	cx, 0			; 左上角: (0, 0)
    mov	dx, 0x0184f		; 右下角: (80, 50)
    int	0x10				; int 10h

    ; 显示字符串 "Booting.....  "
    mov	dh, 0			; "Booting.....  "
    call	DispStr		; 显示字符串

    ; 操作软盘前，现将软驱复位
    xor ah, ah          ; xor:异或，ah = 0
    xor dl, dl          ; dl = 0
    int 0x13

    ; 接下来我们在软盘A中开始寻找文件
    mov word [wSector], SectorNoOfRootDirectory     ; 读取软盘的根目录扇区号
SEARCH_FILE_IN_ROOR_DIR_BEGIN:
    cmp word [wRootDirSizeLoop], 0
    jz NO_FILE                  ; 读完了整个根目录扇区都没找到，所以没有
    dec word [wRootDirSizeLoop] ; wRootDirSizeLoop--

    ; 读取扇区
    mov ax, LOADER_SEG  ; es = LOADER_SEG
    mov es, ax
    mov bx, LOADER_OFFSET
    mov ax, [wSector]
    mov cl, 1
    call ReadSector

    mov si, LoaderFileName      ; ds:si -> Loader的文件名称
    mov di, LOADER_OFFSET       ; es:di -> LOADER_SEG:LOADER_OFFSET -> 加载到内存中的扇区数据
    cld     ; 字符串比较方向，si、di方向向右

    ; 开始在扇区中寻找文件，比较文件名
    mov dx, 16                  ; 一个扇区512字节，FAT目录项是32位，512/32 = 16，一个扇区有512个目录项
SEARCH_FOR_FILE:
    cmp dx, 0
    jz NEXT_SECTOR_IN_ROOT_DIR                 ; 读完整个扇区，依旧没找到，准备加载下一个扇区
    dec dx              ; dx--
    ; 应该开始比较目录项中的文件名了
    mov cx, 11
CMP_FILENAME:
    cmp cx, 0
    jz FILENAME_FOUND           ; cx = 0，整个文件名里的字符都匹配上了，我们发现它了
    dec cx          ; cx--
    lodsb           ; ds:si -> al, si++
    cmp al, byte [es:di]    ; 比较字符
    je GO_ON                ; 字符相同，准备继续比较下一个
    jmp DIFFERENT           ; 只要有一个字符不相同，就表明本目录项不是我们要寻找的文件的目录项

GO_ON:
    inc di
    jmp  CMP_FILENAME
DIFFERENT:
    and di, 0xfff0      ; di &= f0, 11111111 11110000，是为了让它指向本目录项条目的开始。

    add di, 32          ; di += 32， 让di指向下一个目录项
    mov si, LoaderFileName
    jmp SEARCH_FOR_FILE     ; 重新开始在下一个目录项中查找文件并比较

NEXT_SECTOR_IN_ROOT_DIR:
    add word [wSector], 1   ; 准备开始读取下一个扇区
    jmp SEARCH_FILE_IN_ROOR_DIR_BEGIN

FILENAME_FOUND:
    mov dh, 1
    call DispStr    ; 打印"Fount  it!"
    ; 死循环
    jmp $
NO_FILE:
    mov dh, 2
    call DispStr    ; 打印"NO LOADER!"

    ; 死循环
    jmp $

;============================================================================
; 变量
wRootDirSizeLoop    dw      RootDirSectors  ; 根目录占用的扇区数，在循环中将被被逐步递减至0
wSector             dw      0               ; 要读取的扇区号
;============================================================================
; 要显示的字符串
;----------------------------------------------------------------------------
LoaderFileName		db	"LOADER  BIN", 0	; LOADER.BIN 之文件名
; 为简化代码, 下面每个字符串的长度均为 MessageLength
MessageLength		equ	10
BootMessage:		db	"Booting..."  ; 12字节, 不够则用空格补齐. 序号 0
                    db  "Fount  it!"
                    db  "NO LOADER!"
;============================================================================
;----------------------------------------------------------------------------
; 函数名: DispStr
;----------------------------------------------------------------------------
; 作用:
;	显示一个字符串, 函数开始时 dh 中应该是字符串序号(0-based)
DispStr:
	mov	ax, MessageLength
	mul	dh
	add	ax, BootMessage
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
;----------------------------------------------------------------------------
; 函数名: ReadSector
;----------------------------------------------------------------------------
; 作用:
;	从第 ax 个 Sector 开始, 将 cl 个 Sector 读入 es:bx 中
ReadSector:
	; -----------------------------------------------------------------------
	; 怎样由扇区号求扇区在磁盘中的位置 (扇区号 -> 柱面号, 起始扇区, 磁头号)
	; -----------------------------------------------------------------------
	; 设扇区号为 x
	;                           ┌ 柱面号 = y >> 1
	;       x           ┌ 商 y ┤
	; -------------- => ┤      └ 磁头号 = y & 1
	;  每磁道扇区数       │
	;                   └ 余 z => 起始扇区号 = z + 1
	push	bp
	mov	bp, sp
	sub	esp, 2			; 辟出两个字节的堆栈区域保存要读的扇区数: byte [bp-2]

	mov	byte [bp-2], cl
	push	bx			; 保存 bx
	mov	bl, [BPB_SecPerTrk]	; bl: 除数
	div	bl			; y 在 al 中, z 在 ah 中
	inc	ah			; z ++
	mov	cl, ah			; cl <- 起始扇区号
	mov	dh, al			; dh <- y
	shr	al, 1			; y >> 1 (其实是 y/BPB_NumHeads, 这里BPB_NumHeads=2)
	mov	ch, al			; ch <- 柱面号
	and	dh, 1			; dh & 1 = 磁头号
	pop	bx			; 恢复 bx
	; 至此, "柱面号, 起始扇区, 磁头号" 全部得到 ^^^^^^^^^^^^^^^^^^^^^^^^
	mov	dl, [BS_DrvNum]		; 驱动器号 (0 表示 A 盘)
.GoOnReading:
	mov	ah, 2				; 读
	mov	al, byte [bp-2]		; 读 al 个扇区
	int	13h
	jc	.GoOnReading		; 如果读取错误 CF 会被置为 1, 这时就不停地读, 直到正确为止

	add	esp, 2
	pop	bp

	ret
;============================================================================
; times n m        n：重复多少次   m：重复的代码
times 510-($-$$)   db    0  ; 填充剩下的空间，使生成的二进制代码恰好为512字节
dw  0xaa55                  ; 可引导扇区结束标志，必须是55aa，不然 BIOS 无法识别
;============================================================================
