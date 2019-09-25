# config:{"bytes_to_trash": "2,3"}
cmp r1, r0
bgt .good
mov r0, 0xdead
.good:
mov r1, 0xaaaa