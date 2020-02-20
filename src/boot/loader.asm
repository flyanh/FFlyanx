; 加载FlyanxOS的第二步: Loader　跳入保护模式，并加载真正的系统内核
; 这个 加载文件　突破了引导扇区 512 字节的限制(但还是在64K的限制内，但对我们完全够用)，我们可以在这里完成很多事情，所以较为重要
org 0x100
    jmp START           ; 程序开始处
;============================================================================
;   头文件
;----------------------------------------------------------------------------
%include "load.inc"         ; 挂载点相关的信息
%include "fat12hdr.inc"     ; 导入它是因为需要这些信息来加载内核文件
%include "pm.inc"           ; 保护模式的一些信息，各种宏和变量
;============================================================================
;   GDT全局描述符表相关信息以及堆栈信息
;----------------------------------------------------------------------------
; 描述符                        基地址        段界限       段属性
LABEL_GDT:			Descriptor	0,          0,          0							; 空描述符，必须存在，不然CPU无法识别GDT
LABEL_DESC_CODE:	Descriptor	0,          0xfffff,    DA_32 | DA_CR | DA_LIMIT_4K	; 0~4G，32位可读代码段，粒度为4KB
LABEL_DESC_DATA:    Descriptor  0,          0xfffff,    DA_32 | DA_DRW | DA_LIMIT_4K; 0~4G，32位可读写数据段，粒度为4KB
LABEL_DESC_VIDEO:   Descriptor  0xb8000,    0xfffff,    DA_DRW | DA_DPL3            ; 视频段，特权级3（用户特权级）
; GDT全局描述符表 -------------------------------------------------------------
GDTLen              equ $ - LABEL_GDT                           ; GDT的长度
GDTPtr              dw GDTLen - 1                               ; GDT指针.段界限
                    dd LOADER_PHY_ADDR + LABEL_GDT              ; GDT指针.基地址
; GDT选择子 ------------------------------------------------------------------
SelectorCode        equ LABEL_DESC_CODE - LABEL_GDT             ; 代码段选择子
SelectorData        equ LABEL_DESC_DATA - LABEL_GDT             ; 数据段选择子
SelectorVideo       equ LABEL_DESC_VIDEO - LABEL_GDT | SA_RPL3  ; 视频段选择子，特权级3（用户特权级）
; GDT选择子 ------------------------------------------------------------------
BaseOfStack	        equ	0x100                                   ; 基栈
;============================================================================
;   16位实模式代码段
;----------------------------------------------------------------------------
START:                  ; 程序入口
    ; 寄存器复位
    mov	ax, cs
    mov	ds, ax
    mov	es, ax
    mov	ss, ax
    mov	sp, BaseOfStack

    ; 显示字符串 "Loading..."
    mov	dh, 0
    call DispStr		; 显示字符串

   ; 得到内存数
    mov ebx, 0          ; ebx = 后续值，开始时需为0
    mov di, _MemChkBuf  ; es:di 指向一个地址范围描述符结构(Address Range Descriptor Structure)
.MemChkLoop:
	mov eax, 0E820h		; eax = 0000E820h
	mov ecx, 20			; ecx = 地址范围描述符结构的大小
	mov edx, 0534D4150h	; edx = 'SMAP'
	int 15h
	jc .MemChkFail		; 如果产生的进位，即CF = 1，跳转到.MemChkFail
	add di, 20
	inc dword [_ddMCRCount]	; _dwMCRNumber = ARDS　的个数
	cmp ebx, 0
	jne .MemChkLoop		; ebx != 0，继续进行循环
	jmp .MemChkOK		; ebx == 0，得到内存数OK
.MemChkFail:
	mov dword [_ddMCRCount], 0
.MemChkOK:
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
    mov ax, KERNEL_SEG  ; es = KERNEL_SEG
    mov es, ax
    mov bx, KERNEL_OFFSET
    mov ax, [wSector]
    mov cl, 1
    call ReadSector

    mov si, KernelFileName      ; ds:si -> Loader的文件名称
    mov di, KERNEL_OFFSET       ; es:di -> KERNEL_SEG:KERNEL_OFFSET -> 加载到内存中的扇区数据
    cld     ; 字符串比较方向，si、di方向向右

    ; 开始在扇区中寻找文件，比较文件名
    mov dx, 16                  ; 一个扇区512字节，FAT目录项占用32个字节，512/32 = 16，所以一个扇区有16个目录项
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
    mov si, KernelFileName
    jmp SEARCH_FOR_FILE     ; 重新开始在下一个目录项中查找文件并比较

NEXT_SECTOR_IN_ROOT_DIR:
    add word [wSector], 1   ; 准备开始读取下一个扇区
    jmp SEARCH_FILE_IN_ROOR_DIR_BEGIN

