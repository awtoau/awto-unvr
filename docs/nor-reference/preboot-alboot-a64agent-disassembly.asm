
; ==== FUN_01029248 @ 01029248 ====
01029248:  mrs x0,mpidr_el1
0102924c:  mov x1,x0
01029250:  and x0,x0,#0xff
01029254:  and x1,x1,#0xff00
01029258:  lsr x1,x1,#0x6
0102925c:  add x0,x1,x0
01029260:  ret

; ==== FUN_01029268 @ 01029268 ====
01029268:  cmp x2,#0x0
0102926c:  b.eq 0x01029284
01029270:  mrs x2,sctlr_el1
01029274:  and x2,x2,#-0x5
01029278:  msr sctlr_el1,x2
0102927c:  isb 
01029280:  dsb SY
01029284:  mov x12,x0
01029288:  and x12,x12,#-0x2
0102928c:  mov x8,x1
01029290:  mov x10,#0x0
01029294:  msr csselr_el1,x10
01029298:  isb 
0102929c:  mrs x1,ccsidr_el1
010292a0:  and w2,w1,#0x7
010292a4:  add w2,w2,#0x4
010292a8:  ubfx w4,w1,#0x3,#0xa
010292ac:  clz w5,w4
010292b0:  lsl w9,w4,w5
010292b4:  mov w16,#0x1
010292b8:  lsl w16,w16,w5
010292bc:  ubfx w7,w1,#0xd,#0xf
010292c0:  lsl w7,w7,w2
010292c4:  mov w17,#0x1
010292c8:  lsl w17,w17,w2
010292cc:  orr w11,w10,w9
010292d0:  orr w11,w11,w7
010292d4:  cmp x8,#0x0
010292d8:  b.ne 0x010292e4
010292dc:  dc ISW,x11
010292e0:  b 0x010292e8
010292e4:  dc CISW,x11
010292e8:  subs w7,w7,w17
010292ec:  b.ge 0x010292cc
010292f0:  subs x9,x9,x16
010292f4:  b.ge 0x010292bc
010292f8:  add w10,w10,#0x2
010292fc:  cmp w12,w10
01029300:  dsb SY
01029304:  b.ge 0x01029294
01029308:  ret

; ==== FUN_01029398 @ 01029398 ====
01029398:  mrs x0,cntpct_el0
0102939c:  ret

