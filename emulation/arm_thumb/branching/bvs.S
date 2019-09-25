# config:{"bytes_to_trash": "2,3"}
adds r0, r7, r7
BVS .good
mov r0, 0xdead
.good:
mov r1, 0xaaaa