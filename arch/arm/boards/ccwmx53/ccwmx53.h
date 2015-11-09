/*
 * Copyright 2010 Digi International Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#ifndef _CCWMX53_H_
#define _CCWMX53_H_

struct ccwmx53_hwid {
	u8		variant;
	u8		version;
	u32		sn;
	char	mloc;
};

struct ccwmx53_ident {
	const char	*id_string;
	const int	mem_sz;
	const char	industrial;
	const char	eth0;
	const char	eth1;
	const char	wless;
};

extern struct ccwmx53_ident *ccwmx53_id;

#endif	/* _CCWMX53_H_ */
