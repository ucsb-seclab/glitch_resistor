# config:{"bytes_to_trash": "8,9,10,11", "arch":3}
li $t3, 0x1234
li $t4, 0x1234
beq $t3, $t4, .good
.glitch:
	li $t1, 0xdead
	b .glitch
.good:
	li $t2, 0xaaaa
