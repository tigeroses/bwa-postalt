
#pragma once

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>

namespace postalt {
    /**
     * @enum the status of buffer
     */
    enum BufferStatus {
        Free,
        Occupied,
        Processing,
        Finish
    };

    /**
     * @brief A class for manipulating workflows
     */
    class Workflow {
    
    public:
        /**
         * @brief Creats a new workflow
         * @param worker_num the number of worker threads
         * @param buffer_size the size of single buffer for storing data
         */
        Workflow(int worker_num, int buffer_size);

        /**
         * @brief Run the main progress
         * @return true if everything ok
         */
        bool run();

    private:
        /**
         * @brief Generate data
         * @param data_num the number of test data
         */
        void producer(int data_num);

        /**
         * @brief Process data
         * @param id the index of buffer list
         * @return true if everything ok
         */
        bool consumer(int id);

        /**
         * @brief Output result in order
         */
        void output_in_order();
    
    private:
        int m_worker_num;   ///< the number of worker threads
        int m_buffer_size;  ///< the size of single buffer for storing data
        float m_growth_factor; ///< the actual number of buffers relative to worker number
        int m_buffer_num;   ///< the number of buffers

        std::vector<std::vector<std::string>> m_raw_data;   ///< the buffers for storing raw data
        std::vector<std::string> m_res_data;    ///< the buffers for storing result data after processing

        std::vector<BufferStatus> m_buffer_status;   ///< the buffer status
        std::queue<int> m_process_orders;   ///< the buffer orders for processing
        std::queue<int> m_dump_orders;    ///< the buffer orders for writing to disk
        std::mutex m_process_mutex, m_dump_mutex;

        std::atomic<bool> m_producer_finished;  ///< true: the signal of producer finished
        std::atomic<bool> m_consumer_finished;  ///< true: the signal of consumer finished
    };
}