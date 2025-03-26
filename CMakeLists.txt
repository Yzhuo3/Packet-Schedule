cmake_minimum_required(VERSION 3.10)
project(PacketSchedule)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加所有源码文件
add_executable(PacketSchedule
    packet_schedule/packet_schedule.cpp
    packet_schedule/include/TrafficSource.hpp
        packet_schedule/include/SimulationEngine.hpp
        packet_schedule/include/Node.hpp
        packet_schedule/include/Event.hpp
        packet_schedule/include/Packet.hpp
        packet_schedule/include/Queue.hpp
        packet_schedule/include/TrafficSource.hpp
        packet_schedule/include/Stats.hpp 
        packet_schedule/service/Event.cpp
        packet_schedule/service/Node.cpp
        packet_schedule/service/Packet.cpp
        packet_schedule/service/Queue.cpp
        packet_schedule/service/SimulationEngine.cpp
        packet_schedule/service/TrafficSource.cpp
        packet_schedule/service/Stats.cpp 
)