;============================================================================
;   计算内存大小
; 本函数根据之前保存的ARDS，计算内存总大小并保存起来
;----------------------------------------------------------------------------
CalcMemSize:
    push esi
    push ecx
    push edx
    push edi

    mov esi, MemChkBuf      ; ds:esi -> 缓冲区
    mov ecx, [ddMCRCount]   ; ecx = 有多少个ARDS，记为i
.loop:
    mov edx, 5              ; ARDS有5个成员变量，记为j
    mov edi, ARDS           ; ds:edi -> 一个ARDS结构
.1: ; 将缓冲区中的第 i 个ARDS结构拷贝到ds:edi中的ARDS结构
    push dword [esi]
    pop eax                 ; ds:eax -> 缓冲区中的第一个ADRS结构
    stosd                   ; 将ds:eax中的一个dword内容拷贝到ds:edi中，填充ADRS结构
    add esi, 4              ; ds:esi指向ARDS中的下一个成员变量
    dec edx                 ; j--
    cmp edx, 0
    jnz .1                  ; j != 0，继续填充
    ; j == 0，ARDS结构填充完毕
    cmp dword [ddType], 1
    jne .2                  ; 不是OS可使用的内存范围，直接进入下个外循环看下一个ARDS
    ; 是OS可用的地址范围，我们计算这个ARDS的内存大小
    mov eax, [ddBaseAddrLow]; eax = 基地址低32位
    add eax, [ddLengthLow]  ; eax = 基地址低32位 + 长度低32位 --> 这个ARDS结构的指代的内存大小
                            ; 为什么不算高32为？因为32位既可以表示0~4G的内存范围，而32位CPU也只能识别0~4G
                            ; 我们编写的是32位操作系统，所以高32位是为64位操作系统做准备的，我们不需要。
    cmp eax, [ddMemSize]
    jb .2
    mov dword [ddMemSize], eax  ; 内存大小 = 最后一个基地址最大的ARDS的  基地址低32位 + 长度低32位
.2:
    loop .loop              ; jmp .loop, ecx--

    pop edi
    pop edx
    pop ecx
    pop esi
    ret
;============================================================================
;   打印内存大小(以KB显示)
;----------------------------------------------------------------------------
PrintMemSize:
    push ebx
    push ecx

    mov eax, [ddMemSize]    ; eax = 内存大小
    xor edx, edx
    mov ebx, 1024
    div ebx                 ; eax / 1024 --> 内存大小(字节) / 1024 = 内存大小(KB)

    push eax                ; 保存计算好的内存大小
    ; 显示一个字符串"Memory Size: "
    push strMemSize
    call Print
    add esp, 4

    ; 因为之前已经压入eax了，不需要再压入！
    call PrintInt
    add esp, 4

    ; 打印"KB"
    push strKB
    call Print
    add esp, 4

    pop ecx
    pop ebx
    ret
;============================================================================
;   启动分页机制
; 根据内存的大小来计算应初始化多少的PDE以及多少PTE，我们给每页分4K大小(32位操作系统一般分4K，Windows 32也是如此哦)
; 注意：页目录表存放在1M(0x100000)~1.4M处(0x101000)
;      所有页表存放在1.4M(0x101000)~5.4M处(0x501000)
;----------------------------------------------------------------------------
SetupPaging:
    xor edx, edx            ; edx = 0
    mov eax, [ddMemSize]    ; eax = 内存大小
    mov ebx, 0x400000       ; 0x400000 = 4M = 4096 * 1024，即一个页表对于的内存大小
    div ebx                 ; 内存大小 / 4M
    mov ecx, eax            ; ecx = 需要的页表的个数，即 PDE 应该的页数
    test edx, edx
    jz .no_remainder        ; if(edx == 0) jmp .no_remainder，没有余数
    inc ecx                 ; else ecx++，有余数则需要多一个 PDE 去映射它
.no_remainder:
    push ecx                ; 保存页表个数
    ; flyanx 0.11为了简化处理，所有线性地址对应相等的物理地址，并且暂不考虑内存空洞

    ; 首先初始化页目录
    mov ax, SelectorData
    mov es, ax
    mov edi, PAGE_DIR_BASE  ; edi = 页目录存放的首地址
    xor eax, eax
    ; eax = PDE，PG_P（该页存在）、PG_US_U（用户级页）、PG_RW_W（可读、写、执行）
    mov eax, PAGE_TABLE_BASE | PG_P | PG_US_U | PG_RW_W
