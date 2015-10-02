#ifndef UTILITY_HEADER
#define UTILITY_HEADER

// This is actually only included for u8, u16, etc.
#include "common_irrlicht.h"

void writeU32(u8 *data, u32 i);
void writeU16(u8 *data, u16 i);
void writeU8(u8 *data, u8 i);
u32 readU32(u8 *data);
u16 readU16(u8 *data);
u8 readU8(u8 *data);

void writeV3S32(u8 *data, v3s32 p);
v3s32 readV3S32(u8 *data);

// Inlines for signed variants of the above

inline void writeS32(u8 *data, s32 i){
	writeU32(data, (u32)i);
}
inline s32 readS32(u8 *data){
	return (s32)readU32(data);
}

inline void writeS16(u8 *data, s16 i){
	writeU16(data, (u16)i);
}
inline s16 readS16(u8 *data){
	return (s16)readU16(data);
}

/*
	None of these are used at the moment
*/

template <typename T>
class SharedPtr
{
public:
	SharedPtr(T *t)
	{
		assert(t != NULL);
		refcount = new int;
		*refcount = 1;
		ptr = t;
	}
	SharedPtr(SharedPtr<T> &t)
	{
		*this = t;
	}
	~SharedPtr()
	{
		(*refcount)--;
		if(*refcount == 0)
		{
			delete refcount;
			delete ptr;
		}
	}
	SharedPtr<T> & operator=(SharedPtr<T> &t)
	{
		refcount = t.refcount;
		(*refcount)++;
		ptr = t.ptr;
		return *this;
	}
	T* operator->()
	{
		return ptr;
	}
	T & operator*()
	{
		return *ptr;
	}
private:
	T *ptr;
	int *refcount;
};

template <typename T>
class Buffer
{
public:
	Buffer(unsigned int size)
	{
		m_size = size;
		data = new T[size];
	}
	Buffer(const Buffer &buffer)
	{
		m_size = buffer.m_size;
		data = new T[buffer.m_size];
		memcpy(data, buffer.data, buffer.m_size);
	}
	Buffer(T *t, unsigned int size)
	{
		m_size = size;
		data = new T[size];
		memcpy(data, t, size);
	}
	~Buffer()
	{
		delete[] data;
	}
	T & operator[](unsigned int i) const
	{
		return data[i];
	}
	T * operator*() const
	{
		return data;
	}
	unsigned int getSize() const
	{
		return m_size;
	}
private:
	T *data;
	unsigned int m_size;
};

template <typename T>
class SharedBuffer
{
public:
	SharedBuffer(unsigned int size)
	{
		m_size = size;
		data = new T[size];
		refcount = new unsigned int (1);
	}
	SharedBuffer(const SharedBuffer &buffer)
	{
		m_size = buffer.m_size;
		//data = new T[buffer.m_size];
		//memcpy(data, buffer.data, buffer.m_size);
		data = buffer.data;
		refcount = buffer.refcount;
		(*refcount)++;
	}
	/*
		Copies whole buffer
	*/
	SharedBuffer(T *t, unsigned int size)
	{
		m_size = size;
		data = new T[size];
		memcpy(data, t, size);
		refcount = new unsigned int (1);
	}
	/*
		Copies whole buffer
	*/
	SharedBuffer(const Buffer<T> &buffer)
	{
		m_size = buffer.m_size;
		data = new T[buffer.getSize()];
		memcpy(data, *buffer, buffer.getSize());
		refcount = new unsigned int (1);
	}
	~SharedBuffer()
	{
		(*refcount)--;
		if(*refcount == 0)
			delete[] data;
	}
	T & operator[](unsigned int i) const
	{
		return data[i];
	}
	T * operator*() const
	{
		return data;
	}
	unsigned int getSize() const
	{
		return m_size;
	}
private:
	T *data;
	unsigned int m_size;
	unsigned int *refcount;
};

inline SharedBuffer<u8> SharedBufferFromString(const char *string)
{
	SharedBuffer<u8> b((u8*)string, strlen(string)+1);
	return b;
}

#endif