NO_FILE:
    mov dh, 4
    call DispStr    ; 打印"NO KERNEL!"
    ; 死循环
    jmp $
FILENAME_FOUND:
    ; 准备参数，开始读取文件数据扇区
    mov ax, RootDirSectors      ; ax = 根目录占用空间（占用的扇区数）
    and di, 0xfff0              ; di &= f0, 11111111 11110000，是为了让它指向本目录项条目的开始。

    push eax                    ; 保存eax的值
    mov eax, [es:di + 0x1c]     ; FAT目录项第0x1c处偏移是文件大小
    mov dword [dwKernelSize], eax   ; 保存内核文件大小
    cmp eax, KERNEL_HAVE_SPACE  ; 看看内核文件大小有没有超过我们为其保留的大小
    ja KERNEL_FILE_TOO_LARGE    ; 超过了！
    pop eax                     ; 恢复eax
    jmp FILE_START_LAOD         ; 没超过，准备开始加载内核文件
KERNEL_FILE_TOO_LARGE:          ; 内核文件太大了，超过了我们给它留的128KB
    mov dh, 3
    call DispStr                ; 打印"Too Large!"
    jmp $                       ;
FILE_START_LAOD:
    add di, 0x1a                ; FAT目录项第0x1a处偏移是文件数据所在的第一个簇号
    mov cx, word [es:di]        ; cx = 文件数据所在的第一个簇号
    push cx                     ; 保存文件数据所在的第一个簇号
    ; 通过簇号计算它的真正扇区号
    add cx, ax
    add cx, DeltaSectorNo       ; 簇号 + 根目录占用空间 + 文件开始扇区号 == 文件数据的第一个扇区
    mov ax, KERNEL_SEG
    mov es, ax                  ; es <- KERNEL_SEG
    mov bx, KERNEL_OFFSET       ; bx <- KERNEL_OFFSET
    mov ax, cx                  ; ax = 文件数据的第一个扇区
LOADING_FILE:
    ; 我们每读取一个数据扇区，就在“Loading...”之后接着打印一个点，形成一种动态加载的动画。
    ; 0x10中断，0xe功能 --> 在光标后打印一个字符
    push ax
    push bx
    mov ah, 0xe
    mov al, '.'
    mov bl, 0xf
    int 0x10
    pop bx
    pop ax

    mov cl, 1                   ; 读1个
    call ReadSector             ; 读取
    pop ax                      ; 取出前面保存的文件的的簇号
    call GET_FATEntry           ; 通过簇号获得该文件的下一个FAT项的值
    cmp ax, 0xff8
    jae FILE_LOADED             ; 加载完成...
    ; FAT项的值 < 0xff8，那么我们继续设置下一次要读取的扇区的参数
    ; 通过簇号计算它的真正扇区号
    push ax                     ; 保存簇号
    mov dx, RootDirSectors
    add ax, dx
    add ax, DeltaSectorNo       ; 簇号 + 根目录占用空间 + 文件开始扇区号 == 文件数据的扇区
    add bx, [BPB_BytsPerSec]    ; bx += 扇区字节量
    jc KERNEL_GREAT_64KB        ; 如果bx += 扇区字节量，产生了一个进位，说明已经读满64KB，内核文件大于64KB
    jmp CONTINUE_LOADING        ; 内核文件还在64KB内，继续正常加载
KERNEL_GREAT_64KB:
    ; es += 0x1000，es指向下一个段，准备继续加载
    push ax
    mov ax, es
    add ax, 0x1000
    mov es, ax
    pop ax
CONTINUE_LOADING:               ; 继续加载内核文件
    jmp LOADING_FILE
FILE_LOADED:
    call KillMotor  ; 关闭软驱马达
    mov dh, 1
    call DispStr    ; 打印"KERNEL OK!"
;----------------------------------------------------------------------------
; 万事俱备，准备进入32位保护模式
;----------------------------------------------------------------------------
    ; 1 首先，进入保护模式必须有 GDT 全局描述符表，我们加载 gdtr（gdt地址指针）
    lgdt	[GDTPtr]

    ; 2 由于保护模式中断处理的方式和实模式不一样，所以我们需要先关闭中断，否则会引发错误
    cli

    ; 3 打开地址线A20，不打开也可以进入保护模式，但内存寻址能力受限（1MB）
    in al, 92h
    or al, 00000010b
    out 92h, al

    ; 4 进入16位保护模式，设置cr0的第0位：PE（保护模式标志）为1
    mov eax, cr0
    or 	eax, 1
    mov cr0, eax

    ; 5 真正进32位入保护模式！前面的4步已经进入了保护模式
    ; 	现在只需要跳入到一个32位代码段就可以真正进入32位保护模式了！
    jmp dword SelectorCode:LOADER_PHY_ADDR + PM_32_START

    ; 如果上面一切顺利，这一行永远不可能执行的到
    jmp $
