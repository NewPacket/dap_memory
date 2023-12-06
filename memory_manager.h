#pragma once

#ifndef DAP_SUPPRESS_DEBUG_BREAK
#define DEBUGGER_BREAK() __debugbreak()
#else
#define DEBUGGER_BREAK() 
#endif

#define ALLOCATOR_CALL_INFO __FILE__, __LINE__
#define ACI ALLOCATOR_CALL_INFO

namespace dap
{

namespace memory
{

namespace utils 
{

MEM_INLINE u16
get_aligned_distance(mem_ptr current, u16 align)
{
	size_t assigned_memory = reinterpret_cast<size_t>(current);
	u16 needed_more_for_align = (align - (assigned_memory % align)) % align;
	return needed_more_for_align;
}

template<typename T>
MEM_INLINE u16
get_aligned_distance_after(mem_ptr current, u16 align)
{
	size_t assigned_memory_ = reinterpret_cast<size_t>(current);
	u16 aligned_after = ((assigned_memory_ + sizeof(T)) % align);
	u16 needed_more_for_align = (align - aligned_after) % align;
	return needed_more_for_align;
}

MEM_INLINE i64
get_ptr_distance(mem_ptr a, mem_ptr b)
{
	return reinterpret_cast<size_t>(a) - reinterpret_cast<size_t>(b);
}

template <typename T>
MEM_INLINE T
advance_ptr(mem_ptr ptr, size_t advance)
{
	return reinterpret_cast<T>(reinterpret_cast<size_t>(ptr) + advance);
}

MEM_INLINE mem_ptr
advance_ptr(mem_ptr ptr, size_t advance)
{
	return reinterpret_cast<mem_ptr>(reinterpret_cast<size_t>(ptr) + advance);
}

template <typename T>
MEM_INLINE T
recede_ptr(mem_ptr ptr, size_t recede)
{
	//TODO:: add static assert on ptr type
	//static_assert<
	return reinterpret_cast<T>(reinterpret_cast<size_t>(ptr) - recede);
}

MEM_INLINE mem_ptr
recede_ptr(mem_ptr ptr, size_t recede)
{
	//TODO:: add static assert on ptr type
	//static_assert<
	return reinterpret_cast<mem_ptr>(reinterpret_cast<size_t>(ptr) - recede);
}

}

enum memory_allocation_result_types
{
	FAIL = 0, // MB remove later
	WRONG_MANAGER,
	USE_AFTER_FREE,
	OUT_OF_MEMORY,
	CURRENT_BLOCK_BIG_ENOUGH,
	CONTINUE_CURRENT_BLOCK,
	NEW_BLOCK,
};

struct memory_allocation_result
{
	memory_allocation_result() = default;
	explicit memory_allocation_result(memory_allocation_result_types result_) :	result(result_)	{};

	explicit memory_allocation_result(memory_block block_ ,memory_allocation_result_types result_) :
		block(block_),
		result(result_) 
	{};

	memory_allocation_result(mem_ptr memory_ptr_, size_t memory_size_, u16 alignment, memory_allocation_result_types result_) :
		block(memory_ptr_, memory_size_, alignment),
		result(result_)
	{};

	const memory_block block{};
	const memory_allocation_result_types result{};
};

struct memory_manager_statistics
{
	explicit memory_manager_statistics(const memory_block& ínfo) : memory_block_info(ínfo) {};

	memory_block memory_block_info{};
	size_t memory_used = 0;
};



class memory_manager
{
	
protected:

	static inline constexpr u8 default_alignment = 16;

public:

	explicit memory_manager(memory_resource* resource);

	[[nodiscard]]
	memory_allocation_result allocate(u32 required_memory_size, const char* file_name, i32 line)
	{
		return allocate_aligned(required_memory_size, default_alignment, file_name, line);
	};

	[[nodiscard]]
	virtual memory_allocation_result reallocate(memory_block block, u32 required_memory_size, const char* file_name, i32 line) = 0;

	[[nodiscard]]
	virtual memory_allocation_result allocate_aligned(u32 required_memory_size, u16 alignment, const char* file_name, i32 line) = 0;
	

	virtual void free(memory_block free_block, const char* file_name, i32 line) = 0;
	virtual void return_memory(memory_manager* top_allocator) = 0;

	bool is_owned(mem_ptr ptr) { return resource_info.memory_ptr() <= ptr && ptr < end_pointer; };
	bool is_owned(memory_block block) { return is_owned(block.memory_ptr()); };

