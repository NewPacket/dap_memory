#pragma once

namespace dap
{

namespace memory
{

#ifndef MEM_INLINE
#define MEM_INLINE			__forceinline
#endif

#ifndef MEM_ASSERT
#define MEM_ASSERT(X)
#endif

#ifndef MEM_IS_DEFINED
#define MEM_IS_DEFINED(macro) 0
#endif

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef int i32;
typedef long long i64;
typedef unsigned long long u64;
typedef void* mem_ptr;

class memory_manager;

struct memory_block_spec
{
	memory_block_spec() = default;
	memory_block_spec(size_t size_, u16 alignment) : _size(size_), _alignment(alignment) {};

	size_t size() const { return _size; };
	u16 alignment() const { return _alignment; };

protected:

	size_t _size : 48;
	size_t _alignment : 16;
};

bool operator== (const memory_block_spec& rhs, const memory_block_spec& lhs)
{
	return rhs.alignment() == lhs.alignment() && rhs.size() == lhs.size();
}

struct memory_block
{
	memory_block() = default;

	memory_block(mem_ptr memory_ptr_, memory_block_spec memory_size_) :
		_memory_ptr(memory_ptr_),
		_memory_spec(memory_size_)
	{};

	memory_block(mem_ptr memory_ptr_, size_t memory_size_, u16 alignment_) :
		_memory_ptr(memory_ptr_),
		_memory_spec(memory_size_, alignment_)
	{};

	memory_block(const memory_block& other) :
		_memory_ptr(other._memory_ptr),
		_memory_spec(other._memory_spec)
	{};

	memory_block operator=(memory_block other)
	{
		_memory_ptr = other._memory_ptr;
		_memory_spec = other._memory_spec;
		return *this;
	};

	bool operator==(memory_block other) const
	{
		return other._memory_ptr == _memory_ptr && other._memory_spec == _memory_spec;
	}

	bool operator!=(memory_block other) const
	{
		return !(*this == other);
	}

	mem_ptr memory_ptr() const
	{
		return _memory_ptr;
	}

	size_t memory_size() const
	{
		return _memory_spec.size();
	}

	u16 alignment() const
	{
		return _memory_spec.alignment();
	}

protected:

	mem_ptr _memory_ptr = nullptr;
	memory_block_spec _memory_spec = {};
};

enum class memory_resource_growth_type : u8
{
	NON_GROWABLE = 0,
	COMMIT_ALL,
	COMMIT_ON_REQUEST
};

class memory_resource
{

public:

	memory_resource() = default;

	memory_resource(mem_ptr memory_ptr_, size_t memory_size_, u16 alignment_) : 
		memory_block_info(memory_ptr_, memory_size_, alignment_)
	{}
	
	memory_resource(mem_ptr memory_ptr_, size_t memory_size_) :
		memory_resource(memory_ptr_, memory_size_, 0) {};

	memory_block get_info() const
	{
		return memory_block_info;
	}

	void bind_to_manager(memory_manager* manager) { assigned_memory_manager = manager; };

protected:

	memory_block memory_block_info{};
	memory_manager* creator_memory_manager = nullptr;
	memory_manager* assigned_memory_manager = nullptr;
	memory_resource_growth_type growth_type = memory_resource_growth_type::NON_GROWABLE;

};

static_assert(sizeof(memory_resource) == 40);

template <size_t Size>
class fixed_memory_resource : public memory_resource
{
	using memory_resource::memory_block_info;

public:

	fixed_memory_resource() : memory_resource( &memory, Size)
	{
		this->growth_type = memory_resource_growth_type::NON_GROWABLE;
		this->creator_memory_manager = nullptr;
		this->assigned_memory_manager = nullptr;
	}

protected:

	u8 memory[Size];
};

}

}