Right now is more of a draft-in-progress, than done project with motivation being
1. Fun of writing allocators(no comments here)
2. Ability to have more precise control of memory allocation strategy, when needed.
3. Improve my currently used implementations.

Answering more general question WHY? 

1. Std::pmr containers looks good, gut still more of a crutch to not so lucky std::allocator model, but maybe i will work on supporting them
2. With all of infrastructure in place I personally think it will be easier to fix memory related problems if(when) they arise.

Going on is more of a plans/about section

Memory Resource Manager --> Memory Resource --> Memory Manager

1. Memory System. Have access to all memory_managers 
 - OS Memory Manager - Gets memory from OS, Reserves, Commits and frees memory 
 -- Abstract-Memory-Manager - Provides access for memory allocation to containers and classes, have top owning OS memory manager. 
 -- Most? containers should work with Abstract-Memory-Manager as main memory provider, but some funny ones i.e. Ring Buffer may work with OS mem_manger.


2. Memory_manager instead of allocator so no confusion with std type bs.
--Support for address sanitizer
- Default_memory_manager
- General_memory_manager - mix and mash, free_list + buckets or maybe just use mimalloc over provided memory ->
- Bucketed_memory_manager
- Bump_memory_manager - Done
- Stack_memory_manager - Done
- Scoped_memory_manager - WIP 
- Tagged_memory_manager ?
-- Debug_memory_manager_wrapper<Linear_memory_manager> ??

Default memory manager must be thin wrapper over new and delete.
All memory managers must be wrappable in debug_memory_manager_wrapper for logging and other features i.e. changing OS protection for use-after-free detection
Memory Manager must allow this set of operations
- Allocates
- Free
- Reallocate

Allocate return result 

Allocation result
Success
Fail-no-memory
Fail-not-enough-memory

Reallocate(current ptr, new size, policy)
Some mechanism for returning part of used memory is needed, but only for general memory manager
Vectors try to grow based on size, bigger the size, smaller the factor of increase, or make it templated policy
-AlwaysDouble
-InverseSize

Either not nullptr to memory + size
Or nullptr on no mem
Same for reallocation
But reallocation can be multi-step process, or different calls, or parametrized?
ReallocateExact
ReallocateGetNoLess
Or
Policy - 
GetExact
GetNoLess

