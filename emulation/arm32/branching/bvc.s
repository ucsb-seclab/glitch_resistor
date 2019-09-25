# config:{"bytes_to_trash": "4,5,6,7", "arch":2}
adds r0, r0, r0
BVC .good
mov r0, 0xdead
.good:
mov r1, 0xaaaa