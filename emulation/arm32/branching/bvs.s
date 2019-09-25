# config:{"bytes_to_trash": "4,5,6,7", "arch":2}
adds r0, r7, r7
BVS .good
mov r0, 0xdead
.good:
mov r1, 0xaaaa