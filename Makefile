#
# Makefile
# Created by flyan on 2019/11/9.
# QQ: 1341662010
# QQ-Group:909830414
# gitee: https://gitee.com/flyanh/
# Makefile文件包含如何编译工程的规则
#============================================================================
#   变量
#----------------------------------------------------------------------------
# 编译链接中间目录
t = target
tb = target/boot
tk = target/kernel

# 所需软盘镜像，可以指定已存在的软盘镜像，系统内核将被写入到这里
FD = Flyanx.img
# 硬盘镜像 -- 还没有


# 镜像挂载点，自指定，必须存在于自己的计算机上，没有请自己创建一下
ImgMountPoint = /media/floppyDisk

# 所需要的编译器以及编译参数
ASM             = nasm
ASMFlagsOfBoot  = -I src/boot/include/
CC              = gcc
#============================================================================
#   目标程序以及编译的中间文件
#----------------------------------------------------------------------------
FlyanxBoot      = $(tb)/boot.bin $(tb)/loader.bin
#============================================================================
#   所有的功能伪命令
#----------------------------------------------------------------------------
.PHONY: nop all image debug run clean realclean
# 默认选项（输入make但没有跟参数，自动执行），提示一下用户我们的makefile有哪些功能
nop:
	@echo "all              编译所有文件，生成目标文件(二进制文件，boot.bin)"
	@echo "image            生成系统镜像文件"
	@echo "debug            打开bochs进行系统的运行和调试"
	@echo "run              提示用于如何将系统安装到虚拟机上运行"
	@echo "clean            清理所有的中间编译文件"
	@echo "realclean        完全清理：清理所有的中间编译文件以及生成的目标文件（二进制文件）	"

# 编译所有文件
all: $(FlyanxBoot)

# 生成系统镜像文件
image: $(FD) $(FlyanxBoot)
	dd if=$(tb)/boot.bin of=$(FD) bs=512 count=1 conv=notrunc
	sudo mount -o loop $(FD) $(ImgMountPoint)
	sudo cp -fv $(tb)/loader.bin $(ImgMountPoint)
	sudo umount $(ImgMountPoint)

# 打开 bochs 进行系统的运行和调试
debug: $(FD)
	bochs -q

# 运行Flyanx系统：打印提示信息
run: $(FD)
	@echo "使用Vbox等虚拟机挂载Flyanx.img软盘，即可开始运行！"

# 清理所有的中间编译文件
clean:
	@echo ""

# 完全清理：清理所有的中间编译文件以及生成的目标文件（二进制文件）
realclean: clean
	-rm -f $(FlyanxBoot)
#============================================================================
#   目标文件生成规则
#----------------------------------------------------------------------------
# 软盘镜像文件不存在，应该怎么生成？  生成规则
$(FD):
	dd if=/dev/zero of=$(FD) bs=512 count=2880

# 引导程序的生成规则
$(tb)/boot.bin: src/boot/include/fat12hdr.inc
$(tb)/boot.bin: src/boot/boot.asm
	$(ASM) $(ASMFlagsOfBoot) -o $@ $<

# 加载程序Loader
$(tb)/loader.bin: src/boot/loader.asm
	$(ASM) $(ASMFlagsOfBoot) -o $@ $<

#============================================================================
#   中间Obj文件生成规则
#----------------------------------------------------------------------------



#============================================================================