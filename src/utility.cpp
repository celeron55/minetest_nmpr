#include "utility.h"

void writeU32(u8 *data, u32 i)
{
	data[0] = ((i>>24)&0xff);
	data[1] = ((i>>16)&0xff);
	data[2] = ((i>> 8)&0xff);
	data[3] = ((i>> 0)&0xff);
}

void writeU16(u8 *data, u16 i)
{
	data[0] = ((i>> 8)&0xff);
	data[1] = ((i>> 0)&0xff);
}

void writeU8(u8 *data, u8 i)
{
	data[0] = ((i>> 0)&0xff);
}

u32 readU32(u8 *data)
{
	return (data[0]<<24) | (data[1]<<16) | (data[2]<<8) | (data[3]<<0);
}

u16 readU16(u8 *data)
{
	return (data[0]<<8) | (data[1]<<0);
}

u8 readU8(u8 *data)
{
	return (data[0]<<0);
}

void writeV3S32(u8 *data, v3s32 p)
{
	writeS32(&data[0], p.X);
	writeS32(&data[4], p.Y);
	writeS32(&data[8], p.Z);
}

v3s32 readV3S32(u8 *data)
{
	v3s32 p;
	p.X = readS32(&data[0]);
	p.Y = readS32(&data[4]);
	p.Z = readS32(&data[8]);
	return p;
}

