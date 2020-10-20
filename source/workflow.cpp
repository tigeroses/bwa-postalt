
#include "postalt/workflow.h"
#include "postalt/threadpool.h"
#include "postalt/timer.h"

#include <thread>
#include <chrono>
#include <functional>
#include <iostream>

#include <spdlog/spdlog.h>

using namespace postalt;

AccessBufferStatus::AccessBufferStatus()
{
}

AccessBufferStatus::AccessBufferStatus(int num) :
    m_num(num)
{
    m_buffs.resize(m_num, BufferStatus::Free);
    // m_mutexs.resize(m_num);
    for (int i = 0; i < m_num; ++i)
    {
        std::unique_ptr<std::mutex> m(new std::mutex);
        m_mutexs.push_back(std::move(m));
    }
}

BufferStatus AccessBufferStatus::get(int idx)
{
    if (!check(idx))
    {
        throw std::runtime_error("Invalid buffer id for get(): " + std::to_string(idx));
    }
    std::lock_guard<std::mutex> g(*(m_mutexs[idx]));
    return m_buffs[idx];
}

void AccessBufferStatus::set(int idx, BufferStatus status)
{
    if (!check(idx))
    {
        throw std::runtime_error("Invalid buffer id for set(): " + std::to_string(idx));
    }
    std::lock_guard<std::mutex> g(*(m_mutexs[idx]));
    m_buffs[idx] = status;
}

bool AccessBufferStatus::check(int idx)
{
    return (idx >= 0 && idx < m_num);
}

Workflow::Workflow(int worker_num, int buffer_size) : 
    m_worker_num(worker_num),
    m_buffer_size(buffer_size),
    m_growth_factor(1.2),
    m_producer_finished(false),
    m_consumer_finished(false)
{
    m_buffer_num = int(m_worker_num * m_growth_factor);

    m_raw_data.resize(m_buffer_num);
    m_res_data.resize(m_buffer_num);
    
    m_buffer_status = new AccessBufferStatus(m_buffer_num);
}

Workflow::~Workflow()
{
    if (m_buffer_status != nullptr)
    {
        delete m_buffer_status;
        m_buffer_status = nullptr;
    }
}

