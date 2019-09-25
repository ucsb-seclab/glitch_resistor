# config:{"bytes_to_trash": "2,3"}
cmp r0, r5
beq .good
mov r0, 0xdead
.good:
mov r1, 0xaaaa