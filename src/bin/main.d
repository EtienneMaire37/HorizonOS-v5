bin/main.o: src/kernel/main.c src/kernel/cpu/temp.h \
 src/kernel/cpu/cpuid.h src/kernel/cpu/../debug/out.h \
 src/kernel/cpu/../debug/../time/time.h \
 src/kernel/cpu/../debug/../time/../cpu/util.h \
 src/kernel/cpu/../debug/../time/../cpu/../util/cfunc.h \
 src/kernel/cpu/../debug/../time/../multicore/spinlock.h \
 src/kernel/cpu/../debug/../time/../multicore/../cpu/registers.h \
 src/kernel/cpu/../debug/../time/../multicore/../util/error.h \
 src/kernel/cpu/../debug/../time/ktime.h \
 src/kernel/cpu/../debug/../util/evaluate.h \
 src/kernel/cpu/../debug/../io/io.h src/kernel/cpu/msr.h \
 src/kernel/cpu/../util/likely.h src/kernel/boot/limine.h \
 limine-protocol/include/limine.h src/kernel/cpu/units.h \
 src/kernel/util/math.h src/kernel/util/memory.h \
 src/kernel/util/../multitasking/multitasking.h \
 src/kernel/util/../multitasking/../util/hashmap.h \
 src/kernel/util/../multitasking/../util/linked_list.h \
 src/kernel/util/../multitasking/queue.h \
 src/kernel/util/../multitasking/task.h \
 src/kernel/util/../multitasking/../vfs/vfs.h \
 src/kernel/util/../multitasking/../vfs/../multitasking/mutex.h \
 src/kernel/util/../multitasking/../vfs/../initrd/initrd.h \
 src/kernel/util/../multitasking/../vfs/../initrd/../files/ustar.h \
 src/kernel/util/../multitasking/../vfs/pipe.h \
 src/kernel/util/../multitasking/../vfs/../util/ring.h \
 src/kernel/util/../multitasking/../io/keyboard.h \
 src/kernel/util/../multitasking/../gdt/gdt.h \
 src/kernel/util/../multitasking/../cpu/segbase.h \
 src/kernel/graphics/linear_framebuffer.h \
 src/kernel/graphics/../files/psf.h src/kernel/graphics/../boot/limine.h \
 src/kernel/fpu/sse.h src/kernel/fpu/fpu.h src/kernel/fpu/../util/defs.h \
 src/kernel/time/gdn.h src/kernel/cmos/rtc.h src/kernel/cmos/cmos.h \
 src/kernel/pic/apic.h src/kernel/pic/defs.h \
 src/kernel/multitasking/loader.h src/kernel/multitasking/startup_data.h \
 src/kernel/terminal/textio.h src/kernel/terminal/../vga/constants.h \
 src/kernel/paging/paging.h src/kernel/int/int.h \
 src/kernel/memalloc/page_frame_allocator.h src/kernel/memalloc/mmap.h \
 src/kernel/cpu/memory.h src/kernel/int/idt.h src/kernel/pic/pic.h \
 src/kernel/acpi/tables.h src/kernel/acpi/defs.h src/kernel/ps2/ps2.h \
 src/kernel/ps2/keyboard.h src/kernel/pci/pci.h src/kernel/cpu/tsc.h \
 src/kernel/multitasking/signal.h src/kernel/multitasking/syscall.h \
 src/kernel/int/kernel_panic.h src/kernel/int/../memalloc/page_data.h \
 src/kernel/memalloc/virtual_memory_allocator.h src/kernel/vfs/table.h
src/kernel/cpu/temp.h:
src/kernel/cpu/cpuid.h:
src/kernel/cpu/../debug/out.h:
src/kernel/cpu/../debug/../time/time.h:
src/kernel/cpu/../debug/../time/../cpu/util.h:
src/kernel/cpu/../debug/../time/../cpu/../util/cfunc.h:
src/kernel/cpu/../debug/../time/../multicore/spinlock.h:
src/kernel/cpu/../debug/../time/../multicore/../cpu/registers.h:
src/kernel/cpu/../debug/../time/../multicore/../util/error.h:
src/kernel/cpu/../debug/../time/ktime.h:
src/kernel/cpu/../debug/../util/evaluate.h:
src/kernel/cpu/../debug/../io/io.h:
src/kernel/cpu/msr.h:
src/kernel/cpu/../util/likely.h:
src/kernel/boot/limine.h:
limine-protocol/include/limine.h:
src/kernel/cpu/units.h:
src/kernel/util/math.h:
src/kernel/util/memory.h:
src/kernel/util/../multitasking/multitasking.h:
src/kernel/util/../multitasking/../util/hashmap.h:
src/kernel/util/../multitasking/../util/linked_list.h:
src/kernel/util/../multitasking/queue.h:
src/kernel/util/../multitasking/task.h:
src/kernel/util/../multitasking/../vfs/vfs.h:
src/kernel/util/../multitasking/../vfs/../multitasking/mutex.h:
src/kernel/util/../multitasking/../vfs/../initrd/initrd.h:
src/kernel/util/../multitasking/../vfs/../initrd/../files/ustar.h:
src/kernel/util/../multitasking/../vfs/pipe.h:
src/kernel/util/../multitasking/../vfs/../util/ring.h:
src/kernel/util/../multitasking/../io/keyboard.h:
src/kernel/util/../multitasking/../gdt/gdt.h:
src/kernel/util/../multitasking/../cpu/segbase.h:
src/kernel/graphics/linear_framebuffer.h:
src/kernel/graphics/../files/psf.h:
src/kernel/graphics/../boot/limine.h:
src/kernel/fpu/sse.h:
src/kernel/fpu/fpu.h:
src/kernel/fpu/../util/defs.h:
src/kernel/time/gdn.h:
src/kernel/cmos/rtc.h:
src/kernel/cmos/cmos.h:
src/kernel/pic/apic.h:
src/kernel/pic/defs.h:
src/kernel/multitasking/loader.h:
src/kernel/multitasking/startup_data.h:
src/kernel/terminal/textio.h:
src/kernel/terminal/../vga/constants.h:
src/kernel/paging/paging.h:
src/kernel/int/int.h:
src/kernel/memalloc/page_frame_allocator.h:
src/kernel/memalloc/mmap.h:
src/kernel/cpu/memory.h:
src/kernel/int/idt.h:
src/kernel/pic/pic.h:
src/kernel/acpi/tables.h:
src/kernel/acpi/defs.h:
src/kernel/ps2/ps2.h:
src/kernel/ps2/keyboard.h:
src/kernel/pci/pci.h:
src/kernel/cpu/tsc.h:
src/kernel/multitasking/signal.h:
src/kernel/multitasking/syscall.h:
src/kernel/int/kernel_panic.h:
src/kernel/int/../memalloc/page_data.h:
src/kernel/memalloc/virtual_memory_allocator.h:
src/kernel/vfs/table.h:
