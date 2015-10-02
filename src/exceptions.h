#ifndef EXCEPTIONS_HEADER
#define EXCEPTIONS_HEADER

#include <exception>

class BaseException : public std::exception
{
public:
	BaseException(const char *s)
	{
		m_s = s;
	}
	virtual const char * what() const throw()
	{
		return m_s;
	}
	const char *m_s;
};

class AsyncQueuedException : public BaseException
{
public:
	AsyncQueuedException(const char *s):
		BaseException(s)
	{}
};

class NotImplementedException : public BaseException
{
public:
	NotImplementedException(const char *s):
		BaseException(s)
	{}
};

class AlreadyExistsException : public BaseException
{
public:
	AlreadyExistsException(const char *s):
		BaseException(s)
	{}
};

/*
	Some "old-style" interrupts:
*/

class InvalidPositionException : public std::exception
{
	virtual const char * what() const throw()
	{
		return "Somebody tried to get/set something in a nonexistent position.";
	}
};

class TargetInexistentException : public std::exception
{
	virtual const char * what() const throw()
	{
		return "Somebody tried to refer to something that doesn't exist.";
	}
};

class NullPointerException : public std::exception
{
	virtual const char * what() const throw()
	{
		return "NullPointerException";
	}
};

#endif