bool Workflow::run(std::string alt_file)
{
    m_postalt = new Postalt(alt_file);

    std::thread read_thread(&Workflow::producer, this);

    std::thread write_thread(&Workflow::output_in_order, this);

    // std::vector<std::thread> process_threads;
    // for (int i = 0; i < data_num; ++i)
    //     process_threads.emplace_back(std::thread(&Workflow::consumer, this, i));
    std::threadpool executor { (unsigned short)m_worker_num };
    std::vector<std::future<bool>> results;
    while (true)
    {
        if (m_producer_finished)
        {
            // Producer finished, let us check the last time
            while (!m_process_orders.empty())
            {
                int index = m_process_orders.front();
                m_process_orders.pop();

                // Add the new task to threadpool
                results.emplace_back(executor.commit(std::bind(&Workflow::consumer, this, index)));

                // Keep the dump order
                m_dump_mutex.lock();
                m_dump_orders.push(index);
                m_dump_mutex.unlock();
            }
            m_consumer_finished = true;
            // std::cout<<"consumer finished"<<std::endl;
            break;
        }
        // Check is there a new process task?
        m_process_mutex.lock();
        if (m_process_orders.empty())
        {
            m_process_mutex.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        int index = m_process_orders.front();
        m_process_orders.pop();
        m_process_mutex.unlock();

        // Add the new task to threadpool
        results.emplace_back(executor.commit(std::bind(&Workflow::consumer, this, index)));

        // Keep the dump order
        m_dump_mutex.lock();
        m_dump_orders.push(index);
        m_dump_mutex.unlock();
    }
    // Make sure all tasks in threadpool had finished
    for (auto& result : results)
    {
        result.get();
    }

    read_thread.join();
    
    // for (auto& t : process_threads)
    //     t.join();
    write_thread.join();
    
    delete m_postalt;

    return true;
}

void Workflow::producer()
{
    Timer timer;
    constexpr int line_size = 1024;
    char buff[line_size];
    int lines = 0;
    // Initialize the initial index using the first buffer index, it is must be empty
    int curr_index = 0;
    m_buffer_status->set(curr_index, BufferStatus::Occupied);
    while (NULL != fgets(buff, line_size, stdin))
    {
        // // std::cerr<<lines<<" "<<m_buffer_size<<std::endl;
        if (lines == m_buffer_size)
        {
            // The buffer is full, begin a new process task and find another empty buffer
            m_process_mutex.lock();
            m_process_orders.push(curr_index);
            m_process_mutex.unlock();

            spdlog::info("producer id: {} time(ms): {}", curr_index, timer.toc());

            while (true)
            {
                int index = 0;
                while (index < m_buffer_num)
                {
                    if (m_buffer_status->get(index) == BufferStatus::Free)
                    {
                        curr_index = index;
                        break;
                    }
                    ++index;
                }
                // No free buffer to use, wait
                if (index == m_buffer_num)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
                else
                    break;
            }
            m_buffer_status->set(curr_index, BufferStatus::Occupied);
            
            // std::cerr<<"next producer index: "<<curr_index<<std::endl;
            lines = 0;
        }
        ++lines;
        // Add the new line to buffer
        m_raw_data[curr_index].emplace_back(buff);
    }
    // Do not forget the last block
    m_process_mutex.lock();
    m_process_orders.push(curr_index);
    m_process_mutex.unlock();
    
    m_producer_finished = true;
    spdlog::info("producer id: {} time(ms): {}", curr_index, timer.toc());
    // std::cerr<<"producer finished"<<std::endl;
    return;
}

bool Workflow::consumer(int id)
{
    // std::cout<<"consumer id: "<<id<<std::endl;
    // std::cerr <<"consumer index: "<<id<<std::endl;
    // Simulate CPU operation
    // for (int i = 0; i < 100000000; ++i)
    //     ;
    // std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    if (m_buffer_status->get(id) == BufferStatus::Occupied)
    {
        m_buffer_status->set(id, BufferStatus::Processing);
        auto& input = m_raw_data[id];
        auto& output = m_res_data[id];
        // Process Sam
        Timer timer;
        m_postalt->run(input, output);
        spdlog::info("consumer id: {} time(ms): {}", id, timer.toc());
        // for (auto& line : input)
        // {
        //     output += line;
        // }
        m_buffer_status->set(id, BufferStatus::Finish);
    }
    
    return true;
}

void Workflow::output_in_order()
{
    while (true)
    {
        if (m_consumer_finished)
        {
            // Consumer finished, let us check the last time
            while (!m_dump_orders.empty())
            {
                int index = m_dump_orders.front();
                m_dump_orders.pop();
                // std::cerr <<"dump index: "<<index<<std::endl;
                // Dump the result in order
                while (m_buffer_status->get(index) != BufferStatus::Finish)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                Timer timer;
                auto& output = m_res_data[index];
                std::cout<<output;
                spdlog::info("output   id: {} time(ms): {}", index, timer.toc());
                // Clear the raw data and result data after dumping
                output.clear();
                m_raw_data[index].clear();
                m_buffer_status->set(index, BufferStatus::Free);
            }
            break;
        }
        // Check is there a new dump task?
        m_dump_mutex.lock();
        if (m_dump_orders.empty())
        {
            m_dump_mutex.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        int index = m_dump_orders.front();
        m_dump_orders.pop();
        m_dump_mutex.unlock();
        // std::cerr <<"dump index: "<<index<<std::endl;
        // Dump the result in order
        while (m_buffer_status->get(index) != BufferStatus::Finish)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        Timer timer;
        auto& output = m_res_data[index];
        std::cout<<output;

        spdlog::info("output   id: {} time(ms): {}", index, timer.toc());
        
        // Clear the raw data and result data after dumping
        output.clear();
        m_raw_data[index].clear();
        m_buffer_status->set(index, BufferStatus::Free);
    }
    return;
}
