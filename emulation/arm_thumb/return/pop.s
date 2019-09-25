# config:{"bytes_to_trash": "12,13"}
bl .next
mov r1, 0xaaaa
.end:
    b .end
.next:
    push {lr}
    pop {pc}
    mov r0, 0xdead
