// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2021, Intel Corporation */

/**
 * @file
 * C++ pmemobj pool.
 */

#ifndef LIBPMEMOBJ_CPP_POOL_HPP
#define LIBPMEMOBJ_CPP_POOL_HPP

#include <cstddef>
#include <errno.h>
#include <functional>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/ctl.hpp>
#include <libpmemobj++/detail/pool_data.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr_base.hpp>
#include <libpmemobj++/pexceptions.hpp>
#include <libpmemobj/atomic_base.h>
#include <libpmemobj/pool_base.h>

namespace pmem
{

namespace obj
{
template <typename T>
class persistent_ptr;

/**
 * The non-template pool base class.
 *
 * This class is a non-template version of pool. It is useful for places
 * where providing pool template argument is undesirable. The typical usage
 * example would be:
 * @snippet pool/pool.cpp pool_base_example
 */
class pool_base {
public:
	/**
	 * Defaulted constructor.
	 */
	pool_base() noexcept : pop(nullptr)
	{
	}

	/**
	 * Explicit constructor.
	 *
	 * Create pool_base object based on C-style pool handle.
	 *
	 * @param cpop C-style pool handle.
	 */
	explicit pool_base(pmemobjpool *cpop) noexcept : pop(cpop)
	{
	}

	/**
	 * Defaulted copy constructor.
	 */
	pool_base(const pool_base &) noexcept = default;

	/**
	 * Defaulted move constructor.
	 */
	pool_base(pool_base &&) noexcept = default;

	/**
	 * Defaulted copy assignment operator.
	 */
	pool_base &operator=(const pool_base &) noexcept = default;

	/**
	 * Defaulted move assignment operator.
	 */
	pool_base &operator=(pool_base &&) noexcept = default;

	/**
	 * Default virtual destructor.
	 */
	virtual ~pool_base() noexcept = default;

	/**
	 * Opens an existing object store memory pool.
	 *
	 * @param path System path to the file containing the memory
	 *	pool or a pool set.
	 * @param layout Unique identifier of the pool as specified at
	 *	pool creation time.
	 *
	 * @return handle to the opened pool.
	 *
	 * @throw pmem::pool_error when an error during opening occurs.
	 */
	static pool_base
	open(const std::string &path, const std::string &layout)
	{
#ifdef _WIN32
		pmemobjpool *pop = pmemobj_openU(path.c_str(), layout.c_str());
#else
		pmemobjpool *pop = pmemobj_open(path.c_str(), layout.c_str());
#endif
		check_pool(pop, "opening");

		pmemobj_set_user_data(pop, new detail::pool_data);

		return pool_base(pop);
	}

	/**
	 * Creates a new transactional object store pool.
	 *
	 * @param path System path to the file to be created. If exists
	 *	the pool can be created in-place depending on the size
	 *	parameter. Existing file must be zeroed.
	 * @param layout Unique identifier of the pool, can be a
	 *	null-terminated string.
	 * @param size Size of the pool in bytes. If zero and the file
	 *	exists the pool is created in-place.
	 * @param mode File mode for the new file.
	 *
	 * @return handle to the created pool.
	 *
	 * @throw pmem::pool_error when an error during creation occurs.
	 */
	static pool_base
	create(const std::string &path, const std::string &layout,
	       std::size_t size = PMEMOBJ_MIN_POOL, mode_t mode = DEFAULT_MODE)
	{
#ifdef _WIN32
		pmemobjpool *pop = pmemobj_createU(path.c_str(), layout.c_str(),
						   size, mode);
#else
		pmemobjpool *pop = pmemobj_create(path.c_str(), layout.c_str(),
						  size, mode);
#endif
		check_pool(pop, "creating");

		pmemobj_set_user_data(pop, new detail::pool_data);

		return pool_base(pop);
	}

