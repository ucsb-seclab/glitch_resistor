# config:{"bytes_to_trash": "4,5,6,7", "arch":2}
cmp r0, r1
bne .fail
mov r0, 0xdead
.fail:
mov r1, 0xaaaa