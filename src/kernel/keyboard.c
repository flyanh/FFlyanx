/* Copyright (C) 2007 Free Software Foundation, Inc.
 * See the copyright notice in the file /usr/LICENSE.
 * Created by flyan on 2021/2/15.
 * QQ: 1341662010
 * QQ-Group:909830414
 * gitee: https://gitee.com/flyanh/
 *
 * PC 和 AT键盘系统任务（驱动程序）
 */
#include "kernel.h"
#include <flyanx/keymap.h>
#include "keymaps/us-std.h"

/* 标准键盘和AT键盘 */
#define KEYBOARD_DATA       0x60    /* 键盘数据的I/O端口，用于和键盘控制器的底层交互。 */

/* AT键盘 */
#define KEYBOARD_COMMAND    0x64    /* AT上的命令I/o端口 */
#define KEYBOARD_STATUS     0x64    /* AT上的状态I/O端口 */
#define KEYBOARD_ACK        0xFA    /* 键盘相应确认 */

#define KEYBOARD_OUT_FULL   0x01    /* 字符按键按下时该状态位被设置 */
#define KEYBOARD_IN_FULL    0x02    /* 未准备接收字符时该状态位被设置 */
#define LED_CODE            0xED    /* 设置键盘灯的命令 */
#define MAX_KEYBOARD_ACK_RETRIES    0x1000  /* 等待键盘响应的最大等待时间 */
#define MAX_KEYBOARD_BUSY_RETRIES   0x1000  /* 键盘忙时循环的最大时间 */
#define KEY_BIT             0x80    /* 将字符打包传输到键盘的位 */
#define KEY                 0x7f    /* 01111111, 忽略扫描码第 8 位（可以判断键是 make 还是 break） */

/* 它们用于滚屏操作 */
#define SCROLL_UP       0	            /* 前滚，用于滚动屏幕 */
#define SCROLL_DOWN     1	            /* 后滚 */

/* 锁定键激活位，应该要等于键盘上的LED灯位 */
#define SCROLL_LOCK	    0x01    /* 二进制：0001 */
#define NUM_LOCK	    0x02    /* 二进制：0010 */
#define CAPS_LOCK	    0x04    /* 二进制：0100 */
#define DEFAULT_LOCK    0x02    /* 默认：小键盘是打开的 */

/* 键盘缓冲区 */
#define KEYBOARD_IN_BYTES	  32	/* 键盘输入缓冲区的大小 */

/* 其他用途 */
#define ESC_SCAN	        0x01	/* 重启键，当宕机时可用 */
#define SLASH_SCAN	        0x35	/* 识别小键盘区的斜杠 */
#define RSHIFT_SCAN	        0x36	/* 区分左移和右移 */
#define HOME_SCAN	        0x47	/* 数字键盘上的第一个按键 */
#define INS_SCAN	        0x52	/* INS键，为了使用 CTRL-ALT-INS 重启快捷键 */
#define DEL_SCAN	        0x53	/* DEL键，为了使用 CTRL-ALT-DEL 重启快捷键 */

/* 当前键盘所处的各种状态，解释一个按键需要使用这些状态 */
PRIVATE bool_t esc = FALSE;		            /* 是一个转义扫描码？收到一个转义扫描码时，被置位 */
PRIVATE bool_t alt_left = FALSE;		    /* 左ALT键状态 */
PRIVATE bool_t alt_right = FALSE;		    /* 右ALT键状态 */
PRIVATE bool_t alt = FALSE;		            /* ALT键状态，不分左右 */
PRIVATE bool_t ctrl_left = FALSE;		    /* 左CTRL键状态 */
PRIVATE bool_t ctrl_right = FALSE;		    /* 右CTRL键状态 */
PRIVATE bool_t ctrl = FALSE;		        /* CTRL键状态，不分左右 */
PRIVATE bool_t shift_left = FALSE;		    /* 左SHIFT键状态 */
PRIVATE bool_t shift_right = FALSE;         /* 右SHIFT键状态 */
PRIVATE bool_t shift = FALSE;		        /* SHIFT键状态，不分左右 */
PRIVATE u8_t locks[NR_CONSOLES];            /* 每个控制台的锁定键状态 */


/* 数字键盘的转义字符映射 */
PRIVATE FINAL char numpad_map[] =
        {'H', 'Y', 'A',
         'B', 'D', 'C',
         'V', 'U', 'G',
         'S', 'T', '@'};

