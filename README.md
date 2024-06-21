# binaryGadgeteer 
binaryGadgeteer finds gadgets inside X86 & X86-64 (32/64 bit) ELF's.
similer to ROPgadget and ropper.

note that this is using zydis disassembler, and not capstone disassembler unlike ropper and ROPgadget.

(Zydis supports only  X86 & X86-64)
## installation 

$ sudo apt install libelf-dev  
 
$ install from here: https://github.com/zyantific/zydis  
$ git clone --recursive 'https://github.com/zyantific/zydis.git  
$ cd zydis  
$ cmake -B build  
$ cmake --build build -j4  
 
$ sudo apt install doxygen 