	/**
	 * Checks if a given pool is consistent.
	 *
	 * @param path System path to the file containing the memory
	 *	pool or a pool set.
	 * @param layout Unique identifier of the pool as specified at
	 *	pool creation time.
	 *
	 * @return -1 on error, 1 if file is consistent, 0 otherwise.
	 */
	static int
	check(const std::string &path, const std::string &layout) noexcept
	{
#ifdef _WIN32
		return pmemobj_checkU(path.c_str(), layout.c_str());
#else
		return pmemobj_check(path.c_str(), layout.c_str());
#endif
	}

#ifdef _WIN32
	/**
	 * Opens an existing object store memory pool. Wide string variant.
	 * Available only on Windows.
	 *
	 * @param path System path to the file containing the memory
	 *	pool or a pool set.
	 * @param layout Unique identifier of the pool as specified at
	 *	pool creation time.
	 *
	 * @return handle to the opened pool.
	 *
	 * @throw pmem::pool_error when an error during opening occurs.
	 */
	static pool_base
	open(const std::wstring &path, const std::wstring &layout)
	{
		pmemobjpool *pop = pmemobj_openW(path.c_str(), layout.c_str());
		check_pool(pop, "opening");

		pmemobj_set_user_data(pop, new detail::pool_data);

		return pool_base(pop);
	}

	/**
	 * Creates a new transactional object store pool. Wide string variant.
	 * Available only on Windows.
	 *
	 * @param path System path to the file to be created. If exists
	 *	the pool can be created in-place depending on the size
	 *	parameter. Existing file must be zeroed.
	 * @param layout Unique identifier of the pool, can be a
	 *	null-terminated string.
	 * @param size Size of the pool in bytes. If zero and the file
	 *	exists the pool is created in-place.
	 * @param mode File mode for the new file.
	 *
	 * @return handle to the created pool.
	 *
	 * @throw pmem::pool_error when an error during creation occurs.
	 */
	static pool_base
	create(const std::wstring &path, const std::wstring &layout,
	       std::size_t size = PMEMOBJ_MIN_POOL, mode_t mode = DEFAULT_MODE)
	{
		pmemobjpool *pop = pmemobj_createW(path.c_str(), layout.c_str(),
						   size, mode);
		check_pool(pop, "creating");

		pmemobj_set_user_data(pop, new detail::pool_data);

		return pool_base(pop);
	}

	/**
	 * Checks if a given pool is consistent. Wide string variant.
	 * Available only on Windows.
	 *
	 * @param path System path to the file containing the memory
	 *	pool or a pool set.
	 * @param layout Unique identifier of the pool as specified at
	 *	pool creation time.
	 *
	 * @return -1 on error, 1 if file is consistent, 0 otherwise.
	 */
	static int
	check(const std::wstring &path, const std::wstring &layout) noexcept
	{
		return pmemobj_checkW(path.c_str(), layout.c_str());
	}
#endif

	/**
	 * Closes the pool.
	 *
	 * @throw std::logic_error if the pool has already been closed.
	 */
	void
	close()
	{
		if (this->pop == nullptr)
			throw std::logic_error("Pool already closed");

		auto *user_data = static_cast<detail::pool_data *>(
			pmemobj_get_user_data(this->pop));

		if (user_data->initialized.load())
			user_data->cleanup();

		delete user_data;

		pmemobj_close(this->pop);
		this->pop = nullptr;
	}

	/**
	 * Performs persist operation on a given chunk of memory.
	 *
	 * @param[in] addr address of memory chunk
	 * @param[in] len size of memory chunk
	 */
	void
	persist(const void *addr, size_t len) noexcept
	{
		pmemobj_persist(this->pop, addr, len);
	}

	/**
	 * Performs persist operation on a given pmem property.
	 *
	 * @param[in] prop Resides on pmem property
	 */
	template <typename Y>
	void
	persist(const p<Y> &prop) noexcept
	{
		pmemobj_persist(this->pop, &prop, sizeof(Y));
	}