.SetupPDE:  ; 设置 PDE
    stosd                   ; 将ds:eax中的一个dword内容拷贝到ds:edi中，填充页目录项结构
    add eax, 4096           ; 所有页表在内存中连续，PTE 的高20基地址指向下一个要映射的物理内存地址
    loop .SetupPDE           ; 直到ecx = 0，才退出循环，ecx是需要的页表个数

    ; 现在开始初始化所有页表
    pop eax                 ; 取出页表个数
    mov ebx, 1024           ; 每个页表可以存放 1024 个 PTE
    mul ebx                 ; 页表个数 * 1024，得到需要多少个PTE
    mov ecx, eax            ; eax = PTE个数，放在ecx里是因为准备开始循环设置 PTE
    mov edi, PAGE_TABLE_BASE; edi = 页表存放的首地址
    xor eax, eax
    ; eax = PTE，页表从物理地址 0 开始映射，所以0x0 | 后面的属性，该句可有可无，但是这样看着比较直观
    mov eax, 0x0 | PG_P | PG_US_U | PG_RW_W
.SetupPTE:  ; 设置 PTE
    stosd                   ; 将ds:eax中的一个dword内容拷贝到ds:edi中，填充页表项结构
    add eax, 4096           ; 每一页指向 4K 的内存空间
    loop .SetupPTE          ; 直到ecx = 0，才退出循环，ecx是需要的PTE个数

    ; 最后设置 cr3 寄存器和 cr0，开启分页机制
    mov eax, PAGE_DIR_BASE
    mov cr3, eax            ; cr3 -> 页目录表
    mov eax, cr0
    or eax, 0x80000000      ; 将 cr0 中的 PG位（分页机制）置位
    mov cr0, eax
    jmp short .SetupPGOK    ; 和进入保护模式一样，一个跳转指令使其生效，标明它是一个短跳转，其实不标明也OK
.SetupPGOK:
     nop                    ; 一个小延迟，给一点时间让CPU反应一下
     nop                    ; 空指令
     push strSetupPaging
     call Print
     add esp, 4
     ret
;============================================================================
;   打印函数，它类似与C语言中的printf，但它不支持'%'可变参数
; 函数原型：Print(void* ds:ptr)，ptr指向要打印的字符串，字符串以0结尾
;----------------------------------------------------------------------------
Print:
    push esi
    push edi

    mov esi, [esp + 4 * 3]      ; 得到字符串地址
    mov edi, [ddDispPosition]   ; 得到显示位置
    mov ah, 0xf                 ; 黑底白字
.1:
    lodsb                       ; ds:esi -> al, esi++
    test al, al
    jz .PrintEnd                ; 遇到了0，结束打印
    cmp al, 10
    je .2
    ; 如果不是0，也不是'\n'，那么我们认为它是一个可打印的普通字符
    mov [gs:edi], ax
    add edi, 2                  ; 指向下一列
    jmp .1
.2: ; 处理换行符'\n'
    push eax
    mov eax, edi                ; eax = 显示位置
    mov bl, 160
    div bl                      ; 显示位置 / 160，商eax就是当前所在行数
    inc eax                     ; 行数++
    mov bl, 160
    mul bl                      ; 行数 * 160，得出这行的显示位置
    mov edi, eax                ; edi = 新的显示位置
    pop eax
    jmp .1
.PrintEnd:
    mov dword [ddDispPosition], edi ; 打印完毕，更新显示位置

    pop edi
    pop esi
    ret
;============================================================================
;   显示 AL 中的数字
;----------------------------------------------------------------------------
PrintAl:
	push ecx
	push edx
	push edi
	push eax

	mov edi, [ddDispPosition]	; 得到显示位置

	mov ah, 0Fh		; 0000b: 黑底	1111b: 白字
	mov dl, al
	shr al, 4
	mov ecx, 2
.begin:
	and al, 01111b
	cmp al, 9
	ja	.1
	add al, '0'
	jmp	.2
