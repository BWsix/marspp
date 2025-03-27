# Compare input with 50

.data
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
	print_string("Input is equal to the preset value")
	exit()

GT:
	print_string("Input is greater than the preset value")
	exit()

LT:
	print_string("Input is less than the preset value")
	exit()