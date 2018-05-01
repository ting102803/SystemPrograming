.section .data
msg :
	.string "Number = %d Age = %d Name = %s \n"
Number:
	.int 201302423
Age:
	.int 24
Name :
	.string	"ShinJongWook\0"

.section .text
.global main

main:
	movq $msg, %rdi
	movq Number, %rsi
	movq Age, %rdx
	movq $Name, %rcx

	movq $0,%rax
	call printf
	movq $0,%rax
	ret
