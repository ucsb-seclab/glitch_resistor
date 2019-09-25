# config:{"bytes_to_trash": "12,13,14,15", "arch":2}
bl .fn
mov r1, 0xaaaa
.end:
    b .end
.fn:
    bx lr
    mov r0, 0xdead
