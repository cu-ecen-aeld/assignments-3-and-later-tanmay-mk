## Output trace of oops message after executing 'echo “hello_world” > /dev/faulty' in the qemu instance  
  
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000  
Mem abort info:  
  ESR = 0x96000046  
  EC = 0x25: DABT (current EL), IL = 32 bits  
  SET = 0, FnV = 0  
  EA = 0, S1PTW = 0  
Data abort info:  
  ISV = 0, ISS = 0x00000046  
  CM = 0, WnR = 1  
user pgtable: 4k pages, 39-bit VAs, pgdp=0000000042237000  
[0000000000000000] pgd=0000000041ff9003, p4d=0000000041ff9003, pud=0000000041ff9003, pmd=0000000000000000  
Internal error: Oops: 96000046 [#1] SMP  
Modules linked in: hello(O) scull(O) faulty(O)  
CPU: 0 PID: 152 Comm: sh Tainted: G           O      5.10.7 #1  
Hardware name: linux,dummy-virt (DT)  
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO BTYPE=--)  
pc : faulty_write+0x10/0x20 [faulty]  
lr : vfs_write+0xc0/0x290  
sp : ffffffc010c53db0  
x29: ffffffc010c53db0 x28: ffffff8001ff6400  
x27: 0000000000000000 x26: 0000000000000000  
x25: 0000000000000000 x24: 0000000000000000  
x23: 0000000000000000 x22: ffffffc010c53e30  
x21: 00000000004c9940 x20: ffffff8001fa6200  
x19: 0000000000000012 x18: 0000000000000000  
x17: 0000000000000000 x16: 0000000000000000  
x15: 0000000000000000 x14: 0000000000000000  
x13: 0000000000000000 x12: 0000000000000000  
x11: 0000000000000000 x10: 0000000000000000  
x9 : 0000000000000000 x8 : 0000000000000000  
x7 : 0000000000000000 x6 : 0000000000000000  
x5 : ffffff80020177b8 x4 : ffffffc008670000  
x3 : ffffffc010c53e30 x2 : 0000000000000012  
x1 : 0000000000000000 x0 : 0000000000000000  
Call trace:  
 faulty_write+0x10/0x20 [faulty]  
 ksys_write+0x6c/0x100  
 __arm64_sys_write+0x1c/0x30  
 el0_svc_common.constprop.0+0x9c/0x1c0  
 do_el0_svc+0x70/0x90  
 el0_svc+0x14/0x20  
 el0_sync_handler+0xb0/0xc0  
 el0_sync+0x174/0x180  
Code: d2800001 d2800000 d503233f d50323bf (b900003f)  
---[ end trace b2db6f2a75b58db7 ]---  

## Analysis of the oops message  
  
The oops messages are generated due to dereferencing of a NULL pointer or when an erroneous pointer value is used. In this particular case, the oops message was generated because an attempt to dereference a NULL pointer was made. This can be observed by looking at the very first line of the oops message: Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000.  
  
In Linux, all addressess are virtual addresses that are mapped to physical addresses via page tables. The paging mechanism fails to map the pointer to a physical location when an invalid NULL pointer dereference occurs, resulting in a page fault.  
  
The line : 'pc : faulty_write+0x10/0x20 [faulty]'  
tells us that the page fault originated from faulty_write() method.  

ssize_t faulty_write (struct file *filp, const char __user *buf, size_t count,loff_t *pos)  
{  
	/* make a simple fault by dereferencing a NULL pointer */  
	*(int *)0 = 0;  
	return 0;  
}  
By checking the code in faulty_write method, we see that the code is trying to dereference a NULL pointer of type int. Because of this invalid pointer access, a page fault occurs and the kernel generates an oops message.  

