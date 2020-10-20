
#pragma once

#include "postalt/postalt.h"

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <memory>

namespace postalt {
    /**
     * @enum the status of buffer
     */
    enum BufferStatus {
        Free = 0,
        Occupied,
        Processing,
        Finish
    };

    /**
     * @brief A class for access the status of buffer, the mode is multi-read single-write
     */
    class AccessBufferStatus
    {
    public:
        AccessBufferStatus();
        AccessBufferStatus(int num);
        BufferStatus get(int idx);
        void set(int idx, BufferStatus status);

    private:
        bool check(int idx);

    private:
        int m_num;
        std::vector<BufferStatus> m_buffs;
        std::vector<std::unique_ptr<std::mutex>> m_mutexs;
    };

    /**
     * @brief A class for manipulating workflows
     */
    class Workflow {
    
    public:
        /**
         * @brief Creats a new workflow
         * @param worker_num the number of worker threads
         * @param buffer_size the line number of single buffer for storing data
         */
        Workflow(int worker_num, int buffer_size);

        /**
         * @brief Clean the class
         */
        ~Workflow();

        /**
         * @brief Run the main progress
         * @param alt_file the name to alt file
         * @return true if everything ok
         */
        bool run(std::string alt_file);

    private:
        /**
         * @brief Generate data
         */
        void producer();

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

        AccessBufferStatus* m_buffer_status;   ///< the buffer status
        std::queue<int> m_process_orders;   ///< the buffer orders for processing
        std::queue<int> m_dump_orders;    ///< the buffer orders for writing to disk
        std::mutex m_process_mutex, m_dump_mutex;

        std::atomic<bool> m_producer_finished;  ///< true: the signal of producer finished
        std::atomic<bool> m_consumer_finished;  ///< true: the signal of consumer finished

        Postalt* m_postalt;
    };
}