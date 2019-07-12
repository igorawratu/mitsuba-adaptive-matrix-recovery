#ifndef __MANY_LIGHT_POOL_REF_H_
#define __MANY_LIGHT_POOL_REF_H_


#include <mitsuba/core/util.h>
#include <boost/pool/pool_alloc.hpp>
#include <boost/pool/pool.hpp>
#include <boost/container/vector.hpp>
#include <boost/shared_ptr.hpp>

MTS_NAMESPACE_BEGIN

template <typename T> class pool_ref {
public:
	/// Create a NULL reference
	pool_ref() : m_ptr(NULL) { }

	/// Construct a reference from a pointer
	pool_ref(T *ptr) : m_ptr(ptr) { if (m_ptr) ((Object *)m_ptr)->incRef(); }

	/// Copy-constructor
	pool_ref(const pool_ref &pRef) : m_ptr(pRef.m_ptr) { if (m_ptr) ((Object *)m_ptr)->incRef(); }

	/// Destroy this reference
	~pool_ref() { if (m_ptr) (m_ptr)->decRef(); }

	/// Overwrite this reference with another reference
	inline pool_ref& operator= (const pool_ref& r) {
		if (m_ptr == r.m_ptr)
			return *this;
		if (m_ptr)
			(m_ptr)->decRef();
		m_ptr = r.m_ptr;
		if (m_ptr)
			((Object *)m_ptr)->incRef();
		return *this;
	}

	/// Overwrite this reference with a pointer to another object
	inline pool_ref& operator= (T *ptr) {
		if (m_ptr == ptr)
			return *this;
		if (m_ptr)
			(m_ptr)->decRef();
		m_ptr = ptr;
		if (m_ptr)
			((Object *)m_ptr)->incRef();
		return *this;
	}

	/// Compare this reference with another reference
	inline bool operator== (const pool_ref &pref) const { return (m_ptr == pref.m_ptr); }

	/// Compare this reference with another reference
	inline bool operator!= (const pool_ref &pref) const { return (m_ptr != pref.m_ptr); }

	/// Compare this reference with a pointer
	inline bool operator== (const T* ptr) const { return (m_ptr == ptr); }

	/// Compare this reference with a pointer
	inline bool operator!= (const T* ptr) const { return (m_ptr != ptr); }

	/// Check whether this is a NULL reference
	inline bool operator!() const { return (m_ptr == NULL); }

	/// Access the object referenced by this reference
	inline T* operator-> () { return m_ptr; }

	/// Access the object referenced by this reference
	inline const T* operator-> () const { return m_ptr; }

	/// Return a C++ reference to the referenced object
	inline T& operator*() { return *m_ptr; }

	/// Return a C++ reference to the referenced object
	inline const T& operator*() const { return *m_ptr; }

	/// Return a pointer to the referenced object
	inline operator T* () { return m_ptr; }

	/// Return a pointer to the referenced object
	inline T* get() { return m_ptr; }

	/// Return a pointer to the referenced object
	inline const T* get() const { return m_ptr; }

	/**
	* Return a string representation of this reference
	*/
	inline std::string toString() const {
		if (m_ptr == NULL)
			return formatString("pool_ref<%s>[null]",
			T::m_theClass->getName().c_str());
		else
			return formatString("pool_ref<%s>[pool_ref=%i, ptr=%s]",
			m_ptr->getClass()->getName().c_str(),
			m_ptr->getRefCount(), m_ptr->toString().c_str());
	}
private:
	T *m_ptr;
};



struct PoolObject : boost::pool<> {

	PoolObject(uint32_t s) : boost::pool<>(s), m_size(s) {

	}

	uint32_t m_size;
};

struct PoolManager {

	PoolManager() : m_pools(new boost::container::vector<boost::shared_ptr<PoolObject>>()) {
	
	}

	boost::shared_ptr<PoolObject> getPool(uint32_t s) {
		boost::container::vector<boost::shared_ptr<PoolObject>> &pools = *m_pools;
		for (boost::container::vector<boost::shared_ptr<PoolObject>>::iterator i = pools.begin(); i != pools.end(); i++) {
			boost::shared_ptr<PoolObject> p(*i);
			if (p->m_size == s)
				return p;		
		}

		boost::shared_ptr<PoolObject> new_p(new PoolObject(s));
		m_pools->push_back(new_p);
		return m_pools->back();
	}

	boost::shared_ptr<boost::container::vector<boost::shared_ptr<PoolObject>>> m_pools;


private:
	PoolManager(const PoolManager &p) {}
	PoolManager(PoolManager &p) { }
};



template <typename  T>
class PoolAllocator //: public allocator<T>
{


public:
	typedef T                 value_type;
	typedef value_type*       pointer;
	typedef const value_type* const_pointer;
	typedef value_type&       reference;
	typedef const value_type& const_reference;
	typedef std::size_t       size_type;
	typedef std::ptrdiff_t    difference_type;

	boost::shared_ptr<PoolManager> m_manager;
	boost::shared_ptr<PoolObject> m_allocator;

	PoolAllocator(boost::shared_ptr<PoolManager> &manager) : m_manager(manager) {
		m_allocator = m_manager->getPool(sizeof(T));
	}

	template<typename u>
	PoolAllocator(const PoolAllocator<u> &before) : m_manager(before.m_manager) {
		m_allocator = m_manager->getPool(sizeof(T));
	}

	// rebind the class type u
	template <class u>
	struct rebind{
		typedef PoolAllocator<u>  other;
	};

	pointer address(reference x) const { return &x; }
	const_pointer address(const_reference x) const
	{
		return &x;
	}

	pointer allocate(const size_type n)
	{
		const pointer ret = (n == 1) ?
			static_cast<pointer>(m_allocator->malloc()) :
			static_cast<pointer>(m_allocator->ordered_malloc(n));
		if (ret == 0)
			boost::throw_exception(std::bad_alloc());
		return ret;
	}

	pointer allocate(const size_type n, const void * const)
	{ //! Allocate memory .
		return allocate(n);
	}

	pointer allocate()
	{ //! Allocate memory.
		const pointer ret = static_cast<pointer>(
			(m_allocator->malloc)());
		if (ret == 0)
			boost::throw_exception(std::bad_alloc());
		return ret;
	}

	void deallocate(const pointer ptr, const size_type n)
	{ //! Deallocate memory.

#ifdef BOOST_NO_PROPER_STL_DEALLOCATE
		if (ptr == 0 || n == 0)
			return;
#endif
		if (n == 1)
			(m_allocator->free)(ptr);
		else
			(m_allocator->free)(ptr, n);
	}

	void deallocate(const pointer ptr)
	{ //! deallocate/free
		(m_allocator->free)(ptr);
	}


	size_type max_size() const
	{
		return static_cast<size_type>(-1) / sizeof(value_type);
		//return size_type(UNIT_MAX/sizeof(T));
	}

	void construct(pointer p, const value_type& x)
	{
		new(p)value_type(x);
	}

	void destroy(pointer p) { p->~value_type(); }

private:
	void operator = (const PoolAllocator&);
};


MTS_NAMESPACE_END

#endif