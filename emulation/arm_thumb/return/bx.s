# config:{"bytes_to_trash": "10,11"}
bl .fn
mov r1, 0xaaaa
.end:
    b .end
.fn:
    bx lr
    mov r0, 0xdead