	/**
	 * Performs persist operation on a given persistent pointer.
	 * Persist is not performed on the object referenced by this pointer.
	 *
	 * @param[in] ptr Persistent pointer to object
	 */
	template <typename Y>
	void
	persist(const persistent_ptr<Y> &ptr) noexcept
	{
		pmemobj_persist(this->pop, &ptr, sizeof(ptr));
	}

	/**
	 * Performs flush operation on a given chunk of memory.
	 *
	 * @param[in] addr address of memory chunk
	 * @param[in] len size of memory chunk
	 */
	void
	flush(const void *addr, size_t len) noexcept
	{
		pmemobj_flush(this->pop, addr, len);
	}

	/**
	 * Performs flush operation on a given pmem property.
	 *
	 * @param[in] prop Resides on pmem property
	 */
	template <typename Y>
	void
	flush(const p<Y> &prop) noexcept
	{
		pmemobj_flush(this->pop, &prop, sizeof(Y));
	}

	/**
	 * Performs flush operation on a given persistent object.
	 *
	 * @param[in] ptr Persistent pointer to object
	 */
	template <typename Y>
	void
	flush(const persistent_ptr<Y> &ptr) noexcept
	{
		pmemobj_flush(this->pop, &ptr, sizeof(ptr));
	}

	/**
	 * Performs drain operation.
	 */
	void
	drain(void) noexcept
	{
		pmemobj_drain(this->pop);
	}

	/**
	 * Performs memcpy and persist operation on a given chunk of
	 * memory.
	 *
	 * @param[in] dest destination memory address
	 * @param[in] src source memory address
	 * @param[in] len size of memory chunk
	 *
	 * @return A pointer to dest
	 */
	void *
	memcpy_persist(void *dest, const void *src, size_t len) noexcept
	{
		return pmemobj_memcpy_persist(this->pop, dest, src, len);
	}

	/**
	 * Performs memset and persist operation on a given chunk of
	 * memory.
	 *
	 * @param[in] dest destination memory address
	 * @param[in] c constant value to fill the memory
	 * @param[in] len size of memory chunk
	 *
	 * @return A pointer to dest
	 */
	void *
	memset_persist(void *dest, int c, size_t len) noexcept
	{
		return pmemobj_memset_persist(this->pop, dest, c, len);
	}

	/**
	 * Gets the C style handle to the pool.
	 *
	 * Necessary to be able to use the pool with the C API.
	 *
	 * @return pool opaque handle.
	 */
	PMEMobjpool *
	handle() noexcept
	{
		return this->pop;
	}

	POBJ_CPP_DEPRECATED PMEMobjpool *
	get_handle() noexcept
	{
		return pool_base::handle();
	}

	/**
	 * Starts defragmentation using selected pointers within this pool.
	 *
	 * @param[in] ptrv pointer to contiguous space containing
	 *	persistent_ptr's for defrag.
	 * @param[in] oidcnt number of persistent_ptr's passed (in ptrv).
	 * @return result struct containing a number of relocated and total
	 *	processed objects.
	 *
	 * @throw pmem::defrag_error when a failure during defragmentation
	 *	occurs. Even if this error is thrown, some of the objects could
	 *	have been relocated, see defrag_error.result for summary stats.
	 */
	pobj_defrag_result
	defrag(persistent_ptr_base **ptrv, size_t oidcnt)
	{
		pobj_defrag_result result;
		int ret = pmemobj_defrag(this->pop, (PMEMoid **)ptrv, oidcnt,
					 &result);

		if (ret != 0)
			throw defrag_error(result, "Defragmentation failed")
				.with_pmemobj_errormsg();

		return result;
	}

protected:
	static void
	check_pool(pmemobjpool *pop, std::string mode)
	{
		if (pop == nullptr) {
			if (errno == EINVAL || errno == EFBIG ||
			    errno == ENOENT || errno == EEXIST) {
				throw pmem::pool_invalid_argument(
					"Failed " + mode + " pool")
					.with_pmemobj_errormsg();
			} else {
				throw pmem::pool_error("Failed " + mode +
						       " pool")
					.with_pmemobj_errormsg();
			}
		}
	}

