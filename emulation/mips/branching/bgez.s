# config:{"bytes_to_trash": "4,5,6,7", "arch":3}
li $t3, 0x1234
bgez $t3, .good
.glitch:
	li $t1, 0xdead
	b .glitch
.good:
	li $t2, 0xaaaa
