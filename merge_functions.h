#pragma once

namespace Tmpl8
{
	namespace Merge_Functions 
	{

		void sort_tanks_by_health(std::vector<Tank*>& sorted_tanks, int start, int end, std::atomic<int>& threads, ThreadPool& thread_pool);

		void sort_tanks_by_x(std::vector<Tank*>& sorted_tanks, int start, int end, std::atomic<int>& threads, ThreadPool& thread_pool);

		void sort_rockets_by_x(std::vector<Rocket*>& sorted_rockets, int start, int end, std::atomic<int>& threads, ThreadPool& thread_pool);

		void sort_smokes_by_y(std::vector<Smoke*>& sorted_smokes, int start, int end, std::atomic<int>& threads, ThreadPool& thread_pool);
	}
}