/* 缓冲区相关 */
PRIVATE u8_t input_buff[KEYBOARD_IN_BYTES];
PRIVATE int input_count;
PRIVATE u8_t *input_free = input_buff;  /* 指向输入缓冲区的下一个空闲位置 */
PRIVATE u8_t *input_todo = input_buff;  /* 指向被处理并返回给终端的位置 */

FORWARD _PROTOTYPE( u8_t scan_key, (void) );
FORWARD _PROTOTYPE( int keyboard_handler, (int irq) );
FORWARD _PROTOTYPE( int keyboard_wait, (void) );
FORWARD _PROTOTYPE( int keyboard_ack, (void) );
FORWARD _PROTOTYPE( void setting_led, (void) );
FORWARD _PROTOTYPE( u32_t map_key, (u8_t scan_code) );
FORWARD _PROTOTYPE( u32_t key_make_break, (u8_t scan_code) );


/*===========================================================================*
 *				map_key0				     *
 *		返回一个扫描码对应的ASCII码，忽略修饰符      *
 *===========================================================================*/
#define map_key0(scan_code) \
    ((u16_t)keymap[scan_code * MAP_COLS])


/*===========================================================================*
 *				map_key					     *
 *				映射按键
 *===========================================================================*/
PRIVATE u32_t map_key(u8_t scan_code) {
    /* 返回扫描码对应映射文件中的ascii码，完全映射，
     * 包括处理和普通字符同时按下的（多重）修饰组合键。
     */

    /* 数字小键盘上的斜杠 */
    if(scan_code == SLASH_SCAN && esc) {
        return '/';
    }

    /* 确定映射行 */
    u16_t *keys_row = &keymap[scan_code * MAP_COLS];

    /* 确定当前控制台的锁定键状态 */
    u8_t lock = locks[0];
    bool_t caps = shift;    /* 如果同时按下了 shift，默认大写状态  */
    if( (lock & NUM_LOCK) != 0 && HOME_SCAN <= scan_code && scan_code <= DEL_SCAN ) {
        /* 如果小键盘锁定开启，同时这次按下的字符是小键盘的按键，那么此次映射的大写状态发生改变 */
        caps = !caps;
    }
    if( (lock & CAPS_LOCK) && (keys_row[0] & HASCAPS) ) {
        /* 如果大写锁定开启，同时此次扫描码原始字符支持大写锁定，那么此次映射的大写状态发生改变 */
        caps = !caps;
    }

    /* 确定映射列 */
    int col = 0;
    if(alt) {       /* 同时按下了 alt 键 */
        col = 2;    /* alt */
        if(ctrl || alt_right) {
            col = 3;    /* ctrl + alt == alt right */
        }
        if(caps) {      /* 大写锁定 + alt == shift + alt */
            col = 4;
        }
    } else {
        if(caps) {      /* 大写锁定 = shift */
            col = 1;
        }
        if(ctrl) {      /* ctrl */
            col = 5;
        }
    }
    return (keys_row[col] & ~HASCAPS);  /* 将附加的大写锁定支持字节删除，出了这个函数我们就不需要这个状态了。 */
}

/*===========================================================================*
 *				    key_make_break					     *
 *			      执行按键按下/松开处理
 *===========================================================================*/
PRIVATE u32_t key_make_break(u8_t scan_code) {
    /* 将扫描码转换为 ASCII 码然后更新跟踪修饰键状态的变量。
     *
     * 这个例程可以处理只在按下键时中断的键盘，也可以处理在按下键和释放键时中断的键盘。
     * 为了简单和提高效率，中断例程会过滤掉大多数键释放。
     */

    /* 得到这个扫描码按下的状态 */
    bool_t is_make = (scan_code & KEY_BIT) == 0;

    /* 得到映射后的字符代码，它可能还不是纯粹的 ascii 码 */
    u32_t ch = map_key((scan_code &= KEY));

    /* 本次字符需要转义？ */
    bool_t escape = esc;
    esc = FALSE;    /* 只影响一次 */

    /* 处理字符 */
    switch (ch) {
        case CTRL:          /* Ctrl */
            if(escape) {    /* ext + ctrl == ctrl_right */
                ctrl_right = is_make;
            } else {
                ctrl_left = is_make;
            }
            ctrl = ctrl_left | ctrl_right;
            break;
        case ALT:           /* Alt */
            if(escape) {    /* ext + alt == alt_right */
                alt_right = is_make;
            } else {
                alt_left = is_make;
            }
            alt = alt_left | alt_right;
            break;
        case SHIFT:         /* Shift */
            if(scan_code == RSHIFT_SCAN) {
                shift_right = is_make;
            } else {
                shift_left = is_make;
            }
            shift = shift_left | shift_right;
            break;
        case CALOCK:        /* Caps lock */
            /* 只有按下状态我们才改变状态， 下面的另外两个状态也是同样的道理。
             */
            if(is_make) {
                locks[0] ^= CAPS_LOCK;
                setting_led();
            }
            break;
        case NLOCK:         /* Number lock */
            if(is_make) {
                locks[0] ^= NUM_LOCK;
                setting_led();
            }
            break;
        case SLOCK:         /* Scroll lock */
            if(is_make) {
                locks[0] ^= SCROLL_LOCK;
                setting_led();
            }
            break;
        case EXTKEY:        /* 拓展键，例如当点 alt_right 按键时，将会先产生一个符合扫描码 ext + alt 来和 alt_left 区分。 */
            esc = TRUE;     /* 下一个键码将被转义 */
            break;
        default:            /* 普通字符 */
            if(is_make) {
                return ch;
            }
    }

    return -1;  /* 释放键，或移位类型键，都被忽略，返回 -1，因为是无符号，所以这里实际上应该是数据类型的最大值 */
}

/*===========================================================================*
 *				 scan_key					     *
 *			    扫描键盘获得键盘码
 *===========================================================================*/
PRIVATE u8_t scan_key(void) {
    /* 1. 从键盘控制器端口获取键盘码 */
    const u8_t scan_code = in_byte(KEYBOARD_DATA);

    /* 2. 选通键盘 */
    int val = in_byte(PORT_B);
    out_byte(PORT_B, val | KEY_BIT);
    out_byte(PORT_B, val);

    return scan_code;
}

/*===========================================================================*
 *				 keyboard_handler					     *
 *			     键盘中断处理程序
 *===========================================================================*/
PRIVATE int keyboard_handler(int irq) {

    u8_t scan_code = scan_key();
    u32_t ch = key_make_break(scan_code);
    if(ch != -1) {
        printf("%c", ch);
    }

    if(input_count < KEYBOARD_IN_BYTES) {   /* Buffer available. */
        *input_free++ = scan_code;  /* Scan code append to buffer's free zone. */
        ++input_count;

        /* Buffer is full. */
        if(input_count == KEYBOARD_IN_BYTES) {
            input_free = input_buff;
        }
    }

    return ENABLE;
}


/*===========================================================================*
 *				    keyboard_wait					     *
 *			      等待键盘直到其不忙，即就绪状态
 *===========================================================================*/
PRIVATE int keyboard_wait(void) {
    /* CPU等待8042控制器处于可以接收参数的就绪状态；如果超时，返回0 */

    int retries = MAX_KEYBOARD_BUSY_RETRIES;
    u8_t status;
    while (retries-- > 0 && ((status = in_byte(KEYBOARD_STATUS)) & (KEYBOARD_IN_FULL | KEYBOARD_OUT_FULL)) != 0) {
        if((status & KEYBOARD_OUT_FULL) != 0) {
            (void) in_byte(KEYBOARD_DATA); /* 丢弃 */
        }
    }
    return retries; /* 为 0 超时 */
}

/*===========================================================================*
 *				    keyboard_ack					     *
 *			        键盘等待命令响应
 *===========================================================================*/
PRIVATE int keyboard_ack(void) {
    /* CPU 等待 8042 控制器确认最后一条命令响应； 如果超时，返回 0 */

    int retries = MAX_KEYBOARD_ACK_RETRIES;
    for (; retries-- > 0 && in_byte(KEYBOARD_DATA) != KEYBOARD_ACK; );  /* 循环等待响应 */
    return retries;
}

/*===========================================================================*
 *				 setting_led					     *
 *			    设置键盘的LED灯
 *===========================================================================*/
PRIVATE void setting_led(void) {
    keyboard_wait();
    out_byte(KEYBOARD_DATA, LED_CODE);
    keyboard_ack();

    keyboard_wait();
    out_byte(KEYBOARD_DATA, locks[0]);
    keyboard_ack();
}

/*===========================================================================*
 *				keyboard_init					     *
 *				键盘驱动初始化
 *===========================================================================*/
PUBLIC void keyboard_init(void) {
    printf("#{KEY_BD}-> init...\n");

    /* 初始化设置 LED */
    int i = 0;
    for(; i < NR_CONSOLES; ++i) {
        locks[i] = DEFAULT_LOCK;
    }
    setting_led();

    /* 扫描键盘以确保没有残余的键入，清空 */
    (void) scan_key();

    /* 设置键盘中断处理例程 */
    put_irq_handler(KEYBOARD_IRQ, keyboard_handler);
    enable_irq(KEYBOARD_IRQ);
}


