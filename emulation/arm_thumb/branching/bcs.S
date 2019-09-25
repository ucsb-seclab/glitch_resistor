# config:{"bytes_to_trash": "2,3"}
cmp r7, r5
bcs .good
mov r0, 0xdead
.good:
mov r1, 0xaaaa