# MARSPP - MIPS Assembly Preprocessor (Targeting MARS System Call)

## What is MARSPP?

MARSPP is a preprocessor that adds C-like "function calls" to MIPS assembly language. Currently, MARSPP supports the `print_string(label)` function.

For example, given an input file named `input.asm`:

```asm
.data
label:	.asciiz	"Hello world"

.text
	print_string(label)

    # ...
```

After running the following command:

```bash
marspp input.asm output.asm
```

MARSPP will generate an `output.asm` file where the `print_string(label)` directive gets expanded to:

```asm
.data
label:	.asciiz	"Hello world"

.text
	la $a0, label	# load label
	li $v0, 4	# specify print string service
	syscall	# print label

    # ...
```

## Getting Started

You can build MARSPP using CMake.

Usage:

```bash
marspp <input.asm> <output.asm>
```

## References

- c_lexer - [stb_c_lexer.h](https://github.com/nothings/stb/blob/master/stb_c_lexer.h)
- Dynamic array - [stb_ds.h](https://github.com/nothings/stb/blob/master/stb_ds.h)
