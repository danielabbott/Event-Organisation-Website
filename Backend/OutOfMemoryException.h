#include <stdexcept>

struct OutOfMemoryException : public std::exception
{
	const char * what () const throw ()
    {
    	return "Out of Memory";
    }
};