#include <regex>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <unordered_map>

enum class in_type : uint8_t {
    UNARY  = 1,
    BINARY = 2,
    MULTI  = UINT8_MAX
};

/* Type representing an ordinal of a signal. */
using sig_t = int32_t;

/* Type representing a gate name and type of input. */
using gate_t = std::pair<std::string, in_type>;

/* Type representing a gate name and a collection of input signals. */
using gate_input = std::pair<std::string, std::vector<sig_t>>;

/* Type representing a graph-structured logic gate system. */
using gate_graph = std::unordered_map<sig_t, gate_input>;

std::regex get_signal_regex(in_type type) {
    const std::string global_pattern{("( [1-9]\\d{0,8})")};

    if (type == in_type::MULTI)
        return std::regex(global_pattern + "{3,}");

    const auto ordinal{static_cast<int8_t>(type)};
    const auto repetitions{std::to_string(1 + ordinal)};

    return std::regex(global_pattern + "{" + repetitions + "}");
}

static auto regex_matcher() {
    return [](const gate_t& gate) {
        return std::make_pair(gate.first,get_signal_regex(gate.second));
    };
}

int main() {
    // TODO: create macro make_gates
    const std::vector<gate_t> gates{{
            {"NOT", in_type::UNARY},
            {"XOR", in_type::BINARY},
            {"AND", in_type::MULTI},
            {"NAND", in_type::MULTI},
            {"OR", in_type::MULTI},
            {"NOR", in_type::MULTI}
    }};


    std::unordered_map<std::string, std::regex> sig_regex;
    std::transform(begin(gates), end(gates),
                   std::inserter(sig_regex, end(sig_regex)),
                   regex_matcher());


    return EXIT_SUCCESS;
}