#include "machdep.h"
#include "kernel.h"

void init_vm86()
{
    page_map(0, 0, 1, PTE_V|PTE_W|PTE_U);
}

void exit_vm86()
{
    page_unmap(0, 1);
}

#define LADDR(seg,off) ((uint32_t)(((uint16_t)(seg)<<4)+(uint16_t)(off)))
#define EFLAGS_IF   0x00200
int vm86_emulate(struct vm86_context *vm86ctx)
{
    int eaten = 1;
    int data32 = 0, addr32 = 0, rep = 0, segp = 0;
    int done = 0;

    uint16_t ip = LOWORD(vm86ctx->eip),
             sp = LOWORD(vm86ctx->esp);

    do {
        switch(*(uint8_t *)LADDR(vm86ctx->cs, ip)) {
        case 0x66: /*32-bit data*/ data32=1; break;
        case 0x67: /*32-bit addr*/ addr32=1; break;
        case 0xf2: /*REPNZ/REPNE*/    rep=1; break;
        case 0xf3: /*REP/REPZ/REPE*/  rep=1; break;
        case 0x2e: /*CS*/           segp=56; break;
        case 0x3e: /*DS*/           segp=76; break;
        case 0x26: /*ES*/           segp=72; break;
        case 0x36: /*SS*/           segp=68; break;
        case 0x65: /*GS*/           segp=84; break;
        case 0x64: /*FS*/           segp=80; break;
        case 0xf0: /*LOCK*/                  break;
        default: done = 1;                   break;
        }
        if(done)
            break;
        ip++;
    } while(1);

    switch(*(uint8_t *)LADDR(vm86ctx->cs, ip)) {
    case 0xfa: /*CLI*/
        vm86ctx->eflags &= ~EFLAGS_IF;
        ip++;
        break;
    case 0xfb: /*STI*/
        vm86ctx->eflags |= EFLAGS_IF;
        ip++;
        break;
    case 0x9c: /*PUSHF*/
        if(data32) {
            sp -= 4;
            *(uint32_t *)LADDR(vm86ctx->ss, sp) = vm86ctx->eflags & 0x001cffff;
        } else {
            sp -= 2;
            *(uint16_t *)LADDR(vm86ctx->ss, sp) = (uint16_t)vm86ctx->eflags;
        }
        ip++;
        break;
    case 0x9d: /*POPF*/
        if(data32) {
            uint32_t eflags = *(uint32_t *)LADDR(vm86ctx->ss, sp);
            vm86ctx->eflags &= 0x1b3000/*VM, RF, IOPL, VIP, VIF*/;
            eflags &= ~0x1b3000;
            vm86ctx->eflags |= eflags;
            sp += 4;
        } else {
            uint32_t eflags = *(uint16_t *)LADDR(vm86ctx->ss, sp);
            vm86ctx->eflags &= 0xffff3000/*IOPL*/;
            eflags &= ~0xffff3000;
            vm86ctx->eflags |= eflags;
            sp += 2;
        }
        ip++;
        break;
    case 0xcd: /*INT*/
        sp -= 2;
        *(uint16_t *)LADDR(vm86ctx->ss, sp) = (uint16_t)vm86ctx->eflags;
        sp -= 2;
        *(uint16_t *)LADDR(vm86ctx->ss, sp) = vm86ctx->cs;
        sp -= 2;
        *(uint16_t *)LADDR(vm86ctx->ss, sp) = ip + 2;

        {
            uint16_t *ivt = (uint16_t *)0;
            uint8_t x = *(uint8_t *)LADDR(vm86ctx->cs, ip + 1);
            ip = ivt[x * 2 + 0];
            vm86ctx->cs = ivt[x * 2 + 1];
        }
        break;
    case 0xcf: /*IRET*/
        if(data32) {
            ip = *(uint32_t *)LADDR(vm86ctx->ss, sp);
            sp += 4;

            vm86ctx->cs = *(uint32_t *)LADDR(vm86ctx->ss, sp);
            sp += 4;

            uint32_t eflags = *(uint32_t *)LADDR(vm86ctx->ss, sp);
            vm86ctx->eflags &= 0x1a3000/*VM, IOPL, VIP, VIF*/;
            eflags &= ~0x1a3000;
            vm86ctx->eflags |= eflags;
            sp += 4;
        } else {
            ip = *(uint16_t *)LADDR(vm86ctx->ss, sp);
            sp += 2;

            vm86ctx->cs = *(uint16_t *)LADDR(vm86ctx->ss, sp);
            sp += 2;

            uint32_t eflags = *(uint16_t *)LADDR(vm86ctx->ss, sp);
            vm86ctx->eflags &= 0xffff3000/*IOPL*/;
            eflags &= ~0xffff3000;
            vm86ctx->eflags |= eflags;
            sp += 2;
        }
        break;
    case 0xe6:/*OUT imm8, AL*/
        outportb(*(uint8_t *)LADDR(vm86ctx->cs, ip + 1),
                LOBYTE(LOWORD(vm86ctx->eax)));
        ip += 2;
        break;
    case 0xe7:/*OUT imm8, (E)AX*/
        if(data32) {
            outportl(*(uint8_t *)LADDR(vm86ctx->cs, ip + 1),
                     vm86ctx->eax);
        } else {
            outportw(*(uint8_t *)LADDR(vm86ctx->cs, ip + 1),
                     LOWORD(vm86ctx->eax));
        }
        ip += 2;
        break;
    case 0xee:/*OUT DX, AL*/
        outportb(LOWORD(vm86ctx->edx), LOBYTE(LOWORD(vm86ctx->eax)));
        ip++;
        break;
    case 0xef:/*OUT DX, (E)AX*/
        if(data32) {
            outportl(LOWORD(vm86ctx->edx), vm86ctx->eax);
        } else {
            outportw(LOWORD(vm86ctx->edx), LOWORD(vm86ctx->eax));
        }
        ip++;
        break;
    case 0xe4:/*IN AL, imm8*/
        {
            uint8_t al  = inportb(*(uint8_t *)LADDR(vm86ctx->cs, ip + 1));
            vm86ctx->eax = (vm86ctx->eax & 0xffffff00) | al;
        }
        ip += 2;
        break;
    case 0xe5:/*IN (E)AX, imm8*/
        if(data32) {
            vm86ctx->eax = inportl(*(uint8_t *)LADDR(vm86ctx->cs, ip + 1));
        } else {
            uint16_t ax  = inportw(*(uint8_t *)LADDR(vm86ctx->cs, ip + 1));
            vm86ctx->eax = (vm86ctx->eax & 0xffff0000) | ax;
        }
        ip += 2;
        break;
    case 0xec:/*IN AL, DX*/
        {
            uint8_t al  = inportb(LOWORD(vm86ctx->edx));
            vm86ctx->eax = (vm86ctx->eax & 0xffffff00) | al;
        }
        ip += 1;
        break;
    case 0xed:/*IN (E)AX, DX*/
        if(data32) {
            vm86ctx->eax = inportl(LOWORD(vm86ctx->edx));
        } else {
            uint16_t ax  = inportw(LOWORD(vm86ctx->edx));
            vm86ctx->eax = (vm86ctx->eax & 0xffff0000) | ax;
        }
        ip++;
        break;
    case 0x6c:/*INSB; INS m8, DX*/
        {
            if(addr32) {
                uint32_t ecx = rep ? vm86ctx->ecx : 1;

                while(ecx) {
                    *(uint8_t *)LADDR(vm86ctx->es, vm86ctx->edi/*XXX*/) =
                        inportb(LOWORD(vm86ctx->edx));
                    ecx--;
                    vm86ctx->edi += (vm86ctx->eflags & 0x400)?-1:1;
                }

                if(rep)
                    vm86ctx->ecx = ecx;
            } else {
                uint16_t cx=rep ? LOWORD(vm86ctx->ecx) : 1,
                         di=LOWORD(vm86ctx->edi);
                while(cx) {
                    *(uint8_t *)LADDR(vm86ctx->es, di) =
                        inportb(LOWORD(vm86ctx->edx));
                    cx--;
                    di += (vm86ctx->eflags & 0x400)?-1:1;
                }
                if(rep)
                    vm86ctx->ecx = (vm86ctx->ecx & 0xffff0000) | cx;
                vm86ctx->edi = (vm86ctx->edi & 0xffff0000) | di;
            }
        }
        ip++;
        break;
    case 0x6d:/*INSW; INSD; INS m16/m32, DX*/
        {
            if(addr32) {
                uint32_t ecx = rep ? vm86ctx->ecx : 1;

                while(ecx) {
                    if(data32) {
                        *(uint32_t *)LADDR(vm86ctx->es, vm86ctx->edi/*XXX*/) =
                            inportl(LOWORD(vm86ctx->edx));
                        vm86ctx->edi += (vm86ctx->eflags & 0x400)?-4:4;
                    } else {
                        *(uint16_t *)LADDR(vm86ctx->es, vm86ctx->edi/*XXX*/) =
                            inportw(LOWORD(vm86ctx->edx));
                        vm86ctx->edi += (vm86ctx->eflags & 0x400)?-2:2;
                    }
                    ecx--;
                }
                if(rep)
                    vm86ctx->ecx = ecx;
            } else {
                uint16_t cx=rep ? LOWORD(vm86ctx->ecx) : 1,
                         di=LOWORD(vm86ctx->edi);
                while(cx) {
                    if(data32) {
                        *(uint32_t *)LADDR(vm86ctx->es, di) =
                            inportl(LOWORD(vm86ctx->edx));
                        di += (vm86ctx->eflags & 0x400)?-4:4;
                    } else {
                        *(uint16_t *)LADDR(vm86ctx->es, di) =
                            inportw(LOWORD(vm86ctx->edx));
                        di += (vm86ctx->eflags & 0x400)?-2:2;
                    }
                    cx--;
                }
                if(rep)
                    vm86ctx->ecx = (vm86ctx->ecx & 0xffff0000) | cx;
                vm86ctx->edi = (vm86ctx->edi & 0xffff0000) | di;
            }
        }
        ip++;
        break;
    case 0x6e:/*OUTSB; OUTS DX, m8*/
        {
            uint16_t seg = vm86ctx->ds;

            if(segp)
                seg = *(uint16_t *)(((uint8_t *)vm86ctx)+segp);

            if(addr32) {
                uint32_t ecx = rep ? vm86ctx->ecx : 1;

                while(ecx) {
                    outportb(LOWORD(vm86ctx->edx),
                             *(uint8_t *)LADDR(seg, vm86ctx->esi/*XXX*/));
                    ecx--;
                    vm86ctx->esi += (vm86ctx->eflags & 0x400)?-1:1;
                }
                if(rep)
                    vm86ctx->ecx = ecx;
            } else {
                uint16_t cx=rep ? LOWORD(vm86ctx->ecx) : 1,
                         si=LOWORD(vm86ctx->esi);
                while(cx) {
                    outportb(LOWORD(vm86ctx->edx), *(uint8_t *)LADDR(seg, si));
                    cx--;
                    si += (vm86ctx->eflags & 0x400)?-1:1;
                }
                if(rep)
                    vm86ctx->ecx = (vm86ctx->ecx & 0xffff0000) | cx;
                vm86ctx->esi = (vm86ctx->esi & 0xffff0000) | si;
            }
        }
        ip++;
        break;
    case 0x6f:/*OUTSW; OUTSD; OUTS DX, m16/32*/
        {
            uint16_t seg = vm86ctx->ds;

            if(segp)
                seg = *(uint16_t *)(((uint8_t *)vm86ctx)+segp);

            if(addr32) {
                uint32_t ecx = rep ? vm86ctx->ecx : 1;

                while(ecx) {
                    if(data32) {
                        outportl(LOWORD(vm86ctx->edx),
                                 *(uint32_t *)LADDR(seg, vm86ctx->esi/*XXX*/));
                        vm86ctx->esi += (vm86ctx->eflags & 0x400)?-4:4;
                    } else {
                        outportw(LOWORD(vm86ctx->edx),
                                 *(uint16_t *)LADDR(seg, vm86ctx->esi/*XXX*/));
                        vm86ctx->esi += (vm86ctx->eflags & 0x400)?-2:2;
                    }
                    ecx--;
                }
                if(rep)
                    vm86ctx->ecx = ecx;
            } else {
                uint16_t cx=rep ? LOWORD(vm86ctx->ecx) : 1,
                         si=LOWORD(vm86ctx->esi);
                while(cx) {
                    if(data32) {
                        outportl(LOWORD(vm86ctx->edx), *(uint32_t *)LADDR(seg, si));
                        si += (vm86ctx->eflags & 0x400)?-4:4;
                    } else {
                        outportw(LOWORD(vm86ctx->edx), *(uint16_t *)LADDR(seg, si));
                        si += (vm86ctx->eflags & 0x400)?-2:2;
                    }
                    cx--;
                }
                if(rep)
                    vm86ctx->ecx = (vm86ctx->ecx & 0xffff0000) | cx;
                vm86ctx->esi = (vm86ctx->esi & 0xffff0000) | si;
            }
        }
        ip++;
        break;
    default:
        eaten = 0;
        break;
    }

    vm86ctx->eip = (vm86ctx->eip & 0xffff0000) | ip;
    vm86ctx->esp = (vm86ctx->esp & 0xffff0000) | sp;

    return eaten;
}