; ==== FUN_010293a0 @ 010293a0 ====
010293a0:  stp x29,x30,[sp, #-0x50]!
010293a4:  mov x29,sp
010293a8:  stp x21,x22,[sp, #0x20]
010293ac:  mov x22,x0
010293b0:  mov w0,#0x0
010293b4:  stp x19,x20,[sp, #0x10]
010293b8:  adrp x19,0x102e000
010293bc:  mov x20,x2
010293c0:  mov x21,x1
010293c4:  add x19,x19,#0x0
010293c8:  bl 0x0102a684
010293cc:  mov x1,x22
010293d0:  mov x0,x19
010293d4:  bl 0x0102a9c8
010293d8:  mov w0,#0x0
010293dc:  bl 0x0102a6a0
010293e0:  ldp x0,x1,[x20]
010293e4:  stp x0,x1,[x29, #0x30]
010293e8:  ldp x0,x1,[x20, #0x10]
010293ec:  stp x0,x1,[x29, #0x40]
010293f0:  adrp x0,0x102a000
010293f4:  add x3,x29,#0x30
010293f8:  mov x2,x21
010293fc:  mov x1,#0x0
01029400:  add x0,x0,#0x818
01029404:  bl 0x0102a994
01029408:  mov w0,#0x0
0102940c:  bl 0x0102a684
01029410:  adrp x1,0x102e000
01029414:  mov x0,x19
01029418:  add x1,x1,#0x2b6
0102941c:  bl 0x0102a9c8
01029420:  ldp x19,x20,[sp, #0x10]
01029424:  ldp x21,x22,[sp, #0x20]
01029428:  ldp x29,x30,[sp], #0x50
0102942c:  mov w0,#0x0
01029430:  b 0x0102a6a0

; ==== FUN_01029434 @ 01029434 ====
01029434:  stp x29,x30,[sp, #-0x110]!
01029438:  mov x29,sp
0102943c:  str q0,[x29, #0x50]
01029440:  str x1,[x29, #0xd8]
01029444:  add x1,x29,#0x110
01029448:  str x1,[x29, #0x30]
0102944c:  str x1,[x29, #0x38]
01029450:  add x1,x29,#0xd0
01029454:  str x1,[x29, #0x40]
01029458:  mov w1,#0xffffffc8
0102945c:  str w1,[x29, #0x48]
01029460:  mov w1,#0xffffff80
01029464:  str w1,[x29, #0x4c]
01029468:  mov x1,x0
0102946c:  str x2,[x29, #0xe0]
01029470:  adrp x0,0x102e000
01029474:  str x3,[x29, #0xe8]
01029478:  add x0,x0,#0x3
0102947c:  ldp x2,x3,[x29, #0x30]
01029480:  stp x2,x3,[x29, #0x10]
01029484:  ldp x2,x3,[x29, #0x40]
01029488:  stp x2,x3,[x29, #0x20]
0102948c:  add x2,x29,#0x10
01029490:  str x4,[x29, #0xf0]
01029494:  str x5,[x29, #0xf8]
01029498:  str x6,[x29, #0x100]
0102949c:  str q1,[x29, #0x60]
010294a0:  str x7,[x29, #0x108]
010294a4:  str q2,[x29, #0x70]
010294a8:  str q3,[x29, #0x80]
010294ac:  str q4,[x29, #0x90]
010294b0:  str q5,[x29, #0xa0]
010294b4:  str q6,[x29, #0xb0]
010294b8:  str q7,[x29, #0xc0]
010294bc:  bl 0x010293a0
010294c0:  ldp x29,x30,[sp], #0x110
010294c4:  ret

; ==== FUN_010294c8 @ 010294c8 ====
010294c8:  stp x29,x30,[sp, #-0x20]!
010294cc:  mov x29,sp
010294d0:  stp x19,x20,[sp, #0x10]
010294d4:  mov x20,x0
010294d8:  bl 0x01029248
010294dc:  mov w19,w0
010294e0:  bl 0x01029248
010294e4:  mov w1,w0
010294e8:  adrp x0,0x102e000
010294ec:  add x0,x0,#0xc
010294f0:  bl 0x01029434
010294f4:  cbz x20,0x01029518
010294f8:  mov w0,#0x0
010294fc:  bl 0x0102a684
01029500:  adrp x0,0x102e000
01029504:  mov x1,x20
01029508:  add x0,x0,#0x25
0102950c:  bl 0x0102a9c8
01029510:  mov w0,#0x0
01029514:  bl 0x0102a6a0
01029518:  mov w0,#0x80ff
0102951c:  adrp x20,0x102e000
01029520:  bl 0x0102a680
01029524:  ldr w0,[x20, #0xa48]
01029528:  cbnz w0,0x01029540
0102952c:  mov w2,#0x100
01029530:  mov w1,#0x1
01029534:  bl 0x0102a308
01029538:  mov w0,#0x1
0102953c:  str w0,[x20, #0xa48]
01029540:  mov w0,#0x0
01029544:  bl 0x0102a684
01029548:  adrp x0,0x102e000
0102954c:  add x0,x0,#0x45
01029550:  bl 0x0102a9c8
01029554:  mov w0,#0x0
01029558:  bl 0x0102a6a0
0102955c:  mov w0,#0x0
01029560:  bl 0x0102a684
01029564:  adrp x0,0x102e000
01029568:  add x0,x0,#0x68
0102956c:  bl 0x0102a9c8
01029570:  mov w0,#0x0
01029574:  bl 0x0102a6a0
01029578:  mov w0,w19
0102957c:  bl 0x0102a8e0
01029580:  cbz w0,0x01029578
01029584:  mov w0,w19
01029588:  bl 0x0102a800
0102958c:  b 0x01029578

; ==== FUN_01029668 @ 01029668 ====
01029668:  ubfx x0,x0,#0x8,#0x8
0102966c:  ret

; ==== FUN_01029670 @ 01029670 ====
01029670:  uxtb w0,w0
01029674:  ret

; ==== FUN_01029678 @ 01029678 ====
01029678:  stp x29,x30,[sp, #-0x30]!
0102967c:  mov x29,sp
01029680:  stp x19,x20,[sp, #0x10]
01029684:  str x21,[sp, #0x20]
01029688:  mrs x20,mpidr_el1
0102968c:  mov w0,w20
01029690:  bl 0x01029668
01029694:  sxtw x19,w0
01029698:  mov w0,w20
0102969c:  add x19,x19,#0x1
010296a0:  bl 0x01029670
010296a4:  mov w21,w0
010296a8:  adrp x0,0x102e000
010296ac:  mov w2,#0x3
010296b0:  add x0,x0,#0xb40
010296b4:  mov w1,w21
010296b8:  add x19,x0,x19, LSL #0x4
010296bc:  mov x0,x19
010296c0:  bl 0x0102d158
010296c4:  bl 0x0102d684
010296c8:  mrs x20,sctlr_el3
010296cc:  mov w2,#0x1
010296d0:  mov w1,w2
010296d4:  mov w0,w2
010296d8:  bl 0x01029268
010296dc:  mrs x0,sreg(0x3, 0x1, c0xf, c0x2, 0x1)
010296e0:  and x1,x0,#-0x41
010296e4:  msr sreg(0x3, 0x1, c0xf, c0x2, 0x1),x1
010296e8:  isb 
010296ec:  dsb SY
010296f0:  wfi
010296f4:  msr sreg(0x3, 0x1, c0xf, c0x2, 0x1),x0
010296f8:  msr sctlr_el3,x20
010296fc:  bl 0x0102d7d0
01029700:  mov w1,w21
01029704:  mov x0,x19
01029708:  ldr x21,[sp, #0x20]
0102970c:  mov w2,#0x0
01029710:  ldp x19,x20,[sp, #0x10]
01029714:  ldp x29,x30,[sp], #0x30
01029718:  b 0x0102d158

; ==== FUN_01029940 @ 01029940 ====
01029940:  stp x29,x30,[sp, #-0x80]!
01029944:  mov x29,sp
01029948:  stp x19,x20,[sp, #0x10]
0102994c:  stp x21,x22,[sp, #0x20]
01029950:  stp x23,x24,[sp, #0x30]
01029954:  stp x25,x26,[sp, #0x40]
01029958:  str x27,[sp, #0x50]
0102995c:  mrs x0,sctlr_el3
01029960:  orr x0,x0,#0x1000
01029964:  msr sctlr_el3,x0
01029968:  mrs x20,mpidr_el1
0102996c:  mov w0,w20
01029970:  bl 0x01029668
01029974:  sxtw x19,w0
01029978:  mov w0,w20
0102997c:  bl 0x01029670
01029980:  mov w26,w0
01029984:  mrs x20,mpidr_el1
01029988:  mov w0,w20
0102998c:  bl 0x01029668
01029990:  mov w0,w20
01029994:  bl 0x01029670
01029998:  mov w20,w0
0102999c:  mov x1,#0xf0070000
010299a0:  add x0,x29,#0x60
010299a4:  bl 0x0102cea4
010299a8:  mov x2,#0x0
010299ac:  add x1,x29,#0x60
010299b0:  add x0,x29,#0x70
010299b4:  bl 0x0102cf20
010299b8:  mov w2,#0x3
010299bc:  mov w1,w20
010299c0:  add x0,x29,#0x70
010299c4:  bl 0x0102d158
010299c8:  orr w1,w26,w19
010299cc:  adrp x2,0x102e000
010299d0:  adrp x20,0x102e000
010299d4:  cbz w1,0x010299f4
010299d8:  add x0,x2,#0xb40
010299dc:  ldr w3,[x0, #0x20]
010299e0:  cbz w3,0x010299ec
010299e4:  ldr w0,[x0, #0x24]
010299e8:  cbnz w0,0x010299f4
010299ec:  wfe
010299f0:  b 0x010299d0
010299f4:  add x21,x20,#0xb40
010299f8:  sxtw x24,w26
010299fc:  ldr w0,[x21, #0x20]
01029a00:  cbz w0,0x01029a1c
01029a04:  add x0,x24,x19, LSL #0x2
01029a08:  mov x25,#0x0
01029a0c:  add x0,x0,#0x1
01029a10:  add x21,x21,x0, LSL #0x6
01029a14:  ldr x23,[x21, #0x8]
01029a18:  b 0x01029ad4
01029a1c:  mov x0,x21
01029a20:  mov x1,#0xf0070000
01029a24:  bl 0x0102cea4
01029a28:  adrp x0,0x102e000
01029a2c:  mov x1,x21
01029a30:  mov x2,#0x0
01029a34:  add x0,x0,#0xb50
01029a38:  bl 0x0102cf20
01029a3c:  adrp x2,0x1029000
01029a40:  mov w0,#0xf000
01029a44:  add x2,x2,#0x5c4
01029a48:  mov w1,#0x84000000
01029a4c:  movk w0,#0x8fff, LSL #16
01029a50:  bl 0x01029f2c
01029a54:  adrp x2,0x1029000
01029a58:  mov w1,#0x82000000
01029a5c:  add x2,x2,#0x50c
01029a60:  mov w0,#0x8fffffff
01029a64:  bl 0x01029f2c
01029a68:  adrp x2,0x1029000
01029a6c:  mov w1,#0x1
01029a70:  add x2,x2,#0xba0
01029a74:  movk w1,#0x8200, LSL #16
01029a78:  mov w0,#0x8fffffff
01029a7c:  bl 0x01029f2c
01029a80:  adrp x2,0x1029000
01029a84:  mov w1,#0x3
01029a88:  add x2,x2,#0xe20
01029a8c:  movk w1,#0x8200, LSL #16
01029a90:  mov w0,#0x8fffffff
01029a94:  bl 0x01029f2c
01029a98:  adrp x2,0x1029000
01029a9c:  mov w1,#0x4
01029aa0:  add x2,x2,#0xd7c
01029aa4:  movk w1,#0x8200, LSL #16
01029aa8:  mov w0,#0x8fffffff
01029aac:  bl 0x01029f2c
01029ab0:  mov x0,#0x40f8
01029ab4:  movk x0,#0xfbff, LSL #16
01029ab8:  ldr w23,[x0]
01029abc:  ldr w25,[x0, #0x4]
01029ac0:  mov w23,w23
01029ac4:  mov w25,w25
01029ac8:  dsb SY
01029acc:  mov w0,#0x1
01029ad0:  str w0,[x21, #0x20]
01029ad4:  cbnz x23,0x01029af4
01029ad8:  add x3,x19,#0x1
01029adc:  add x0,x20,#0xb40
01029ae0:  mov w2,#0x3
01029ae4:  mov w1,w26
01029ae8:  add x0,x0,x3, LSL #0x4
01029aec:  bl 0x0102d158
01029af0:  wfi
01029af4:  add x21,x19,#0x1
01029af8:  add x22,x20,#0xb40
01029afc:  mov w2,#0x0
01029b00:  mov w1,w26
01029b04:  add x21,x22,x21, LSL #0x4
01029b08:  mov x0,x21
01029b0c:  bl 0x0102d158
01029b10:  add x0,x24,x19, LSL #0x2
01029b14:  add x0,x0,#0x1
01029b18:  add x0,x22,x0, LSL #0x6
01029b1c:  ldr x0,[x0, #0x8]
01029b20:  ldr w0,[x22, #0x24]
01029b24:  cbnz w0,0x01029b48
01029b28:  mov x0,#0xf0000000
01029b2c:  bl 0x0102d6c0
01029b30:  dsb SY
01029b34:  mov w0,#0x1
01029b38:  str w0,[x22, #0x24]
01029b3c:  dsb SY
01029b40:  isb 
01029b44:  sev
01029b48:  add x22,x24,x19, LSL #0x2
01029b4c:  add x27,x20,#0xb40
01029b50:  add x22,x22,#0x1
01029b54:  lsl x22,x22,#0x6
01029b58:  ldr w0,[x27, x22, LSL #0x0]
01029b5c:  cbnz w0,0x01029b6c
01029b60:  bl 0x0102d6f4
01029b64:  mov w0,#0x1
01029b68:  str w0,[x27, x22, LSL #0x0]
01029b6c:  add x1,x19,#0x8
01029b70:  add x0,x20,#0xb40
01029b74:  add x0,x0,x1, LSL #0x2
01029b78:  cbz w26,0x01029b8c
01029b7c:  ldr w1,[x0, #0x8]
01029b80:  cbnz w1,0x01029b8c
01029b84:  wfe
01029b88:  b 0x01029b78
01029b8c:  add x0,x19,#0x8
01029b90:  add x22,x20,#0xb40
01029b94:  add x22,x22,x0, LSL #0x2
01029b98:  ldr w0,[x22, #0x8]
01029b9c:  cbnz w0,0x01029bc4
01029ba0:  mov w2,#0x10
01029ba4:  mov x0,x21
01029ba8:  mov w1,w2
01029bac:  bl 0x0102d04c
01029bb0:  bl 0x0102d6f0
01029bb4:  mov w0,#0x1
01029bb8:  bl 0x0102d810
01029bbc:  mov w0,#0x1
01029bc0:  str w0,[x22, #0x8]
01029bc4:  add x1,x24,x19, LSL #0x2
01029bc8:  add x0,x20,#0xb40
01029bcc:  add x1,x1,#0x1
01029bd0:  mov w2,#0x2
01029bd4:  add x1,x0,x1, LSL #0x6
01029bd8:  str w2,[x1, #0x4]
01029bdc:  bl 0x0102d040
01029be0:  mov w0,w0
01029be4:  msr cntfrq_el0,x0
01029be8:  bl 0x0102d7d0
01029bec:  bl 0x0102a0e4
01029bf0:  bl 0x0102a168
01029bf4:  bl 0x0102a12c
01029bf8:  cbz x25,0x01029c10
01029bfc:  blr x25
01029c00:  dmb SY
01029c04:  mov x0,#0x2000
01029c08:  movk x0,#0xfd88, LSL #16
01029c0c:  bl 0x0102c270
01029c10:  add x19,x24,x19, LSL #0x2
01029c14:  add x20,x20,#0xb40
01029c18:  add x19,x19,#0x1
01029c1c:  mov x0,x23
01029c20:  add x19,x20,x19, LSL #0x6
01029c24:  ldr x1,[x19, #0x10]
01029c28:  bl 0x0102a07c
01029c2c:  ldr x27,[sp, #0x50]
01029c30:  ldp x19,x20,[sp, #0x10]
01029c34:  ldp x21,x22,[sp, #0x20]
01029c38:  ldp x23,x24,[sp, #0x30]
01029c3c:  ldp x25,x26,[sp, #0x40]
01029c40:  ldp x29,x30,[sp], #0x80
01029c44:  ret

; ==== FUN_01029c48 @ 01029c48 ====
01029c48:  mul w1,w1,w2
01029c4c:  mov w5,#0x1
01029c50:  and w4,w1,#0x1f
01029c54:  ubfx x1,x1,#0x5,#0x1b
01029c58:  lsl x1,x1,#0x2
01029c5c:  lsl w2,w5,w2
01029c60:  sub w2,w2,#0x1
01029c64:  lsl w3,w3,w4
01029c68:  lsl w2,w2,w4
01029c6c:  ldr w5,[x0, x1, LSL #0x0]
01029c70:  and w3,w2,w3
01029c74:  bic w5,w5,w2
01029c78:  orr w3,w5,w3
01029c7c:  str w3,[x0, x1, LSL #0x0]
01029c80:  ret

; ==== FUN_01029c84 @ 01029c84 ====
01029c84:  stp x29,x30,[sp, #-0x20]!
01029c88:  mov x29,sp
01029c8c:  str x19,[sp, #0x10]
01029c90:  mrs x19,sreg(0x3, 0x0, c0xc, c0x8, 0x0)
01029c94:  and w1,w19,#0x3ff
01029c98:  cmp w1,#0x3ff
01029c9c:  b.eq 0x01029cec
01029ca0:  mov x0,#0x0
01029ca4:  adrp x2,0x102e000
01029ca8:  add x4,x2,#0xa60
01029cac:  mov w3,w0
01029cb0:  ldr w4,[x4, x0, LSL #0x2]
01029cb4:  cmp w4,w1
01029cb8:  b.ne 0x01029cd8
01029cbc:  adrp x0,0x102e000
01029cc0:  mov w1,#0x0
01029cc4:  add x0,x0,#0xa88
01029cc8:  ldr x2,[x0, w3, SXTW #0x3]
01029ccc:  mov w0,w19
01029cd0:  blr x2
01029cd4:  b 0x01029ce4
01029cd8:  add x0,x0,#0x1
01029cdc:  cmp x0,#0xa
01029ce0:  b.ne 0x01029ca8
01029ce4:  and x19,x19,#0xffffffff
01029ce8:  msr sreg(0x3, 0x0, c0xc, c0x8, 0x1),x19
01029cec:  ldr x19,[sp, #0x10]
01029cf0:  ldp x29,x30,[sp], #0x20
01029cf4:  ret

; ==== FUN_01029cf8 @ 01029cf8 ====
01029cf8:  stp x29,x30,[sp, #-0x50]!
01029cfc:  mov x29,sp
01029d00:  stp x19,x20,[sp, #0x10]
01029d04:  str x25,[sp, #0x40]
01029d08:  mov w20,w2
01029d0c:  stp x21,x22,[sp, #0x20]
01029d10:  stp x23,x24,[sp, #0x30]
01029d14:  and w25,w3,#0x1
01029d18:  adrp x2,0x102e000
01029d1c:  tbz w3,#0x0,0x01029d8c
01029d20:  mov x0,#0x0
01029d24:  add x5,x2,#0xa60
01029d28:  mov w4,w0
01029d2c:  ldr w5,[x5, x0, LSL #0x2]
01029d30:  cmp w5,w20
01029d34:  b.ne 0x01029d48
01029d38:  adrp x0,0x102e000
01029d3c:  add x0,x0,#0xa88
01029d40:  str x1,[x0, w4, SXTW #0x3]
01029d44:  b 0x01029eb8
01029d48:  add x0,x0,#0x1
01029d4c:  cmp x0,#0xa
01029d50:  b.ne 0x01029d24
01029d54:  mov x0,#0x0
01029d58:  add x4,x2,#0xa60
01029d5c:  mov w5,w0
01029d60:  ldr w6,[x4, x0, LSL #0x2]
01029d64:  cbnz w6,0x01029d7c
01029d68:  adrp x0,0x102e000
01029d6c:  str w20,[x4, w5, SXTW #0x2]
01029d70:  add x0,x0,#0xa88
01029d74:  str x1,[x0, w5, SXTW #0x3]
01029d78:  b 0x01029dc4
01029d7c:  add x0,x0,#0x1
01029d80:  cmp x0,#0xa
01029d84:  b.ne 0x01029d58
01029d88:  b 0x01029dc4
01029d8c:  mov x0,#0x0
01029d90:  add x4,x2,#0xa60
01029d94:  mov w1,w0
01029d98:  ldr w5,[x4, x0, LSL #0x2]
01029d9c:  cmp w5,w20
01029da0:  b.ne 0x01029db8
01029da4:  str wzr,[x4, w0, SXTW #0x2]
01029da8:  adrp x0,0x102e000
01029dac:  add x0,x0,#0xa88
01029db0:  str xzr,[x0, w1, SXTW #0x3]
01029db4:  b 0x01029eb8
01029db8:  add x0,x0,#0x1
01029dbc:  cmp x0,#0xa
01029dc0:  b.ne 0x01029d90
01029dc4:  mrs x0,daif
01029dc8:  and x0,x0,#-0x41
01029dcc:  msr daif,x0
01029dd0:  cmp w20,#0x1f
01029dd4:  uxtb w25,w25
01029dd8:  and w22,w3,#0x2
01029ddc:  mov x19,#0xf0200000
01029de0:  b.gt 0x01029df8
01029de4:  bl 0x01029248
01029de8:  add w0,w0,#0x14
01029dec:  lsl w0,w0,#0x11
01029df0:  mov x19,#0xf0010000
01029df4:  add x19,x19,w0, SXTW 
01029df8:  ubfx x21,x20,#0x5,#0x1b
01029dfc:  mov w24,#0x1
01029e00:  lsl x21,x21,#0x2
01029e04:  and w23,w20,#0x1f
01029e08:  add x0,x21,#0x180
01029e0c:  mov w3,#0x0
01029e10:  mov w2,w24
01029e14:  mov w1,w20
01029e18:  lsl w23,w24,w23
01029e1c:  str w23,[x19, x0, LSL #0x0]
01029e20:  add x0,x19,#0x80
01029e24:  bl 0x01029c48
01029e28:  mov w3,#0x0
01029e2c:  mov w2,#0x8
01029e30:  mov w1,w20
01029e34:  add x0,x19,#0x400
01029e38:  bl 0x01029c48
01029e3c:  cbnz w25,0x01029e84
01029e40:  mov w0,#0x0
01029e44:  bl 0x0102a684
01029e48:  adrp x4,0x102e000
01029e4c:  adrp x3,0x102e000
01029e50:  adrp x1,0x102e000
01029e54:  adrp x0,0x102e000
01029e58:  mov w5,#0x53
01029e5c:  add x4,x4,#0x290
01029e60:  add x3,x3,#0x218
01029e64:  mov w2,w24
01029e68:  add x1,x1,#0x284
01029e6c:  add x0,x0,#0xc1
01029e70:  bl 0x0102a9c8
01029e74:  mov w0,#0x0
01029e78:  bl 0x0102a6a0
01029e7c:  mov x0,#0x0
01029e80:  bl 0x010294c8
01029e84:  lsl w0,w20,#0x3
01029e88:  cmp w22,wzr
01029e8c:  mov w3,#0x3
01029e90:  add x21,x21,#0x100
01029e94:  sxtw x0,w0
01029e98:  csinc w3,w3,wzr,ne
01029e9c:  add x0,x0,#0x6, LSL #12
01029ea0:  mov w2,#0x2
01029ea4:  mov w1,w20
01029ea8:  str xzr,[x19, x0, LSL #0x0]
01029eac:  add x0,x19,#0xc00
01029eb0:  bl 0x01029c48
01029eb4:  str w23,[x19, x21, LSL #0x0]
01029eb8:  mov w0,#0x0
01029ebc:  ldr x25,[sp, #0x40]
01029ec0:  ldp x19,x20,[sp, #0x10]
01029ec4:  ldp x21,x22,[sp, #0x20]
01029ec8:  ldp x23,x24,[sp, #0x30]
01029ecc:  ldp x29,x30,[sp], #0x50
01029ed0:  ret

; ==== FUN_01029ee8 @ 01029ee8 ====
01029ee8:  stp x29,x30,[sp, #-0x10]!
01029eec:  mov x29,sp
01029ef0:  ldr x1,[x0, #0xad8]
01029ef4:  cbnz x1,0x01029f18
01029ef8:  mov w0,#0x0
01029efc:  bl 0x0102a684
01029f00:  adrp x0,0x102e000
01029f04:  add x0,x0,#0x2a8
01029f08:  bl 0x0102a9c8
01029f0c:  mov w0,#0x0
01029f10:  bl 0x0102a6a0
01029f14:  b 0x01029f14
01029f18:  mov x0,#0x0
01029f1c:  blr x1
01029f20:  mov w0,#0x0
01029f24:  ldp x29,x30,[sp], #0x10
01029f28:  ret

; ==== FUN_01029f2c @ 01029f2c ====
01029f2c:  adrp x3,0x102e000
01029f30:  mov x4,#0x0
01029f34:  add x3,x3,#0xc80
01029f38:  lsl x6,x4,#0x4
01029f3c:  ldr w6,[x6, x3, LSL #0x0]
01029f40:  cbz w6,0x01029f58
01029f44:  add x4,x4,#0x1
01029f48:  cmp x4,#0xa
01029f4c:  b.ne 0x01029f38
01029f50:  mov w0,#0xffffffff
01029f54:  b 0x01029f70
01029f58:  sbfiz x4,x4,#0x4,#0x20
01029f5c:  add x5,x3,x4
01029f60:  str w0,[x3, x4, LSL #0x0]
01029f64:  mov w0,#0x0
01029f68:  str w1,[x5, #0x4]
01029f6c:  str x2,[x5, #0x8]
01029f70:  ret

; ==== FUN_0102a07c @ 0102a07c ====
0102a07c:  mov x4,x0
0102a080:  mov x0,x1
0102a084:  mov x3,xzr
0102a088:  mov x2,xzr
0102a08c:  mov x1,xzr
0102a090:  br x4

; ==== FUN_0102a0e4 @ 0102a0e4 ====
0102a0e4:  mrs x0,currentel
0102a0e8:  cmp x0,#0xc
0102a0ec:  ccmp x0,#0xd,#0x4,ne
0102a0f0:  b.eq 0x0102a0f8
0102a0f4:  ret
0102a0f8:  ldr x0,0x102a180
0102a0fc:  msr scr_el3,x0
0102a100:  ldr x0,0x102a188
0102a104:  msr actlr_el2,x0
0102a108:  msr actlr_el3,x0
0102a10c:  ldr x0,0x102a190
0102a110:  msr vbar_el3,x0
0102a114:  mrs x0,sreg(0x3, 0x1, c0xf, c0x2, 0x1)
0102a118:  orr x0,x0,#0x40
0102a11c:  msr sreg(0x3, 0x1, c0xf, c0x2, 0x1),x0
0102a120:  mov x0,#0xf
0102a124:  msr sreg(0x3, 0x6, c0xc, c0xc, 0x5),x0
0102a128:  ret

; ==== FUN_0102a12c @ 0102a12c ====
0102a12c:  mov x0,#0x3c9
0102a130:  msr spsr_el3,x0
0102a134:  adr x0,0x102a160
0102a138:  msr elr_el3,x0
0102a13c:  mov x2,x30
0102a140:  bl 0x01029248
0102a144:  mov x30,x2
0102a148:  ldr x1,0x102a198
0102a14c:  add x1,x1,x0, LSL #0x3
0102a150:  ldr x1,[x1]
0102a154:  mov x0,sp
0102a158:  mov sp,x1
0102a15c:  eret

; ==== FUN_0102a168 @ 0102a168 ====
0102a168:  ret

; ==== FUN_0102a308 @ 0102a308 ====
0102a308:  mov w3,w0
0102a30c:  mov w0,#0xffffffea
0102a310:  cmp w3,#0x10
0102a314:  b.gt 0x0102a344
0102a318:  cbnz w1,0x0102a330
0102a31c:  lsl w3,w3,#0x18
0102a320:  mov w2,w2
0102a324:  add x3,x2,w3, SXTW 
0102a328:  msr sreg(0x3, 0x0, c0xc, c0xb, 0x5),x3
0102a32c:  b 0x0102a340
0102a330:  mov x0,#0x9f00
0102a334:  add w3,w3,w2, LSL #0x10
0102a338:  movk x0,#0xf000, LSL #16
0102a33c:  str w3,[x0]
0102a340:  mov w0,#0x0
0102a344:  ret

; ==== FUN_0102a348 @ 0102a348 ====
0102a348:  adrp x0,0x102e000
0102a34c:  ldr w0,[x0, #0xb04]
0102a350:  cmp w0,#0x2
0102a354:  b.ne 0x0102a364
0102a358:  adrp x0,0x102e000
0102a35c:  ldr w0,[x0, #0xae8]
0102a360:  b 0x0102a36c
0102a364:  adrp x0,0x102e000
0102a368:  ldr w0,[x0, #0xae0]
0102a36c:  ret

; ==== FUN_0102a370 @ 0102a370 ====
0102a370:  adrp x0,0x102e000
0102a374:  stp x29,x30,[sp, #-0x10]!
0102a378:  mov x29,sp
0102a37c:  ldr w0,[x0, #0xb04]
0102a380:  cbnz w0,0x0102a38c
0102a384:  bl 0x01029398
0102a388:  b 0x0102a3b4
0102a38c:  cmp w0,#0x1
0102a390:  b.ne 0x0102a3a4
0102a394:  adrp x0,0x102e000
0102a398:  ldr x0,[x0, #0xa40]
0102a39c:  ldr w0,[x0, #0x1008]
0102a3a0:  b 0x0102a3b4
0102a3a4:  adrp x0,0x102e000
0102a3a8:  add x0,x0,#0xaf0
0102a3ac:  bl 0x0102ce38
0102a3b0:  mvn w0,w0
0102a3b4:  ldp x29,x30,[sp], #0x10
0102a3b8:  ret

; ==== FUN_0102a3bc @ 0102a3bc ====
0102a3bc:  adrp x4,0x102e000
0102a3c0:  lsr w2,w2,#0x4
0102a3c4:  str w0,[x4, #0xb04]
0102a3c8:  adrp x0,0x102e000
0102a3cc:  str w1,[x0, #0xb00]
0102a3d0:  adrp x0,0x102e000
0102a3d4:  mov w1,#0x1
0102a3d8:  str w2,[x0, #0xae0]
0102a3dc:  adrp x0,0x102e000
0102a3e0:  str w3,[x0, #0xae8]
0102a3e4:  adrp x0,0x102e000
0102a3e8:  str w1,[x0, #0xae4]
0102a3ec:  ret

; ==== FUN_0102a3f0 @ 0102a3f0 ====
0102a3f0:  stp x29,x30,[sp, #-0x40]!
0102a3f4:  mov x29,sp
0102a3f8:  stp x19,x20,[sp, #0x10]
0102a3fc:  mov w19,w0
0102a400:  adrp x0,0x102e000
0102a404:  mov w20,w1
0102a408:  str x23,[sp, #0x30]
0102a40c:  stp x21,x22,[sp, #0x20]
0102a410:  ldr w1,[x0, #0xae4]
0102a414:  mov w22,w2
0102a418:  mov w23,w3
0102a41c:  mov x21,x0
0102a420:  cbz w1,0x0102a468
0102a424:  mov w0,#0x0
0102a428:  bl 0x0102a684
0102a42c:  adrp x4,0x102e000
0102a430:  adrp x3,0x102e000
0102a434:  adrp x1,0x102e000
0102a438:  adrp x0,0x102e000
0102a43c:  mov w5,#0xe3
0102a440:  add x4,x4,#0x358
0102a444:  add x3,x3,#0x2c0
0102a448:  mov w2,#0x1
0102a44c:  add x1,x1,#0x2da
0102a450:  add x0,x0,#0xc1
0102a454:  bl 0x0102a9c8
0102a458:  mov w0,#0x0
0102a45c:  bl 0x0102a6a0
0102a460:  mov x0,#0x0
0102a464:  bl 0x010294c8
0102a468:  mov w3,w23
0102a46c:  mov w2,w22
0102a470:  mov w1,w20
0102a474:  mov w0,w19
0102a478:  bl 0x0102a3bc
0102a47c:  cmp w19,#0x2
0102a480:  b.ne 0x0102a530
0102a484:  adrp x19,0x102e000
0102a488:  mov w3,#0x0
0102a48c:  mov w2,#0x0
0102a490:  mov x1,#0xfd890000
0102a494:  add x0,x19,#0xaf0
0102a498:  bl 0x0102cafc
0102a49c:  mov w20,w0
0102a4a0:  cbz w0,0x0102a4e8
0102a4a4:  mov w0,#0x0
0102a4a8:  bl 0x0102a684
0102a4ac:  adrp x4,0x102e000
0102a4b0:  adrp x3,0x102e000
0102a4b4:  adrp x1,0x102e000
0102a4b8:  adrp x0,0x102e000
0102a4bc:  mov w5,#0x7e
0102a4c0:  add x4,x4,#0x348
0102a4c4:  add x3,x3,#0x2c0
0102a4c8:  mov w2,w20
0102a4cc:  add x1,x1,#0x2f2
0102a4d0:  add x0,x0,#0xc1
0102a4d4:  bl 0x0102a9c8
0102a4d8:  mov w0,#0x0
0102a4dc:  bl 0x0102a6a0
0102a4e0:  mov x0,#0x0
0102a4e4:  bl 0x010294c8
0102a4e8:  add x19,x19,#0xaf0
0102a4ec:  mov w1,#0x0
0102a4f0:  mov x0,x19
0102a4f4:  bl 0x0102cdb0
0102a4f8:  mov w3,#0x0
0102a4fc:  mov w2,#0x0
0102a500:  mov w1,#0x1
0102a504:  mov x0,x19
0102a508:  bl 0x0102cbcc
0102a50c:  mov w1,#0xffffffff
0102a510:  mov x0,x19
0102a514:  bl 0x0102ccb8
0102a518:  mov w1,#0x0
0102a51c:  mov x0,x19
0102a520:  bl 0x0102cd28
0102a524:  mov w1,#0x1
0102a528:  mov x0,x19
0102a52c:  bl 0x0102cdb0
0102a530:  mov w0,#0x1
0102a534:  ldr x23,[sp, #0x30]
0102a538:  str w0,[x21, #0xae4]
0102a53c:  ldp x19,x20,[sp, #0x10]
0102a540:  ldp x21,x22,[sp, #0x20]
0102a544:  ldp x29,x30,[sp], #0x40
0102a548:  ret

; ==== FUN_0102a54c @ 0102a54c ====
0102a54c:  stp x29,x30,[sp, #-0x50]!
0102a550:  mov x29,sp
0102a554:  stp x19,x20,[sp, #0x10]
0102a558:  mov w20,w0
0102a55c:  adrp x0,0x102e000
0102a560:  stp x21,x22,[sp, #0x20]
0102a564:  stp x23,x24,[sp, #0x30]
0102a568:  ldr w0,[x0, #0xae4]
0102a56c:  stp x25,x26,[sp, #0x40]
0102a570:  cmp w0,#0x1
0102a574:  b.eq 0x0102a5bc
0102a578:  mov w0,#0x0
0102a57c:  bl 0x0102a684
0102a580:  adrp x4,0x102e000
0102a584:  adrp x3,0x102e000
0102a588:  adrp x1,0x102e000
0102a58c:  adrp x0,0x102e000
0102a590:  mov w5,#0xfb
0102a594:  add x4,x4,#0x2b8
0102a598:  add x3,x3,#0x2c0
0102a59c:  mov w2,#0x1
0102a5a0:  add x1,x1,#0x32e
0102a5a4:  add x0,x0,#0xc1
0102a5a8:  bl 0x0102a9c8
0102a5ac:  mov w0,#0x0
0102a5b0:  bl 0x0102a6a0
0102a5b4:  mov x0,#0x0
0102a5b8:  bl 0x010294c8
0102a5bc:  mov w21,#0x3e8
0102a5c0:  bl 0x0102a348
0102a5c4:  mov w23,#0x2710
0102a5c8:  udiv w25,w0,w21
0102a5cc:  cbz w20,0x0102a620
0102a5d0:  cmp w20,w23
0102a5d4:  csel w24,w20,w23,ls
0102a5d8:  mul w19,w24,w25
0102a5dc:  udiv w19,w19,w21
0102a5e0:  bl 0x0102a370
0102a5e4:  cmp w19,wzr
0102a5e8:  mov w22,w0
0102a5ec:  add w26,w0,#0x1
0102a5f0:  b.le 0x0102a618
0102a5f4:  bl 0x0102a370
0102a5f8:  cmp w0,w22
0102a5fc:  b.cs 0x0102a60c
0102a600:  sub w19,w19,w0
0102a604:  add w19,w26,w19
0102a608:  b 0x0102a5e4
0102a60c:  add w19,w19,w22
0102a610:  sub w19,w19,w0
0102a614:  b 0x0102a5e4
0102a618:  sub w20,w20,w24
0102a61c:  b 0x0102a5cc
0102a620:  mov w0,#0x0
0102a624:  ldp x19,x20,[sp, #0x10]
0102a628:  ldp x21,x22,[sp, #0x20]
0102a62c:  ldp x23,x24,[sp, #0x30]
0102a630:  ldp x25,x26,[sp, #0x40]
0102a634:  ldp x29,x30,[sp], #0x50
0102a638:  ret

; ==== FUN_0102a63c @ 0102a63c ====
0102a63c:  stp x29,x30,[sp, #-0x20]!
0102a640:  mov w1,w0
0102a644:  mov x29,sp
0102a648:  str x19,[sp, #0x10]
0102a64c:  mov w19,w0
0102a650:  mov w0,#0x0
0102a654:  bl 0x0102a76c
0102a658:  cmp w19,#0xa
0102a65c:  b.ne 0x0102a674
0102a660:  ldr x19,[sp, #0x10]
0102a664:  mov w1,#0xd
0102a668:  ldp x29,x30,[sp], #0x20
0102a66c:  mov w0,#0x0
0102a670:  b 0x0102a76c
0102a674:  ldr x19,[sp, #0x10]
0102a678:  ldp x29,x30,[sp], #0x20
0102a67c:  ret

; ==== FUN_0102a680 @ 0102a680 ====
0102a680:  ret

; ==== FUN_0102a684 @ 0102a684 ====
0102a684:  mov x2,#0xf0070000
0102a688:  mov w1,w0
0102a68c:  add x1,x1,#0x500
0102a690:  ldr w1,[x2, x1, LSL #0x2]
0102a694:  cbnz w1,0x0102a688
0102a698:  mov w0,#0x1
0102a69c:  ret

; ==== FUN_0102a6a0 @ 0102a6a0 ====
0102a6a0:  mov w0,w0
0102a6a4:  mov x1,#0xf0070000
0102a6a8:  add x0,x0,#0x500
0102a6ac:  str wzr,[x1, x0, LSL #0x2]
0102a6b0:  mov w0,#0x1
0102a6b4:  ret

; ==== FUN_0102a6b8 @ 0102a6b8 ====
0102a6b8:  stp x29,x30,[sp, #-0x30]!
0102a6bc:  cmp w0,#0x3
0102a6c0:  mov x29,sp
0102a6c4:  stp x21,x22,[sp, #0x20]
0102a6c8:  stp x19,x20,[sp, #0x10]
0102a6cc:  mov w22,w0
0102a6d0:  mov w21,w1
0102a6d4:  mov w20,w2
0102a6d8:  b.le 0x0102a720
0102a6dc:  mov w0,#0x0
0102a6e0:  bl 0x0102a684
0102a6e4:  adrp x4,0x102e000
0102a6e8:  adrp x3,0x102e000
0102a6ec:  adrp x1,0x102e000
0102a6f0:  adrp x0,0x102e000
0102a6f4:  mov w5,#0x70
0102a6f8:  add x4,x4,#0x408
0102a6fc:  add x3,x3,#0x3a0
0102a700:  mov w2,#0x1
0102a704:  add x1,x1,#0x3c0
0102a708:  add x0,x0,#0xc1
0102a70c:  bl 0x0102a9c8
0102a710:  mov w0,#0x0
0102a714:  bl 0x0102a6a0
0102a718:  mov x0,#0x0
0102a71c:  bl 0x010294c8
0102a720:  adrp x0,0x102e000
0102a724:  sxtw x19,w22
0102a728:  add x0,x0,#0x3e8
0102a72c:  ldr x1,[x0, w22, SXTW #0x3]
0102a730:  adrp x0,0x102e000
0102a734:  add x0,x0,#0xdd8
0102a738:  add x0,x0,x19, LSL #0x4
0102a73c:  bl 0x0102c6d8
0102a740:  adrp x0,0x102e000
0102a744:  mov x1,#0x28
0102a748:  add x0,x0,#0xd38
0102a74c:  madd x19,x19,x1,x0
0102a750:  mov w0,#0x0
0102a754:  str w21,[x19, #0xc]
0102a758:  str w20,[x19, #0x10]
0102a75c:  ldp x21,x22,[sp, #0x20]
0102a760:  ldp x19,x20,[sp, #0x10]
0102a764:  ldp x29,x30,[sp], #0x30
0102a768:  ret

; ==== FUN_0102a76c @ 0102a76c ====
0102a76c:  stp x29,x30,[sp, #-0x30]!
0102a770:  cmp w0,#0x3
0102a774:  mov x29,sp
0102a778:  str x19,[sp, #0x10]
0102a77c:  mov w19,w0
0102a780:  strb w1,[x29, #0x2f]
0102a784:  b.le 0x0102a7cc
0102a788:  mov w0,#0x0
0102a78c:  bl 0x0102a684
0102a790:  adrp x4,0x102e000
0102a794:  adrp x3,0x102e000
0102a798:  adrp x1,0x102e000
0102a79c:  adrp x0,0x102e000
0102a7a0:  mov w5,#0x7e
0102a7a4:  add x4,x4,#0x368
0102a7a8:  add x3,x3,#0x3a0
0102a7ac:  mov w2,#0x1
0102a7b0:  add x1,x1,#0x3c0
0102a7b4:  add x0,x0,#0xc1
0102a7b8:  bl 0x0102a9c8
0102a7bc:  mov w0,#0x0
0102a7c0:  bl 0x0102a6a0
0102a7c4:  mov x0,#0x0
0102a7c8:  bl 0x010294c8
0102a7cc:  adrp x1,0x102e000
0102a7d0:  sbfiz x19,x19,#0x4,#0x20
0102a7d4:  add x1,x1,#0xdd8
0102a7d8:  add x0,x1,x19
0102a7dc:  ldr x1,[x1, x19, LSL #0x0]
0102a7e0:  cbz x1,0x0102a7f4
0102a7e4:  mov w3,#0xffffffff
0102a7e8:  add x2,x29,#0x2f
0102a7ec:  mov w1,#0x1
0102a7f0:  bl 0x0102c914
0102a7f4:  ldr x19,[sp, #0x10]
0102a7f8:  ldp x29,x30,[sp], #0x30
0102a7fc:  ret

; ==== FUN_0102a800 @ 0102a800 ====
0102a800:  stp x29,x30,[sp, #-0x30]!
0102a804:  mov x29,sp
0102a808:  stp x19,x20,[sp, #0x10]
0102a80c:  sxtw x20,w0
0102a810:  cmp w20,#0x3
0102a814:  b.le 0x0102a85c
0102a818:  mov w0,#0x0
0102a81c:  bl 0x0102a684
0102a820:  adrp x4,0x102e000
0102a824:  adrp x3,0x102e000
0102a828:  adrp x1,0x102e000
0102a82c:  adrp x0,0x102e000
0102a830:  mov w5,#0x8b
0102a834:  add x4,x4,#0x378
0102a838:  add x3,x3,#0x3a0
0102a83c:  mov w2,#0x1
0102a840:  add x1,x1,#0x3c0
0102a844:  add x0,x0,#0xc1
0102a848:  bl 0x0102a9c8
0102a84c:  mov w0,#0x0
0102a850:  bl 0x0102a6a0
0102a854:  mov x0,#0x0
0102a858:  bl 0x010294c8
0102a85c:  adrp x0,0x102e000
0102a860:  lsl x1,x20,#0x4
0102a864:  add x2,x0,#0xdd8
0102a868:  mov x19,x0
0102a86c:  ldr x1,[x2, x1, LSL #0x0]
0102a870:  cbnz x1,0x0102a8b8
0102a874:  mov w0,#0x0
0102a878:  bl 0x0102a684
0102a87c:  adrp x4,0x102e000
0102a880:  adrp x3,0x102e000
0102a884:  adrp x1,0x102e000
0102a888:  adrp x0,0x102e000
0102a88c:  mov w5,#0x8c
0102a890:  add x4,x4,#0x378
0102a894:  add x3,x3,#0x3a0
0102a898:  mov w2,#0x1
0102a89c:  add x1,x1,#0x3cc
0102a8a0:  add x0,x0,#0xc1
0102a8a4:  bl 0x0102a9c8
0102a8a8:  mov w0,#0x0
0102a8ac:  bl 0x0102a6a0
0102a8b0:  mov x0,#0x0
0102a8b4:  bl 0x010294c8
0102a8b8:  add x0,x19,#0xdd8
0102a8bc:  add x2,x29,#0x2f
0102a8c0:  mov w3,#0xffffffff
0102a8c4:  mov w1,#0x1
0102a8c8:  add x0,x0,x20, LSL #0x4
0102a8cc:  bl 0x0102c744
0102a8d0:  ldrb w0,[x29, #0x2f]
0102a8d4:  ldp x19,x20,[sp, #0x10]
0102a8d8:  ldp x29,x30,[sp], #0x30
0102a8dc:  ret

; ==== FUN_0102a8e0 @ 0102a8e0 ====
0102a8e0:  stp x29,x30,[sp, #-0x20]!
0102a8e4:  cmp w0,#0x3
0102a8e8:  mov x29,sp
0102a8ec:  str x19,[sp, #0x10]
0102a8f0:  mov w19,w0
0102a8f4:  b.le 0x0102a93c
0102a8f8:  mov w0,#0x0
0102a8fc:  bl 0x0102a684
0102a900:  adrp x4,0x102e000
0102a904:  adrp x3,0x102e000
0102a908:  adrp x1,0x102e000
0102a90c:  adrp x0,0x102e000
0102a910:  mov w5,#0x96
0102a914:  add x4,x4,#0x388
0102a918:  add x3,x3,#0x3a0
0102a91c:  mov w2,#0x1
0102a920:  add x1,x1,#0x3c0
0102a924:  add x0,x0,#0xc1
0102a928:  bl 0x0102a9c8
0102a92c:  mov w0,#0x0
0102a930:  bl 0x0102a6a0
0102a934:  mov x0,#0x0
0102a938:  bl 0x010294c8
0102a93c:  adrp x1,0x102e000
0102a940:  sbfiz x19,x19,#0x4,#0x20
0102a944:  add x1,x1,#0xdd8
0102a948:  add x0,x1,x19
0102a94c:  ldr x1,[x1, x19, LSL #0x0]
0102a950:  cbz x1,0x0102a960
0102a954:  ldr x19,[sp, #0x10]
0102a958:  ldp x29,x30,[sp], #0x20
0102a95c:  b 0x0102caec
0102a960:  mov w0,#0x0
0102a964:  ldr x19,[sp, #0x10]
0102a968:  ldp x29,x30,[sp], #0x20
0102a96c:  ret

; ==== FUN_0102a970 @ 0102a970 ====
0102a970:  stp x29,x30,[sp, #-0x20]!
0102a974:  mov x29,sp
0102a978:  str x19,[sp, #0x10]
0102a97c:  mov w19,w0
0102a980:  bl 0x0102a63c
0102a984:  mov w0,w19
0102a988:  ldr x19,[sp, #0x10]
0102a98c:  ldp x29,x30,[sp], #0x20
0102a990:  ret

; ==== FUN_0102a994 @ 0102a994 ====
0102a994:  stp x29,x30,[sp, #-0x30]!
0102a998:  mov x29,sp
0102a99c:  cbnz x1,0x0102a9bc
0102a9a0:  ldp x4,x5,[x3]
0102a9a4:  stp x4,x5,[x29, #0x10]
0102a9a8:  ldp x4,x5,[x3, #0x10]
0102a9ac:  stp x4,x5,[x29, #0x20]
0102a9b0:  add x3,x29,#0x10
0102a9b4:  mov x4,#0x0
0102a9b8:  bl 0x0102afdc
0102a9bc:  mov w0,#0x0
0102a9c0:  ldp x29,x30,[sp], #0x30
0102a9c4:  ret

; ==== FUN_0102a9c8 @ 0102a9c8 ====
0102a9c8:  stp x29,x30,[sp, #-0x110]!
0102a9cc:  mov x29,sp
0102a9d0:  str q0,[x29, #0x50]
0102a9d4:  str x1,[x29, #0xd8]
0102a9d8:  add x1,x29,#0x110
0102a9dc:  str x1,[x29, #0x30]
0102a9e0:  str x1,[x29, #0x38]
0102a9e4:  add x1,x29,#0xd0
0102a9e8:  str x1,[x29, #0x40]
0102a9ec:  mov w1,#0xffffffc8
0102a9f0:  str w1,[x29, #0x48]
0102a9f4:  mov w1,#0xffffff80
0102a9f8:  str w1,[x29, #0x4c]
0102a9fc:  mov x1,#0x0
0102aa00:  str x2,[x29, #0xe0]
0102aa04:  str x3,[x29, #0xe8]
0102aa08:  ldp x2,x3,[x29, #0x30]
0102aa0c:  stp x2,x3,[x29, #0x10]
0102aa10:  ldp x2,x3,[x29, #0x40]
0102aa14:  stp x2,x3,[x29, #0x20]
0102aa18:  mov x2,x0
0102aa1c:  adrp x0,0x102a000
0102aa20:  str x4,[x29, #0xf0]
0102aa24:  add x3,x29,#0x10
0102aa28:  mov x4,#0x0
0102aa2c:  add x0,x0,#0x818
0102aa30:  str x5,[x29, #0xf8]
0102aa34:  str x6,[x29, #0x100]
0102aa38:  str q1,[x29, #0x60]
0102aa3c:  str x7,[x29, #0x108]
0102aa40:  str q2,[x29, #0x70]
0102aa44:  str q3,[x29, #0x80]
0102aa48:  str q4,[x29, #0x90]
0102aa4c:  str q5,[x29, #0xa0]
0102aa50:  str q6,[x29, #0xb0]
0102aa54:  str q7,[x29, #0xc0]
0102aa58:  bl 0x0102afdc
0102aa5c:  ldp x29,x30,[sp], #0x110
0102aa60:  ret

; ==== FUN_0102aa64 @ 0102aa64 ====
0102aa64:  mov x4,x0
0102aa68:  mov w0,w2
0102aa6c:  cbz x1,0x0102aac8
0102aa70:  cbz x3,0x0102aab0
0102aa74:  ldr w2,[x3]
0102aa78:  cbz w2,0x0102aacc
0102aa7c:  cmp w2,#0x1
0102aa80:  ldr x2,[x1]
0102aa84:  b.ne 0x0102aa90
0102aa88:  strb wzr,[x2]
0102aa8c:  b 0x0102aa94
0102aa90:  strb w0,[x2]
0102aa94:  ldr x0,[x1]
0102aa98:  add x0,x0,#0x1
0102aa9c:  str x0,[x1]
0102aaa0:  ldr w0,[x3]
0102aaa4:  sub w0,w0,#0x1
0102aaa8:  str w0,[x3]
0102aaac:  b 0x0102aacc
0102aab0:  ldr x2,[x1]
0102aab4:  strb w0,[x2]
0102aab8:  ldr x0,[x1]
0102aabc:  add x0,x0,#0x1
0102aac0:  str x0,[x1]
0102aac4:  b 0x0102aacc
0102aac8:  br x4
0102aacc:  ret

; ==== FUN_0102aad0 @ 0102aad0 ====
0102aad0:  stp x29,x30,[sp, #-0x70]!
0102aad4:  mov x2,#0x8
0102aad8:  mov w1,#0x0
0102aadc:  mov x29,sp
0102aae0:  stp x25,x26,[sp, #0x40]
0102aae4:  mov x25,x0
0102aae8:  add x0,x29,#0x68
0102aaec:  stp x19,x20,[sp, #0x10]
0102aaf0:  stp x21,x22,[sp, #0x20]
0102aaf4:  stp x23,x24,[sp, #0x30]
0102aaf8:  stp x27,x28,[sp, #0x50]
0102aafc:  bl 0x0102c210
0102ab00:  mov w19,#0x14
0102ab04:  mov w0,#0x1
0102ab08:  mov w4,#0xa
0102ab0c:  mov w7,#0xcccc
0102ab10:  mov w5,#0x30
0102ab14:  mov w6,#0x7fffffff
0102ab18:  movk w7,#0xccc, LSL #16
0102ab1c:  mov w3,#0x4
0102ab20:  mov w9,#0x5
0102ab24:  mov w10,#0x1a
0102ab28:  mov w11,#0x2
0102ab2c:  mov w12,#0x8
0102ab30:  mov w13,#0xb
0102ab34:  mov w14,#0x13
0102ab38:  mov w8,#0x11
0102ab3c:  mov w15,#0x17
0102ab40:  mov w16,#0x19
0102ab44:  mov w17,w0
0102ab48:  mov w18,#0x7
0102ab4c:  mov w30,w4
0102ab50:  mov w20,#0x16
0102ab54:  mov w21,#0x10
0102ab58:  mov w22,#0xf
0102ab5c:  mov w23,#0x6
0102ab60:  mov w24,#0x18
0102ab64:  ldrb w1,[x25]
0102ab68:  cbnz w1,0x0102ab74
0102ab6c:  mov w0,#0x0
0102ab70:  b 0x0102afc0
0102ab74:  cmp w1,#0x25
0102ab78:  add x25,x25,#0x1
0102ab7c:  mov w26,#0x0
0102ab80:  b.ne 0x0102ab64
0102ab84:  ldrb w1,[x25]
0102ab88:  add x2,x25,#0x1
0102ab8c:  cmp w1,#0x63
0102ab90:  b.eq 0x0102ae34
0102ab94:  b.gt 0x0102ac34
0102ab98:  cmp w1,#0x2e
0102ab9c:  b.eq 0x0102ad04
0102aba0:  b.gt 0x0102abf0
0102aba4:  cmp w1,#0x27
0102aba8:  b.ne 0x0102abb4
0102abac:  mov x25,x2
0102abb0:  b 0x0102ab84
0102abb4:  b.gt 0x0102abcc
0102abb8:  cmp w1,#0x20
0102abbc:  b.eq 0x0102abac
0102abc0:  cmp w1,#0x23
0102abc4:  b.eq 0x0102abac
0102abc8:  b 0x0102afb4
0102abcc:  cmp w1,#0x2b
0102abd0:  b.eq 0x0102abac
0102abd4:  cmp w1,#0x2d
0102abd8:  b.eq 0x0102abac
0102abdc:  cmp w1,#0x2a
0102abe0:  b.ne 0x0102afb4
0102abe4:  mov x27,x2
0102abe8:  mov w1,#0x0
0102abec:  b 0x0102acf0
0102abf0:  cmp w1,#0x44
0102abf4:  b.eq 0x0102ae44
0102abf8:  b.gt 0x0102ac18
0102abfc:  cmp w1,#0x30
0102ac00:  b.eq 0x0102abac
0102ac04:  b.lt 0x0102afb4
0102ac08:  cmp w1,#0x39
0102ac0c:  b.gt 0x0102afb4
0102ac10:  mov w25,#0x0
0102ac14:  b 0x0102adb8
0102ac18:  cmp w1,#0x55
0102ac1c:  b.eq 0x0102af34
0102ac20:  cmp w1,#0x58
0102ac24:  b.eq 0x0102af38
0102ac28:  orr w26,w26,#0x10
0102ac2c:  cmp w1,#0x4f
0102ac30:  b 0x0102acc4
0102ac34:  cmp w1,#0x6f
0102ac38:  b.eq 0x0102af38
0102ac3c:  b.gt 0x0102ac80
0102ac40:  cmp w1,#0x69
0102ac44:  b.eq 0x0102ae48
0102ac48:  b.gt 0x0102ac60
0102ac4c:  cmp w1,#0x64
0102ac50:  b.eq 0x0102ae48
0102ac54:  cmp w1,#0x68
0102ac58:  b.eq 0x0102ade0
0102ac5c:  b 0x0102afb4
0102ac60:  cmp w1,#0x6c
0102ac64:  b.eq 0x0102ae00
0102ac68:  cmp w1,#0x6e
0102ac6c:  b.eq 0x0102aebc
0102ac70:  cmp w1,#0x6a
0102ac74:  b.ne 0x0102afb4
0102ac78:  orr w26,w26,#0x1000
0102ac7c:  b 0x0102abac
0102ac80:  cmp w1,#0x74
0102ac84:  b.eq 0x0102ae24
0102ac88:  b.gt 0x0102acb0
0102ac8c:  cmp w1,#0x71
0102ac90:  b.eq 0x0102ae1c
0102ac94:  b.lt 0x0102af20
0102ac98:  cmp w1,#0x73
0102ac9c:  b.ne 0x0102afb4
0102aca0:  sub x1,x29,#0xf90
0102aca4:  add x1,x1,w0, SXTW 
0102aca8:  strb w22,[x1, #0xff8]
0102acac:  b 0x0102af2c
0102acb0:  cmp w1,#0x78
0102acb4:  b.eq 0x0102af38
0102acb8:  cmp w1,#0x7a
0102acbc:  b.eq 0x0102ae2c
0102acc0:  cmp w1,#0x75
0102acc4:  b.eq 0x0102af38
0102acc8:  b 0x0102afb4
0102accc:  cmp w1,w7
0102acd0:  b.gt 0x0102afbc
0102acd4:  mul w1,w1,w4
0102acd8:  sub w25,w5,w25
0102acdc:  add w25,w25,w6
0102ace0:  cmp w1,w25
0102ace4:  b.gt 0x0102afbc
0102ace8:  add w1,w1,w28
0102acec:  add x27,x27,#0x1
0102acf0:  ldrb w25,[x27]
0102acf4:  sub w28,w25,#0x30
0102acf8:  cmp w28,#0x9
0102acfc:  b.ls 0x0102accc
0102ad00:  b 0x0102ad54
0102ad04:  ldrb w1,[x2]
0102ad08:  add x27,x2,#0x1
0102ad0c:  mov x2,x27
0102ad10:  cmp w1,#0x2a
0102ad14:  b.ne 0x0102ad84
0102ad18:  mov w1,#0x0
0102ad1c:  ldrb w25,[x27]
0102ad20:  sub w28,w25,#0x30
0102ad24:  cmp w28,#0x9
0102ad28:  b.hi 0x0102ad54
0102ad2c:  cmp w1,w7
0102ad30:  b.gt 0x0102afbc
0102ad34:  mul w1,w1,w4
0102ad38:  sub w25,w5,w25
0102ad3c:  add w25,w25,w6
0102ad40:  cmp w1,w25
0102ad44:  b.gt 0x0102afbc
0102ad48:  add w1,w1,w28
0102ad4c:  add x27,x27,#0x1
0102ad50:  b 0x0102ad1c
0102ad54:  cmp w25,#0x24
0102ad58:  b.ne 0x0102ad70
0102ad5c:  sub x2,x29,#0xf90
0102ad60:  add x1,x2,w1, SXTW 
0102ad64:  add x2,x27,#0x1
0102ad68:  strb w3,[x1, #0xff8]
0102ad6c:  b 0x0102abac
0102ad70:  sub x1,x29,#0xf90
0102ad74:  add x1,x1,w0, SXTW 
0102ad78:  add w0,w0,#0x1
0102ad7c:  strb w3,[x1, #0xff8]
0102ad80:  b 0x0102abac
0102ad84:  sub w25,w1,#0x30
0102ad88:  cmp w25,#0x9
0102ad8c:  b.hi 0x0102ab8c
0102ad90:  ldrb w1,[x2], #0x1
0102ad94:  b 0x0102ad84
0102ad98:  sub w1,w1,#0x30
0102ad9c:  add w25,w25,w1
0102ada0:  ldrb w1,[x2], #0x1
0102ada4:  sub w27,w1,#0x30
0102ada8:  cmp w27,#0x9
0102adac:  b.hi 0x0102add0
0102adb0:  cmp w25,w7
0102adb4:  b.gt 0x0102afbc
0102adb8:  mul w25,w25,w4
0102adbc:  sub w27,w5,w1
0102adc0:  add w27,w27,w6
0102adc4:  cmp w25,w27
0102adc8:  b.le 0x0102ad98
0102adcc:  b 0x0102afbc
0102add0:  cmp w1,#0x24
0102add4:  b.ne 0x0102ab8c
0102add8:  mov w0,w25
0102addc:  b 0x0102abac
0102ade0:  ldrb w1,[x2]
0102ade4:  cmp w1,#0x68
0102ade8:  b.ne 0x0102adf8
0102adec:  add x2,x2,#0x1
0102adf0:  orr w26,w26,#0x800
0102adf4:  b 0x0102abac
0102adf8:  orr w26,w26,#0x40
0102adfc:  b 0x0102abac
0102ae00:  ldrb w1,[x2]
0102ae04:  cmp w1,#0x6c
0102ae08:  b.ne 0x0102ae14
0102ae0c:  add x2,x2,#0x1
0102ae10:  b 0x0102ae1c
0102ae14:  orr w26,w26,#0x10
0102ae18:  b 0x0102abac
0102ae1c:  orr w26,w26,#0x20
0102ae20:  b 0x0102abac
0102ae24:  orr w26,w26,#0x200
0102ae28:  b 0x0102abac
0102ae2c:  orr w26,w26,#0x400
0102ae30:  b 0x0102abac
0102ae34:  sub x1,x29,#0xf90
0102ae38:  add x1,x1,w0, SXTW 
0102ae3c:  strb w3,[x1, #0xff8]
0102ae40:  b 0x0102af2c
0102ae44:  orr w26,w26,#0x10
0102ae48:  add w1,w0,#0x1
0102ae4c:  sxtw x0,w0
0102ae50:  tbz w26,#0xc,0x0102ae64
0102ae54:  sub x25,x29,#0xf90
0102ae58:  add x0,x25,x0
0102ae5c:  strb w20,[x0, #0xff8]
0102ae60:  b 0x0102ae9c
0102ae64:  tbnz w26,#0x9,0x0102af58
0102ae68:  sub x25,x29,#0xf90
0102ae6c:  add x0,x25,x0
0102ae70:  tbz w26,#0xa,0x0102ae7c
0102ae74:  strb w19,[x0, #0xff8]
0102ae78:  b 0x0102ae9c
0102ae7c:  tbz w26,#0x5,0x0102ae88
0102ae80:  strb w30,[x0, #0xff8]
0102ae84:  b 0x0102ae9c
0102ae88:  tbz w26,#0x4,0x0102ae94
0102ae8c:  strb w18,[x0, #0xff8]
0102ae90:  b 0x0102ae9c
0102ae94:  tbz w26,#0x6,0x0102aea8
0102ae98:  strb w17,[x0, #0xff8]
0102ae9c:  mov w0,w1
0102aea0:  mov x25,x2
0102aea4:  b 0x0102ab64
0102aea8:  tbz w26,#0xb,0x0102aeb4
0102aeac:  strb w16,[x0, #0xff8]
0102aeb0:  b 0x0102ae9c
0102aeb4:  strb w3,[x0, #0xff8]
0102aeb8:  b 0x0102ae9c
0102aebc:  sub x25,x29,#0xf90
0102aec0:  add w1,w0,#0x1
0102aec4:  sxtw x0,w0
0102aec8:  add x0,x25,x0
0102aecc:  tbz w26,#0x5,0x0102aed8
0102aed0:  mov w25,#0xc
0102aed4:  b 0x0102af04
0102aed8:  tbz w26,#0x4,0x0102aee4
0102aedc:  mov w25,#0x9
0102aee0:  b 0x0102af04
0102aee4:  tbz w26,#0x6,0x0102aef0
0102aee8:  mov w25,#0x3
0102aeec:  b 0x0102af04
0102aef0:  tbz w26,#0x9,0x0102aefc
0102aef4:  mov w25,#0x12
0102aef8:  b 0x0102af04
0102aefc:  tbz w26,#0xa,0x0102af0c
0102af00:  mov w25,#0x15
0102af04:  strb w25,[x0, #0xff8]
0102af08:  b 0x0102ae9c
0102af0c:  tbz w26,#0xc,0x0102af18
0102af10:  strb w24,[x0, #0xff8]
0102af14:  b 0x0102ae9c
0102af18:  strb w23,[x0, #0xff8]
0102af1c:  b 0x0102ae9c
0102af20:  sub x1,x29,#0xf90
0102af24:  add x1,x1,w0, SXTW 
0102af28:  strb w21,[x1, #0xff8]
0102af2c:  add w0,w0,#0x1
0102af30:  b 0x0102aea0
0102af34:  orr w26,w26,#0x10
0102af38:  add w1,w0,#0x1
0102af3c:  sxtw x0,w0
0102af40:  tbz w26,#0xc,0x0102af54
0102af44:  sub x25,x29,#0xf90
0102af48:  add x0,x25,x0
0102af4c:  strb w15,[x0, #0xff8]
0102af50:  b 0x0102ae9c
0102af54:  tbz w26,#0x9,0x0102af68
0102af58:  sub x25,x29,#0xf90
0102af5c:  add x0,x25,x0
0102af60:  strb w8,[x0, #0xff8]
0102af64:  b 0x0102ae9c
0102af68:  sub x25,x29,#0xf90
0102af6c:  add x0,x25,x0
0102af70:  tbz w26,#0xa,0x0102af7c
0102af74:  strb w14,[x0, #0xff8]
0102af78:  b 0x0102ae9c
0102af7c:  tbz w26,#0x5,0x0102af88
0102af80:  strb w13,[x0, #0xff8]
0102af84:  b 0x0102ae9c
0102af88:  tbz w26,#0x4,0x0102af94
0102af8c:  strb w12,[x0, #0xff8]
0102af90:  b 0x0102ae9c
0102af94:  tbz w26,#0x6,0x0102afa0
0102af98:  strb w11,[x0, #0xff8]
0102af9c:  b 0x0102ae9c
0102afa0:  tbz w26,#0xb,0x0102afac
0102afa4:  strb w10,[x0, #0xff8]
0102afa8:  b 0x0102ae9c
0102afac:  strb w9,[x0, #0xff8]
0102afb0:  b 0x0102ae9c
0102afb4:  cbnz w1,0x0102aea0
0102afb8:  b 0x0102ab6c
0102afbc:  mov w0,#0xffffffff
0102afc0:  ldp x19,x20,[sp, #0x10]
0102afc4:  ldp x21,x22,[sp, #0x20]
0102afc8:  ldp x23,x24,[sp, #0x30]
0102afcc:  ldp x25,x26,[sp, #0x40]
0102afd0:  ldp x27,x28,[sp, #0x50]
0102afd4:  ldp x29,x30,[sp], #0x70
0102afd8:  ret

; ==== FUN_0102afdc @ 0102afdc ====
0102afdc:  stp x29,x30,[sp, #-0x1c0]!
0102afe0:  mov x29,sp
0102afe4:  stp x19,x20,[sp, #0x10]
0102afe8:  stp x23,x24,[sp, #0x30]
0102afec:  str x2,[x29, #0xa0]
0102aff0:  add x2,x29,#0xf8
0102aff4:  stp x21,x22,[sp, #0x20]
0102aff8:  stp x27,x28,[sp, #0x50]
0102affc:  stp x25,x26,[sp, #0x40]
0102b000:  str x0,[x29, #0xc0]
0102b004:  mov x24,#0x0
0102b008:  str x1,[x29, #0xb8]
0102b00c:  mov w28,#0x1
0102b010:  ldp x0,x1,[x3]
0102b014:  stp x0,x1,[x2]
0102b018:  add x2,x29,#0x108
0102b01c:  str x4,[x29, #0xb0]
0102b020:  ldp x0,x1,[x3, #0x10]
0102b024:  ldr x20,[x3]
0102b028:  stp x0,x1,[x2]
0102b02c:  mov x2,#0x4
0102b030:  mov w1,#0x0
0102b034:  add x0,x29,#0xf4
0102b038:  ldr x22,[x3, #0x8]
0102b03c:  ldr w23,[x3, #0x18]
0102b040:  bl 0x0102c210
0102b044:  ldr x19,[x29, #0xa0]
0102b048:  adrp x0,0x102f000
0102b04c:  str wzr,[x29, #0xcc]
0102b050:  add x0,x0,#0x450
0102b054:  str x0,[x29, #0x98]
0102b058:  mov x21,x19
0102b05c:  ldrb w0,[x21]
0102b060:  cbz w0,0x0102b080
0102b064:  cmp w0,#0x25
0102b068:  add x1,x21,#0x1
0102b06c:  b.ne 0x0102c1cc
0102b070:  cmp x21,x19
0102b074:  mov w25,#0x1
0102b078:  b.ne 0x0102b08c
0102b07c:  b 0x0102b0e4
0102b080:  cmp x21,x19
0102b084:  b.eq 0x0102c1d4
0102b088:  mov w25,#0x0
0102b08c:  subs x26,x21,x19
0102b090:  b.pl 0x0102b0a0
0102b094:  mov w0,#0xffffffff
0102b098:  str w0,[x29, #0xcc]
0102b09c:  b 0x0102c1d4
0102b0a0:  ldr w1,[x29, #0xcc]
0102b0a4:  mov w0,#0x7fffffff
0102b0a8:  sub w0,w0,w1
0102b0ac:  cmp x26,w0, SXTW 
0102b0b0:  b.gt 0x0102b094
0102b0b4:  cmp x19,x21
0102b0b8:  b.eq 0x0102b0d4
0102b0bc:  ldr x3,[x29, #0xb0]
0102b0c0:  ldrb w2,[x19], #0x1
0102b0c4:  ldr x1,[x29, #0xb8]
0102b0c8:  ldr x0,[x29, #0xc0]
0102b0cc:  bl 0x0102aa64
0102b0d0:  b 0x0102b0b4
0102b0d4:  ldr w0,[x29, #0xcc]
0102b0d8:  add w0,w0,w26
0102b0dc:  str w0,[x29, #0xcc]
0102b0e0:  cbz w25,0x0102c1d4
0102b0e4:  add x0,x19,#0x1
0102b0e8:  mov w9,#0x0
0102b0ec:  mov w2,#0xffffffff
0102b0f0:  mov w26,#0x0
0102b0f4:  mov w25,#0x0
0102b0f8:  mov w21,#0xa
0102b0fc:  ldrb w11,[x0]
0102b100:  add x19,x0,#0x1
0102b104:  mov w3,#0x7fffffff
0102b108:  cmp w11,#0x63
0102b10c:  b.eq 0x0102b5b4
0102b110:  b.gt 0x0102b1bc
0102b114:  cmp w11,#0x2e
0102b118:  b.eq 0x0102b35c
0102b11c:  b.gt 0x0102b174
0102b120:  cmp w11,#0x27
0102b124:  b.ne 0x0102b130
0102b128:  mov x0,x19
0102b12c:  b 0x0102b0fc
0102b130:  b.gt 0x0102b14c
0102b134:  cmp w11,#0x20
0102b138:  b.eq 0x0102b248
0102b13c:  cmp w11,#0x23
0102b140:  b.ne 0x0102bda0
0102b144:  orr w25,w25,#0x1
0102b148:  b 0x0102b128
0102b14c:  cmp w11,#0x2b
0102b150:  b.eq 0x0102b354
0102b154:  cmp w11,#0x2d
0102b158:  b.eq 0x0102b34c
0102b15c:  cmp w11,#0x2a
0102b160:  b.ne 0x0102bda0
0102b164:  mov x27,x19
0102b168:  mov w26,#0x0
0102b16c:  mov w3,#0x7fffffff
0102b170:  b 0x0102b284
0102b174:  cmp w11,#0x44
0102b178:  b.eq 0x0102b608
0102b17c:  b.gt 0x0102b19c
0102b180:  cmp w11,#0x30
0102b184:  b.eq 0x0102b4c0
0102b188:  b.lt 0x0102bda0
0102b18c:  cmp w11,#0x39
0102b190:  b.gt 0x0102bda0
0102b194:  mov w0,#0x0
0102b198:  b 0x0102b4f0
0102b19c:  cmp w11,#0x55
0102b1a0:  b.eq 0x0102ba4c
0102b1a4:  cmp w11,#0x58
0102b1a8:  b.eq 0x0102bb78
0102b1ac:  cmp w11,#0x4f
0102b1b0:  b.ne 0x0102bda0
0102b1b4:  orr w25,w25,#0x10
0102b1b8:  b 0x0102b840
0102b1bc:  cmp w11,#0x6f
0102b1c0:  b.eq 0x0102b840
0102b1c4:  b.gt 0x0102b208
0102b1c8:  cmp w11,#0x69
0102b1cc:  b.eq 0x0102b60c
0102b1d0:  b.gt 0x0102b1e8
0102b1d4:  cmp w11,#0x64
0102b1d8:  b.eq 0x0102b60c
0102b1dc:  cmp w11,#0x68
0102b1e0:  b.eq 0x0102b560
0102b1e4:  b 0x0102bda0
0102b1e8:  cmp w11,#0x6c
0102b1ec:  b.eq 0x0102b580
0102b1f0:  cmp w11,#0x6e
0102b1f4:  b.eq 0x0102b72c
0102b1f8:  cmp w11,#0x6a
0102b1fc:  b.ne 0x0102bda0
0102b200:  orr w25,w25,#0x1000
0102b204:  b 0x0102b128
0102b208:  cmp w11,#0x74
0102b20c:  b.eq 0x0102b5a4
0102b210:  b.gt 0x0102b22c
0102b214:  cmp w11,#0x71
0102b218:  b.eq 0x0102b59c
0102b21c:  b.lt 0x0102b954
0102b220:  cmp w11,#0x73
0102b224:  b.eq 0x0102b9ac
0102b228:  b 0x0102bda0
0102b22c:  cmp w11,#0x78
0102b230:  b.eq 0x0102bb64
0102b234:  cmp w11,#0x7a
0102b238:  b.eq 0x0102b5ac
0102b23c:  cmp w11,#0x75
0102b240:  b.ne 0x0102bda0
0102b244:  b 0x0102ba50
0102b248:  cmp w9,wzr
0102b24c:  csel w9,w9,w11,ne
0102b250:  b 0x0102b128
0102b254:  mov w4,#0xcccc
0102b258:  movk w4,#0xccc, LSL #16
0102b25c:  cmp w26,w4
0102b260:  b.gt 0x0102b094
0102b264:  mov w4,#0x30
0102b268:  mul w26,w26,w21
0102b26c:  sub w0,w4,w0
0102b270:  add w0,w0,w3
0102b274:  cmp w26,w0
0102b278:  b.gt 0x0102b094
0102b27c:  add w26,w26,w1
0102b280:  add x27,x27,#0x1
0102b284:  ldrb w0,[x27]
0102b288:  sub w1,w0,#0x30
0102b28c:  cmp w1,#0x9
0102b290:  b.ls 0x0102b254
0102b294:  cmp w0,#0x24
0102b298:  b.ne 0x0102b2e8
0102b29c:  cbnz x24,0x0102b2d8
0102b2a0:  add x0,x29,#0xf8
0102b2a4:  str w9,[x29, #0xa8]
0102b2a8:  str w2,[x29, #0xc8]
0102b2ac:  add x24,x29,#0x118
0102b2b0:  ldp x0,x1,[x0]
0102b2b4:  stp x0,x1,[x29, #0xd0]
0102b2b8:  add x0,x29,#0x108
0102b2bc:  ldp x0,x1,[x0]
0102b2c0:  stp x0,x1,[x29, #0xe0]
0102b2c4:  ldr x0,[x29, #0xa0]
0102b2c8:  add x1,x29,#0xd0
0102b2cc:  bl 0x0102aad0
0102b2d0:  ldr w9,[x29, #0xa8]
0102b2d4:  ldr w2,[x29, #0xc8]
0102b2d8:  sbfiz x26,x26,#0x3,#0x20
0102b2dc:  add x19,x27,#0x1
0102b2e0:  ldr w26,[x24, x26, LSL #0x0]
0102b2e4:  b 0x0102b338
0102b2e8:  add w0,w28,#0x1
0102b2ec:  cbz x24,0x0102b300
0102b2f0:  sbfiz x6,x28,#0x3,#0x20
0102b2f4:  mov w28,w0
0102b2f8:  ldr w26,[x24, x6, LSL #0x0]
0102b2fc:  b 0x0102b338
0102b300:  mov w28,w0
0102b304:  tbnz w23,#0x1f,0x0102b314
0102b308:  add x1,x20,#0xb
0102b30c:  mov w0,w23
0102b310:  b 0x0102b328
0102b314:  add w0,w23,#0x8
0102b318:  add x4,x22,w23, SXTW 
0102b31c:  cmp w0,wzr
0102b320:  b.le 0x0102b330
0102b324:  add x1,x20,#0xb
0102b328:  mov x4,x20
0102b32c:  and x20,x1,#-0x8
0102b330:  ldr w26,[x4]
0102b334:  mov w23,w0
0102b338:  tbz w26,#0x1f,0x0102b128
0102b33c:  mov w0,#0x80000000
0102b340:  cmp w26,w0
0102b344:  b.eq 0x0102b094
0102b348:  neg w26,w26
0102b34c:  orr w25,w25,#0x4
0102b350:  b 0x0102b128
0102b354:  mov w9,w11
0102b358:  b 0x0102b128
0102b35c:  ldrb w11,[x19]
0102b360:  add x1,x19,#0x1
0102b364:  cmp w11,#0x2a
0102b368:  b.eq 0x0102b378
0102b36c:  mov x19,x1
0102b370:  mov w0,#0x0
0102b374:  b 0x0102b4a4
0102b378:  mov x27,x1
0102b37c:  mov w19,#0x0
0102b380:  mov w3,#0x7fffffff
0102b384:  ldrb w0,[x27]
0102b388:  sub w2,w0,#0x30
0102b38c:  cmp w2,#0x9
0102b390:  b.hi 0x0102b3c8
0102b394:  mov w4,#0xcccc
0102b398:  movk w4,#0xccc, LSL #16
0102b39c:  cmp w19,w4
0102b3a0:  b.gt 0x0102b094
0102b3a4:  mov w4,#0x30
0102b3a8:  mul w19,w19,w21
0102b3ac:  sub w0,w4,w0
0102b3b0:  add w0,w0,w3
0102b3b4:  cmp w19,w0
0102b3b8:  b.gt 0x0102b094
0102b3bc:  add w19,w19,w2
0102b3c0:  add x27,x27,#0x1
0102b3c4:  b 0x0102b384
0102b3c8:  cmp w0,#0x24
0102b3cc:  b.ne 0x0102b414
0102b3d0:  cbnz x24,0x0102b404
0102b3d4:  add x0,x29,#0xf8
0102b3d8:  str w9,[x29, #0xc8]
0102b3dc:  add x24,x29,#0x118
0102b3e0:  ldp x0,x1,[x0]
0102b3e4:  stp x0,x1,[x29, #0xd0]
0102b3e8:  add x0,x29,#0x108
0102b3ec:  ldp x0,x1,[x0]
0102b3f0:  stp x0,x1,[x29, #0xe0]
0102b3f4:  ldr x0,[x29, #0xa0]
0102b3f8:  add x1,x29,#0xd0
0102b3fc:  bl 0x0102aad0
0102b400:  ldr w9,[x29, #0xc8]
0102b404:  sbfiz x19,x19,#0x3,#0x20
0102b408:  add x1,x27,#0x1
0102b40c:  ldr w2,[x24, x19, LSL #0x0]
0102b410:  b 0x0102b464
0102b414:  add w0,w28,#0x1
0102b418:  cbz x24,0x0102b42c
0102b41c:  sbfiz x6,x28,#0x3,#0x20
0102b420:  mov w28,w0
0102b424:  ldr w2,[x24, x6, LSL #0x0]
0102b428:  b 0x0102b464
0102b42c:  mov w28,w0
0102b430:  tbnz w23,#0x1f,0x0102b440
0102b434:  add x2,x20,#0xb
0102b438:  mov w0,w23
0102b43c:  b 0x0102b454
0102b440:  add w0,w23,#0x8
0102b444:  add x4,x22,w23, SXTW 
0102b448:  cmp w0,wzr
0102b44c:  b.le 0x0102b45c
0102b450:  add x2,x20,#0xb
0102b454:  mov x4,x20
0102b458:  and x20,x2,#-0x8
0102b45c:  ldr w2,[x4]
0102b460:  mov w23,w0
0102b464:  cmp w2,wzr
0102b468:  mov x19,x1
0102b46c:  csinv w2,w2,wzr,ge
0102b470:  b 0x0102b128
0102b474:  mov w4,#0xcccc
0102b478:  movk w4,#0xccc, LSL #16
0102b47c:  cmp w0,w4
0102b480:  b.gt 0x0102b094
0102b484:  mov w4,#0x30
0102b488:  mul w0,w0,w21
0102b48c:  sub w11,w4,w11
0102b490:  add w11,w11,w3
0102b494:  cmp w0,w11
0102b498:  b.gt 0x0102b094
0102b49c:  ldrb w11,[x19], #0x1
0102b4a0:  add w0,w0,w1
0102b4a4:  sub w1,w11,#0x30
0102b4a8:  cmp w1,#0x9
0102b4ac:  b.ls 0x0102b474
0102b4b0:  cmp w11,#0x24
0102b4b4:  b.eq 0x0102b51c
0102b4b8:  mov w2,w0
0102b4bc:  b 0x0102b108
0102b4c0:  orr w25,w25,#0x80
0102b4c4:  b 0x0102b128
0102b4c8:  sub w11,w11,#0x30
0102b4cc:  add w0,w0,w11
0102b4d0:  ldrb w11,[x19], #0x1
0102b4d4:  sub w1,w11,#0x30
0102b4d8:  cmp w1,#0x9
0102b4dc:  b.hi 0x0102b50c
0102b4e0:  mov w1,#0xcccc
0102b4e4:  movk w1,#0xccc, LSL #16
0102b4e8:  cmp w0,w1
0102b4ec:  b.gt 0x0102b094
0102b4f0:  mov w1,#0x30
0102b4f4:  mul w0,w0,w21
0102b4f8:  sub w1,w1,w11
0102b4fc:  add w1,w1,w3
0102b500:  cmp w0,w1
0102b504:  b.le 0x0102b4c8
0102b508:  b 0x0102b094
0102b50c:  cmp w11,#0x24
0102b510:  b.eq 0x0102b51c
0102b514:  mov w26,w0
0102b518:  b 0x0102b108
0102b51c:  mov w28,w0
0102b520:  cbnz x24,0x0102b128
0102b524:  add x0,x29,#0xf8
0102b528:  str w9,[x29, #0xa8]
0102b52c:  str w2,[x29, #0xc8]
0102b530:  add x24,x29,#0x118
0102b534:  ldp x0,x1,[x0]
0102b538:  stp x0,x1,[x29, #0xd0]
0102b53c:  add x0,x29,#0x108
0102b540:  ldp x0,x1,[x0]
0102b544:  stp x0,x1,[x29, #0xe0]
0102b548:  ldr x0,[x29, #0xa0]
0102b54c:  add x1,x29,#0xd0
0102b550:  bl 0x0102aad0
0102b554:  ldr w2,[x29, #0xc8]
0102b558:  ldr w9,[x29, #0xa8]
0102b55c:  b 0x0102b128
0102b560:  ldrb w0,[x19]
0102b564:  cmp w0,#0x68
0102b568:  b.ne 0x0102b578
0102b56c:  add x19,x19,#0x1
0102b570:  orr w25,w25,#0x800
0102b574:  b 0x0102b128
0102b578:  orr w25,w25,#0x40
0102b57c:  b 0x0102b128
0102b580:  ldrb w0,[x19]
0102b584:  cmp w0,#0x6c
0102b588:  b.ne 0x0102b594
0102b58c:  add x19,x19,#0x1
0102b590:  b 0x0102b59c
0102b594:  orr w25,w25,#0x10
0102b598:  b 0x0102b128
0102b59c:  orr w25,w25,#0x20
0102b5a0:  b 0x0102b128
0102b5a4:  orr w25,w25,#0x200
0102b5a8:  b 0x0102b128
0102b5ac:  orr w25,w25,#0x400
0102b5b0:  b 0x0102b128
0102b5b4:  mov w0,w23
0102b5b8:  add w2,w28,#0x1
0102b5bc:  cbz x24,0x0102b5d0
0102b5c0:  sbfiz x6,x28,#0x3,#0x20
0102b5c4:  mov w28,w2
0102b5c8:  ldrb w1,[x24, x6, LSL ]
0102b5cc:  b 0x0102b5f8
0102b5d0:  mov w28,w2
0102b5d4:  tbz w23,#0x1f,0x0102b5e8
0102b5d8:  add w0,w23,#0x8
0102b5dc:  add x4,x22,w23, SXTW 
0102b5e0:  cmp w0,wzr
0102b5e4:  b.le 0x0102b5f4
0102b5e8:  add x1,x20,#0xb
0102b5ec:  mov x4,x20
0102b5f0:  and x20,x1,#-0x8
0102b5f4:  ldrb w1,[x4]
0102b5f8:  add x21,x29,#0x1c0
0102b5fc:  mov w23,w0
0102b600:  strb w1,[x21, #-0x68]!
0102b604:  b 0x0102bdac
0102b608:  orr w25,w25,#0x10
0102b60c:  mov w0,w23
0102b610:  add w3,w28,#0x1
0102b614:  tbnz w25,#0xc,0x0102b6dc
0102b618:  tbz w25,#0x5,0x0102b62c
0102b61c:  cbz x24,0x0102b63c
0102b620:  ldr x1,[x24, w28, SXTW #0x3]
0102b624:  mov w28,w3
0102b628:  b 0x0102b714
0102b62c:  tbnz w25,#0x4,0x0102b61c
0102b630:  tbnz w25,#0x9,0x0102b61c
0102b634:  tbz w25,#0xa,0x0102b65c
0102b638:  b 0x0102b61c
0102b63c:  mov w28,w3
0102b640:  tbz w23,#0x1f,0x0102b654
0102b644:  add w23,w23,#0x8
0102b648:  add x0,x22,w0, SXTW 
0102b64c:  cmp w23,wzr
0102b650:  b.le 0x0102b710
0102b654:  add x1,x20,#0xf
0102b658:  b 0x0102b708
0102b65c:  tbz w25,#0x6,0x0102b69c
0102b660:  cbz x24,0x0102b670
0102b664:  sbfiz x6,x28,#0x3,#0x20
0102b668:  ldrsh w1,[x24, x6, LSL #0x0]
0102b66c:  b 0x0102b624
0102b670:  mov w28,w3
0102b674:  tbz w23,#0x1f,0x0102b688
0102b678:  add w23,w23,#0x8
0102b67c:  add x0,x22,w0, SXTW 
0102b680:  cmp w23,wzr
0102b684:  b.le 0x0102b694
0102b688:  add x1,x20,#0xb
0102b68c:  mov x0,x20
0102b690:  and x20,x1,#-0x8
0102b694:  ldrsh w1,[x0]
0102b698:  b 0x0102b714
0102b69c:  tbz w25,#0xb,0x0102b6dc
0102b6a0:  cbz x24,0x0102b6b0
0102b6a4:  sbfiz x6,x28,#0x3,#0x20
0102b6a8:  ldrsb w1,[x24, x6, LSL ]
0102b6ac:  b 0x0102b624
0102b6b0:  mov w28,w3
0102b6b4:  tbz w23,#0x1f,0x0102b6c8
0102b6b8:  add w23,w23,#0x8
0102b6bc:  add x0,x22,w0, SXTW 
0102b6c0:  cmp w23,wzr
0102b6c4:  b.le 0x0102b6d4
0102b6c8:  add x1,x20,#0xb
0102b6cc:  mov x0,x20
0102b6d0:  and x20,x1,#-0x8
0102b6d4:  ldrsb w1,[x0]
0102b6d8:  b 0x0102b714
0102b6dc:  cbz x24,0x0102b6ec
0102b6e0:  sbfiz x6,x28,#0x3,#0x20
0102b6e4:  ldr w1,[x24, x6, LSL #0x0]
0102b6e8:  b 0x0102b624
0102b6ec:  mov w28,w3
0102b6f0:  tbz w23,#0x1f,0x0102b704
0102b6f4:  add w23,w23,#0x8
0102b6f8:  add x0,x22,w0, SXTW 
0102b6fc:  cmp w23,wzr
0102b700:  b.le 0x0102b710
0102b704:  add x1,x20,#0xb
0102b708:  mov x0,x20
0102b70c:  and x20,x1,#-0x8
0102b710:  ldr w1,[x0]
0102b714:  tbz w1,#0x1f,0x0102bcb8
0102b718:  neg w1,w1
0102b71c:  mov w27,w2
0102b720:  mov w11,#0x0
0102b724:  mov w9,#0x2d
0102b728:  b 0x0102bcc0
0102b72c:  mov w0,w23
0102b730:  add w1,w28,#0x1
0102b734:  tbnz w25,#0x5,0x0102b7c4
0102b738:  tbnz w25,#0x4,0x0102b7c4
0102b73c:  tbz w25,#0x6,0x0102b77c
0102b740:  cbz x24,0x0102b74c
0102b744:  ldr x0,[x24, w28, SXTW #0x3]
0102b748:  b 0x0102b770
0102b74c:  tbz w23,#0x1f,0x0102b760
0102b750:  add w23,w23,#0x8
0102b754:  add x0,x22,w0, SXTW 
0102b758:  cmp w23,wzr
0102b75c:  b.le 0x0102b76c
0102b760:  add x2,x20,#0xf
0102b764:  mov x0,x20
0102b768:  and x20,x2,#-0x8
0102b76c:  ldr x0,[x0]
0102b770:  ldrh w2,[x29, #0xcc]
0102b774:  strh w2,[x0]
0102b778:  b 0x0102b838
0102b77c:  tbz w25,#0xb,0x0102b7bc
0102b780:  cbz x24,0x0102b78c
0102b784:  ldr x0,[x24, w28, SXTW #0x3]
0102b788:  b 0x0102b7b0
0102b78c:  tbz w23,#0x1f,0x0102b7a0
0102b790:  add w23,w23,#0x8
0102b794:  add x0,x22,w0, SXTW 
0102b798:  cmp w23,wzr
0102b79c:  b.le 0x0102b7ac
0102b7a0:  add x2,x20,#0xf
0102b7a4:  mov x0,x20
0102b7a8:  and x20,x2,#-0x8
0102b7ac:  ldr x0,[x0]
0102b7b0:  ldrb w2,[x29, #0xcc]
0102b7b4:  strb w2,[x0]
0102b7b8:  b 0x0102b838
0102b7bc:  tbnz w25,#0x9,0x0102b7c4
0102b7c0:  tbz w25,#0xa,0x0102b800
0102b7c4:  cbz x24,0x0102b7d0
0102b7c8:  ldr x0,[x24, w28, SXTW #0x3]
0102b7cc:  b 0x0102b7f4
0102b7d0:  tbz w23,#0x1f,0x0102b7e4
0102b7d4:  add w23,w23,#0x8
0102b7d8:  add x0,x22,w0, SXTW 
0102b7dc:  cmp w23,wzr
0102b7e0:  b.le 0x0102b7f0
0102b7e4:  add x2,x20,#0xf
0102b7e8:  mov x0,x20
0102b7ec:  and x20,x2,#-0x8
0102b7f0:  ldr x0,[x0]
0102b7f4:  ldrsw x2,[x29, #0xcc]
0102b7f8:  str x2,[x0]
0102b7fc:  b 0x0102b838
0102b800:  cbz x24,0x0102b80c
0102b804:  ldr x0,[x24, w28, SXTW #0x3]
0102b808:  b 0x0102b830
0102b80c:  tbz w23,#0x1f,0x0102b820
0102b810:  add w23,w23,#0x8
0102b814:  add x0,x22,w0, SXTW 
0102b818:  cmp w23,wzr
0102b81c:  b.le 0x0102b82c
0102b820:  add x2,x20,#0xf
0102b824:  mov x0,x20
0102b828:  and x20,x2,#-0x8
0102b82c:  ldr x0,[x0]
0102b830:  ldr w2,[x29, #0xcc]
0102b834:  str w2,[x0]
0102b838:  mov w28,w1
0102b83c:  b 0x0102b058
0102b840:  mov x0,x20
0102b844:  mov w3,w23
0102b848:  add w9,w28,#0x1
0102b84c:  tbnz w25,#0xc,0x0102b90c
0102b850:  tbnz w25,#0x5,0x0102b860
0102b854:  tbnz w25,#0x4,0x0102b860
0102b858:  tbnz w25,#0x9,0x0102b860
0102b85c:  tbz w25,#0xa,0x0102b88c
0102b860:  cbz x24,0x0102b86c
0102b864:  ldr x1,[x24, w28, SXTW #0x3]
0102b868:  b 0x0102b918
0102b86c:  mov w28,w9
0102b870:  tbz w23,#0x1f,0x0102b884
0102b874:  add w3,w23,#0x8
0102b878:  add x4,x22,w23, SXTW 
0102b87c:  cmp w3,wzr
0102b880:  b.le 0x0102b944
0102b884:  add x0,x20,#0xf
0102b888:  b 0x0102b93c
0102b88c:  tbz w25,#0x6,0x0102b8cc
0102b890:  cbz x24,0x0102b8a0
0102b894:  sbfiz x6,x28,#0x3,#0x20
0102b898:  ldrh w1,[x24, x6, LSL #0x0]
0102b89c:  b 0x0102b918
0102b8a0:  mov w28,w9
0102b8a4:  tbz w23,#0x1f,0x0102b8b8
0102b8a8:  add w3,w23,#0x8
0102b8ac:  add x4,x22,w23, SXTW 
0102b8b0:  cmp w3,wzr
0102b8b4:  b.le 0x0102b8c4
0102b8b8:  add x0,x20,#0xb
0102b8bc:  mov x4,x20
0102b8c0:  and x0,x0,#-0x8
0102b8c4:  ldrh w1,[x4]
0102b8c8:  b 0x0102b948
0102b8cc:  tbz w25,#0xb,0x0102b90c
0102b8d0:  cbz x24,0x0102b8e0
0102b8d4:  sbfiz x6,x28,#0x3,#0x20
0102b8d8:  ldrb w1,[x24, x6, LSL ]
0102b8dc:  b 0x0102b918
0102b8e0:  mov w28,w9
0102b8e4:  tbz w23,#0x1f,0x0102b8f8
0102b8e8:  add w3,w23,#0x8
0102b8ec:  add x4,x22,w23, SXTW 
0102b8f0:  cmp w3,wzr
0102b8f4:  b.le 0x0102b904
0102b8f8:  add x0,x20,#0xb
0102b8fc:  mov x4,x20
0102b900:  and x0,x0,#-0x8
0102b904:  ldrb w1,[x4]
0102b908:  b 0x0102b948
0102b90c:  cbz x24,0x0102b920
0102b910:  sbfiz x6,x28,#0x3,#0x20
0102b914:  ldr w1,[x24, x6, LSL #0x0]
0102b918:  mov w28,w9
0102b91c:  b 0x0102b948
0102b920:  mov w28,w9
0102b924:  tbz w23,#0x1f,0x0102b938
0102b928:  add w3,w23,#0x8
0102b92c:  add x4,x22,w23, SXTW 
0102b930:  cmp w3,wzr
0102b934:  b.le 0x0102b944
0102b938:  add x0,x20,#0xb
0102b93c:  mov x4,x20
0102b940:  and x0,x0,#-0x8
0102b944:  ldr w1,[x4]
0102b948:  mov w11,#0x0
0102b94c:  mov w12,#0x0
0102b950:  b 0x0102bca4
0102b954:  mov w3,w23
0102b958:  add w0,w28,#0x1
0102b95c:  cbz x24,0x0102b96c
0102b960:  ldr x1,[x24, w28, SXTW #0x3]
0102b964:  mov w28,w0
0102b968:  b 0x0102b994
0102b96c:  mov w28,w0
0102b970:  tbz w23,#0x1f,0x0102b984
0102b974:  add w3,w23,#0x8
0102b978:  add x4,x22,w23, SXTW 
0102b97c:  cmp w3,wzr
0102b980:  b.le 0x0102b990
0102b984:  add x0,x20,#0xf
0102b988:  mov x4,x20
0102b98c:  and x20,x0,#-0x8
0102b990:  ldr w1,[x4]
0102b994:  adrp x4,0x102e000
0102b998:  mov x0,x20
0102b99c:  add x4,x4,#0x418
0102b9a0:  mov w11,#0x78
0102b9a4:  str x4,[x29, #0x90]
0102b9a8:  b 0x0102bca0
0102b9ac:  str w2,[x29, #0xa8]
0102b9b0:  mov w0,w23
0102b9b4:  add w1,w28,#0x1
0102b9b8:  cbz x24,0x0102b9c8
0102b9bc:  ldr x21,[x24, w28, SXTW #0x3]
0102b9c0:  mov w28,w1
0102b9c4:  b 0x0102b9f0
0102b9c8:  mov w28,w1
0102b9cc:  tbz w23,#0x1f,0x0102b9e0
0102b9d0:  add w23,w23,#0x8
0102b9d4:  add x0,x22,w0, SXTW 
0102b9d8:  cmp w23,wzr
0102b9dc:  b.le 0x0102b9ec
0102b9e0:  add x1,x20,#0xf
0102b9e4:  mov x0,x20
0102b9e8:  and x20,x1,#-0x8
0102b9ec:  ldr x21,[x0]
0102b9f0:  cbnz x21,0x0102b9fc
0102b9f4:  adrp x13,0x102e000
0102b9f8:  add x21,x13,#0x448
0102b9fc:  cmn w2,#0x1
0102ba00:  b.eq 0x0102ba28
0102ba04:  mov w1,#0x0
0102ba08:  sxtw x2,w2
0102ba0c:  mov x0,x21
0102ba10:  bl 0x0102c22c
0102ba14:  sub w1,w0,w21
0102ba18:  cmp x0,xzr
0102ba1c:  ldr w0,[x29, #0xa8]
0102ba20:  csel w0,w1,w0,ne
0102ba24:  b 0x0102ba3c
0102ba28:  mov x0,x21
0102ba2c:  bl 0x0102c1f4
0102ba30:  orr x1,xzr,#0x7fffffff
0102ba34:  cmp x0,x1
0102ba38:  b.hi 0x0102b094
0102ba3c:  str w0,[x29, #0xa8]
0102ba40:  mov w11,#0x0
0102ba44:  mov w9,#0x0
0102ba48:  b 0x0102bdbc
0102ba4c:  orr w25,w25,#0x10
0102ba50:  mov x0,x20
0102ba54:  mov w3,w23
0102ba58:  add w9,w28,#0x1
0102ba5c:  tbnz w25,#0xc,0x0102bb1c
0102ba60:  tbnz w25,#0x5,0x0102ba70
0102ba64:  tbnz w25,#0x4,0x0102ba70
0102ba68:  tbnz w25,#0x9,0x0102ba70
0102ba6c:  tbz w25,#0xa,0x0102ba9c
0102ba70:  cbz x24,0x0102ba7c
0102ba74:  ldr x1,[x24, w28, SXTW #0x3]
0102ba78:  b 0x0102bb28
0102ba7c:  mov w28,w9
0102ba80:  tbz w23,#0x1f,0x0102ba94
0102ba84:  add w3,w23,#0x8
0102ba88:  add x4,x22,w23, SXTW 
0102ba8c:  cmp w3,wzr
0102ba90:  b.le 0x0102bb5c
0102ba94:  add x0,x20,#0xf
0102ba98:  b 0x0102bb54
0102ba9c:  tbz w25,#0x6,0x0102badc
0102baa0:  cbz x24,0x0102bab0
0102baa4:  sbfiz x6,x28,#0x3,#0x20
0102baa8:  ldrh w1,[x24, x6, LSL #0x0]
0102baac:  b 0x0102bb28
0102bab0:  mov w28,w9
0102bab4:  tbz w23,#0x1f,0x0102bac8
0102bab8:  add w3,w23,#0x8
0102babc:  add x4,x22,w23, SXTW 
0102bac0:  cmp w3,wzr
0102bac4:  b.le 0x0102bad4
0102bac8:  add x0,x20,#0xb
0102bacc:  mov x4,x20
0102bad0:  and x0,x0,#-0x8
0102bad4:  ldrh w1,[x4]
0102bad8:  b 0x0102bb2c
0102badc:  tbz w25,#0xb,0x0102bb1c
0102bae0:  cbz x24,0x0102baf0
0102bae4:  sbfiz x6,x28,#0x3,#0x20
0102bae8:  ldrb w1,[x24, x6, LSL ]
0102baec:  b 0x0102bb28
0102baf0:  mov w28,w9
0102baf4:  tbz w23,#0x1f,0x0102bb08
0102baf8:  add w3,w23,#0x8
0102bafc:  add x4,x22,w23, SXTW 
0102bb00:  cmp w3,wzr
0102bb04:  b.le 0x0102bb14
0102bb08:  add x0,x20,#0xb
0102bb0c:  mov x4,x20
0102bb10:  and x0,x0,#-0x8
0102bb14:  ldrb w1,[x4]
0102bb18:  b 0x0102bb2c
0102bb1c:  cbz x24,0x0102bb38
0102bb20:  sbfiz x6,x28,#0x3,#0x20
0102bb24:  ldr w1,[x24, x6, LSL #0x0]
0102bb28:  mov w28,w9
0102bb2c:  mov w11,#0x0
0102bb30:  mov w12,#0x1
0102bb34:  b 0x0102bca4
0102bb38:  mov w28,w9
0102bb3c:  tbz w23,#0x1f,0x0102bb50
0102bb40:  add w3,w23,#0x8
0102bb44:  add x4,x22,w23, SXTW 
0102bb48:  cmp w3,wzr
0102bb4c:  b.le 0x0102bb5c
0102bb50:  add x0,x20,#0xb
0102bb54:  mov x4,x20
0102bb58:  and x0,x0,#-0x8
0102bb5c:  ldr w1,[x4]
0102bb60:  b 0x0102bb2c
0102bb64:  adrp x1,0x102e000
0102bb68:  mov w3,w23
0102bb6c:  mov x0,x20
0102bb70:  add x1,x1,#0x418
0102bb74:  b 0x0102bb88
0102bb78:  adrp x1,0x102e000
0102bb7c:  mov x0,x20
0102bb80:  mov w3,w23
0102bb84:  add x1,x1,#0x428
0102bb88:  str x1,[x29, #0x90]
0102bb8c:  add w9,w28,#0x1
0102bb90:  tbnz w25,#0xc,0x0102bc58
0102bb94:  tbz w25,#0x5,0x0102bba8
0102bb98:  cbz x24,0x0102bbb8
0102bb9c:  ldr x1,[x24, w28, SXTW #0x3]
0102bba0:  mov w28,w9
0102bba4:  b 0x0102bc90
0102bba8:  tbnz w25,#0x4,0x0102bb98
0102bbac:  tbnz w25,#0x9,0x0102bb98
0102bbb0:  tbz w25,#0xa,0x0102bbd8
0102bbb4:  b 0x0102bb98
0102bbb8:  mov w28,w9
0102bbbc:  tbz w23,#0x1f,0x0102bbd0
0102bbc0:  add w3,w23,#0x8
0102bbc4:  add x4,x22,w23, SXTW 
0102bbc8:  cmp w3,wzr
0102bbcc:  b.le 0x0102bc8c
0102bbd0:  add x0,x20,#0xf
0102bbd4:  b 0x0102bc84
0102bbd8:  tbz w25,#0x6,0x0102bc18
0102bbdc:  cbz x24,0x0102bbec
0102bbe0:  sbfiz x6,x28,#0x3,#0x20
0102bbe4:  ldrh w1,[x24, x6, LSL #0x0]
0102bbe8:  b 0x0102bba0
0102bbec:  mov w28,w9
0102bbf0:  tbz w23,#0x1f,0x0102bc04
0102bbf4:  add w3,w23,#0x8
0102bbf8:  add x4,x22,w23, SXTW 
0102bbfc:  cmp w3,wzr
0102bc00:  b.le 0x0102bc10
0102bc04:  add x0,x20,#0xb
0102bc08:  mov x4,x20
0102bc0c:  and x0,x0,#-0x8
0102bc10:  ldrh w1,[x4]
0102bc14:  b 0x0102bc90
0102bc18:  tbz w25,#0xb,0x0102bc58
0102bc1c:  cbz x24,0x0102bc2c
0102bc20:  sbfiz x6,x28,#0x3,#0x20
0102bc24:  ldrb w1,[x24, x6, LSL ]
0102bc28:  b 0x0102bba0
0102bc2c:  mov w28,w9
0102bc30:  tbz w23,#0x1f,0x0102bc44
0102bc34:  add w3,w23,#0x8
0102bc38:  add x4,x22,w23, SXTW 
0102bc3c:  cmp w3,wzr
0102bc40:  b.le 0x0102bc50
0102bc44:  add x0,x20,#0xb
0102bc48:  mov x4,x20
0102bc4c:  and x0,x0,#-0x8
0102bc50:  ldrb w1,[x4]
0102bc54:  b 0x0102bc90
0102bc58:  cbz x24,0x0102bc68
0102bc5c:  sbfiz x6,x28,#0x3,#0x20
0102bc60:  ldr w1,[x24, x6, LSL #0x0]
0102bc64:  b 0x0102bba0
0102bc68:  mov w28,w9
0102bc6c:  tbz w23,#0x1f,0x0102bc80
0102bc70:  add w3,w23,#0x8
0102bc74:  add x4,x22,w23, SXTW 
0102bc78:  cmp w3,wzr
0102bc7c:  b.le 0x0102bc8c
0102bc80:  add x0,x20,#0xb
0102bc84:  mov x4,x20
0102bc88:  and x0,x0,#-0x8
0102bc8c:  ldr w1,[x4]
0102bc90:  tbz w25,#0x0,0x0102bc9c
0102bc94:  uxtb w11,w11
0102bc98:  cbnz w1,0x0102bca0
0102bc9c:  mov w11,#0x0
0102bca0:  mov w12,#0x2
0102bca4:  mov w27,w2
0102bca8:  mov w23,w3
0102bcac:  mov x20,x0
0102bcb0:  mov w9,#0x0
0102bcb4:  b 0x0102bcc4
0102bcb8:  mov w27,w2
0102bcbc:  mov w11,#0x0
0102bcc0:  mov w12,#0x1
0102bcc4:  and w0,w25,#0xffffff7f
0102bcc8:  cmn w2,#0x1
0102bccc:  csel w25,w0,w25,ne
0102bcd0:  cbnz w2,0x0102bcdc
0102bcd4:  add x21,x29,#0x1bc
0102bcd8:  cbz w1,0x0102bd88
0102bcdc:  cmp w12,#0x1
0102bce0:  b.eq 0x0102bcf4
0102bce4:  cmp w12,#0x2
0102bce8:  b.eq 0x0102bd6c
0102bcec:  add x21,x29,#0x1bc
0102bcf0:  b 0x0102bd00
0102bcf4:  add x0,x29,#0x1bc
0102bcf8:  mov w2,#0xa
0102bcfc:  b 0x0102bd34
0102bd00:  and w0,w1,#0x7
0102bd04:  lsr w1,w1,#0x3
0102bd08:  add w0,w0,#0x30
0102bd0c:  strb w0,[x21, #-0x1]!
0102bd10:  cbnz w1,0x0102bd00
0102bd14:  mov x1,x21
0102bd18:  tbz w25,#0x0,0x0102bd88
0102bd1c:  cmp w0,#0x30
0102bd20:  b.eq 0x0102bd88
0102bd24:  mov w0,#0x30
0102bd28:  sub x21,x21,#0x1
0102bd2c:  sturb w0,[x1, #-0x1]
0102bd30:  b 0x0102bd88
0102bd34:  cmp w1,#0x9
0102bd38:  sub x13,x0,#0x1
0102bd3c:  b.ls 0x0102bd5c
0102bd40:  udiv w3,w1,w2
0102bd44:  mov x0,x13
0102bd48:  msub w1,w3,w2,w1
0102bd4c:  add w1,w1,#0x30
0102bd50:  strb w1,[x13]
0102bd54:  mov w1,w3
0102bd58:  b 0x0102bd34
0102bd5c:  add w1,w1,#0x30
0102bd60:  mov x21,x13
0102bd64:  sturb w1,[x0, #-0x1]
0102bd68:  b 0x0102bd88
0102bd6c:  add x21,x29,#0x1bc
0102bd70:  and x0,x1,#0xf
0102bd74:  ldr x2,[x29, #0x90]
0102bd78:  lsr w1,w1,#0x4
0102bd7c:  ldrb w0,[x2, x0, LSL ]
0102bd80:  strb w0,[x21, #-0x1]!
0102bd84:  cbnz w1,0x0102bd70
0102bd88:  add x0,x29,#0x1bc
0102bd8c:  sub x0,x0,x21
0102bd90:  str w0,[x29, #0xa8]
0102bd94:  cmp w0,#0x64
0102bd98:  b.le 0x0102bdc0
0102bd9c:  b 0x0102b094
0102bda0:  cbz w11,0x0102c1d4
0102bda4:  add x21,x29,#0x1c0
0102bda8:  strb w11,[x21, #-0x68]!
0102bdac:  mov w0,#0x1
0102bdb0:  mov w11,#0x0
0102bdb4:  mov w9,#0x0
0102bdb8:  str w0,[x29, #0xa8]
0102bdbc:  mov w27,#0x0
0102bdc0:  ldr w0,[x29, #0xa8]
0102bdc4:  cmp w0,w27
0102bdc8:  csel w0,w0,w27,ge
0102bdcc:  cmp w9,wzr
0102bdd0:  mov w0,w0
0102bdd4:  cinc w0,w0,ne
0102bdd8:  str w0,[x29, #0xc8]
0102bddc:  cmp w11,wzr
0102bde0:  ldr w1,[x29, #0xc8]
0102bde4:  mov w0,w0
0102bde8:  add w0,w0,#0x2
0102bdec:  csel w0,w0,w1,ne
0102bdf0:  str w0,[x29, #0xc8]
0102bdf4:  mov w0,#0x84
0102bdf8:  ands w0,w25,w0
0102bdfc:  str w0,[x29, #0x88]
0102be00:  b.eq 0x0102be28
0102be04:  cbz w9,0x0102bf04
0102be08:  ldr x3,[x29, #0xb0]
0102be0c:  mov w2,w9
0102be10:  ldr x1,[x29, #0xb8]
0102be14:  ldr x0,[x29, #0xc0]
0102be18:  str w11,[x29, #0x80]
0102be1c:  bl 0x0102aa64
0102be20:  ldr w11,[x29, #0x80]
0102be24:  b 0x0102bf04
0102be28:  ldr w0,[x29, #0xc8]
0102be2c:  sub w15,w26,w0
0102be30:  cmp w15,wzr
0102be34:  mov w14,w15
0102be38:  b.le 0x0102be04
0102be3c:  cmp w14,#0x10
0102be40:  b.le 0x0102bea0
0102be44:  mov x12,#0x0
0102be48:  adrp x0,0x102e000
0102be4c:  ldr x3,[x29, #0xb0]
0102be50:  add x0,x0,#0x450
0102be54:  ldr x1,[x29, #0xb8]
0102be58:  str w15,[x29, #0x6c]
0102be5c:  str w9,[x29, #0x70]
0102be60:  ldrb w2,[x0, x12, LSL ]
0102be64:  ldr x0,[x29, #0xc0]
0102be68:  str w11,[x29, #0x78]
0102be6c:  str w14,[x29, #0x7c]
0102be70:  str x12,[x29, #0x80]
0102be74:  bl 0x0102aa64
0102be78:  ldr x12,[x29, #0x80]
0102be7c:  ldr w14,[x29, #0x7c]
0102be80:  add x12,x12,#0x1
0102be84:  ldr w11,[x29, #0x78]
0102be88:  cmp x12,#0x10
0102be8c:  ldr w9,[x29, #0x70]
0102be90:  ldr w15,[x29, #0x6c]
0102be94:  b.ne 0x0102be48
0102be98:  sub w14,w14,#0x10
0102be9c:  b 0x0102be3c
0102bea0:  sub w12,w15,#0x1
0102bea4:  ldr x0,[x29, #0x98]
0102bea8:  lsr w12,w12,#0x4
0102beac:  add x14,x0,#0x1
0102beb0:  sub w12,w15,w12, LSL #0x4
0102beb4:  sub w12,w12,#0x1
0102beb8:  add x12,x12,#0x2
0102bebc:  add x12,x0,x12
0102bec0:  ldurb w2,[x14, #-0x1]
0102bec4:  ldr x3,[x29, #0xb0]
0102bec8:  ldr x1,[x29, #0xb8]
0102becc:  ldr x0,[x29, #0xc0]
0102bed0:  str x12,[x29, #0x70]
0102bed4:  str w9,[x29, #0x78]
0102bed8:  str w11,[x29, #0x7c]
0102bedc:  str x14,[x29, #0x80]
0102bee0:  bl 0x0102aa64
0102bee4:  ldr x14,[x29, #0x80]
0102bee8:  ldr x12,[x29, #0x70]
0102beec:  add x14,x14,#0x1
0102bef0:  ldr w11,[x29, #0x7c]
0102bef4:  cmp x14,x12
0102bef8:  ldr w9,[x29, #0x78]
0102befc:  b.ne 0x0102bec0
0102bf00:  b 0x0102be04
0102bf04:  cbz w11,0x0102bf38
0102bf08:  ldr x3,[x29, #0xb0]
0102bf0c:  mov w2,#0x30
0102bf10:  ldr x1,[x29, #0xb8]
0102bf14:  ldr x0,[x29, #0xc0]
0102bf18:  str w11,[x29, #0x80]
0102bf1c:  bl 0x0102aa64
0102bf20:  ldr w11,[x29, #0x80]
0102bf24:  ldr x3,[x29, #0xb0]
0102bf28:  ldr x1,[x29, #0xb8]
0102bf2c:  mov w2,w11
0102bf30:  ldr x0,[x29, #0xc0]
0102bf34:  bl 0x0102aa64
0102bf38:  ldr w0,[x29, #0x88]
0102bf3c:  cmp w0,#0x80
0102bf40:  b.eq 0x0102bf60
0102bf44:  ldr w0,[x29, #0xa8]
0102bf48:  sub w27,w27,w0
0102bf4c:  cmp w27,wzr
0102bf50:  b.le 0x0102c074
0102bf54:  mov w11,w27
0102bf58:  adrp x12,0x102e000
0102bf5c:  b 0x0102c0c0
0102bf60:  ldr w0,[x29, #0xc8]
0102bf64:  sub w12,w26,w0
0102bf68:  cmp w12,wzr
0102bf6c:  b.le 0x0102bf44
0102bf70:  mov w11,w12
0102bf74:  adrp x14,0x102e000
0102bf78:  cmp w11,#0x10
0102bf7c:  b.le 0x0102bfd0
0102bf80:  mov x9,#0x0
0102bf84:  add x0,x14,#0x438
0102bf88:  ldr x3,[x29, #0xb0]
0102bf8c:  ldr x1,[x29, #0xb8]
0102bf90:  str w12,[x29, #0x78]
0102bf94:  ldrb w2,[x0, x9, LSL ]
0102bf98:  ldr x0,[x29, #0xc0]
0102bf9c:  str w11,[x29, #0x7c]
0102bfa0:  str x14,[x29, #0x80]
0102bfa4:  str x9,[x29, #0x88]
0102bfa8:  bl 0x0102aa64
0102bfac:  ldr x9,[x29, #0x88]
0102bfb0:  ldr x14,[x29, #0x80]
0102bfb4:  add x9,x9,#0x1
0102bfb8:  ldr w11,[x29, #0x7c]
0102bfbc:  cmp x9,#0x10
0102bfc0:  ldr w12,[x29, #0x78]
0102bfc4:  b.ne 0x0102bf84
0102bfc8:  sub w11,w11,#0x10
0102bfcc:  b 0x0102bf78
0102bfd0:  sub w0,w12,#0x1
0102bfd4:  adrp x9,0x102e000
0102bfd8:  lsr w0,w0,#0x4
0102bfdc:  add x9,x9,#0x438
0102bfe0:  add x11,x9,#0x1
0102bfe4:  sub w0,w12,w0, LSL #0x4
0102bfe8:  sub w0,w0,#0x1
0102bfec:  add x0,x0,#0x2
0102bff0:  add x9,x9,x0
0102bff4:  ldurb w2,[x11, #-0x1]
0102bff8:  ldr x3,[x29, #0xb0]
0102bffc:  ldr x1,[x29, #0xb8]
0102c000:  ldr x0,[x29, #0xc0]
0102c004:  str x9,[x29, #0x80]
0102c008:  str x11,[x29, #0x88]
0102c00c:  bl 0x0102aa64
0102c010:  ldr x11,[x29, #0x88]
0102c014:  ldr x9,[x29, #0x80]
0102c018:  add x11,x11,#0x1
0102c01c:  cmp x11,x9
0102c020:  b.ne 0x0102bff4
0102c024:  b 0x0102bf44
0102c028:  sub w0,w27,#0x1
0102c02c:  adrp x1,0x102f000
0102c030:  lsr w0,w0,#0x4
0102c034:  add x1,x1,#0x438
0102c038:  add x9,x1,#0x1
0102c03c:  sub w27,w27,w0, LSL #0x4
0102c040:  sub w27,w27,#0x1
0102c044:  add x27,x27,#0x2
0102c048:  add x27,x1,x27
0102c04c:  ldurb w2,[x9, #-0x1]
0102c050:  ldr x3,[x29, #0xb0]
0102c054:  ldr x1,[x29, #0xb8]
0102c058:  ldr x0,[x29, #0xc0]
0102c05c:  str x9,[x29, #0x88]
0102c060:  bl 0x0102aa64
0102c064:  ldr x9,[x29, #0x88]
0102c068:  add x9,x9,#0x1
0102c06c:  cmp x9,x27
0102c070:  b.ne 0x0102c04c
0102c074:  ldr w4,[x29, #0xa8]
0102c078:  mov x27,#0x0
0102c07c:  b 0x0102c0d0
0102c080:  add x0,x12,#0x438
0102c084:  ldr x3,[x29, #0xb0]
0102c088:  ldr x1,[x29, #0xb8]
0102c08c:  str w11,[x29, #0x7c]
0102c090:  ldrb w2,[x0, x9, LSL ]
0102c094:  ldr x0,[x29, #0xc0]
0102c098:  str x12,[x29, #0x80]
0102c09c:  str x9,[x29, #0x88]
0102c0a0:  bl 0x0102aa64
0102c0a4:  ldr x9,[x29, #0x88]
0102c0a8:  ldr x12,[x29, #0x80]
0102c0ac:  add x9,x9,#0x1
0102c0b0:  ldr w11,[x29, #0x7c]
0102c0b4:  cmp x9,#0x10
0102c0b8:  b.ne 0x0102c080
0102c0bc:  sub w11,w11,#0x10
0102c0c0:  cmp w11,#0x10
0102c0c4:  b.le 0x0102c028
0102c0c8:  mov x9,#0x0
0102c0cc:  b 0x0102c080
0102c0d0:  cmp x27,x4
0102c0d4:  b.eq 0x0102c0fc
0102c0d8:  ldrb w2,[x21, x27, LSL ]
0102c0dc:  add x27,x27,#0x1
0102c0e0:  ldr x3,[x29, #0xb0]
0102c0e4:  ldr x1,[x29, #0xb8]
0102c0e8:  ldr x0,[x29, #0xc0]
0102c0ec:  str x4,[x29, #0xa8]
0102c0f0:  bl 0x0102aa64
0102c0f4:  ldr x4,[x29, #0xa8]
0102c0f8:  b 0x0102c0d0
0102c0fc:  tbnz w25,#0x2,0x0102c130
0102c100:  ldr w0,[x29, #0xc8]
0102c104:  ldr w1,[x29, #0xcc]
0102c108:  cmp w0,w26
0102c10c:  csel w25,w0,w26,ge
0102c110:  mov w0,#0x7fffffff
0102c114:  sub w0,w0,w1
0102c118:  cmp w25,w0
0102c11c:  b.gt 0x0102b094
0102c120:  ldr w0,[x29, #0xcc]
0102c124:  add w0,w0,w25
0102c128:  str w0,[x29, #0xcc]
0102c12c:  b 0x0102b058
0102c130:  ldr w0,[x29, #0xc8]
0102c134:  sub w7,w26,w0
0102c138:  cmp w7,wzr
0102c13c:  mov w27,w7
0102c140:  b.le 0x0102c100
0102c144:  cmp w27,#0x10
0102c148:  b.le 0x0102c188
0102c14c:  mov x21,#0x0
0102c150:  adrp x0,0x102f000
0102c154:  ldr x3,[x29, #0xb0]
0102c158:  add x0,x0,#0x450
0102c15c:  ldr x1,[x29, #0xb8]
0102c160:  str w7,[x29, #0xa8]
0102c164:  ldrb w2,[x0, x21, LSL ]
0102c168:  add x21,x21,#0x1
0102c16c:  ldr x0,[x29, #0xc0]
0102c170:  bl 0x0102aa64
0102c174:  cmp x21,#0x10
0102c178:  ldr w7,[x29, #0xa8]
0102c17c:  b.ne 0x0102c150
0102c180:  sub w27,w27,#0x10
0102c184:  b 0x0102c144
0102c188:  sub w21,w7,#0x1
0102c18c:  ldr x0,[x29, #0x98]
0102c190:  lsr w21,w21,#0x4
0102c194:  add x27,x0,#0x1
0102c198:  sub w21,w7,w21, LSL #0x4
0102c19c:  sub w21,w21,#0x1
0102c1a0:  add x21,x21,#0x2
0102c1a4:  add x21,x0,x21
0102c1a8:  ldurb w2,[x27, #-0x1]
0102c1ac:  add x27,x27,#0x1
0102c1b0:  ldr x3,[x29, #0xb0]
0102c1b4:  ldr x1,[x29, #0xb8]
0102c1b8:  ldr x0,[x29, #0xc0]
0102c1bc:  bl 0x0102aa64
0102c1c0:  cmp x27,x21
0102c1c4:  b.ne 0x0102c1a8
0102c1c8:  b 0x0102c100
0102c1cc:  mov x21,x1
0102c1d0:  b 0x0102b05c
0102c1d4:  ldr w0,[x29, #0xcc]
0102c1d8:  ldp x19,x20,[sp, #0x10]
0102c1dc:  ldp x21,x22,[sp, #0x20]
0102c1e0:  ldp x23,x24,[sp, #0x30]
0102c1e4:  ldp x25,x26,[sp, #0x40]
0102c1e8:  ldp x27,x28,[sp, #0x50]
0102c1ec:  ldp x29,x30,[sp], #0x1c0
0102c1f0:  ret

; ==== FUN_0102c1f4 @ 0102c1f4 ====
0102c1f4:  mov x1,x0
0102c1f8:  ldrb w2,[x1]
0102c1fc:  cbz w2,0x0102c208
0102c200:  add x1,x1,#0x1
0102c204:  b 0x0102c1f8
0102c208:  sub x0,x1,x0
0102c20c:  ret

; ==== FUN_0102c210 @ 0102c210 ====
0102c210:  mov x3,#0x0
0102c214:  cbz x2,0x0102c228
0102c218:  strb w1,[x0, x3, LSL ]
0102c21c:  add x3,x3,#0x1
0102c220:  cmp x3,x2
0102c224:  b.ne 0x0102c218
0102c228:  ret

; ==== FUN_0102c22c @ 0102c22c ====
0102c22c:  mov x5,x0
0102c230:  mov x0,#0x0
0102c234:  cbz x2,0x0102c26c
0102c238:  mov x3,x5
0102c23c:  uxtb w1,w1
0102c240:  mov x4,x3
0102c244:  ldrb w0,[x4], #0x1
0102c248:  cmp w0,w1
0102c24c:  b.eq 0x0102c268
0102c250:  sub x0,x2,x4
0102c254:  mov x3,x4
0102c258:  cmn x5,x0
0102c25c:  b.ne 0x0102c240
0102c260:  mov x0,#0x0
0102c264:  b 0x0102c26c
0102c268:  mov x0,x3
0102c26c:  ret

; ==== FUN_0102c270 @ 0102c270 ====
0102c270:  mov w1,#0xf
0102c274:  strh w1,[x0, #0xf4]
0102c278:  ret

; ==== FUN_0102c27c @ 0102c27c ====
0102c27c:  stp x29,x30,[sp, #-0x20]!
0102c280:  mov x29,sp
0102c284:  stp x19,x20,[sp, #0x10]
0102c288:  mov x20,x0
0102c28c:  mov x19,x1
0102c290:  cbnz x0,0x0102c2d8
0102c294:  mov w0,#0x0
0102c298:  bl 0x0102a684
0102c29c:  adrp x4,0x102e000
0102c2a0:  adrp x3,0x102e000
0102c2a4:  adrp x1,0x102e000
0102c2a8:  adrp x0,0x102e000
0102c2ac:  mov w5,#0x2bf
0102c2b0:  add x4,x4,#0x540
0102c2b4:  add x3,x3,#0x4a0
0102c2b8:  mov w2,#0x1
0102c2bc:  add x1,x1,#0x516
0102c2c0:  add x0,x0,#0xc1
0102c2c4:  bl 0x0102a9c8
0102c2c8:  mov w0,#0x0
0102c2cc:  bl 0x0102a6a0
0102c2d0:  mov x0,#0x0
0102c2d4:  bl 0x010294c8
0102c2d8:  cbnz x19,0x0102c320
0102c2dc:  mov w0,#0x0
0102c2e0:  bl 0x0102a684
0102c2e4:  adrp x4,0x102e000
0102c2e8:  adrp x3,0x102e000
0102c2ec:  adrp x1,0x102e000
0102c2f0:  adrp x0,0x102e000
0102c2f4:  mov w5,#0x2c0
0102c2f8:  add x4,x4,#0x540
0102c2fc:  add x3,x3,#0x4a0
0102c300:  mov w2,#0x1
0102c304:  add x1,x1,#0x529
0102c308:  add x0,x0,#0xc1
0102c30c:  bl 0x0102a9c8
0102c310:  mov w0,#0x0
0102c314:  bl 0x0102a6a0
0102c318:  mov x0,#0x0
0102c31c:  bl 0x010294c8
0102c320:  ldr w20,[x20, #0x110]
0102c324:  mov w1,#0x7840
0102c328:  movk w1,#0x17d, LSL #16
0102c32c:  ubfx x0,x20,#0x13,#0x1
0102c330:  and w2,w20,#0xf
0102c334:  cmp w0,#0x1
0102c338:  mov w0,#0xe100
0102c33c:  movk w0,#0x5f5, LSL #16
0102c340:  sub w2,w2,#0x1
0102c344:  csel w1,w0,w1,eq
0102c348:  cmp w2,#0xe
0102c34c:  str w1,[x19, #0x14]
0102c350:  mov w0,w1
0102c354:  b.hi 0x0102c420
0102c358:  adrp x0,0x102e000
0102c35c:  add x0,x0,#0x460
0102c360:  ldrb w0,[x0, w2, UXTW ]
0102c364:  adr x2,0x102c370
0102c368:  add x0,x2,w0, SXTB  #0x2
0102c36c:  br x0
0102c420:  ubfx x2,x20,#0x4,#0x3
0102c424:  str w0,[x19]
0102c428:  sub w2,w2,#0x1
0102c42c:  mov w0,w1
0102c430:  cmp w2,#0x6
0102c434:  b.hi 0x0102c4a0
0102c438:  adrp x0,0x102e000
0102c43c:  add x0,x0,#0x470
0102c440:  ldrb w0,[x0, w2, UXTW ]
0102c444:  adr x2,0x102c450
0102c448:  add x0,x2,w0, SXTB  #0x2
0102c44c:  br x0
0102c4a0:  str w0,[x19, #0x4]
0102c4a4:  ubfx x0,x20,#0x7,#0x2
0102c4a8:  cbz x0,0x0102c4cc
0102c4ac:  cmp x0,#0x1
0102c4b0:  mov w3,#0x2f00
0102c4b4:  mov w0,#0x5e00
0102c4b8:  movk w3,#0x5968, LSL #16
0102c4bc:  movk w0,#0xb2d0, LSL #16
0102c4c0:  mov w2,#0x0
0102c4c4:  csel w0,w0,w3,eq
0102c4c8:  b 0x0102c4d4
0102c4cc:  mov w0,w1
0102c4d0:  mov w2,#0x1
0102c4d4:  str w0,[x19, #0x8]
0102c4d8:  mov w0,w1
0102c4dc:  cbnz w2,0x0102c544
0102c4e0:  ubfx x0,x20,#0x9,#0x2
0102c4e4:  cmp w0,#0x2
0102c4e8:  b.eq 0x0102c50c
0102c4ec:  cmp w0,#0x3
0102c4f0:  b.eq 0x0102c520
0102c4f4:  cmp w0,#0x1
0102c4f8:  b.ne 0x0102c534
0102c4fc:  mov w1,#0xbc0
0102c500:  movk w1,#0x165a, LSL #16
0102c504:  mov w0,w1
0102c508:  b 0x0102c544
0102c50c:  mov w0,#0xbc0
0102c510:  mov w1,#0xc300
0102c514:  movk w0,#0x165a, LSL #16
0102c518:  movk w1,#0x1982, LSL #16
0102c51c:  b 0x0102c544
0102c520:  mov w0,#0xbc0
0102c524:  mov w1,#0x6500
0102c528:  movk w0,#0x165a, LSL #16
0102c52c:  movk w1,#0x1dcd, LSL #16
0102c530:  b 0x0102c544
0102c534:  mov w0,#0xbc0
0102c538:  mov w1,#0xb280
0102c53c:  movk w0,#0x165a, LSL #16
0102c540:  movk w1,#0xee6, LSL #16
0102c544:  str w1,[x19, #0xc]
0102c548:  ubfx x1,x20,#0xb,#0x1
0102c54c:  str w1,[x19, #0x18]
0102c550:  ubfx x1,x20,#0xc,#0x2
0102c554:  str w0,[x19, #0x10]
0102c558:  cmp w1,#0x2
0102c55c:  mov w0,#0x1
0102c560:  str w0,[x19, #0x40]
0102c564:  b.eq 0x0102c584
0102c568:  cmp w1,#0x3
0102c56c:  b.eq 0x0102c578
0102c570:  cmp w1,w0
0102c574:  b.ne 0x0102c58c
0102c578:  mov w1,#0x1
0102c57c:  mov w0,#0x0
0102c580:  b 0x0102c590
0102c584:  mov w1,#0x0
0102c588:  b 0x0102c590
0102c58c:  mov w1,w0
0102c590:  str w0,[x19, #0x1c]
0102c594:  ubfx x0,x20,#0xe,#0x1
0102c598:  str w0,[x19, #0x24]
0102c59c:  ubfx x0,x20,#0xf,#0x3
0102c5a0:  sub w0,w0,#0x2
0102c5a4:  str w1,[x19, #0x20]
0102c5a8:  cmp w0,#0x5
0102c5ac:  b.hi 0x0102c5d8
0102c5b0:  adrp x1,0x102e000
0102c5b4:  add x1,x1,#0x478
0102c5b8:  ldrb w0,[x1, w0, UXTW ]
0102c5bc:  adr x1,0x102c5c8
0102c5c0:  add x0,x1,w0, SXTB  #0x2
0102c5c4:  br x0
0102c5d8:  mov w0,#0x0
0102c5dc:  bl 0x0102a684
0102c5e0:  adrp x4,0x102e000
0102c5e4:  adrp x3,0x102e000
0102c5e8:  adrp x1,0x102e000
0102c5ec:  adrp x0,0x102e000
0102c5f0:  mov w5,#0x20b
0102c5f4:  add x4,x4,#0x480
0102c5f8:  add x3,x3,#0x4a0
0102c5fc:  mov w2,#0x1
0102c600:  add x1,x1,#0x536
0102c604:  add x0,x0,#0xc1
0102c608:  bl 0x0102a9c8
0102c60c:  mov w0,#0x0
0102c610:  bl 0x0102a6a0
0102c614:  mov x0,#0x0
0102c618:  bl 0x010294c8
0102c61c:  mov w0,#0x3
0102c620:  b 0x0102c628
0102c628:  ubfx x1,x20,#0x14,#0x2
0102c62c:  str w0,[x19, #0x28]
0102c630:  cmp w1,#0x1
0102c634:  ubfx x0,x20,#0x12,#0x1
0102c638:  str w0,[x19, #0x2c]
0102c63c:  mov w0,w1
0102c640:  b.eq 0x0102c6a0
0102c644:  mov w0,#0x0
0102c648:  cbz w1,0x0102c6a0
0102c64c:  cmp w1,#0x3
0102c650:  mov w0,#0x2
0102c654:  b.eq 0x0102c6a0
0102c658:  mov w0,#0x0
0102c65c:  bl 0x0102a684
0102c660:  adrp x4,0x102e000
0102c664:  adrp x3,0x102e000
0102c668:  adrp x1,0x102e000
0102c66c:  adrp x0,0x102e000
0102c670:  mov w5,#0x258
0102c674:  add x4,x4,#0x558
0102c678:  add x3,x3,#0x4a0
0102c67c:  mov w2,#0x1
0102c680:  add x1,x1,#0x536
0102c684:  add x0,x0,#0xc1
0102c688:  bl 0x0102a9c8
0102c68c:  mov w0,#0x0
0102c690:  bl 0x0102a6a0
0102c694:  mov x0,#0x0
0102c698:  bl 0x010294c8
0102c69c:  mov w0,#0x0
0102c6a0:  str w0,[x19, #0x30]
0102c6a4:  ubfx x0,x20,#0x16,#0x1
0102c6a8:  ubfx x20,x20,#0x17,#0x1
0102c6ac:  str w0,[x19, #0x34]
0102c6b0:  cmp w20,#0x1
0102c6b4:  mov w1,#0x50
0102c6b8:  mov w0,#0x57
0102c6bc:  str wzr,[x19, #0x3c]
0102c6c0:  csel w0,w0,w1,eq
0102c6c4:  strb w0,[x19, #0x38]
0102c6c8:  mov w0,#0x0
0102c6cc:  ldp x19,x20,[sp, #0x10]
0102c6d0:  ldp x29,x30,[sp], #0x20
0102c6d4:  ret

; ==== FUN_0102c6d8 @ 0102c6d8 ====
0102c6d8:  stp x29,x30,[sp, #-0x20]!
0102c6dc:  mov x29,sp
0102c6e0:  stp x19,x20,[sp, #0x10]
0102c6e4:  mov x20,x0
0102c6e8:  mov x19,x1
0102c6ec:  cbnz x1,0x0102c734
0102c6f0:  mov w0,#0x0
0102c6f4:  bl 0x0102a684
0102c6f8:  adrp x4,0x102e000
0102c6fc:  adrp x3,0x102e000
0102c700:  adrp x1,0x102e000
0102c704:  adrp x0,0x102e000
0102c708:  mov w5,#0x37
0102c70c:  add x4,x4,#0x588
0102c710:  add x3,x3,#0x5a0
0102c714:  mov w2,#0x1
0102c718:  add x1,x1,#0x611
0102c71c:  add x0,x0,#0xc1
0102c720:  bl 0x0102a9c8
0102c724:  mov w0,#0x0
0102c728:  bl 0x0102a6a0
0102c72c:  mov x0,#0x0
0102c730:  bl 0x010294c8
0102c734:  str x19,[x20]
0102c738:  ldp x19,x20,[sp, #0x10]
0102c73c:  ldp x29,x30,[sp], #0x20
0102c740:  ret

; ==== FUN_0102c744 @ 0102c744 ====
0102c744:  stp x29,x30,[sp, #-0x50]!
0102c748:  cmn w3,#0x1
0102c74c:  mov x29,sp
0102c750:  stp x19,x20,[sp, #0x10]
0102c754:  stp x23,x24,[sp, #0x30]
0102c758:  str x25,[sp, #0x40]
0102c75c:  mov x20,x0
0102c760:  stp x21,x22,[sp, #0x20]
0102c764:  mov w19,w1
0102c768:  mov x23,x2
0102c76c:  mov w25,w3
0102c770:  cset w24,eq
0102c774:  cbnz x0,0x0102c7bc
0102c778:  mov w0,#0x0
0102c77c:  bl 0x0102a684
0102c780:  adrp x4,0x102e000
0102c784:  adrp x3,0x102e000
0102c788:  adrp x1,0x102e000
0102c78c:  adrp x0,0x102e000
0102c790:  mov w5,#0x7e
0102c794:  add x4,x4,#0x660
0102c798:  add x3,x3,#0x5a0
0102c79c:  mov w2,#0x1
0102c7a0:  add x1,x1,#0x623
0102c7a4:  add x0,x0,#0xc1
0102c7a8:  bl 0x0102a9c8
0102c7ac:  mov w0,#0x0
0102c7b0:  bl 0x0102a6a0
0102c7b4:  mov x0,#0x0
0102c7b8:  bl 0x010294c8
0102c7bc:  cbnz x23,0x0102c804
0102c7c0:  mov w0,#0x0
0102c7c4:  bl 0x0102a684
0102c7c8:  adrp x4,0x102e000
0102c7cc:  adrp x3,0x102e000
0102c7d0:  adrp x1,0x102e000
0102c7d4:  adrp x0,0x102e000
0102c7d8:  mov w5,#0x7f
0102c7dc:  add x4,x4,#0x660
0102c7e0:  add x3,x3,#0x5a0
0102c7e4:  mov w2,#0x1
0102c7e8:  add x1,x1,#0x638
0102c7ec:  add x0,x0,#0xc1
0102c7f0:  bl 0x0102a9c8
0102c7f4:  mov w0,#0x0
0102c7f8:  bl 0x0102a6a0
0102c7fc:  mov x0,#0x0
0102c800:  bl 0x010294c8
0102c804:  cbnz w19,0x0102c84c
0102c808:  mov w0,#0x0
0102c80c:  bl 0x0102a684
0102c810:  adrp x4,0x102e000
0102c814:  adrp x3,0x102e000
0102c818:  adrp x1,0x102e000
0102c81c:  adrp x0,0x102e000
0102c820:  mov w5,#0x80
0102c824:  add x4,x4,#0x660
0102c828:  add x3,x3,#0x5a0
0102c82c:  mov w2,#0x1
0102c830:  add x1,x1,#0x642
0102c834:  add x0,x0,#0xc1
0102c838:  bl 0x0102a9c8
0102c83c:  mov w0,#0x0
0102c840:  bl 0x0102a6a0
0102c844:  mov x0,#0x0
0102c848:  bl 0x010294c8
0102c84c:  ldr x21,[x20]
0102c850:  mov w20,#0x0
0102c854:  ldr w22,[x21, #0x8]
0102c858:  and w22,w22,#0xc0
0102c85c:  cbz w19,0x0102c8f8
0102c860:  cbz w22,0x0102c870
0102c864:  ldr w0,[x21, #0x84]
0102c868:  cbz w0,0x0102c8ac
0102c86c:  b 0x0102c87c
0102c870:  ldr w0,[x21, #0x14]
0102c874:  tbz w0,#0x0,0x0102c8ac
0102c878:  mov w0,#0x1
0102c87c:  mov w1,w19
0102c880:  sub w0,w0,w19
0102c884:  cmn w0,w1
0102c888:  b.eq 0x0102c8a0
0102c88c:  cbz w1,0x0102c8a0
0102c890:  ldr w2,[x21]
0102c894:  sub w1,w1,#0x1
0102c898:  strb w2,[x23], #0x1
0102c89c:  b 0x0102c884
0102c8a0:  mov w19,w1
0102c8a4:  mov w20,#0x0
0102c8a8:  b 0x0102c8bc
0102c8ac:  cbnz w24,0x0102c8bc
0102c8b0:  mov w0,#0x1
0102c8b4:  add w20,w20,#0x1
0102c8b8:  bl 0x0102a54c
0102c8bc:  cmp w20,w25
0102c8c0:  cset w0,gt
0102c8c4:  cmp w0,w24
0102c8c8:  b.ls 0x0102c85c
0102c8cc:  mov w0,#0x0
0102c8d0:  bl 0x0102a684
0102c8d4:  adrp x1,0x102e000
0102c8d8:  adrp x0,0x102e000
0102c8dc:  add x1,x1,#0x660
0102c8e0:  add x0,x0,#0x64d
0102c8e4:  bl 0x0102a9c8
0102c8e8:  mov w0,#0x0
0102c8ec:  bl 0x0102a6a0
0102c8f0:  mov w0,#0xffffffc2
0102c8f4:  b 0x0102c8fc
0102c8f8:  mov w0,#0x0
0102c8fc:  ldp x19,x20,[sp, #0x10]
0102c900:  ldp x21,x22,[sp, #0x20]
0102c904:  ldp x23,x24,[sp, #0x30]
0102c908:  ldr x25,[sp, #0x40]
0102c90c:  ldp x29,x30,[sp], #0x50
0102c910:  ret

; ==== FUN_0102c914 @ 0102c914 ====
0102c914:  stp x29,x30,[sp, #-0x50]!
0102c918:  cmn w3,#0x1
0102c91c:  mov x29,sp
0102c920:  stp x19,x20,[sp, #0x10]
0102c924:  stp x23,x24,[sp, #0x30]
0102c928:  stp x21,x22,[sp, #0x20]
0102c92c:  stp x25,x26,[sp, #0x40]
0102c930:  mov x20,x0
0102c934:  mov w19,w1
0102c938:  mov x22,x2
0102c93c:  mov w24,w3
0102c940:  cset w23,eq
0102c944:  cbnz x0,0x0102c98c
0102c948:  mov w0,#0x0
0102c94c:  bl 0x0102a684
0102c950:  adrp x4,0x102e000
0102c954:  adrp x3,0x102e000
0102c958:  adrp x1,0x102e000
0102c95c:  adrp x0,0x102e000
0102c960:  mov w5,#0xb8
0102c964:  add x4,x4,#0x578
0102c968:  add x3,x3,#0x5a0
0102c96c:  mov w2,#0x1
0102c970:  add x1,x1,#0x623
0102c974:  add x0,x0,#0xc1
0102c978:  bl 0x0102a9c8
0102c97c:  mov w0,#0x0
0102c980:  bl 0x0102a6a0
0102c984:  mov x0,#0x0
0102c988:  bl 0x010294c8
0102c98c:  cbnz x22,0x0102c9d4
0102c990:  mov w0,#0x0
0102c994:  bl 0x0102a684
0102c998:  adrp x4,0x102e000
0102c99c:  adrp x3,0x102e000
0102c9a0:  adrp x1,0x102e000
0102c9a4:  adrp x0,0x102e000
0102c9a8:  mov w5,#0xb9
0102c9ac:  add x4,x4,#0x578
0102c9b0:  add x3,x3,#0x5a0
0102c9b4:  mov w2,#0x1
0102c9b8:  add x1,x1,#0x638
0102c9bc:  add x0,x0,#0xc1
0102c9c0:  bl 0x0102a9c8
0102c9c4:  mov w0,#0x0
0102c9c8:  bl 0x0102a6a0
0102c9cc:  mov x0,#0x0
0102c9d0:  bl 0x010294c8
0102c9d4:  cbnz w19,0x0102ca1c
0102c9d8:  mov w0,#0x0
0102c9dc:  bl 0x0102a684
0102c9e0:  adrp x4,0x102e000
0102c9e4:  adrp x3,0x102e000
0102c9e8:  adrp x1,0x102e000
0102c9ec:  adrp x0,0x102e000
0102c9f0:  mov w5,#0xba
0102c9f4:  add x4,x4,#0x578
0102c9f8:  add x3,x3,#0x5a0
0102c9fc:  mov w2,#0x1
0102ca00:  add x1,x1,#0x642
0102ca04:  add x0,x0,#0xc1
0102ca08:  bl 0x0102a9c8
0102ca0c:  mov w0,#0x0
0102ca10:  bl 0x0102a6a0
0102ca14:  mov x0,#0x0
0102ca18:  bl 0x010294c8
0102ca1c:  ldr x21,[x20]
0102ca20:  mov w26,#0x40
0102ca24:  mov w20,#0x0
0102ca28:  ldr w25,[x21, #0x8]
0102ca2c:  and w25,w25,#0xc0
0102ca30:  cbz w19,0x0102cad0
0102ca34:  cbz w25,0x0102ca48
0102ca38:  ldr w0,[x21, #0x80]
0102ca3c:  subs w0,w26,w0
0102ca40:  b.eq 0x0102ca84
0102ca44:  b 0x0102ca54
0102ca48:  ldr w0,[x21, #0x14]
0102ca4c:  tbz w0,#0x5,0x0102ca84
0102ca50:  mov w0,#0x1
0102ca54:  mov w1,w19
0102ca58:  sub w0,w0,w19
0102ca5c:  cmn w0,w1
0102ca60:  b.eq 0x0102ca78
0102ca64:  cbz w1,0x0102ca78
0102ca68:  ldrb w2,[x22], #0x1
0102ca6c:  sub w1,w1,#0x1
0102ca70:  str w2,[x21]
0102ca74:  b 0x0102ca5c
0102ca78:  mov w19,w1
0102ca7c:  mov w20,#0x0
0102ca80:  b 0x0102ca94
0102ca84:  cbnz w23,0x0102ca94
0102ca88:  mov w0,#0x1
0102ca8c:  add w20,w20,#0x1
0102ca90:  bl 0x0102a54c
0102ca94:  cmp w20,w24
0102ca98:  cset w0,gt
0102ca9c:  cmp w0,w23
0102caa0:  b.ls 0x0102ca30
0102caa4:  mov w0,#0x0
0102caa8:  bl 0x0102a684
0102caac:  adrp x1,0x102e000
0102cab0:  adrp x0,0x102e000
0102cab4:  add x1,x1,#0x578
0102cab8:  add x0,x0,#0x64d
0102cabc:  bl 0x0102a9c8
0102cac0:  mov w0,#0x0
0102cac4:  bl 0x0102a6a0
0102cac8:  mov w0,#0xffffffc2
0102cacc:  b 0x0102cad4
0102cad0:  mov w0,#0x0
0102cad4:  ldp x19,x20,[sp, #0x10]
0102cad8:  ldp x21,x22,[sp, #0x20]
0102cadc:  ldp x23,x24,[sp, #0x30]
0102cae0:  ldp x25,x26,[sp, #0x40]
0102cae4:  ldp x29,x30,[sp], #0x50
0102cae8:  ret

; ==== FUN_0102caec @ 0102caec ====
0102caec:  ldr x0,[x0]
0102caf0:  ldr w0,[x0, #0x14]
0102caf4:  and w0,w0,#0x1
0102caf8:  ret

; ==== FUN_0102cafc @ 0102cafc ====
0102cafc:  stp x29,x30,[sp, #-0x30]!
0102cb00:  mov x29,sp
0102cb04:  stp x19,x20,[sp, #0x10]
0102cb08:  stp x21,x22,[sp, #0x20]
0102cb0c:  mov x19,x0
0102cb10:  mov x20,x1
0102cb14:  uxtb w22,w2
0102cb18:  uxtb w21,w3
0102cb1c:  cbnz x0,0x0102cb64
0102cb20:  mov w0,#0x0
0102cb24:  bl 0x0102a684
0102cb28:  adrp x4,0x102e000
0102cb2c:  adrp x3,0x102e000
0102cb30:  adrp x1,0x102e000
0102cb34:  adrp x0,0x102e000
0102cb38:  mov w5,#0x62
0102cb3c:  add x4,x4,#0x7a0
0102cb40:  add x3,x3,#0x6b8
0102cb44:  mov w2,#0x1
0102cb48:  add x1,x1,#0x733
0102cb4c:  add x0,x0,#0xc1
0102cb50:  bl 0x0102a9c8
0102cb54:  mov w0,#0x0
0102cb58:  bl 0x0102a6a0
0102cb5c:  mov x0,#0x0
0102cb60:  bl 0x010294c8
0102cb64:  cbnz x20,0x0102cbac
0102cb68:  mov w0,#0x0
0102cb6c:  bl 0x0102a684
0102cb70:  adrp x4,0x102e000
0102cb74:  adrp x3,0x102e000
0102cb78:  adrp x1,0x102e000
0102cb7c:  adrp x0,0x102e000
0102cb80:  mov w5,#0x63
0102cb84:  add x4,x4,#0x7a0
0102cb88:  add x3,x3,#0x6b8
0102cb8c:  mov w2,#0x1
0102cb90:  add x1,x1,#0x74f
0102cb94:  add x0,x0,#0xc1
0102cb98:  bl 0x0102a9c8
0102cb9c:  mov w0,#0x0
0102cba0:  bl 0x0102a6a0
0102cba4:  mov x0,#0x0
0102cba8:  bl 0x010294c8
0102cbac:  strb w22,[x19, #0x8]
0102cbb0:  mov w0,#0x0
0102cbb4:  strb w21,[x19, #0x9]
0102cbb8:  str x20,[x19]
0102cbbc:  ldp x21,x22,[sp, #0x20]
0102cbc0:  ldp x19,x20,[sp, #0x10]
0102cbc4:  ldp x29,x30,[sp], #0x30
0102cbc8:  ret

; ==== FUN_0102cbcc @ 0102cbcc ====
0102cbcc:  stp x29,x30,[sp, #-0x30]!
0102cbd0:  mov x29,sp
0102cbd4:  stp x19,x20,[sp, #0x10]
0102cbd8:  stp x21,x22,[sp, #0x20]
0102cbdc:  mov w19,w2
0102cbe0:  mov x21,x0
0102cbe4:  mov w22,w1
0102cbe8:  mov w20,w3
0102cbec:  cbnz x0,0x0102cc34
0102cbf0:  mov w0,#0x0
0102cbf4:  bl 0x0102a684
0102cbf8:  adrp x4,0x102e000
0102cbfc:  adrp x3,0x102e000
0102cc00:  adrp x1,0x102e000
0102cc04:  adrp x0,0x102e000
0102cc08:  mov w5,#0x77
0102cc0c:  add x4,x4,#0x670
0102cc10:  add x3,x3,#0x6b8
0102cc14:  mov w2,#0x1
0102cc18:  add x1,x1,#0x733
0102cc1c:  add x0,x0,#0xc1
0102cc20:  bl 0x0102a9c8
0102cc24:  mov w0,#0x0
0102cc28:  bl 0x0102a6a0
0102cc2c:  mov x0,#0x0
0102cc30:  bl 0x010294c8
0102cc34:  ldr x2,[x21]
0102cc38:  cmp w19,#0x1
0102cc3c:  ldr w3,[x2, #0x8]
0102cc40:  b.eq 0x0102cc5c
0102cc44:  cbz w19,0x0102cc64
0102cc48:  cmp w19,#0x2
0102cc4c:  b.ne 0x0102cc70
0102cc50:  mov w0,#0x1
0102cc54:  mov w1,w0
0102cc58:  b 0x0102cc78
0102cc5c:  mov w0,#0x40
0102cc60:  b 0x0102cc68
0102cc64:  mov w0,#0x0
0102cc68:  mov w1,#0x41
0102cc6c:  b 0x0102cc78
0102cc70:  mov w0,#0x0
0102cc74:  mov w1,#0x0
0102cc78:  orr w4,w0,#0x2
0102cc7c:  cmp w22,#0x1
0102cc80:  csel w0,w4,w0,eq
0102cc84:  orr w1,w1,#0xe
0102cc88:  cbz w20,0x0102cc9c
0102cc8c:  orr w4,w0,#0x4
0102cc90:  cmp w20,#0x1
0102cc94:  orr w0,w0,#0x8
0102cc98:  csel w0,w0,w4,ne
0102cc9c:  bic w1,w3,w1
0102cca0:  orr w0,w0,w1
0102cca4:  str w0,[x2, #0x8]
0102cca8:  ldp x19,x20,[sp, #0x10]
0102ccac:  ldp x21,x22,[sp, #0x20]
0102ccb0:  ldp x29,x30,[sp], #0x30
0102ccb4:  ret

; ==== FUN_0102ccb8 @ 0102ccb8 ====
0102ccb8:  stp x29,x30,[sp, #-0x20]!
0102ccbc:  mov x29,sp
0102ccc0:  stp x19,x20,[sp, #0x10]
0102ccc4:  mov x19,x0
0102ccc8:  mov w20,w1
0102cccc:  cbnz x0,0x0102cd14
0102ccd0:  mov w0,#0x0
0102ccd4:  bl 0x0102a684
0102ccd8:  adrp x4,0x102e000
0102ccdc:  adrp x3,0x102e000
0102cce0:  adrp x1,0x102e000
0102cce4:  adrp x0,0x102e000
0102cce8:  mov w5,#0x8c
0102ccec:  add x4,x4,#0x6a0
0102ccf0:  add x3,x3,#0x6b8
0102ccf4:  mov w2,#0x1
0102ccf8:  add x1,x1,#0x733
0102ccfc:  add x0,x0,#0xc1
0102cd00:  bl 0x0102a9c8
0102cd04:  mov w0,#0x0
0102cd08:  bl 0x0102a6a0
0102cd0c:  mov x0,#0x0
0102cd10:  bl 0x010294c8
0102cd14:  ldr x0,[x19]
0102cd18:  str w20,[x0]
0102cd1c:  ldp x19,x20,[sp, #0x10]
0102cd20:  ldp x29,x30,[sp], #0x20
0102cd24:  ret

; ==== FUN_0102cd28 @ 0102cd28 ====
0102cd28:  stp x29,x30,[sp, #-0x20]!
0102cd2c:  mov x29,sp
0102cd30:  stp x19,x20,[sp, #0x10]
0102cd34:  mov x19,x0
0102cd38:  mov w20,w1
0102cd3c:  cbnz x0,0x0102cd84
0102cd40:  mov w0,#0x0
0102cd44:  bl 0x0102a684
0102cd48:  adrp x4,0x102e000
0102cd4c:  adrp x3,0x102e000
0102cd50:  adrp x1,0x102e000
0102cd54:  adrp x0,0x102e000
0102cd58:  mov w5,#0xa9
0102cd5c:  add x4,x4,#0x688
0102cd60:  add x3,x3,#0x6b8
0102cd64:  mov w2,#0x1
0102cd68:  add x1,x1,#0x733
0102cd6c:  add x0,x0,#0xc1
0102cd70:  bl 0x0102a9c8
0102cd74:  mov w0,#0x0
0102cd78:  bl 0x0102a6a0
0102cd7c:  mov x0,#0x0
0102cd80:  bl 0x010294c8
0102cd84:  ldr x2,[x19]
0102cd88:  cmp w20,#0x1
0102cd8c:  mov w1,#0x20
0102cd90:  csel w1,w1,wzr,eq
0102cd94:  ldr w0,[x2, #0x8]
0102cd98:  ldp x19,x20,[sp, #0x10]
0102cd9c:  and w0,w0,#0xffffffdf
0102cda0:  orr w0,w1,w0
0102cda4:  str w0,[x2, #0x8]
0102cda8:  ldp x29,x30,[sp], #0x20
0102cdac:  ret

; ==== FUN_0102cdb0 @ 0102cdb0 ====
0102cdb0:  stp x29,x30,[sp, #-0x20]!
0102cdb4:  mov x29,sp
0102cdb8:  stp x19,x20,[sp, #0x10]
0102cdbc:  mov x19,x0
0102cdc0:  mov w20,w1
0102cdc4:  cbnz x0,0x0102ce0c
0102cdc8:  mov w0,#0x0
0102cdcc:  bl 0x0102a684
0102cdd0:  adrp x4,0x102e000
0102cdd4:  adrp x3,0x102e000
0102cdd8:  adrp x1,0x102e000
0102cddc:  adrp x0,0x102e000
0102cde0:  mov w5,#0xbf
0102cde4:  add x4,x4,#0x778
0102cde8:  add x3,x3,#0x6b8
0102cdec:  mov w2,#0x1
0102cdf0:  add x1,x1,#0x733
0102cdf4:  add x0,x0,#0xc1
0102cdf8:  bl 0x0102a9c8
0102cdfc:  mov w0,#0x0
0102ce00:  bl 0x0102a6a0
0102ce04:  mov x0,#0x0
0102ce08:  bl 0x010294c8
0102ce0c:  ldr x2,[x19]
0102ce10:  cmp w20,#0x1
0102ce14:  mov w1,#0x80
0102ce18:  csel w1,w1,wzr,eq
0102ce1c:  ldr w0,[x2, #0x8]
0102ce20:  ldp x19,x20,[sp, #0x10]
0102ce24:  and w0,w0,#0xffffff7f
0102ce28:  orr w0,w1,w0
0102ce2c:  str w0,[x2, #0x8]
0102ce30:  ldp x29,x30,[sp], #0x20
0102ce34:  ret

; ==== FUN_0102ce38 @ 0102ce38 ====
0102ce38:  stp x29,x30,[sp, #-0x20]!
0102ce3c:  mov x29,sp
0102ce40:  str x19,[sp, #0x10]
0102ce44:  mov x19,x0
0102ce48:  cbnz x0,0x0102ce90
0102ce4c:  mov w0,#0x0
0102ce50:  bl 0x0102a684
0102ce54:  adrp x4,0x102e000
0102ce58:  adrp x3,0x102e000
0102ce5c:  adrp x1,0x102e000
0102ce60:  adrp x0,0x102e000
0102ce64:  mov w5,#0xf8
0102ce68:  add x4,x4,#0x788
0102ce6c:  add x3,x3,#0x6b8
0102ce70:  mov w2,#0x1
0102ce74:  add x1,x1,#0x733
0102ce78:  add x0,x0,#0xc1
0102ce7c:  bl 0x0102a9c8
0102ce80:  mov w0,#0x0
0102ce84:  bl 0x0102a6a0
0102ce88:  mov x0,#0x0
0102ce8c:  bl 0x010294c8
0102ce90:  ldr x0,[x19]
0102ce94:  ldr x19,[sp, #0x10]
0102ce98:  ldp x29,x30,[sp], #0x20
0102ce9c:  ldr w0,[x0, #0x4]
0102cea0:  ret

; ==== FUN_0102cea4 @ 0102cea4 ====
0102cea4:  stp x29,x30,[sp, #-0x20]!
0102cea8:  mov x29,sp
0102ceac:  stp x19,x20,[sp, #0x10]
0102ceb0:  mov x19,x0
0102ceb4:  mov x20,x1
0102ceb8:  cbnz x0,0x0102cf00
0102cebc:  mov w0,#0x0
0102cec0:  bl 0x0102a684
0102cec4:  adrp x4,0x102e000
0102cec8:  adrp x3,0x102e000
0102cecc:  adrp x1,0x102e000
0102ced0:  adrp x0,0x102e000
0102ced4:  mov w5,#0x10
0102ced8:  add x4,x4,#0x9f0
0102cedc:  add x3,x3,#0x848
0102cee0:  mov w2,#0x1
0102cee4:  add x1,x1,#0x8cc
0102cee8:  add x0,x0,#0xc1
0102ceec:  bl 0x0102a9c8
0102cef0:  mov w0,#0x0
0102cef4:  bl 0x0102a6a0
0102cef8:  mov x0,#0x0
0102cefc:  bl 0x010294c8
0102cf00:  add x0,x20,#0x4, LSL #12
0102cf04:  ldr w0,[x0, #0x400]
0102cf08:  str x20,[x19, #0x8]
0102cf0c:  ubfx x0,x0,#0x8,#0x8
0102cf10:  str w0,[x19]
0102cf14:  ldp x19,x20,[sp, #0x10]
0102cf18:  ldp x29,x30,[sp], #0x20
0102cf1c:  ret

; ==== FUN_0102cf20 @ 0102cf20 ====
0102cf20:  stp x29,x30,[sp, #-0x30]!
0102cf24:  mov x29,sp
0102cf28:  stp x19,x20,[sp, #0x10]
0102cf2c:  str x21,[sp, #0x20]
0102cf30:  mov x19,x0
0102cf34:  mov x21,x1
0102cf38:  mov x20,x2
0102cf3c:  cbnz x0,0x0102cf84
0102cf40:  mov w0,#0x0
0102cf44:  bl 0x0102a684
0102cf48:  adrp x4,0x102e000
0102cf4c:  adrp x3,0x102e000
0102cf50:  adrp x1,0x102e000
0102cf54:  adrp x0,0x102e000
0102cf58:  mov w5,#0x20
0102cf5c:  add x4,x4,#0x7d8
0102cf60:  add x3,x3,#0x848
0102cf64:  mov w2,#0x1
0102cf68:  add x1,x1,#0x8cc
0102cf6c:  add x0,x0,#0xc1
0102cf70:  bl 0x0102a9c8
0102cf74:  mov w0,#0x0
0102cf78:  bl 0x0102a6a0
0102cf7c:  mov x0,#0x0
0102cf80:  bl 0x010294c8
0102cf84:  cbnz x21,0x0102cfcc
0102cf88:  mov w0,#0x0
0102cf8c:  bl 0x0102a684
0102cf90:  adrp x4,0x102e000
0102cf94:  adrp x3,0x102e000
0102cf98:  adrp x1,0x102e000
0102cf9c:  adrp x0,0x102e000
0102cfa0:  mov w5,#0x21
0102cfa4:  add x4,x4,#0x7d8
0102cfa8:  add x3,x3,#0x848
0102cfac:  mov w2,#0x1
0102cfb0:  add x1,x1,#0x8d6
0102cfb4:  add x0,x0,#0xc1
0102cfb8:  bl 0x0102a9c8
0102cfbc:  mov w0,#0x0
0102cfc0:  bl 0x0102a6a0
0102cfc4:  mov x0,#0x0
0102cfc8:  bl 0x010294c8
0102cfcc:  ldr w0,[x21]
0102cfd0:  cmp w0,#0x3
0102cfd4:  b.ls 0x0102cfe0
0102cfd8:  cbnz x20,0x0102d028
0102cfdc:  b 0x0102cfe4
0102cfe0:  cbz x20,0x0102d028
0102cfe4:  mov w0,#0x0
0102cfe8:  bl 0x0102a684
0102cfec:  adrp x4,0x102e000
0102cff0:  adrp x3,0x102e000
0102cff4:  adrp x1,0x102e000
0102cff8:  adrp x0,0x102e000
0102cffc:  mov w5,#0x26
0102d000:  add x4,x4,#0x7d8
0102d004:  add x3,x3,#0x848
0102d008:  mov w2,#0x1
0102d00c:  add x1,x1,#0x8e7
0102d010:  add x0,x0,#0xc1
0102d014:  bl 0x0102a9c8
0102d018:  mov w0,#0x0
0102d01c:  bl 0x0102a6a0
0102d020:  mov x0,#0x0
0102d024:  bl 0x010294c8
0102d028:  str x21,[x19]
0102d02c:  str x20,[x19, #0x8]
0102d030:  ldr x21,[sp, #0x20]
0102d034:  ldp x19,x20,[sp, #0x10]
0102d038:  ldp x29,x30,[sp], #0x30
0102d03c:  ret

; ==== FUN_0102d040 @ 0102d040 ====
0102d040:  ldr x0,[x0, #0x8]
0102d044:  ldr w0,[x0, #0x1004]
0102d048:  ret

; ==== FUN_0102d04c @ 0102d04c ====
0102d04c:  stp x29,x30,[sp, #-0x30]!
0102d050:  mov x29,sp
0102d054:  stp x19,x20,[sp, #0x10]
0102d058:  str x21,[sp, #0x20]
0102d05c:  mov x19,x0
0102d060:  mov w21,w1
0102d064:  mov w20,w2
0102d068:  cbnz x0,0x0102d0b0
0102d06c:  mov w0,#0x0
0102d070:  bl 0x0102a684
0102d074:  adrp x4,0x102f000
0102d078:  adrp x3,0x102f000
0102d07c:  adrp x1,0x102f000
0102d080:  adrp x0,0x102f000
0102d084:  mov w5,#0xf3
0102d088:  add x4,x4,#0x800
0102d08c:  add x3,x3,#0x848
0102d090:  mov w2,#0x1
0102d094:  add x1,x1,#0x8cc
0102d098:  add x0,x0,#0xc1
0102d09c:  bl 0x0102a9c8
0102d0a0:  mov w0,#0x0
0102d0a4:  bl 0x0102a6a0
0102d0a8:  mov x0,#0x0
0102d0ac:  bl 0x010294c8
0102d0b0:  ldr x0,[x19]
0102d0b4:  ldr w1,[x0]
0102d0b8:  cmp w1,#0x3
0102d0bc:  b.hi 0x0102d0dc
0102d0c0:  ldr x0,[x0, #0x8]
0102d0c4:  str w21,[x0, #0x100]
0102d0c8:  str w20,[x0, #0x104]
0102d0cc:  ldr x21,[sp, #0x20]
0102d0d0:  ldp x19,x20,[sp, #0x10]
0102d0d4:  ldp x29,x30,[sp], #0x30
0102d0d8:  ret
0102d0dc:  mov w0,#0x0
0102d0e0:  bl 0x0102a684
0102d0e4:  ldr x0,[x19]
0102d0e8:  adrp x19,0x102f000
0102d0ec:  add x19,x19,#0x800
0102d0f0:  mov x1,x19
0102d0f4:  ldr w2,[x0]
0102d0f8:  adrp x0,0x102f000
0102d0fc:  add x0,x0,#0x965
0102d100:  bl 0x0102a9c8
0102d104:  mov w0,#0x0
0102d108:  bl 0x0102a6a0
0102d10c:  mov w0,#0x0
0102d110:  bl 0x0102a684
0102d114:  adrp x3,0x102f000
0102d118:  adrp x1,0x102f000
0102d11c:  adrp x0,0x102f000
0102d120:  mov x4,x19
0102d124:  mov w5,#0xfd
0102d128:  add x3,x3,#0x848
0102d12c:  mov w2,#0x1
0102d130:  add x1,x1,#0x536
0102d134:  add x0,x0,#0xc1
0102d138:  bl 0x0102a9c8
0102d13c:  mov w0,#0x0
0102d140:  bl 0x0102a6a0
0102d144:  ldr x21,[sp, #0x20]
0102d148:  ldp x19,x20,[sp, #0x10]
0102d14c:  ldp x29,x30,[sp], #0x30
0102d150:  mov x0,#0x0
0102d154:  b 0x010294c8

; ==== FUN_0102d158 @ 0102d158 ====
0102d158:  stp x29,x30,[sp, #-0x30]!
0102d15c:  mov x29,sp
0102d160:  stp x19,x20,[sp, #0x10]
0102d164:  str x21,[sp, #0x20]
0102d168:  mov x20,x0
0102d16c:  mov w19,w1
0102d170:  mov w21,w2
0102d174:  cbnz x0,0x0102d1bc
0102d178:  mov w0,#0x0
0102d17c:  bl 0x0102a684
0102d180:  adrp x4,0x102e000
0102d184:  adrp x3,0x102e000
0102d188:  adrp x1,0x102e000
0102d18c:  adrp x0,0x102e000
0102d190:  mov w5,#0x11b
0102d194:  add x4,x4,#0x828
0102d198:  add x3,x3,#0x848
0102d19c:  mov w2,#0x1
0102d1a0:  add x1,x1,#0x8cc
0102d1a4:  add x0,x0,#0xc1
0102d1a8:  bl 0x0102a9c8
0102d1ac:  mov w0,#0x0
0102d1b0:  bl 0x0102a6a0
0102d1b4:  mov x0,#0x0
0102d1b8:  bl 0x010294c8
0102d1bc:  cmp w19,#0x3
0102d1c0:  b.ls 0x0102d208
0102d1c4:  mov w0,#0x0
0102d1c8:  bl 0x0102a684
0102d1cc:  adrp x4,0x102e000
0102d1d0:  adrp x3,0x102e000
0102d1d4:  adrp x1,0x102e000
0102d1d8:  adrp x0,0x102e000
0102d1dc:  mov w5,#0x11c
0102d1e0:  add x4,x4,#0x828
0102d1e4:  add x3,x3,#0x848
0102d1e8:  mov w2,#0x1
0102d1ec:  add x1,x1,#0x959
0102d1f0:  add x0,x0,#0xc1
0102d1f4:  bl 0x0102a9c8
0102d1f8:  mov w0,#0x0
0102d1fc:  bl 0x0102a6a0
0102d200:  mov x0,#0x0
0102d204:  bl 0x010294c8
0102d208:  ldr x0,[x20]
0102d20c:  ldr w1,[x0]
0102d210:  cmp w1,#0x3
0102d214:  b.hi 0x0102d238
0102d218:  ldr x0,[x0, #0x8]
0102d21c:  ubfiz x19,x19,#0x8,#0x20
0102d220:  add x19,x0,x19
0102d224:  str w21,[x19, #0x2020]
0102d228:  ldp x19,x20,[sp, #0x10]
0102d22c:  ldr x21,[sp, #0x20]
0102d230:  ldp x29,x30,[sp], #0x30
0102d234:  ret
0102d238:  mov w0,#0x0
0102d23c:  adrp x19,0x102e000
0102d240:  add x19,x19,#0x828
0102d244:  bl 0x0102a684
0102d248:  ldr x0,[x20]
0102d24c:  mov x1,x19
0102d250:  ldr w2,[x0]
0102d254:  adrp x0,0x102e000
0102d258:  add x0,x0,#0x965
0102d25c:  bl 0x0102a9c8
0102d260:  mov w0,#0x0
0102d264:  bl 0x0102a6a0
0102d268:  mov w0,#0x0
0102d26c:  bl 0x0102a684
0102d270:  adrp x3,0x102e000
0102d274:  adrp x1,0x102e000
0102d278:  adrp x0,0x102e000
0102d27c:  mov x4,x19
0102d280:  mov w5,#0x125
0102d284:  add x3,x3,#0x848
0102d288:  mov w2,#0x1
0102d28c:  add x1,x1,#0x536
0102d290:  add x0,x0,#0xc1
0102d294:  bl 0x0102a9c8
0102d298:  mov w0,#0x0
0102d29c:  bl 0x0102a6a0
0102d2a0:  ldr x21,[sp, #0x20]
0102d2a4:  ldp x19,x20,[sp, #0x10]
0102d2a8:  ldp x29,x30,[sp], #0x30
0102d2ac:  mov x0,#0x0
0102d2b0:  b 0x010294c8

; ==== FUN_0102d5c0 @ 0102d5c0 ====
0102d5c0:  mov x2,#0x9004
0102d5c4:  mov x1,#0x4
0102d5c8:  cmp w0,wzr
0102d5cc:  movk x2,#0xf000, LSL #16
0102d5d0:  movk x1,#0xf020, LSL #16
0102d5d4:  mov w3,#0x20
0102d5d8:  csel x1,x1,x2,eq
0102d5dc:  mov x4,#0x200000
0102d5e0:  mov x5,#0x9000
0102d5e4:  mov x6,#0xf0000000
0102d5e8:  mov w7,#0xffffffff
0102d5ec:  ldr w1,[x1]
0102d5f0:  and w1,w1,#0x1f
0102d5f4:  add w1,w1,#0x1
0102d5f8:  lsl w1,w1,#0x5
0102d5fc:  cmp w3,w1
0102d600:  b.ge 0x0102d624
0102d604:  asr w2,w3,#0x3
0102d608:  cmp w0,wzr
0102d60c:  add w2,w2,#0x80
0102d610:  csel x8,x4,x5,eq
0102d614:  add w3,w3,#0x20
0102d618:  add x2,x6,w2, SXTW 
0102d61c:  str w7,[x2, x8, LSL #0x0]
0102d620:  b 0x0102d5fc
0102d624:  mov x2,#0x420
0102d628:  orr w4,wzr,#0xffffc00
0102d62c:  movk x2,#0xf000, LSL #16
0102d630:  mov x5,#0x200000
0102d634:  mov x6,#0x9000
0102d638:  orr w7,wzr,#0x80808080
0102d63c:  add w3,w2,w4
0102d640:  cmp w3,w1
0102d644:  b.ge 0x0102d65c
0102d648:  cmp w0,wzr
0102d64c:  csel x3,x5,x6,eq
0102d650:  str w7,[x3, x2, LSL #0x0]
0102d654:  add x2,x2,#0x4
0102d658:  b 0x0102d63c
0102d65c:  mov x1,#0x9000
0102d660:  cmp w0,wzr
0102d664:  movk x1,#0xf000, LSL #16
0102d668:  mov x0,#0xf0200000
0102d66c:  csel x0,x0,x1,eq
0102d670:  mov w1,#0x11
0102d674:  ldr w2,[x0]
0102d678:  orr w1,w2,w1
0102d67c:  str w1,[x0]
0102d680:  ret

; ==== FUN_0102d684 @ 0102d684 ====
0102d684:  stp x29,x30,[sp, #-0x10]!
0102d688:  mov x29,sp
0102d68c:  bl 0x01029248
0102d690:  add w0,w0,#0x14
0102d694:  lsl w0,w0,#0x11
0102d698:  mov x1,#0x14
0102d69c:  movk x1,#0xf000, LSL #16
0102d6a0:  mov w3,#0x2
0102d6a4:  sxtw x0,w0
0102d6a8:  add x2,x0,x1
0102d6ac:  str w3,[x0, x1, LSL #0x0]
0102d6b0:  ldr w0,[x2]
0102d6b4:  tbz w0,#0x2,0x0102d6b0
0102d6b8:  ldp x29,x30,[sp], #0x10
0102d6bc:  ret

; ==== FUN_0102d6c0 @ 0102d6c0 ====
0102d6c0:  stp x29,x30,[sp, #-0x20]!
0102d6c4:  add x0,x0,#0x90, LSL #12
0102d6c8:  mov x29,sp
0102d6cc:  str x19,[sp, #0x10]
0102d6d0:  mov w19,#0x1
0102d6d4:  str w19,[x0, #0x8]
0102d6d8:  mov w0,#0x0
0102d6dc:  bl 0x0102d5c0
0102d6e0:  mov w0,w19
0102d6e4:  ldr x19,[sp, #0x10]
0102d6e8:  ldp x29,x30,[sp], #0x20
0102d6ec:  b 0x0102d5c0

; ==== FUN_0102d6f0 @ 0102d6f0 ====
0102d6f0:  ret

; ==== FUN_0102d6f4 @ 0102d6f4 ====
0102d6f4:  stp x29,x30,[sp, #-0x10]!
0102d6f8:  mov x29,sp
0102d6fc:  bl 0x01029248
0102d700:  add w0,w0,#0x14
0102d704:  lsl w0,w0,#0x11
0102d708:  mov x1,#0xf0010000
0102d70c:  mov w2,#0xffffffff
0102d710:  orr w3,wzr,#0x80808080
0102d714:  sxtw x0,w0
0102d718:  add x1,x0,x1
0102d71c:  str w2,[x1, #0x80]
0102d720:  mov x2,#0x400
0102d724:  mov x1,#0x420
0102d728:  movk x2,#0xf001, LSL #16
0102d72c:  movk x1,#0xf001, LSL #16
0102d730:  add x2,x0,x2
0102d734:  add x0,x0,x1
0102d738:  str w3,[x2], #0x4
0102d73c:  orr w1,wzr,#0x80808080
0102d740:  cmp x2,x0
0102d744:  b.ne 0x0102d738
0102d748:  mov x0,#0x4
0102d74c:  mov x5,#0xf0100000
0102d750:  movk x0,#0xf010, LSL #16
0102d754:  mov w4,#0xff
0102d758:  str w4,[x0]
0102d75c:  mov w0,#0xfffffd82
0102d760:  ldr w2,[x5]
0102d764:  and w3,w2,w0
0102d768:  mov w2,#0x79
0102d76c:  orr w2,w3,w2
0102d770:  str w2,[x5]
0102d774:  mov x2,#0x9000
0102d778:  mov w3,#0xffffffff
0102d77c:  movk x2,#0xf000, LSL #16
0102d780:  str w3,[x2, #0x80]
0102d784:  str w1,[x2, #0x400]
0102d788:  str w1,[x2, #0x404]
0102d78c:  str w1,[x2, #0x408]
0102d790:  str w1,[x2, #0x40c]
0102d794:  str w1,[x2, #0x410]
0102d798:  str w1,[x2, #0x414]
0102d79c:  str w1,[x2, #0x418]
0102d7a0:  str w1,[x2, #0x41c]
0102d7a4:  mov x1,#0xa004
0102d7a8:  movk x1,#0xf000, LSL #16
0102d7ac:  str w4,[x1]
0102d7b0:  mov x1,#0xa000
0102d7b4:  movk x1,#0xf000, LSL #16
0102d7b8:  ldr w2,[x1]
0102d7bc:  and w0,w2,w0
0102d7c0:  orr w0,w0,#0x78
0102d7c4:  str w0,[x1]
0102d7c8:  ldp x29,x30,[sp], #0x10
0102d7cc:  ret

; ==== FUN_0102d7d0 @ 0102d7d0 ====
0102d7d0:  stp x29,x30,[sp, #-0x10]!
0102d7d4:  mov x29,sp
0102d7d8:  bl 0x01029248
0102d7dc:  add w0,w0,#0x14
0102d7e0:  lsl w0,w0,#0x11
0102d7e4:  mov x1,#0xf0000000
0102d7e8:  add x0,x1,w0, SXTW 
0102d7ec:  str wzr,[x0, #0x14]
0102d7f0:  ldr w1,[x0, #0x14]
0102d7f4:  tbnz w1,#0x2,0x0102d7f0
0102d7f8:  mov x0,#0x4
0102d7fc:  mov w1,#0xff
0102d800:  movk x0,#0xf010, LSL #16
0102d804:  str w1,[x0]
0102d808:  ldp x29,x30,[sp], #0x10
0102d80c:  ret

; ==== FUN_0102d810 @ 0102d810 ====
0102d810:  ldr x2,0x102d870
0102d814:  mov w3,#0x1
0102d818:  str w3,[x2]
0102d81c:  ldr x2,0x102d878
0102d820:  ldr w3,[x2]
0102d824:  cmp w3,#0x2
0102d828:  b.ne 0x0102d820
0102d82c:  mrs x2,sreg(0x3, 0x1, c0xf, c0x0, 0x0)
0102d830:  orr x2,x2,#0x8
0102d834:  and x2,x2,#-0x11
0102d838:  orr x2,x2,#0x100
0102d83c:  msr sreg(0x3, 0x1, c0xf, c0x0, 0x0),x2
0102d840:  mrs x2,sreg(0x3, 0x1, c0xb, c0x0, 0x2)
0102d844:  and x2,x2,#-0x8
0102d848:  orr x2,x2,#0x2
0102d84c:  and x2,x2,#-0x1c1
0102d850:  orr x2,x2,#0x80
0102d854:  cmp x0,#0x0
0102d858:  b.eq 0x0102d860
0102d85c:  orr x2,x2,#0x200000
0102d860:  msr sreg(0x3, 0x1, c0xb, c0x0, 0x2),x2
0102d864:  isb 
0102d868:  ret
