#include "http_router/problem_router.hpp"

#include "http_router/problem_route_groups.hpp"

#include <array>
#include <vector>

namespace{
    using endpoint_descriptor = problem_route_groups::endpoint_descriptor;

    std::vector<endpoint_descriptor> make_problem_route_table(){
        const auto query_routes = problem_route_groups::query_routes();
        const auto command_routes = problem_route_groups::command_routes();
        const auto testcase_routes = problem_route_groups::testcase_routes();

        std::vector<endpoint_descriptor> route_table;
        route_table.reserve(21);

        route_table.push_back(query_routes[0]);
        route_table.push_back(command_routes[0]);
        route_table.push_back(query_routes[1]);
        route_table.push_back(command_routes[1]);
        route_table.push_back(command_routes[2]);
        route_table.push_back(query_routes[2]);
        route_table.push_back(command_routes[3]);
        route_table.push_back(command_routes[4]);
        route_table.push_back(query_routes[3]);
        route_table.push_back(command_routes[5]);
        route_table.push_back(command_routes[6]);
        route_table.push_back(command_routes[7]);
        route_table.insert(
            route_table.end(),
            testcase_routes.begin(),
            testcase_routes.end()
        );
        route_table.push_back(command_routes[8]);

        return route_table;
    }

    const auto& problem_route_table(){
        static const auto route_table = make_problem_route_table();
        return route_table;
    }
}

problem_router::response_type problem_router::route(
    context_type& context,
    std::string_view path
){
    return http_route::dispatch_route_table(
        context,
        path,
        std::span<const endpoint_descriptor>{problem_route_table()}
    );
}