	template<typename T, typename ...Args>
	T* construct(Args&&... args);

	template<typename T>
	void destroy(T* memory_ptr);

	memory_manager() = default;
	virtual ~memory_manager() = default;

	memory_manager(memory_manager&) = delete;
	memory_manager& operator=(const memory_manager&) = delete;

protected:

	memory_block resource_info;
	mem_ptr end_pointer;
	memory_resource* assigned_memory_resouce;

};

memory_manager::memory_manager(memory_resource* resource) : assigned_memory_resouce(resource), resource_info(resource->get_info())
{
	resource->bind_to_manager(this);
	end_pointer = utils::advance_ptr(resource_info.memory_ptr(), resource_info.memory_size());
};

template<typename T, typename ...Args>
T* memory_manager::construct(Args&&... args)
{
	memory_allocation_result result = allocate_aligned(sizeof(T), alignof(T), ACI);
	if (result.result == memory_allocation_result_types::NEW_BLOCK)
	{
		new(result.block.memory_ptr()) T(args...);
		return (T*)result.block.memory_ptr();
	}
	else
	{
		return nullptr;
	}
}

template<typename T>
void memory_manager::destroy(T* memory_ptr)
{
	static_cast<T*>(memory_ptr)->~T();
	free({ memory_ptr, sizeof(T), alignof(T) }, nullptr, 0);
};

struct bump_manager_statistics : memory_manager_statistics
{
	using memory_manager_statistics::memory_manager_statistics;
};

class bump_memory_manager : public memory_manager
{

public:

	explicit bump_memory_manager(memory_resource* resource);

	memory_allocation_result allocate_aligned(u32 required_memory_size, u16 alignment, const char* file_name, i32 line) override;
	memory_allocation_result reallocate(memory_block block, u32 required_memory_size, const char* file_name, i32 line) override;
	void free(memory_block free_block, const char* file_name, i32 line) override;
	void return_memory(memory_manager* top_allocator) override;

protected:

	memory_block last_allocated_block{};
	size_t currently_used_memory = 0;
	mem_ptr next_ptr = nullptr;

public:

	bump_manager_statistics get_statistics() const;

};

bump_memory_manager::bump_memory_manager(memory_resource* resource) : memory_manager(resource)
{
	MEM_ASSERT(resource_info.memory_size() > 16);
	next_ptr = resource_info.memory_ptr();
};

bump_manager_statistics 
bump_memory_manager::get_statistics() const
{
	bump_manager_statistics stats(assigned_memory_resouce->get_info());
	stats.memory_used = currently_used_memory;
	return stats;
};

memory_allocation_result
bump_memory_manager::allocate_aligned(u32 required_memory_size, u16 alignment, const char* file_name, i32 line)
{
	if (resource_info.memory_size() < currently_used_memory + required_memory_size)
	{
		//DEBUGGER_BREAK();
		return memory_allocation_result{ OUT_OF_MEMORY };
	}

	size_t needed_more_for_align = utils::get_aligned_distance(next_ptr, alignment);
	mem_ptr next_aligned = utils::advance_ptr(next_ptr, needed_more_for_align);
	size_t memory_needed_with_aligned = required_memory_size + needed_more_for_align;
	size_t new_possible_memory_used = currently_used_memory + memory_needed_with_aligned;

	if (resource_info.memory_size() < new_possible_memory_used)
	{
		DEBUGGER_BREAK();
		return memory_allocation_result{ OUT_OF_MEMORY };
	}

	currently_used_memory = new_possible_memory_used;
	next_ptr = utils::advance_ptr(next_ptr, memory_needed_with_aligned);

	memory_allocation_result result{ next_aligned, required_memory_size, alignment, memory_allocation_result_types::NEW_BLOCK };
	last_allocated_block = result.block;

	if (MEM_IS_DEFINED(_DEBUG_LOG_ALLOCATIONS))
	{
	}

	return result;
};

memory_allocation_result
bump_memory_manager::reallocate(memory_block current_memory_block, u32 required_memory_size, const char* file_name, i32 line)
{
	if (!is_owned(current_memory_block))
	{
		DEBUGGER_BREAK();
		return memory_allocation_result{WRONG_MANAGER};
	}

	if (current_memory_block.memory_size() >= required_memory_size)
	{
		return memory_allocation_result{ current_memory_block,	memory_allocation_result_types::CURRENT_BLOCK_BIG_ENOUGH };
	}

	u16 alignment = current_memory_block.alignment();
	if (last_allocated_block != current_memory_block)
	{
		return allocate_aligned(required_memory_size, alignment, file_name, line);
	}

	size_t need_more = required_memory_size - current_memory_block.memory_size();
	size_t new_possible_memory_used = currently_used_memory + need_more;
	if (resource_info.memory_size() < new_possible_memory_used)
	{
		return memory_allocation_result{OUT_OF_MEMORY};
	}

	currently_used_memory = new_possible_memory_used;
	next_ptr = utils::advance_ptr(next_ptr, need_more);

	memory_allocation_result result
	{ 
		current_memory_block.memory_ptr(),
		required_memory_size,
		alignment,
		memory_allocation_result_types::CONTINUE_CURRENT_BLOCK 
	};
	return result;
}

void
bump_memory_manager::free(memory_block freed_block, const char* file_name, i32 line)
{
	if (!is_owned(freed_block))
	{
		DEBUGGER_BREAK();
		return;
	}

	if (freed_block == last_allocated_block)
	{
		// Alignment size in block size ???
		next_ptr = freed_block.memory_ptr();
		currently_used_memory -= freed_block.memory_size();
		last_allocated_block = {};
	}

	if (MEM_IS_DEFINED(_DEBUG_LOG_ALLOCATIONS))
	{
		//LOG free
	}
};

void
bump_memory_manager::return_memory(memory_manager* top_allocator) {};


struct dap_stack_manager_block_header_t;

struct stack_manager_statistics : memory_manager_statistics
{
	using memory_manager_statistics::memory_manager_statistics;

