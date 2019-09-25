# config:{"bytes_to_trash": "4,5,6,7", "arch":2}
subs r0, r1, r7
bmi .good
mov r0, 0xdead
.good:
mov r1, 0xaaaa