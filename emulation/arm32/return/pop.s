# config:{"bytes_to_trash": "16,17,18,19", "arch":2}
bl .next
mov r1, 0xaaaa
.end:
    b .end
.next:
    push {lr}
    pop {pc}
    mov r0, 0xdead
