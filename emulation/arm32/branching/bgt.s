# config:{"bytes_to_trash": "4,5,6,7", "arch":2}
cmp r1, r0
bgt .good
mov r0, 0xdead
.good:
mov r1, 0xaaaa