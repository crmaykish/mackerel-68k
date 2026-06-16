#ifndef _MEM_H
#define _MEM_H

#define MEM(address) (*(volatile uint8_t *)(address))
#define MEM16(address) (*(volatile uint16_t *)(address))
#define MEM32(address) (*(volatile uint32_t *)(address))

#endif
