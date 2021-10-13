#include <iostream>
#include <regex>
#include <unordered_map>
#include <vector>
#include <numeric>

namespace logic {
    using binseq = std::vector<bool>;

    using binfunc = std::function<bool(binseq)>;

    struct loperator {
        virtual bool operator()(const binseq& seq) const = 0;
    };

    struct lnot : public loperator {
        bool operator()(const binseq& seq) const override {
            return !seq[0];
        }
    };

    struct lxor : public loperator {
        bool operator()(const binseq& seq) const override {
            return seq[0] != seq[1];
        }
    };

    struct land : public loperator {
        bool operator()(const binseq& seq) const override {
            return std::accumulate(begin(seq), end(seq), true, std::logical_and<>());
        }
    };

    struct lor : public loperator {
        bool operator()(const binseq& seq) const override {
            return std::accumulate(begin(seq), end(seq), false, std::logical_or<>());
        }
    };

    struct lnand : public loperator {
        bool operator()(const binseq& seq) const override {
            return !land()(seq);
        }
    };

    struct lnor : public loperator {
        bool operator()(const binseq& seq) const override {
            return !lor()(seq);
        }
    };

    binfunc match_function(const std::string& name) {
        if (name == "NOT")
            return lnot();
        else if (name == "XOR")
            return lxor();
        else if (name == "AND")
            return land();
        else if (name == "OR")
            return lor();
        else if (name == "NAND")
            return lnand();
        else if (name == "NOR")
            return lnor();
        throw std::runtime_error("Operator " + name + " does not exist.");
    }
}

namespace {
    enum class in_type : uint8_t {
        UNARY = 1,
        BINARY = 2,
        MULTI = UINT8_MAX
    };

    using sig_t = int32_t;

    using sigvector = std::vector<sig_t>;

    template<typename T>
    using sigmap = std::unordered_map<sig_t, T>;

    using gate_t = std::pair<std::string, logic::binfunc>;

    using gate_input = std::pair<logic::binfunc, sigvector>;

    using gate_graph = sigmap<gate_input>;

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
        auto repetitions{std::to_string(ordinal + 1)};

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
    
    void visit_node(sig_t node, const gate_graph &graph,
                    std::vector<sig_t> &sorted_input,
                    std::unordered_map<sig_t, bool> &visited_status) {
        if (visited_status.contains(node) && visited_status.at(node))
            return;
        if (visited_status.contains(node) && !visited_status.at(node)) {
            error::print_circuit_cycle_message();
            return; // TODO: flaga?
        }
        visited_status.insert({node, false});
        for (sig_t input_signal: graph.at(node).second) {
            visit_node(input_signal, graph, sorted_input, visited_status);
        }
        visited_status[node] = true;
        sorted_input.push_back(node);
    }

    std::vector<sig_t> order_graph_by_input(const gate_graph &graph) {
      sigmap<bool> marked_status;
      sigvector sorted_input;
      // To nie robi tego co opisuje algorytm, tylko przechodzi raz po wszystkich,
      // ale może się mylę???
      for (const auto &node : graph) {
        if (!marked_status.contains(node.first) || !marked_status.at(node.first))
          visit_node(node.first, graph, sorted_input, marked_status);
      }
      return sorted_input;
    }

    std::pair<sigvector, sig_t> parse_signals(const std::string &signals) {
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

    size_t count_inputs(const gate_graph &circuit, const sigvector &signals) {
        size_t counter = 0;

        while (circuit.contains(signals[counter]))
            counter++;

        return counter;
    }

    bool evaluate_gate(const gate_input &in, sigmap<bool> &values) {
        logic::binseq input;

        for (int i: in.second)
            input.push_back(values.at(i));

        return in.first(input);
    }

    void compute_and_print_single_circuit_run(const gate_graph &circuit,
                                              sigmap<bool> &signal_values,
                                              const std::vector<sig_t> &signals,
                                              long input_size) {
        // tu z tymi iteratorami to się zagubiłam w akcji totalnie
        for (auto it = signals.cbegin() + input_size; it != signals.end(); ++it) {
            signal_values.insert_or_assign(
                    *it, evaluate_gate((circuit.at(*it)), signal_values));
        }

        for (const auto& [_, value] : signal_values)
            std::cout << value;
    }

    void compute_and_print_all_possible_circuit_runs(const gate_graph &circuit, const std::vector<sig_t> &signals) {
        const size_t inputs_size = count_inputs(circuit, signals);

        sigmap<bool> values;
        auto possible_inputs{static_cast<int32_t>(1L << inputs_size)};
        auto input_end{std::next(begin(signals), possible_inputs)};

        std::sort(begin(signals), input_end, std::greater<>());

        for (size_t i = 0; i < possible_inputs; i++) {
            size_t input_combination_no = i;
            for (size_t j = 0; j < inputs_size; j++) {
                size_t signal_value = input_combination_no % 2;
                values.insert_or_assign(signals[j], signal_value);
                input_combination_no /= 2;
            }
            compute_and_print_single_circuit_run(circuit, values, signals,
                                                 (long) inputs_size);
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
                graph[output] = {logic::match_function(name), input};
            } else {
                error::print_repetitive_output_message();
                return EXIT_FAILURE;
            }
        } else {
            error::print_invalid_parsing_message(line, gate_info);
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
