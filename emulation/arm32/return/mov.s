# config:{"bytes_to_trash": "4,5,6,7", "arch":2}
adr r0, .good
mov pc, r0
mov r0, 0xdead
.good:
mov r1, 0xaaaa
