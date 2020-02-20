/* Copyright (C) 2007 Free Software Foundation, Inc. 
 * See the copyright notice in the file /usr/LICENSE.
 * Created by flyan on 2020/2/20.
 * QQ: 1341662010
 * QQ-Group:909830414
 * gitee: https://gitee.com/flyanh/
 *
 * 该文件包含flyanx的主程序。
 *
 * 该文件的入口点是：
 *  - flyanx_main:      flyanx的主程序
 */

int display_position = (80 * 6 + 0) * 2;     // 从第 6 行第 0 列开始显示
void low_print(char* str);

void flyanx_main(void){

    low_print("Hello OS!!!\n");
    while (1){}
}
