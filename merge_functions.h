#pragma once

namespace Tmpl8
{
	namespace Merge_Functions {

		void Merge_Functions::merge_sort_tanks_by_health(std::vector<Tank*>& sorted_tanks, int start, int end, std::atomic<int>& threads, ThreadPool& thread_pool);
	}
}

