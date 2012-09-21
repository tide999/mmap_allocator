#include <memory>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

namespace mmap_allocator_namespace
{
	enum access_mode {
		READ_ONLY,  /* Readonly modus. Segfaults when vector content is written to */
		READ_WRITE_PRIVATE, /* Read/write access, writes are not propagated to disk */
		READ_WRITE_SHARED, /* Read/write access, writes are propagated to disk (file is modified) */
	};

	class mmap_allocator_exception: public std::exception {
public:
		mmap_allocator_exception() throw(): 
			std::exception(),
			msg("Unknown reason")
		{
		}

		mmap_allocator_exception(const char *msg_param) throw(): 
			std::exception(),
			msg(msg_param)
		{
		}

		virtual ~mmap_allocator_exception(void) throw()
		{
		}

		const char *message(void)
		{
			return msg.c_str();
		}
private:
		std::string msg; 
	};

	template <typename T> 
	class mmap_allocator: public std::allocator<T>
	{
public:
		typedef size_t size_type; // TODO: use the inherited types
		typedef T* pointer;
		typedef const T* const_pointer;

		template<typename _Tp1>
	        struct rebind
        	{ 
			typedef mmap_allocator<_Tp1> other; 
		};

		pointer allocate(size_type n, const void *hint=0)
		{
			fprintf(stderr, "Alloc %d bytes.\n", n);
			open_and_mmap_file(n);
			fprintf(stderr, "mem-area = %p\n", memory_area);
			return (pointer)memory_area;
		}

		void deallocate(pointer p, size_type n)
		{
			fprintf(stderr, "Dealloc %d bytes (%p).\n", n, p);
			return std::allocator<T>::deallocate(p, n);
		}

		mmap_allocator() throw(): 
			std::allocator<T>(),
			filename(""),
			offset(0),
			access_mode(READ_ONLY),
			fd(-1),
			memory_area(NULL)
		{ }
		mmap_allocator(const mmap_allocator &a) throw():
			std::allocator<T>(a),
			filename(a.filename),
			offset(a.offset),
			access_mode(a.access_mode),
                        fd(a.fd),
			memory_area(a.memory_area)
		{ }
		mmap_allocator(const std::string filename_param, enum access_mode access_mode_param = READ_ONLY, off_t offset_param = 0) throw():
			std::allocator<T>(),
			filename(filename_param),
			offset(offset_param),
			access_mode(access_mode_param),
			fd(-1),
			memory_area(NULL)
		{
		}
			
		~mmap_allocator() throw() { }

private:
		std::string filename;
		off_t offset;
		enum access_mode access_mode;
		int fd;
		void *memory_area;

		void open_and_mmap_file(size_t length)
		/* Must not be called from within constructors since they must not throw exceptions */
		{
			if (filename.c_str()[0] == '\0') {
				throw mmap_allocator_exception("mmap_allocator not correctly initialized: filename is empty.");
			}
			int mode;
			int prot;
			int mmap_mode;

			switch (access_mode) {
			case READ_ONLY: mode = O_RDONLY; prot = PROT_READ; mmap_mode = MAP_SHARED; break;
			case READ_WRITE_SHARED: mode = O_RDWR; prot = PROT_READ | PROT_WRITE; mmap_mode = MAP_SHARED; break;
			case READ_WRITE_PRIVATE: mode = O_RDWR; prot = PROT_READ | PROT_WRITE; mmap_mode = MAP_PRIVATE; break;
			}

			fd = open(filename.c_str(), mode);
			if (fd < 0) {
				throw mmap_allocator_exception("No such file or directory");
			}
			memory_area = mmap(NULL, length, prot, mmap_mode, fd, offset);
			if (memory_area == MAP_FAILED) {
				throw mmap_allocator_exception("Error in mmap");
			}
		}
	};
}