	dap_stack_manager_block_header_t* next_block;
};

__declspec(align(16))
typedef struct dap_stack_manager_block_header_t
{
	u32 block_pattern = block_pattern;
	u32 block_size = 0;
	dap_stack_manager_block_header_t* previous_block = nullptr;
} dap_stack_manager_block_header_t;

static_assert(sizeof(dap_stack_manager_block_header_t) == 16);

class stack_memory_manager : public memory_manager
{
	constexpr static inline size_t control_block_size = sizeof(dap_stack_manager_block_header_t);
	static inline constexpr u32 block_pattern = 0xDEADBEEF;
	static inline constexpr u32 sentry_pattern = 0xBEEFDEAD;

public:

	explicit stack_memory_manager(memory_resource* resource);

	stack_manager_statistics get_statistics() const;

	memory_allocation_result allocate_aligned(u32 required_memory_size, u16 alignment, const char* file_name, i32 line) override;
	memory_allocation_result reallocate(memory_block block, u32 required_memory_size, const char* file_name, i32 line) override;
	void free(memory_block free_block, const char* file_name, i32 line) override;
	void return_memory(memory_manager* top_allocator) override;

protected:

	dap_stack_manager_block_header_t* last_allocated_control_block = nullptr;
	dap_stack_manager_block_header_t* next_control_block = nullptr;
	size_t currently_used_memory = 0;

