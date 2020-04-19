;================================================================================================
; 文件：ipc.asm
; 作用：i386体系的消息通信向外暴露的的系统调用
; 作者： Flyan
; 联系QQ： 1341662010
;================================================================================================
; 库函数开始
[SECTION .lib]

; 导出库函数
global  send
global  receive
global  send_rec
global  in_outbox

; 常量定义，请查看 include/flyanx/common.h 和 src/kernel/const.h 文件下的定义
SEND        equ     0x1
RECEIVE     equ     0x2
SEND_REC    equ     0x3
IN_OUTBOX   equ     0x4
SYS_VEC     equ     0x94            ; 系统调用向量

SRC_DEST_MSGP   equ 4 + 4 + 4
                ; ebx + ecx + eip
MSG_PTR         equ SRC_DEST_MSGP + 4
;*========================================================================*
;                           send                            *
;                       执行系统调用 SEND
;*========================================================================*
; 本例程执行系统调用函数，op为SEND，系统调用原型:
;   int sys_call(op, src_dest)
; 本例程只是对op = SEND的封装。
send:
    ; ebx/ecx 会被用到，所以我们将其保存一下
    push ebx
    push ecx

    mov ecx, SEND       ; ecx = 调用操作是发送消息，op = SEND
    jmp com             ; 公共的处理
;*========================================================================*
;                           receive                               *
;                         执行系统调用 RECIIVE
;*========================================================================*
; 本例程执行系统调用函数，op 为 RECIIVE
receive:
    ; ebx/ecx 会被用到，所以我们将其保存一下
    push ebx
    push ecx

    mov ecx, RECEIVE        ; ecx = 调用操作是接收消息，op = RECEIVE
    jmp com                 ; 公共的处理
;*========================================================================*
;                           send_rec                           *
;                        执行系统调用 SEND_REC
;*========================================================================*
; 本例程执行系统调用函数，op 为 SEND_REC
send_rec:
    ; ebx/ecx 会被用到，所以我们将其保存一下
    push ebx
    push ecx

    mov ecx, SEND_REC               ; ecx = 调用操作是发送消息并等待对方响应，op = SEND_REC
    jmp com
;*========================================================================*
;                           in_outbox                               *
;                         执行系统调用 IN_OUTBOX
;*========================================================================*
; 本例程执行系统调用函数，op 为 IN_OUTBOX
in_outbox:
    ; ebx/ecx 会被用到，所以我们将其保存一下
    push ebx
    push ecx

    mov ecx, IN_OUTBOX      ; ecx = 调用操作是设置发件箱，op = OUTBOX
; 公共处理
com:
    mov eax, [esp + SRC_DEST_MSGP]          ; eax = src_dest_msgp
    mov ebx, [esp + MSG_PTR]                ; ebx = msg_ptr
    int SYS_VEC                             ; 执行系统调用

    pop ecx
    pop ebx
    ret