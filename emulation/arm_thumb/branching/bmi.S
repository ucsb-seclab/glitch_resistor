# config:{"bytes_to_trash": "2,3"}
subs r0, r1, r7
bmi .good
mov r0, 0xdead
.good:
mov r1, 0xaaaa