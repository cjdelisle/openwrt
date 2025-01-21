/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *
 * Copyright (C) 2025 Caleb James DeLisle <cjd@cjdns.fr>
 */

#ifndef _EN75_COMMON_H__
#define _EN75_COMMON_H__

#ifdef CONFIG_EN75_PROM_DEBUGF
void prom_debugf(const char *fmt, ...);
#else
#define prom_debugf(fmt, args...) do {} while(0)
#endif

#endif /* _EN75_COMMON_H__ */
