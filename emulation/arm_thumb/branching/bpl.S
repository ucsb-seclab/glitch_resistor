# config:{"bytes_to_trash": "2,3"}
adds r0, r1, r2
BPL .good
mov r0, 0xdead
.good:
mov r1, 0xaaaa