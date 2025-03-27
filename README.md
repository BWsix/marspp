## What is MARS++?

MARS++ adds C-like "function calls" to MIPS assembly language (targeting MARS system call). Currently, the following "function" are supported:

- print_string(label | "string literal" [, label | "string literal"])
- exit()

## Example

Given an input file named `input.asm`:

```asm
.text
	print_string("Hello world!")
	exit()
```

After running the following command:

```bash
marspp input.asm output.asm
```

MARS++ will generate an `output.asm` with the corresponding assembly code:

```asm
.text
	la $a0, label_marspp_0	# load Hello world!
	li $v0, 4	# specify print string service
	syscall	# print Hello world!
	li $v0, 10	# specify exit service
	syscall	# exit
	
.data

label_marspp_0:	.asciiz	"Hello world!"
```

## Getting Started

Usage:

```bash
marspp <input.asm> <output.asm>
```

## References

- c_lexer - [stb_c_lexer.h](https://github.com/nothings/stb/blob/master/stb_c_lexer.h)
- Dynamic array - [stb_ds.h](https://github.com/nothings/stb/blob/master/stb_ds.h)
