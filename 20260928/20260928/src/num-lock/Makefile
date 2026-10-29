CROSS = x86_64-elf-

all: boot.bin boot.dis

boot.o: boot.s
	$(CROSS)gcc -m32 -c boot.s

boot.bin: boot.o
	$(CROSS)objcopy -O binary boot.o  boot.bin

boot.dis: boot.o
	$(CROSS)objdump -D -mi8086 boot.o > boot.dis

qemu:
	-qemu-system-x86_64 boot.bin

qemu-monitor:
	-qemu-system-x86_64 -monitor stdio boot.bin
# QEMU上で num-lock の状態を調べる方法は（今は）たぶん無い


clean:
	rm -f boot.bin boot.o *~ \#*\# boot.dis 
