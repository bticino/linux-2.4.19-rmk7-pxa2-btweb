/*
 *  linux/arch/arm/lib/getuser.S
 *
 *  Copyright (C) 2001 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Idea from x86 version, (C) Copyright 1998 Linus Torvalds
 *
 * These functions have a non-standard call interface to make them more
 * efficient, especially as they return an error value in addition to
 * the "real" return value.
 *
 * __get_user_X
 *
 * Inputs:	r0 contains the address
 * Outputs:	r0 is the error code
 *		r1, r2 contains the zero-extended value
 *		ip, lr corrupted
 *
 * No other registers must be altered.  (see include/asm-arm/uaccess.h
 * for specific ASM register usage).
 *
 * Note that ADDR_LIMIT is either 0 or 0xc0000000.
 * Note also that it is intended that __get_user_bad is not global.
 */
#include <asm/constants.h>

	.global	__get_user_1
__get_user_1:
	bic	r1, sp, #0x1f00
	bic	r1, r1, #0x00ff
	ldr	r1, [r1, #TSK_ADDR_LIMIT]
	sub	r1, r1, #1
	cmp	r0, r1
1:	ldrlsbt	r1, [r0]
	movls	r0, #0
	movls	pc, lr
	b	__get_user_bad

	.global	__get_user_2
__get_user_2:
	bic	ip, sp, #0x1f00
	bic	ip, ip, #0x00ff
	ldr	ip, [ip, #TSK_ADDR_LIMIT]
	sub	ip, ip, #2
	cmp	r0, ip
2:	ldrlsbt	r1, [r0], #1
3:	ldrlsbt	ip, [r0]
	orrls	r1, r1, ip, lsl #8
	movls	r0, #0
	movls	pc, lr
	b	__get_user_bad

	.global	__get_user_4
__get_user_4:
	bic	r1, sp, #0x1f00
	bic	r1, r1, #0x00ff
	ldr	r1, [r1, #TSK_ADDR_LIMIT]
	sub	r1, r1, #4
	cmp	r0, r1
4:	ldrlst	r1, [r0]
	movls	r0, #0
	movls	pc, lr
	b	__get_user_bad

	.global	__get_user_8
__get_user_8:
	bic	r2, sp, #0x1f00
	bic	r2, r2, #0x00ff
	ldr	r2, [r2, #TSK_ADDR_LIMIT]
	sub	r2, r2, #8
	cmp	r0, r2
5:	ldrlst	r1, [r0], #4
6:	ldrlst	r2, [r0]
	movls	r0, #0
	movls	pc, lr

	/* fall through */

__get_user_bad_8:
	mov	r2, #0
__get_user_bad:
	mov	r1, #0
	mov	r0, #-14
	mov	pc, lr

.section __ex_table, "a"
	.long	1b, __get_user_bad
	.long	2b, __get_user_bad
	.long	3b, __get_user_bad
	.long	4b, __get_user_bad
	.long	5b, __get_user_bad_8
	.long	6b, __get_user_bad_8
.previous
