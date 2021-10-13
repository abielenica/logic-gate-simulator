#include <iostream>
#include <numeric>
#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <list>

namespace {
    enum class in_type : uint8_t {
        UNARY = 1,
        BINARY = 2,
        MULTI = UINT8_MAX
    };

/* Type representing an ordinal of a signal. */
    using sig_t = int32_t;

/* Type representing a gate name and type of input. */
    using gate_t = std::pair<std::string, in_type>;

/* Type representing a gate name and a collection of input signals. */
    using gate_input = std::pair<std::string, std::vector<sig_t>>;

/* Type representing a graph-structured logic gate system. */
    using gate_graph = std::unordered_map<sig_t, gate_input>;

    namespace error {
        void print_invalid_parsing_message(uint64_t line, const std::string &info) {
            std::cerr << "Error in line " << line << ": " << info << std::endl;
        }

        void print_repetitive_output_message(uint16_t line, sig_t signal) {
            std::cerr << "Error in line " << line << ": " << "signal " << signal
                      << " is assigned to multiple outputs." << std::endl;
        }

        void print_circuit_cycle_message() {
            std::cerr << "Error: sequential logic analysis "
                      << "has not yet been implemented." << std::endl;
        }
    }

    std::regex make_sig_regex(in_type type) {
        const std::string sig_pattern{("( [1-9]\\d{0,8})")};

        if (type == in_type::MULTI)
            return std::regex(sig_pattern + "{3,}");

        auto ordinal{static_cast<int8_t>(type)};
        auto repetitions{std::to_string(1 + ordinal)};

        return std::regex(sig_pattern + "{" + repetitions + "}");
    }

    std::pair<std::string, std::string> split_by_first_space(std::string &src) {
        const auto delim_pos{src.find_first_of(' ')};

        if (delim_pos == std::string::npos)
            return {src, std::string()};

        auto first{src.substr(0, delim_pos)};
        auto second{src.substr(delim_pos, src.size() - delim_pos)};

        return {first, second};
    }

    auto regex_matcher() {
        return [](const gate_t &gate) {
            return std::make_pair(gate.first, make_sig_regex(gate.second));
        };
    }

    std::vector<sig_t> order_graph_by_input(const gate_graph &graph) {
        return {}; // TODO: topological sort and check for cycle
    }

    std::pair<std::vector<sig_t>, sig_t> parse_signals(const std::string &signals) {
        std::istringstream sigstream{signals};

        sig_t output;
        std::vector<sig_t> input;

        for (sig_t signal; sigstream >> signal;) {
            if (input.empty()) {
                output = signal;
            } else {
                input.push_back(signal);
            }
        }

        return {input, output};
    }


    size_t count_input_signals(const gate_graph &circuit,
                               const std::vector<sig_t> &signals) {
        size_t i = 0;
        while (circuit.at(signals[i]).first == "NONE")
            i++;
        return i;
    }

    bool compute_single_gate(const gate_input &gate,
                             std::map<sig_t, bool> &signal_values) {
        std::vector<bool> gate_input_signals;
        for (int i: gate.second) {
            gate_input_signals.push_back(signal_values.at(i));
        }
        if (gate.first == "NOT") {
            return !gate_input_signals[0];
        } else if (gate.first == "XOR") {
            return (gate_input_signals[0] != gate_input_signals[1]);
        } else if (gate.first == "AND") {
            return std::accumulate(gate_input_signals.begin(), gate_input_signals.end(),
                                   false, std::logical_and<>());
        } else if (gate.first == "NAND") {
            return !std::accumulate(gate_input_signals.begin(),
                                    gate_input_signals.end(), false,
                                    std::logical_and<>());
        } else if (gate.first == "OR") {
            return std::accumulate(gate_input_signals.begin(), gate_input_signals.end(),
                                   false, std::logical_or<>());
        } else
            return !std::accumulate(gate_input_signals.begin(),
                                    gate_input_signals.end(), false,
                                    std::logical_or<>());
    }

    void compute_and_print_single_circuit_run(const gate_graph &circuit,
                                              std::map<sig_t, bool> &signal_values,
                                              const std::vector<sig_t> &signals,
                                              long input_size) {
        // tu z tymi iteratorami to się zagubiłam w akcji totalnie
        for (auto it = signals.cbegin() + input_size; it != signals.end(); ++it) {
            signal_values.insert_or_assign(
                    *it, compute_single_gate((circuit.at(*it)), signal_values));
        }
        for_each(begin(signal_values), end(signal_values),
                 [](auto entry) { std::cout << entry.second; });
    }

    void compute_and_print_all_possible_circuit_runs(
            const gate_graph &circuit, const std::vector<sig_t> &signals) {
        size_t input_size = count_input_signals(circuit, signals);
        const size_t how_many_possible_inputs = 1 << input_size;
        std::sort(signals.begin(), signals.begin() + (long) input_size,
                  std::greater<>());

        std::map<sig_t, bool> signal_values;

        for (size_t i = 0; i < how_many_possible_inputs; i++) {
            size_t input_combination_no = i;
            for (size_t j = 0; j < input_size; j++) {
                size_t signal_value = input_combination_no % 2;
                signal_values.insert_or_assign(signals[j], signal_value);
                input_combination_no /= 2;
            }
            compute_and_print_single_circuit_run(circuit, signal_values, signals,
                                                 (long) input_size);
        }
    }
}

int main() {
    const std::vector<gate_t> gates{{
            {"NOT", in_type::UNARY},
            {"XOR", in_type::BINARY},
            {"AND", in_type::MULTI},
            {"NAND",in_type::MULTI},
            {"OR",  in_type::MULTI},
            {"NOR", in_type::MULTI}
    }};

    std::unordered_map<std::string, std::regex> sig_regex;
    std::transform(begin(gates), end(gates),
                   std::inserter(sig_regex, end(sig_regex)),
                   regex_matcher());

    gate_graph graph;
    std::string gate_info;

    for (uint64_t line = 1; std::getline(std::cin, gate_info); line++) {
        auto [name, signals]{split_by_first_space(gate_info)};
        if (std::regex_match(signals, sig_regex[name])) {
            auto [input, output]{parse_signals(name)};
            if (!graph.contains(output)) {
                graph[output] = {name, input};
            } else {
                error::print_repetitive_output_message();
            }
        } else {
            error::print_invalid_parsing_message(line, gate_info);
        }
    }


    return EXIT_SUCCESS;
}