	/* The pool opaque handle */
	PMEMobjpool *pop;

#ifndef _WIN32
	/* Default create mode */
	static const int DEFAULT_MODE = S_IWUSR | S_IRUSR;
#else
	/* Default create mode */
	static const int DEFAULT_MODE = S_IWRITE | S_IREAD;
#endif
};

/**
 * PMEMobj pool class
 *
 * This class is the pmemobj pool handler. It provides basic primitives
 * for operations on pmemobj pools. The template parameter defines the
 * type of the root object within the pool. This pool class inherits also
 * some methods from the base class: pmem::obj::pool_base.
 *
 * The typical usage example would be:
 * @snippet pool/pool.cpp pool_example
 *
 * This API should not be mixed with C API. For example explicitly calling
 * pmemobj_set_user_data(pop) on pool which is handled by C++ pool object
 * is undefined behaviour.
 */
template <typename T>
class pool : public pool_base {
public:
	/**
	 * Defaulted constructor.
	 */
	pool() noexcept = default;

	/**
	 * Defaulted copy constructor.
	 */
	pool(const pool &) noexcept = default;

	/**
	 * Defaulted move constructor.
	 */
	pool(pool &&) noexcept = default;

	/**
	 * Defaulted copy assignment operator.
	 */
	pool &operator=(const pool &) noexcept = default;

	/**
	 * Defaulted move assignment operator.
	 */
	pool &operator=(pool &&) noexcept = default;

	/**
	 * Default destructor.
	 */
	~pool() noexcept = default;

	/**
	 * Defaulted copy constructor.
	 */
	explicit pool(const pool_base &pb) noexcept : pool_base(pb)
	{
	}

	/**
	 * Defaulted move constructor.
	 */
	explicit pool(pool_base &&pb) noexcept : pool_base(pb)
	{
	}

	/**
	 * Query libpmemobj state at pool scope.
	 *
	 * @param[in] name name of entry point
	 *
	 * @returns variable representing internal state
	 *
	 * For more details, see:
	 * https://pmem.io/pmdk/manpages/linux/master/libpmemobj/pmemobj_ctl_get.3
	 */
	template <typename M>
	M
	ctl_get(const std::string &name)
	{
		return ctl_get_detail<M>(pop, name);
	}

	/**
	 * Modify libpmemobj state at pool scope.
	 *
	 * @param[in] name name of entry point
	 * @param[in] arg extra argument
	 *
	 * @returns copy of arg, possibly modified by query
	 *
	 * For more details, see:
	 * https://pmem.io/pmdk/manpages/linux/master/libpmemobj/pmemobj_ctl_get.3
	 */
	template <typename M>
	M
	ctl_set(const std::string &name, M arg)
	{
		return ctl_set_detail(pop, name, arg);
	}

	/**
	 * Execute function at pool scope.
	 *
	 * @param[in] name name of entry point
	 * @param[in] arg extra argument
	 *
	 * @returns copy of arg, possibly modified by query
	 *
	 * For more details, see:
	 * https://pmem.io/pmdk/manpages/linux/master/libpmemobj/pmemobj_ctl_get.3
	 */
	template <typename M>
	M
	ctl_exec(const std::string &name, M arg)
	{
		return ctl_exec_detail(pop, name, arg);
	}

#ifdef _WIN32
	/**
	 * Query libpmemobj state at pool scope.
	 *
	 * @param[in] name name of entry point
	 *
	 * @returns variable representing internal state
	 *
	 * For more details, see:
	 * https://pmem.io/pmdk/manpages/linux/master/libpmemobj/pmemobj_ctl_get.3
	 */
	template <typename M>
	M
	ctl_get(const std::wstring &name)
	{
		return ctl_get_detail<M>(pop, name);
	}

