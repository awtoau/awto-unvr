; UNVR stage3 (multi_dt) sysid->DTB-instance selection - NEW preboot (5.1.25)
; carved from tmp/sections/01-uboot.bin preboot payload (container[0x21000:]), ARM32 @0x01000000
; boardid read from SPI-NOR EEPROM partition: flash off 0x1F000C (2B, BE), devid 0x1F0010 (4B)
; dispatch: idx = boardid-0xea16; range<=N; ldrls pc,[pc,idx,lsl#2]; slot targets 'b common', mov r2,#instance at target+4
;
 100167c:	e3097308 	movw	r7, #37640	@ 0x9308
 1001680:	e3a0000c 	mov	r0, #12
 1001684:	e5953000 	ldr	r3, [r5]
 1001688:	e3407104 	movt	r7, #260	@ 0x104
 100168c:	e340001f 	movt	r0, #31
 1001690:	e2871002 	add	r1, r7, #2
 1001694:	e3a02002 	mov	r2, #2
 1001698:	e12fff33 	blx	r3
 100169c:	e2506000 	subs	r6, r0, #0
 10016a0:	1a0001e2 	bne	0x1001e30
 10016a4:	e1d720b2 	ldrh	r2, [r7, #2]
 10016a8:	e30008cc 	movw	r0, #2252	@ 0x8cc
 10016ac:	e3400103 	movt	r0, #259	@ 0x103
 10016b0:	e1a03422 	lsr	r3, r2, #8
 10016b4:	e1833402 	orr	r3, r3, r2, lsl #8
 10016b8:	e6ff3073 	uxth	r3, r3
 10016bc:	e1a01003 	mov	r1, r3
 10016c0:	e1c730b2 	strh	r3, [r7, #2]
 10016c4:	eb003d4d 	bl	0x1010c00
 10016c8:	e3a02004 	mov	r2, #4
 10016cc:	e3a00010 	mov	r0, #16
 10016d0:	e5953000 	ldr	r3, [r5]
 10016d4:	e340001f 	movt	r0, #31
 10016d8:	e0871002 	add	r1, r7, r2
 10016dc:	e12fff33 	blx	r3
 10016e0:	e2506000 	subs	r6, r0, #0
 10016e4:	1a0002cd 	bne	0x1002220
 10016e8:	e5973004 	ldr	r3, [r7, #4]
 10016ec:	e300090c 	movw	r0, #2316	@ 0x90c
 10016f0:	e3400103 	movt	r0, #259	@ 0x103
 10016f4:	e6bf3f33 	rev	r3, r3
 10016f8:	e1a01003 	mov	r1, r3
 10016fc:	e5c73001 	strb	r3, [r7, #1]
 1001700:	e5873004 	str	r3, [r7, #4]
 1001704:	eb003d3d 	bl	0x1010c00
 1001708:	e1d730b2 	ldrh	r3, [r7, #2]
 100170c:	e2433cea 	sub	r3, r3, #59904	@ 0xea00
 1001710:	e2433016 	sub	r3, r3, #22
 1001714:	e353001a 	cmp	r3, #26
 1001718:	979ff103 	ldrls	pc, [pc, r3, lsl #2]
 100171c:	ea000322 	b	0x10023ac
 1001720:	01002398 			@ <UNDEFINED> instruction: 0x01002398
 1001724:	010023a8 	smlatbeq	r0, r8, r3, r2
 1001728:	010023a8 	smlatbeq	r0, r8, r3, r2
 100172c:	010023a8 	smlatbeq	r0, r8, r3, r2
 1001730:	01002388 	smlabbeq	r0, r8, r3, r2
 1001734:	010023a8 	smlatbeq	r0, r8, r3, r2
 1001738:	010023a8 	smlatbeq	r0, r8, r3, r2
 100173c:	010023a8 	smlatbeq	r0, r8, r3, r2
 1001740:	010023a8 	smlatbeq	r0, r8, r3, r2
 1001744:	010023a8 	smlatbeq	r0, r8, r3, r2
 1001748:	01002378 	tsteq	r0, r8, ror r3
 100174c:	01002368 	tsteq	r0, r8, ror #6
 1001750:	010023a8 	smlatbeq	r0, r8, r3, r2
 1001754:	010023a8 	smlatbeq	r0, r8, r3, r2
 1001758:	010023a8 	smlatbeq	r0, r8, r3, r2
 100175c:	010023a8 	smlatbeq	r0, r8, r3, r2
 1001760:	010023a8 	smlatbeq	r0, r8, r3, r2
 1001764:	010023a8 	smlatbeq	r0, r8, r3, r2
 1001768:	010023a8 	smlatbeq	r0, r8, r3, r2
 100176c:	010023a8 	smlatbeq	r0, r8, r3, r2
 1001770:	010023a8 	smlatbeq	r0, r8, r3, r2
 1001774:	010023a8 	smlatbeq	r0, r8, r3, r2
 1001778:	010023a8 	smlatbeq	r0, r8, r3, r2
 100177c:	010023a8 	smlatbeq	r0, r8, r3, r2
 1001780:	010023a8 	smlatbeq	r0, r8, r3, r2
 1001784:	010023a8 	smlatbeq	r0, r8, r3, r2
 1001788:	01002358 	tsteq	r0, r8, asr r3
 100178c:	e3095d8c 	movw	r5, #40332	@ 0x9d8c
 1001790:	e300380c 	movw	r3, #2060	@ 0x80c
 1001794:	e3405104 	movt	r5, #260	@ 0x104
 1001798:	e3403100 	movt	r3, #256	@ 0x100
 100179c:	e3a02001 	mov	r2, #1
 10017a0:	e5853000 	str	r3, [r5]
 10017a4:	e5842780 	str	r2, [r4, #1920]	@ 0x780
 10017a8:	eaffff94 	b	0x1001600
 10017ac:	e3095d8c 	movw	r5, #40332	@ 0x9d8c
 10017b0:	e3003768 	movw	r3, #1896	@ 0x768
 10017b4:	e3405104 	movt	r5, #260	@ 0x104
 10017b8:	e3403100 	movt	r3, #256	@ 0x100
 10017bc:	e5853000 	str	r3, [r5]
 10017c0:	eaffff8e 	b	0x1001600
 10017c4:	e3a03cca 	mov	r3, #51712	@ 0xca00
 10017c8:	e3433b9a 	movt	r3, #15258	@ 0x3b9a
 10017cc:	e584381c 	str	r3, [r4, #2076]	@ 0x81c
 10017d0:	eafffeed 	b	0x100138c
 10017d4:	e3a03c4e 	mov	r3, #19968	@ 0x4e00
 10017d8:	e3453372 	movt	r3, #21362	@ 0x5372
 10017dc:	e584381c 	str	r3, [r4, #2076]	@ 0x81c
 10017e0:	eafffee9 	b	0x100138c
 10017e4:	e3a03cf1 	mov	r3, #61696	@ 0xf100
 10017e8:	e3463553 	movt	r3, #25939	@ 0x6553
 10017ec:	e584381c 	str	r3, [r4, #2076]	@ 0x81c
 10017f0:	eafffee5 	b	0x100138c
 10017f4:	e3a03b25 	mov	r3, #37888	@ 0x9400
 10017f8:	e3473735 	movt	r3, #30517	@ 0x7735
 10017fc:	e584381c 	str	r3, [r4, #2076]	@ 0x81c
; --- case handlers (instance set in r2/[r7]) ---
 1002350:	e3408104 	movt	r8, #260	@ 0x104
 1002354:	eb003a29 	bl	0x1010c00
 1002358:	eafffefe 	b	0x1001f58
 100235c:	e3a02004 	mov	r2, #4
 1002360:	e1a06002 	mov	r6, r2
 1002364:	e5c72000 	strb	r2, [r7]
 1002368:	eafffeb5 	b	0x1001e44
 100236c:	e3a02002 	mov	r2, #2
 1002370:	e1a06002 	mov	r6, r2
 1002374:	e5c72000 	strb	r2, [r7]
 1002378:	eafffeb1 	b	0x1001e44
 100237c:	e3a02001 	mov	r2, #1
 1002380:	e1a06002 	mov	r6, r2
 1002384:	e5c72000 	strb	r2, [r7]
 1002388:	eafffead 	b	0x1001e44
 100238c:	e3a02003 	mov	r2, #3
 1002390:	e1a06002 	mov	r6, r2
 1002394:	e5c72000 	strb	r2, [r7]
 1002398:	eafffea9 	b	0x1001e44
 100239c:	e3a02000 	mov	r2, #0
 10023a0:	e1a06002 	mov	r6, r2
 10023a4:	e5c72000 	strb	r2, [r7]
 10023a8:	eafffea5 	b	0x1001e44
 10023ac:	e3e02000 	mvn	r2, #0
