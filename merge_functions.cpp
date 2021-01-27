#include "precomp.h"
#include "merge_functions.h"

namespace Tmpl8
{
	namespace Merge_Functions {

        // Merges two subvectors of vector.
        void merge_tanks_by_health(std::vector<Tank*>& sorted_tanks, int start, int middle, int end)
        {
            int save_value = ((middle - start) + 1);
            int save_value2 = (end - middle);
            std::vector<Tank*> left_vector(save_value);
            std::vector<Tank*> right_vector(save_value2);

            // fill in left vector
            for (int i = 0; i < left_vector.size(); ++i)
            {
                save_value = start + i;
                left_vector[i] = sorted_tanks[save_value];
            }

            // fill in right vector
            for (int i = 0; i < right_vector.size(); ++i)
            {
                save_value = (middle + 1 + i);
                right_vector[i] = sorted_tanks[save_value];
            }

            /* Merge the temp vectors */

            // initial indexes of first and second subvector
            int left_index = 0, right_index = 0;

            // the index we will start at when adding the subvector back into the main vector
            int current_index = start;

            // compare each index of the subvector adding the lowest value to the currentIndex
            while (left_index < left_vector.size() && right_index < right_vector.size())
            {

                if (left_vector[left_index]->health <= right_vector[right_index]->health)
                {
                    sorted_tanks[current_index] = left_vector[left_index];
                    left_index++;
                }

                else
                {
                    sorted_tanks[current_index] = right_vector[right_index];
                    right_index++;
                }

                current_index++;
            }

            // copy remaining elements of left_vector if any
            while (left_index < left_vector.size())
                sorted_tanks[current_index++] = left_vector[left_index++];

            // copy remaining elements of right_vector if any
            while (right_index < right_vector.size())
                sorted_tanks[current_index++] = right_vector[right_index++];
        }