	/**
	 * Modify libpmemobj state at pool scope.
	 *
	 * @param[in] name name of entry point
	 * @param[in] arg extra argument
	 *
	 * @returns copy of arg, possibly modified by query
	 *
	 * For more details, see:
	 * https://pmem.io/pmdk/manpages/linux/master/libpmemobj/pmemobj_ctl_get.3
	 */
	template <typename M>
	M
	ctl_set(const std::wstring &name, M arg)
	{
		return ctl_set_detail(pop, name, arg);
	}

	/**
	 * Execute function at pool scope.
	 *
	 * @param[in] name name of entry point
	 * @param[in] arg extra argument
	 *
	 * @returns copy of arg, possibly modified by query
	 *
	 * For more details, see:
	 * https://pmem.io/pmdk/manpages/linux/master/libpmemobj/pmemobj_ctl_get.3
	 */
	template <typename M>
	M
	ctl_exec(const std::wstring &name, M arg)
	{
		return ctl_exec_detail(pop, name, arg);
	}
#endif

	/**
	 * Retrieves pool's root object.
	 *
	 * @return persistent pointer to the root object.
	 *
	 * @throw pmem::pool_error when pool handle is incorrect.
	 */
	persistent_ptr<T>
	root()
	{
		if (pop == nullptr)
			throw pmem::pool_error("Invalid pool handle");

		persistent_ptr<T> root = pmemobj_root(this->pop, sizeof(T));
		return root;
	}

	POBJ_CPP_DEPRECATED persistent_ptr<T>
	get_root()
	{
		return pool::root();
	}

	/**
	 * Opens an existing object store memory pool.
	 *
	 * @param path System path to the file containing the memory
	 *	pool or a pool set.
	 * @param layout Unique identifier of the pool as specified at
	 *	pool creation time.
	 *
	 * @return handle to the opened pool.
	 *
	 * @throw pmem::pool_error when an error during opening occurs.
	 */
	static pool<T>
	open(const std::string &path, const std::string &layout)
	{
		return pool<T>(pool_base::open(path, layout));
	}

	/**
	 * Creates a new transactional object store pool.
	 *
	 * @param path System path to the file to be created. If exists
	 *	the pool can be created in-place depending on the size
	 *	parameter. Existing file must be zeroed.
	 * @param layout Unique identifier of the pool, can be a
	 *	null-terminated string.
	 * @param size Size of the pool in bytes. If zero and the file
	 *	exists the pool is created in-place.
	 * @param mode File mode for the new file.
	 *
	 * @return handle to the created pool.
	 *
	 * @throw pmem::pool_error when an error during creation occurs.
	 */
	static pool<T>
	create(const std::string &path, const std::string &layout,
	       std::size_t size = PMEMOBJ_MIN_POOL, mode_t mode = DEFAULT_MODE)
	{
		return pool<T>(pool_base::create(path, layout, size, mode));
	}

	/**
	 * Checks if a given pool is consistent.
	 *
	 * @param path System path to the file containing the memory
	 *	pool or a pool set.
	 * @param layout Unique identifier of the pool as specified at
	 *	pool creation time.
	 *
	 * @return -1 on error, 1 if file is consistent, 0 otherwise.
	 */
	static int
	check(const std::string &path, const std::string &layout)
	{
		return pool_base::check(path, layout);
	}

#ifdef _WIN32
	/**
	 * Opens an existing object store memory pool. Wide string variant.
	 * Available only on Windows.
	 *
	 * @param path System path to the file containing the memory
	 *	pool or a pool set.
	 * @param layout Unique identifier of the pool as specified at
	 *	pool creation time.
	 *
	 * @return handle to the opened pool.
	 *
	 * @throw pmem::pool_error when an error during opening occurs.
	 */
	static pool<T>
	open(const std::wstring &path, const std::wstring &layout)
	{
		return pool<T>(pool_base::open(path, layout));
	}

