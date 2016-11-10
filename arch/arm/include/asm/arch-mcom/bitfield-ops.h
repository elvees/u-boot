/*
 * Copyright 2016 ELVEES NeoTek JSC
 * Copyright 2017 RnD Center "ELVEES", OJSC
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef _BITFIELD_OPS_H
#define _BITFIELD_OPS_H

#define _bf_shf(x) (__builtin_ffsll(x) - 1)

/**
 * FIELD_PREP() - prepare a bitfield element
 * @_mask: shifted mask defining the field's length and position
 * @_val:  value to put in the field
 *
 * FIELD_PREP() masks and shifts up the value.  The result should
 * be combined with other fields of the bitfield using logical OR.
 */
#define FIELD_PREP(_mask, _val)                                         \
	({                                                              \
		((typeof(_mask))(_val) << _bf_shf(_mask)) & (_mask);    \
	})

/**
 * FIELD_GET() - extract a bitfield element
 * @_mask: shifted mask defining the field's length and position
 * @_reg:  32bit value of entire bitfield
 *
 * FIELD_GET() extracts the field specified by @_mask from the
 * bitfield passed in as @_reg by masking and shifting it down.
 */
#define FIELD_GET(_mask, _reg)                                          \
	({                                                              \
		(typeof(_mask))(((_reg) & (_mask)) >> _bf_shf(_mask));  \
	})

#endif /* _BITFIELD_OPS_H */
