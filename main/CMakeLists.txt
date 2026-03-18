function(online_judge_add_executable target_name source_file)
    add_executable(${target_name}
        ${source_file}
    )

    target_link_libraries(${target_name}
        PRIVATE
            lib
    )
endfunction()

online_judge_add_executable(http_server http_server.cpp)
online_judge_add_executable(judge_server judge_server.cpp)