;============================================================================
; 变量
wRootDirSizeLoop    dw      RootDirSectors  ; 根目录占用的扇区数，在循环中将被被逐步递减至0
wSector             dw      0               ; 要读取的扇区号
isOdd               db      0               ; 读取的FAT条目是不是奇数项?
dwKernelSize        dd      0               ; 内核文件的大小
;============================================================================
; 要显示的字符串
;----------------------------------------------------------------------------
KernelFileName		db	"KERNEL  BIN", 0	; KERNEL.BIN 之文件名
; 为简化代码, 下面每个字符串的长度均为 MessageLength
MessageLength		equ	10
Message:		    db	"Loading..."    ; 10, 不够则用空格补齐. 序号 0
                    db  "KERNEL OK!"    ; 序号1
                    db  "MemChkFail"    ; 序号2
                    db  "Too large!"    ; 序号3
                    db  "NO KERNEL!"    ; 序号4
;============================================================================
;   16位支持函数库
;----------------------------------------------------------------------------
%include "loader_16lib.inc"
;============================================================================
;============================================================================
;   32位代码段
;----------------------------------------------------------------------------
[section .code32]
align 32
[bits 32]
PM_32_START:            ; 跳转到这里，说明已经进入32位保护模式
    mov ax, SelectorData
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov ss, ax              ; ds = es = fs = ss = 数据段
    mov esp, TopOfStack     ; 设置栈顶
    mov ax, SelectorVideo
    mov gs, ax              ; gs = 视频段

    ; 计算内存大小
    call CalcMemSize
    ; 打印内存信息
    call PrintMemSize
    ; 启动分页机制
    call SetupPaging

    ; 死循环
    jmp $
;============================================================================
;   32位支持函数库
;----------------------------------------------------------------------------
%include "loader_32lib.inc"
;============================================================================
;   32位数据段
;----------------------------------------------------------------------------
[section .data32]
align 32
DATA32:
;----------------------------------------------------------------------------
;   16位实模式下使用的数据地址
;----------------------------------------------------------------------------
; 字符串 ---
_strMemSize:        db "Memory Size: ", 0
_strKB:             db "KB", 10, 0
_strSetupPaging:    db "Setup paging.", 10, 0
; 变量 ---
_ddMCRCount:        dd 0        ; 检查完成的ARDS的数量，为0则代表检查失败
_ddMemSize:         dd 0        ; 内存大小
_ddDispPosition:    dd (80 * 4 + 0) * 2 ; 初始化显示位置为第 4 行第 0 列
; 地址范围描述符结构(Address Range Descriptor Structure)
_ARDS:
    _ddBaseAddrLow:  dd 0        ; 基地址低32位
    _ddBaseAddrHigh: dd 0        ; 基地址高32位
    _ddLengthLow:    dd 0        ; 内存长度（字节）低32位
    _ddLengthHigh:   dd 0        ; 内存长度（字节）高32位
    _ddType:         dd 0        ; ARDS的类型，用于判断是否可以被OS使用
; 内存检查结果缓冲区，用于存放没存检查的ARDS结构，256字节是为了对齐32位，256/20=12.8
; ，所以这个缓冲区可以存放12个ARDS。
_MemChkBuf:          times 256 db 0
;----------------------------------------------------------------------------
;   32位保护模式下的数据地址符号
strMemSize          equ LOADER_PHY_ADDR + _strMemSize
strKB               equ LOADER_PHY_ADDR + _strKB
strSetupPaging               equ LOADER_PHY_ADDR + _strSetupPaging

ddMCRCount          equ LOADER_PHY_ADDR + _ddMCRCount
ddMemSize           equ LOADER_PHY_ADDR + _ddMemSize
ddDispPosition      equ LOADER_PHY_ADDR + _ddDispPosition
ARDS                equ LOADER_PHY_ADDR + _ARDS
    ddBaseAddrLow   equ LOADER_PHY_ADDR + _ddBaseAddrLow
    ddBaseAddrHigh  equ LOADER_PHY_ADDR + _ddBaseAddrHigh
    ddLengthLow     equ LOADER_PHY_ADDR + _ddLengthLow
    ddLengthHigh    equ LOADER_PHY_ADDR + _ddLengthHigh
    ddType          equ LOADER_PHY_ADDR + _ddType
MemChkBuf           equ LOADER_PHY_ADDR + _MemChkBuf
; 堆栈就在数据段的末尾，一共给这个32位代码段堆栈分配4KB
StackSpace: times 0x1000    db 0
TopOfStack  equ LOADER_PHY_ADDR + $     ; 栈顶
;============================================================================
