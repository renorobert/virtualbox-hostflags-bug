# CVE-2018-2843 

**Bug: offLocation provided by guest is not validated in hgsmiChannelHandler (HGSMIHost.cpp)**

HGSMI_CC_HOST_FLAGS_LOCATION is used by guest to tell the host the location of HGSMIHOSTFLAGS structure in VRAM buffer. However
pLoc->offLocation used for calculating the address pHGFlags in VRAM buffer is not validated. This results in memory corruption.

```c
static DECLCALLBACK(int) hgsmiChannelHandler (void *pvHandler, uint16_t u16ChannelInfo, void *pvBuffer, HGSMISIZE cbBuffer)
{
. . .
    switch (u16ChannelInfo)
    {
        case HGSMI_CC_HOST_FLAGS_LOCATION:
        {
            if (cbBuffer < sizeof (HGSMIBUFFERLOCATION))
            {
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            HGSMIBUFFERLOCATION *pLoc = (HGSMIBUFFERLOCATION *)pvBuffer;
            if (pLoc->cbLocation != sizeof(HGSMIHOSTFLAGS))
            {
                rc = VERR_INVALID_PARAMETER;
                break;
            }

            pIns->pHGFlags = (HGSMIHOSTFLAGS*)HGSMIOffsetToPointer (&pIns->area, pLoc->offLocation);
. . .
}
```

**Triggering the Bug:**

One of the code path to trigger the issue is vgaR3IOPortHGSMIWrite() in DevVGA.cpp. The provided PoC triggers a crash by writing to .text segment of VBoxSharedFolders.so, mapped immediately after the VRAM buffer. The bug provides limited control over data written.

```c
static DECLCALLBACK(int) vgaR3IOPortHGSMIWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb)
{
. . .
    if (cb == 4)
    {
        switch (Port)
        {
            case VGA_PORT_HGSMI_HOST: /* Host */
            {
# if defined(VBOX_WITH_VIDEOHWACCEL) || defined(VBOX_WITH_VDMA) || defined(VBOX_WITH_WDDM)
                if (u32 == HGSMIOFFSET_VOID)
                {
                    PDMCritSectEnter(&pThis->CritSectIRQ, VERR_SEM_BUSY);

                    if (pThis->fu32PendingGuestFlags == 0)
                    {
                        PDMDevHlpPCISetIrqNoWait(pDevIns, 0, PDM_IRQ_LEVEL_LOW);
                        HGSMIClearHostGuestFlags(pThis->pHGSMI,
                                                 HGSMIHOSTFLAGS_IRQ
#  ifdef VBOX_VDMA_WITH_WATCHDOG
                                                 | HGSMIHOSTFLAGS_WATCHDOG
#  endif
                                                 | HGSMIHOSTFLAGS_VSYNC
                                                 | HGSMIHOSTFLAGS_HOTPLUG
                                                 | HGSMIHOSTFLAGS_CURSOR_CAPABILITIES
                                                );
                    }
                    else
                    {
                        HGSMISetHostGuestFlags(pThis->pHGSMI, HGSMIHOSTFLAGS_IRQ | pThis->fu32PendingGuestFlags);
                        pThis->fu32PendingGuestFlags = 0;
. . .
}
```
```
renorobert@ubuntuguest:~/CVE-2018-2843$ sudo ./poc 
[sudo] password for renorobert: 
poc: [+] VRAM buffer size = 0x1000000
poc: [+] VRAM buffer mapped @ 0x7f7416e24000
poc: [+] Setting up payload...
```

```
gdb-peda$ c
Continuing.
 
Thread 14 "EMT-1" received signal SIGSEGV, Segmentation fault.
[Switching to Thread 0x7fffba3f9700 (LWP 10535)]

[----------------------------------registers-----------------------------------]
RAX: 0x7fff89ea4000 --> 0x10102464c457f 
RBX: 0x7fffdc022000 --> 0xffe40030 
RCX: 0x0 
RDX: 0xffff800000000000 
RSI: 0xffffff8d 
RDI: 0x7fff9c508110 --> 0x7ffff7e30000 --> 0x5 
RBP: 0x7fffba3f8ce0 --> 0x7fffba3f8d00 --> 0x7fffba3f8d50 --> 0x7fffba3f8d80 --> 0x7fffba3f8db0 --> 0x7fffba3f8df0 (--> ...)
RSP: 0x7fffba3f8ce0 --> 0x7fffba3f8d00 --> 0x7fffba3f8d50 --> 0x7fffba3f8d80 --> 0x7fffba3f8db0 --> 0x7fffba3f8df0 (--> ...)
RIP: 0x7fff8ac097ec (lock and DWORD PTR [rax],esi)
R8 : 0x0 
R9 : 0x0 
R10: 0xffffffff7fff8000 
R11: 0x246 
R12: 0x7fffdc035f10 --> 0x19790326 
R13: 0x7fffdc022000 --> 0xffe40030 
R14: 0x7fff8abab980 (cmp    r8d,0x4)
R15: 0x4
EFLAGS: 0x10246 (carry PARITY adjust ZERO sign trap INTERRUPT direction overflow)
[-------------------------------------code-------------------------------------]
   0x7fff8ac097e5:	test   rax,rdx
   0x7fff8ac097e8:	jne    0x7fff8ac097ef
   0x7fff8ac097ea:	not    esi
=> 0x7fff8ac097ec:	lock and DWORD PTR [rax],esi
   0x7fff8ac097ef:	pop    rbp
   0x7fff8ac097f0:	ret    
   0x7fff8ac097f1:	nop    DWORD PTR [rax+rax*1+0x0]
   0x7fff8ac097f6:	nop    WORD PTR cs:[rax+rax*1+0x0]
[------------------------------------stack-------------------------------------]
0000| 0x7fffba3f8ce0 --> 0x7fffba3f8d00 --> 0x7fffba3f8d50 --> 0x7fffba3f8d80 --> 0x7fffba3f8db0 --> 0x7fffba3f8df0 (--> ...)
0008| 0x7fffba3f8ce8 --> 0x7fff8ababa1a (mov    rdi,r12)
0016| 0x7fffba3f8cf0 --> 0x3b0 
0024| 0x7fffba3f8cf8 --> 0x7ffff7e37c60 --> 0x19280620 
0032| 0x7fffba3f8d00 --> 0x7fffba3f8d50 --> 0x7fffba3f8d80 --> 0x7fffba3f8db0 --> 0x7fffba3f8df0 --> 0x7fffba3f8e80 (--> ...)
0040| 0x7fffba3f8d08 --> 0x7fffdf424142 (<IOMIOPortWrite+194>:	mov    rdi,QWORD PTR [r13+0x40])
0048| 0x7fffba3f8d10 --> 0x0 
0056| 0x7fffba3f8d18 --> 0xfffffffff7e56000 
[------------------------------------------------------------------------------]
Legend: code, data, rodata, value
Stopped reason: SIGSEGV
0x00007fff8ac097ec in ?? () from /usr/lib/virtualbox/VBoxDD.so
gdb-peda$ 

gdb-peda$ vmmap $rax
Start              End                Perm	Name
0x00007fff89ea4000 0x00007fff89ead000 r-xp	/usr/lib/virtualbox/VBoxSharedFolders.so
```
## Environment

**VirtualBox:** Version 5.2.6 r120293

**Host OS:** Ubuntu Desktop 16.04.4 LTS 64-bit

**Guest OS:** Ubuntu Server 16.04.3 LTS 64-bit

The bug affects VirtualBox in default configuration for any Host OS
