; nasm -f elf64 -l 1-nasm.lst 1-nasm.s  ;  ld -s -o 1-nasm 1-nasm.o

; rdi rsi rdx rcx r8 r9

; gcc -no-pie main.c asm.o

section .text

global _GetTick                               ; predefined entry point name for ld

_GetTick:       rdtsc
                shl rdx, 32
                add rax, rdx
                ret

