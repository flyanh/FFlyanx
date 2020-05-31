/* Copyright (C) 2007 Free Software Foundation, Inc.
 * See the copyright notice in the file /usr/LICENSE.
 * Created by flyan on 2020/3/16.
 * QQ: 1341662010
 * QQ-Group:909830414
 * gitee: https://gitee.com/flyanh/
 *
 * CMOS 相关头文件
 */

/* CMOS */
#define CLK_ELE		0x70	/* CMOS RAM address register port (write only)
                             * Bit 7 = 1  NMI disable
                             *	   0  NMI enable
                             * Bits 6-0 = RAM address
                             */

#define CLK_IO		0x71	/* CMOS RAM data register port (read/write) */

#define  YEAR             9	/* Clock register addresses in CMOS RAM	*/
#define  MONTH            8
#define  DAY              7
#define  HOUR             4
#define  MINUTE           2
#define  SECOND           0
#define  CLK_STATUS    0x0B	/* Status register B: RTC configuration	*/
#define  CLK_HEALTH    0x0E	/* Diagnostic status: (should be set by Power
                             * On Self-Test [POST])
                             * Bit  7 = RTC lost power
                             *	6 = Checksum (for addr 0x10-0x2d) bad
                             *	5 = Config. Info. bad at POST
                             *	4 = Mem. size error at POST
                             *	3 = I/O board failed initialization
                             *	2 = CMOS time invalid
                             *    1-0 =     reserved
                             */

