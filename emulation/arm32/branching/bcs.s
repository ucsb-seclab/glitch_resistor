# config:{"bytes_to_trash": "4,5,6,7", "arch":2}
cmp r7, r5
bcs .good
mov r0, 0xdead
.good:
mov r1, 0xaaaa