.1:
	sub al, 10
	add al, 'A'
.2:
	mov [gs:edi], ax
	add edi, 2

	mov al, dl
	loop .begin

	mov [ddDispPosition], edi	; 显示完毕后，设置新的显示位置

    pop eax
	pop edi
	pop edx
	pop ecx

	ret
;============================================================================
;   显示一个整形数
;----------------------------------------------------------------------------
PrintInt:
    mov	ah, 0Fh			; 0000b: 黑底    1111b: 白字
    mov	al, '0'
    push	edi
    mov	edi, [ddDispPosition]
    mov	[gs:edi], ax
    add edi, 2
    mov	al, 'x'
    mov	[gs:edi], ax
    add	edi, 2
    mov	[ddDispPosition], edi	; 显示完毕后，设置新的显示位置
    pop edi

	mov	eax, [esp + 4]
	shr	eax, 24
	call	PrintAl

	mov	eax, [esp + 4]
	shr	eax, 16
	call	PrintAl

	mov	eax, [esp + 4]
	shr	eax, 8
	call	PrintAl

	mov	eax, [esp + 4]
	call	PrintAl

	ret
;============================================================================
;   内存拷贝，仿C语言memcpy
; 函数原型：
;   void *MemCpy(void *es:dest, void *ds:src, int size)
; 参数：
;   dest    目的地地址
;   src     源数据地址
;   size    要拷贝的数据量
; 返回值：
;   拷贝完成后的新数据的地址
;----------------------------------------------------------------------------
MemCpy:
    push esi
    push edi
    push ecx

    mov edi, [esp + 4 * 4]      ; edi = dest
    mov esi, [esp + 4 * 5]      ; esi = src
    mov ecx, [esp + 4 * 6]      ; ecx = size
.Copy:
    cmp ecx, 0
    jz .CpyEnd                  ; eax == 0，拷贝结束
    ; eax != 0，继续拷贝下一个字节
    mov al, [ds:esi]            ; 拷贝一字节给al
    inc esi                     ; esi++，指向下一个要拷贝的字节
    mov [es:edi], al            ; 拷贝al字节给目的地
    inc edi

    loop .Copy                  ; 循环拷贝，直至ecx == 0
.CpyEnd:
    mov eax, [esp + 4 * 4]      ; 设置返回值

    pop ecx
    pop edi
    pop esi
    ret
;============================================================================
;   初始化内核
; 将 KERNEL.BIN 的内容经过调整对齐之后放到内核挂载点处
; 我们通过遍历 Program Header，根据 Program Header 中的信息来确定把什么放入内存，放到什么位置，以及要放多少
;----------------------------------------------------------------------------
InitKernelFile:
    xor esi, esi
    xor ecx, ecx
    mov cx, word [KERNEL_PHY_ADDR + 44]     ; cx = e_phnum(Program Header的数量)
    mov esi, [KERNEL_PHY_ADDR + 28]         ; esi = e_phoff(Program header table在文件中的偏移)
    add esi, KERNEL_PHY_ADDR                ; ds:esi -> 第一个 Program header
.Begin:
    mov eax, [esi + 0]                      ; eax -> p_type(段类型)
    cmp eax, 0
    jz .NoAction                            ; p_type == 0，是一个不可用的段
    ; p_type != 0，是一个可用的段
    push dword [esi + 16]                   ; 压入p_filesz（段在文件中的长度），作为MemCpy的最后一个参数size
    mov eax, [esi + 4]                      ; eax = p_offset，段的第一个字节在文件中的偏移
    add eax, KERNEL_PHY_ADDR                ; eax -> 段第一个字节
    push eax                                ; 压入段第一个字节的地址，作为MemCpy的src参数
    push dword [esi + 8]                    ; 压入p_vaddr（段的第一个字节在内存中的虚拟地址），作为MemCpy的dest参数
    call MemCpy                             ; 开始拷贝
    add esp, 4 * 3                          ; 清理堆栈
.NoAction:
    add esi, 32                             ; esi += Program header结构大小
    dec ecx
    cmp ecx, 0
    jnz .Begin                              ; 继续看下一个Program header

    ret
