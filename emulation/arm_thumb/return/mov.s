# config:{"bytes_to_trash": "2,3"}
adr r0, .good
mov pc, r0
mov r0, 0xdead
.good:
mov r1, 0xaaaa
