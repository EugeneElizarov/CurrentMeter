#ifndef __TOOLS_H__

#define __TOOLS_H__

#define U16FROMA8(aname, index)		((uint16_t)((((uint16_t)aname[index + 1]) << 8) | aname[index]))

#endif
