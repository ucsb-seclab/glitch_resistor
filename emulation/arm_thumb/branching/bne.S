# config:{"bytes_to_trash": "2,3"}
cmp r0, r1
bne .fail
mov r0, 0xdead
.fail:
mov r1, 0xaaaa