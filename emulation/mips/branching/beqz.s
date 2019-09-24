# config:{"bytes_to_trash": "4,5,6,7", "arch":3}
li $t3, 0
beqz $t3, .good
.glitched:
	li $t1, 0xdead
	b .glitched
.good:
	li $t2, 0xaaaa
