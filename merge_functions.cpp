#include "precomp.h"
#include "merge_functions.h"

namespace Tmpl8
{
	namespace Merge_Functions {

        // Merge sort that sorts array[start..end]
        void Merge_Functions::merge_sort_tanks_by_health(std::vector<Tank*>& sorted_tanks, int start, int end, std::atomic<int>& threads, ThreadPool& thread_pool)
        {

            if (start < end)
            {
                // find the middle point
                int middle = (start + end) / 2;

                if (threads >= 1)
                {
                    threads--;

                    future<void> future_left = thread_pool.enqueue([&]
                        {
                            merge_sort_tanks_by_health(sorted_tanks, start, middle, threads, thread_pool);
                        });

                    merge_sort_tanks_by_health(sorted_tanks, middle + 1, end, threads, thread_pool);

                    future_left.wait();

                    threads++;

                    merge(sorted_tanks, start, middle, end);
                }

                else
                {
                    merge_sort_tanks_by_health(sorted_tanks, start, middle, threads, thread_pool);
                    merge_sort_tanks_by_health(sorted_tanks, middle + 1, end, threads, thread_pool);

                    merge(sorted_tanks, start, middle, end);
                }
            }

        }

	}
}