	[[nodiscard]] u16 place_next_control_block(mem_ptr from);
};

stack_memory_manager::stack_memory_manager(memory_resource* resource) : memory_manager(resource)
{
	MEM_ASSERT(resource_info.memory_size() > 16);
	size_t needed_more_for_align = utils::get_aligned_distance(resource_info.memory_ptr(), default_alignment);
	mem_ptr next_aligned = utils::advance_ptr(resource_info.memory_ptr(), needed_more_for_align);

	next_control_block = (dap_stack_manager_block_header_t*)next_aligned;
	*next_control_block = {};

	currently_used_memory = needed_more_for_align;
};

stack_manager_statistics
stack_memory_manager::get_statistics() const
{
	stack_manager_statistics stats(assigned_memory_resouce->get_info());
	stats.memory_used = currently_used_memory;
	stats.next_block = next_control_block;
	return stats;
};

memory_allocation_result 
stack_memory_manager::allocate_aligned(u32 required_memory_size, u16 alignment, const char* file_name, i32 line)
{
	if (next_control_block == nullptr)
	{
		DEBUGGER_BREAK();
		return memory_allocation_result{ OUT_OF_MEMORY };
	}

	size_t need_to_have = currently_used_memory + required_memory_size + stack_memory_manager::control_block_size;
	if (resource_info.memory_size() <= need_to_have)
	{
		//DEBUGGER_BREAK();
		return memory_allocation_result{ OUT_OF_MEMORY };
	}

	using header_t = dap_stack_manager_block_header_t;

	header_t* new_block = next_control_block;
	u32 needed_more_for_align = utils::get_aligned_distance_after<header_t>(new_block, alignment);
	bool can_align = needed_more_for_align == 0 || needed_more_for_align >= stack_memory_manager::control_block_size;
	if (!can_align)
	{
		MEM_ASSERT(can_align);
		DEBUGGER_BREAK();
		return memory_allocation_result{ FAIL };
	}

	size_t new_possible_memory_used = need_to_have + needed_more_for_align;
	if (resource_info.memory_size() < new_possible_memory_used)
	{
		DEBUGGER_BREAK();
		return memory_allocation_result{ OUT_OF_MEMORY };
	}
	  
	mem_ptr result_pointer = nullptr;
	if (needed_more_for_align > 0)
	{
		header_t* sentry_block = new_block;
		sentry_block->block_pattern = stack_memory_manager::sentry_pattern;
		sentry_block->block_size = static_cast<u32>(needed_more_for_align);
		sentry_block->previous_block = last_allocated_control_block;
		last_allocated_control_block = sentry_block;
		new_block = utils::advance_ptr<header_t*>(sentry_block, sentry_block->block_size);
		MEM_ASSERT(reinterpret_cast<size_t>(new_block + 1) % alignment == 0);
		//first used block is already counted in
		currently_used_memory += static_cast<u32>(sentry_block->block_size);
	}

	new_block->previous_block = last_allocated_control_block;
	new_block->block_pattern = stack_memory_manager::block_pattern;
	new_block->block_size = static_cast<u32>(required_memory_size) + stack_memory_manager::control_block_size;
	result_pointer = reinterpret_cast<mem_ptr>(new_block + 1);

	MEM_ASSERT(reinterpret_cast<size_t>(result_pointer) % alignment == 0);
	last_allocated_control_block = new_block;

	u16 used_for_next_align = place_next_control_block(utils::advance_ptr(result_pointer, required_memory_size));
	new_block->block_size += used_for_next_align;
	currently_used_memory += new_block->block_size;
	memory_allocation_result result{ result_pointer, required_memory_size, alignment, memory_allocation_result_types::NEW_BLOCK};

	if (MEM_IS_DEFINED(_DEBUG_LOG_ALLOCATIONS))
	{}

	return result;
};

memory_allocation_result 
stack_memory_manager::reallocate(memory_block reallocated_memory_block, u32 required_memory_size, const char* file_name, i32 line)
{
	if (!is_owned(reallocated_memory_block))
	{
		DEBUGGER_BREAK();
		return memory_allocation_result{ WRONG_MANAGER };
	}

	if (reallocated_memory_block.memory_size() >= required_memory_size)
	{
		return memory_allocation_result{ reallocated_memory_block,	memory_allocation_result_types::CURRENT_BLOCK_BIG_ENOUGH };
	}

	using header_t = dap_stack_manager_block_header_t;
	header_t* realloc_block_header = utils::recede_ptr<header_t*>(reallocated_memory_block.memory_ptr(), sizeof(header_t));
	if (stack_memory_manager::sentry_pattern == realloc_block_header->block_pattern)
	{
		return memory_allocation_result{ USE_AFTER_FREE };
	}

	u16 alignment = reallocated_memory_block.alignment();
	size_t size_with_next_block_alignment = realloc_block_header->block_size - sizeof(header_t);
	if (size_with_next_block_alignment >= required_memory_size)
	{
		return memory_allocation_result{ reallocated_memory_block.memory_ptr(), required_memory_size, alignment, CONTINUE_CURRENT_BLOCK };
	}

	if (realloc_block_header != last_allocated_control_block)
	{
		realloc_block_header->block_pattern = stack_memory_manager::sentry_pattern;
		return allocate_aligned(required_memory_size, alignment, file_name, line);
	}
	else
	{
		size_t need_more = required_memory_size - reallocated_memory_block.memory_size();
		size_t new_possible_memory_used = currently_used_memory + need_more;
		if (resource_info.memory_size() < new_possible_memory_used)
		{
			return memory_allocation_result{ OUT_OF_MEMORY };
		}

		realloc_block_header->block_size += static_cast<u32>(need_more);
		currently_used_memory += need_more;

		memory_allocation_result result
		{
			reallocated_memory_block.memory_ptr(),
			required_memory_size,
			alignment,
			memory_allocation_result_types::CONTINUE_CURRENT_BLOCK
		};

		u16 used_for_next_align = place_next_control_block(utils::advance_ptr(reallocated_memory_block.memory_ptr(), required_memory_size));
		realloc_block_header->block_size += used_for_next_align;
		currently_used_memory += used_for_next_align;

		return result;
	}
}

[[nodiscard]]
u16
stack_memory_manager::place_next_control_block(mem_ptr from)
{
	bool can_place_new_block = resource_info.memory_size() > (currently_used_memory + stack_memory_manager::control_block_size * 2);
	u32 needed_more_for_align = 0;
	if (!can_place_new_block)
	{
		next_control_block = nullptr;
	}
	else
	{
		needed_more_for_align = utils::get_aligned_distance(from, default_alignment);
		next_control_block = utils::advance_ptr<dap_stack_manager_block_header_t*>(from, needed_more_for_align);
	}
	return needed_more_for_align;
}

void 
stack_memory_manager::free(memory_block freed_block, const char* file_name, i32 line)
{
	if (!is_owned(freed_block))
	{
		DEBUGGER_BREAK();
		return;
	}

	using header_t = dap_stack_manager_block_header_t;
	header_t* freed_block_header = utils::recede_ptr<header_t*>(freed_block.memory_ptr(), sizeof(header_t));

	if (block_pattern != freed_block_header->block_pattern)
	{
		DEBUGGER_BREAK();
		return;
	}

	// handle sentry blocks
	constexpr size_t header_size = sizeof(header_t);
	bool is_last_block = freed_block_header == last_allocated_control_block;
	// TODO:: Cleanup required
	if (is_last_block)
	{
		MEM_ASSERT(currently_used_memory > freed_block_header->block_size);
		currently_used_memory -= freed_block_header->block_size;
		last_allocated_control_block = freed_block_header->previous_block;

		if (last_allocated_control_block && last_allocated_control_block->block_pattern == sentry_pattern)
		{
			i32 fall_counter = 0;
			header_t* next_block = freed_block_header;
			// Fallthrough sentry blocks, reducing memory used
			while (last_allocated_control_block && last_allocated_control_block->block_pattern == sentry_pattern && fall_counter++ < 100)
			{
				MEM_ASSERT(currently_used_memory > last_allocated_control_block->block_size);
				currently_used_memory -= last_allocated_control_block->block_size;

				next_block = last_allocated_control_block;
				last_allocated_control_block = last_allocated_control_block->previous_block;
			}

			if (fall_counter > 5)
			{
				DEBUGGER_BREAK();
				// Log bad placing
			}
			next_control_block = next_block;
		}
		else
		{
			next_control_block = freed_block_header;
		}

		next_control_block->block_pattern = block_pattern;

		next_control_block->block_size = header_size;
	}
	else
	{
		freed_block_header->block_pattern = sentry_pattern;
	}

	if (MEM_IS_DEFINED(_DEBUG_LOG_ALLOCATIONS))
	{
		//LOG free
	}
};

void 
stack_memory_manager::return_memory(memory_manager* top_allocator)  {};


struct scoped_manager_statistics : memory_manager_statistics
{
	using memory_manager_statistics::memory_manager_statistics;
};

class scoped_memory_manager : public memory_manager
{

public:

	explicit scoped_memory_manager(memory_resource* resource);

	//memory_allocation_result allocate_aligned(u32 required_memory_size, u16 alignment, const char* file_name, i32 line) override;
	//memory_allocation_result reallocate(memory_block block, u32 required_memory_size, const char* file_name, i32 line) override;
	//void free(memory_block free_block, const char* file_name, i32 line) override;
	//void return_memory(memory_manager* top_allocator) override;

	scoped_manager_statistics get_statistics() const;

protected:

	size_t currently_used_memory = 0;

	[[nodiscard]] u16 place_next_control_block(mem_ptr from);
};

//void stack_allocator::restart(const char* file_name, i32 line)
//{
//	currently_used_memory = 0;
//	next_free_memory_ptr = initial_memory_ptr;
//}
//
//void stack_allocator::clear_and_restart(const char* file_name, i32 line)
//{
//	memset((u8*)initial_memory_ptr, 0x00, currently_used_memory);
//	restart(file_name, line);
//}

//class general_memory_manager : public memory_manager
//{
//
//};

}

//###### Stack allocator ######

}