        // Merge sort that sorts a tank array[start..end] by health from low to high
        void Merge_Functions::sort_tanks_by_health(std::vector<Tank*>& sorted_tanks, int start, int end, std::atomic<int>& threads, ThreadPool& thread_pool)
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
                            sort_tanks_by_health(sorted_tanks, start, middle, threads, thread_pool);
                        });

                    sort_tanks_by_health(sorted_tanks, middle + 1, end, threads, thread_pool);

                    future_left.wait();

                    threads++;

                    merge_tanks_by_health(sorted_tanks, start, middle, end);
                }

                else
                {
                    sort_tanks_by_health(sorted_tanks, start, middle, threads, thread_pool);
                    sort_tanks_by_health(sorted_tanks, middle + 1, end, threads, thread_pool);

                    merge_tanks_by_health(sorted_tanks, start, middle, end);
                }
            }

        }


        // Merge sorted array by tank x from left to right
        void merge_tanks_by_x(std::vector<Tank*>& sorted_tanks, int start, int middle, int end)
        {
            int save_value = ((middle - start) + 1);
            int save_value2 = (end - middle);
            std::vector<Tank*> left_vector(save_value);
            std::vector<Tank*> right_vector(save_value2);

            // fill in left vector
            for (int i = 0; i < left_vector.size(); ++i)
            {
                save_value = start + i;
                left_vector[i] = sorted_tanks[save_value];
            }

            // fill in right vector
            for (int i = 0; i < right_vector.size(); ++i)
            {
                save_value = (middle + 1 + i);
                right_vector[i] = sorted_tanks[save_value];
            }

            /* Merge the temp vectors */

            // initial indexes of first and second subvector
            int left_index = 0, right_index = 0;

            // the index we will start at when adding the subvector back into the main vector
            int current_index = start;

            // compare each index of the subvector adding the lowest value to the currentIndex
            while (left_index < left_vector.size() && right_index < right_vector.size())
            {

                if (left_vector[left_index]->get_position().x <= right_vector[right_index]->get_position().x)
                {
                    sorted_tanks[current_index] = left_vector[left_index];
                    left_index++;
                }

                else
                {
                    sorted_tanks[current_index] = right_vector[right_index];
                    right_index++;
                }

                current_index++;
            }

            // copy remaining elements of left_vector if any
            while (left_index < left_vector.size())
                sorted_tanks[current_index++] = left_vector[left_index++];

            // copy remaining elements of right_vector if any
            while (right_index < right_vector.size())
                sorted_tanks[current_index++] = right_vector[right_index++];
        }


        // Sort tanks by x value from left to right
        void Merge_Functions::sort_tanks_by_x(std::vector<Tank*>& sorted_tanks, int start, int end, std::atomic<int>& threads, ThreadPool& thread_pool)
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
                            sort_tanks_by_x(sorted_tanks, start, middle, threads, thread_pool);
                        });

                    sort_tanks_by_x(sorted_tanks, middle + 1, end, threads, thread_pool);

                    future_left.wait();

                    threads++;

                    merge_tanks_by_x(sorted_tanks, start, middle, end);
                }

                else
                {
                    sort_tanks_by_x(sorted_tanks, start, middle, threads, thread_pool);
                    sort_tanks_by_x(sorted_tanks, middle + 1, end, threads, thread_pool);

                    merge_tanks_by_x(sorted_tanks, start, middle, end);
                }
            }

        }

        // Merge sorted array by rocket x from left to right
        void merge_rockets_by_x(std::vector<Rocket*>& sorted_rockets, int start, int middle, int end)
        {
            int save_value = ((middle - start) + 1);
            int save_value2 = (end - middle);
            std::vector<Rocket*> left_vector(save_value);
            std::vector<Rocket*> right_vector(save_value2);

            // fill in left vector
            for (int i = 0; i < left_vector.size(); ++i)
            {
                save_value = start + i;
                left_vector[i] = sorted_rockets[save_value];
            }

            // fill in right vector
            for (int i = 0; i < right_vector.size(); ++i)
            {
                save_value = (middle + 1 + i);
                right_vector[i] = sorted_rockets[save_value];
            }

            /* Merge the temp vectors */

            // initial indexes of first and second subvector
            int left_index = 0, right_index = 0;

            // the index we will start at when adding the subvector back into the main vector
            int current_index = start;

            // compare each index of the subvector adding the lowest value to the currentIndex
            while (left_index < left_vector.size() && right_index < right_vector.size())
            {

                if (left_vector[left_index]->position.x <= right_vector[right_index]->position.x)
                {
                    sorted_rockets[current_index] = left_vector[left_index];
                    left_index++;
                }

                else
                {
                    sorted_rockets[current_index] = right_vector[right_index];
                    right_index++;
                }

                current_index++;
            }

            // copy remaining elements of left_vector if any
            while (left_index < left_vector.size())
                sorted_rockets[current_index++] = left_vector[left_index++];

            // copy remaining elements of right_vector if any
            while (right_index < right_vector.size())
                sorted_rockets[current_index++] = right_vector[right_index++];
        }


        // Sort rockets by x value from left to right
        void Merge_Functions::sort_rockets_by_x(std::vector<Rocket*>& sorted_rockets, int start, int end, std::atomic<int>& threads, ThreadPool& thread_pool)
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
                            sort_rockets_by_x(sorted_rockets, start, middle, threads, thread_pool);
                        });

                    sort_rockets_by_x(sorted_rockets, middle + 1, end, threads, thread_pool);

                    future_left.wait();

                    threads++;

                    merge_rockets_by_x(sorted_rockets, start, middle, end);
                }

                else
                {
                    sort_rockets_by_x(sorted_rockets, start, middle, threads, thread_pool);
                    sort_rockets_by_x(sorted_rockets, middle + 1, end, threads, thread_pool);

                    merge_rockets_by_x(sorted_rockets, start, middle, end);
                }
            }

        }


        // Merge sorted array by smoke y from top to bottom (low to high y)
        void merge_smokes_by_y(std::vector<Smoke*>& sorted_smokes, int start, int middle, int end)
        {
            int save_value = ((middle - start) + 1);
            int save_value2 = (end - middle);
            std::vector<Smoke*> left_vector(save_value);
            std::vector<Smoke*> right_vector(save_value2);

            // fill in left vector
            for (int i = 0; i < left_vector.size(); ++i)
            {
                save_value = start + i;
                left_vector[i] = sorted_smokes[save_value];
            }

            // fill in right vector
            for (int i = 0; i < right_vector.size(); ++i)
            {
                save_value = (middle + 1 + i);
                right_vector[i] = sorted_smokes[save_value];
            }

            /* Merge the temp vectors */

            // initial indexes of first and second subvector
            int left_index = 0, right_index = 0;

            // the index we will start at when adding the subvector back into the main vector
            int current_index = start;

            // compare each index of the subvector adding the lowest value to the currentIndex
            while (left_index < left_vector.size() && right_index < right_vector.size())
            {

                if (left_vector[left_index]->position.y <= right_vector[right_index]->position.y)
                {
                    sorted_smokes[current_index] = left_vector[left_index];
                    left_index++;
                }

                else
                {
                    sorted_smokes[current_index] = right_vector[right_index];
                    right_index++;
                }

                current_index++;
            }

            // copy remaining elements of left_vector if any
            while (left_index < left_vector.size())
                sorted_smokes[current_index++] = left_vector[left_index++];

            // copy remaining elements of right_vector if any
            while (right_index < right_vector.size())
                sorted_smokes[current_index++] = right_vector[right_index++];
        }

        // Sort smokes by y value from top to bottom (low to high)
        void Merge_Functions::sort_smokes_by_y(std::vector<Smoke*>& sorted_smokes, int start, int end, std::atomic<int>& threads, ThreadPool& thread_pool)
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
                            sort_smokes_by_y(sorted_smokes, start, middle, threads, thread_pool);
                        });

                    sort_smokes_by_y(sorted_smokes, middle + 1, end, threads, thread_pool);

                    future_left.wait();

                    threads++;

                    merge_smokes_by_y(sorted_smokes, start, middle, end);
                }

                else
                {
                    sort_smokes_by_y(sorted_smokes, start, middle, threads, thread_pool);
                    sort_smokes_by_y(sorted_smokes, middle + 1, end, threads, thread_pool);

                    merge_smokes_by_y(sorted_smokes, start, middle, end);
                }
            }

        }


	}
}