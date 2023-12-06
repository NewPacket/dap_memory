#pragma once

//#import "memory_resource.h"

namespace dap
{

namespace memory
{

class memory_resource_manager
{

public:

	[[nodiscard]]
	memory_resource request_memory_from_os() { return memory_resource{}; };

	void return_memory_to_os() {};

	void change_protection() {};

	void grow_memory() {};

protected:

	
};

}

}