# Compare input with 50

.data
label_marspp_0:	.asciiz	"Input is equal to the preset value"
label_marspp_1:	.asciiz	"Input is greater than the preset value"
label_marspp_2:	.asciiz	"Input is less than the preset value"
value:	.word	50	# integer of a preset value

.text
	li		$v0, 5				#specify read integer service
	syscall						#input integer
	la		$t1, value			#load the memory address of the preset value
	lw		$t1, 0($t1)			#load the preset value
	sub		$t2, $v0, $t1		#compare input with the preset value
	beq		$t2, $zero, EQ		#jump to "EQ" if input equal to the preset value
	bgtz	$t2, GT				#jump to "GT" if input greater than the preset value
	bltz	$t2, LT				#jump to "LT" if input greater than the preset value

EQ:
	la $a0, label_marspp_0	# load Input is equal to the preset value
	li $v0, 4	# specify print string service
	syscall	# print Input is equal to the preset value
	li $v0, 10	# specify exit service
	syscall	# exit
	

GT:
	la $a0, label_marspp_1	# load Input is greater than the preset value
	li $v0, 4	# specify print string service
	syscall	# print Input is greater than the preset value
	li $v0, 10	# specify exit service
	syscall	# exit
	

LT:
	la $a0, label_marspp_2	# load Input is less than the preset value
	li $v0, 4	# specify print string service
	syscall	# print Input is less than the preset value
	li $v0, 10	# specify exit service
	syscall	# exit
	