	/**
	 * Creates a new transactional object store pool. Wide string variant.
	 * Available only on Windows.
	 *
	 * @param path System path to the file to be created. If exists
	 *	the pool can be created in-place depending on the size
	 *	parameter. Existing file must be zeroed.
	 * @param layout Unique identifier of the pool, can be a
	 *	null-terminated string.
	 * @param size Size of the pool in bytes. If zero and the file
	 *	exists the pool is created in-place.
	 * @param mode File mode for the new file.
	 *
	 * @return handle to the created pool.
	 *
	 * @throw pmem::pool_error when an error during creation occurs.
	 */
	static pool<T>
	create(const std::wstring &path, const std::wstring &layout,
	       std::size_t size = PMEMOBJ_MIN_POOL, mode_t mode = DEFAULT_MODE)
	{
		return pool<T>(pool_base::create(path, layout, size, mode));
	}

	/**
	 * Checks if a given pool is consistent. Wide string variant.
	 * Available only on Windows.
	 *
	 * @param path System path to the file containing the memory
	 *	pool or a pool set.
	 * @param layout Unique identifier of the pool as specified at
	 *	pool creation time.
	 *
	 * @return -1 on error, 1 if file is consistent, 0 otherwise.
	 */
	static int
	check(const std::wstring &path, const std::wstring &layout)
	{
		return pool_base::check(path, layout);
	}
#endif
};

/**
 * Query libpmemobj state at global scope.
 *
 * @param[in] name name of entry point
 *
 * @returns variable representing internal state
 *
 * For more details, see:
 * https://pmem.io/pmdk/manpages/linux/master/libpmemobj/pmemobj_ctl_get.3
 */
template <typename T>
T
ctl_get(const std::string &name)
{
	return ctl_get_detail<T>(nullptr, name);
}

/**
 * Modify libpmemobj state at global scope.
 *
 * @param[in] name name of entry point
 * @param[in] arg extra argument
 *
 * @returns copy of arg, possibly modified by query
 *
 * For more details, see:
 * https://pmem.io/pmdk/manpages/linux/master/libpmemobj/pmemobj_ctl_get.3
 */
template <typename T>
T
ctl_set(const std::string &name, T arg)
{
	return ctl_set_detail(nullptr, name, arg);
}

/**
 * Execute function at global scope.
 *
 * @param[in] name name of entry point
 * @param[in] arg extra argument
 *
 * @returns copy of arg, possibly modified by query
 *
 * For more details, see:
 * https://pmem.io/pmdk/manpages/linux/master/libpmemobj/pmemobj_ctl_get.3
 */
template <typename T>
T
ctl_exec(const std::string &name, T arg)
{
	return ctl_exec_detail(nullptr, name, arg);
}

#ifdef _WIN32
/**
 * Query libpmemobj state at global scope.
 *
 * @param[in] name name of entry point
 *
 * @returns variable representing internal state
 *
 * For more details, see:
 * https://pmem.io/pmdk/manpages/linux/master/libpmemobj/pmemobj_ctl_get.3
 */
template <typename T>
T
ctl_get(const std::wstring &name)
{
	return ctl_get_detail<T>(nullptr, name);
}

/**
 * Modify libpmemobj state at global scope.
 *
 * @param[in] name name of entry point
 * @param[in] arg extra argument
 *
 * @returns copy of arg, possibly modified by query
 *
 * For more details, see:
 * https://pmem.io/pmdk/manpages/linux/master/libpmemobj/pmemobj_ctl_get.3
 */
template <typename T>
T
ctl_set(const std::wstring &name, T arg)
{
	return ctl_set_detail(nullptr, name, arg);
}

/**
 * Execute function at global scope.
 *
 * @param[in] name name of entry point
 * @param[in] arg extra argument
 *
 * @returns copy of arg, possibly modified by query
 *
 * For more details, see:
 * https://pmem.io/pmdk/manpages/linux/master/libpmemobj/pmemobj_ctl_get.3
 */
template <typename T>
T
ctl_exec(const std::wstring &name, T arg)
{
	return ctl_exec_detail(nullptr, name, arg);
}
#endif

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_POOL_HPP */
