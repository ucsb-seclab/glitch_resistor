# config:{"bytes_to_trash": "4,5,6,7", "arch":3}
li $ra, 0x1014
jr $ra
.glitched:
li $t1, 0xdead
b .glitched
li $t2, 0xaaaa
#jr $ra
