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

        static const std::string name() {
            return "NOT";
        }
    };

    struct lxor : public loperator {
        bool operator()(const binseq& seq) const override {
            return seq[0] != seq[1];
        }

        static const std::string name() {
            return "XOR";
        }
    };

    struct land : public loperator {
        bool operator()(const binseq& seq) const override {
            return std::accumulate(begin(seq), end(seq), true, std::logical_and<>());
        }

        static const std::string name() {
            return "AND";
        }
    };

    struct lor : public loperator {
        bool operator()(const binseq& seq) const override {
            return std::accumulate(begin(seq), end(seq), false, std::logical_or<>());
        }

        static const std::string name() {
            return "OR";
        }
    };

    struct lnand : public loperator {
        bool operator()(const binseq& seq) const override {
            return !land()(seq);
        }

        static const std::string name() {
            return "NAND";
        }
    };

    struct lnor : public loperator {
        bool operator()(const binseq& seq) const override {
            return !lor()(seq);
        }

        static const std::string name() {
            return "NOR";
        }
    };

    binfunc operator_of(const std::string& name) {
        if (name == lnot::name())
            return lnot();
        else if (name == lxor::name())
            return lxor();
        else if (name == land::name())
            return land();
        else if (name == lor::name())
            return lor();
        else if (name == lnand::name())
            return lnand();
        else if (name == lnor::name())
            return lnor();
        throw std::runtime_error("Operator " + name + " does not exist.");
    }

    std::vector<std::string> unary_names() {
        return {lnot::name()};
    }

    std::vector<std::string> binary_names() {
        return {lxor::name()};
    }

    std::vector<std::string> multi_names() {
        return {land::name(), lnand::name(), lor::name(), lnor::name()};
    }
}

namespace {
    using sig_t = int32_t;

    using sigvector = std::vector<sig_t>;

    template<typename T>
    using sigmap = std::map<sig_t, T>;

    using gate_input = std::pair<logic::binfunc, sigvector>;

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


    std::string pattern_of(const std::vector<std::string>& names) {
        const auto prefix{"\\s*("};
        const auto postfix{")(\\s+[1-9]\\d{0,8})"};

        if (names.size() == 1)
            return prefix + names[0] + postfix;

        std::string infix;

        for (size_t i{0}; i < names.size(); i++) {
            infix += "(" + names[i] + ")";
            if (i != names.size() - 1)
                infix += "|";
        }

        return prefix + infix + postfix;
    }

    bool is_valid_input(const std::string& input) {
        const std::vector<std::regex> regexes = {
                std::regex{pattern_of(logic::unary_names()) + "{2}\\s*"},
                std::regex{pattern_of(logic::binary_names()) + "{3}\\s*"},
                std::regex{pattern_of(logic::multi_names()) + "{3,}\\s*"}
        };

        return std::any_of(begin(regexes), end(regexes), [&](const auto& pattern) {
            return std::regex_match(input, pattern);
        });
    }

    std::pair<std::string, std::string> split_by_name(const std::string &input) {
        size_t div_pos{0};

        while (!isalpha(input[div_pos]) || isalpha(input[div_pos + 1]))
            div_pos++;

        auto name{input.substr(0, div_pos + 1)};
        auto signals{input.substr(div_pos + 1, input.size() - div_pos - 1)};

        return {name, signals};
    }

    std::pair<sigvector, sig_t> parse_signals(const std::string &signals) {
        sig_t output;
        sigvector inputs{};
        std::istringstream sigstream{signals};

        sigstream >> output;
        for (sig_t signal; sigstream >> signal;)
            inputs.push_back(signal);

        return {inputs, output};
    }

    void visit_gate(sig_t output, const gate_graph& circuit,
                    sigvector& order, sigmap<bool>& visited) {
        if (visited.contains(output) && visited.at(output))
            return;

        if (visited.contains(output) && !visited.at(output)) {
            error::print_circuit_cycle_message();
            exit(EXIT_FAILURE);
        }

        visited[output] = false;

        if (circuit.contains(output)) {
            const auto &inputs{circuit.at(output).second};

            std::for_each(begin(inputs), end(inputs), [&](sig_t input) {
                visit_gate(input, circuit, order, visited);
            });
        }

        visited[output] = true;
        order.push_back(output);
    }

    sigvector get_signal_evaluation_order(const gate_graph& circuit) {
        sigvector order;
        sigmap<bool> visited;

        std::for_each(begin(circuit), end(circuit), [&](auto entry) {
            const auto& output{entry.first};
            if (!visited.contains(output) || !visited.at(output))
                visit_gate(output, circuit, order, visited);
        });

        return order;
    }

    size_t count_inputs(const gate_graph &circuit, const sigvector& order) {
        return std::count_if(begin(order), end(order), [&](sig_t signal) {
            return !circuit.contains(signal);
        });
    }

    bool compute_gate(const gate_input& gate_in, sigmap<bool> &values) {
        logic::binseq input_values;
        const auto& inputs{gate_in.second};

        std::for_each(begin(inputs), end(inputs), [&](sig_t input) {
            input_values.push_back(values[input]);
        });

        return gate_in.first(input_values);
    }

    void print_circuit_output(const gate_graph& circuit, sigmap<bool>& values,
                              const sigvector& order, size_t input_size) {
        auto start{std::next(begin(order), static_cast<int32_t>(input_size))};

        std::for_each(start, end(order), [&](sig_t signal) {
            if (circuit.contains(signal)) {
                const auto &outputs{circuit.at(signal)};
                values[signal] = compute_gate(outputs, values);
            }
        });

        for (const auto& [_, value] : values)
            std::cout << value;
        std::cout << std::endl;
    }

    void print_all_circuit_outputs(const gate_graph& circuit) {
        sigvector order{get_signal_evaluation_order(circuit)};
        const auto input_count{count_inputs(circuit, order)};
        const auto combinations{1L << input_count};
        auto input_end{std::next(begin(order), static_cast<int32_t>(input_count))};

        std::sort(begin(order), end(order), [&](sig_t sigl, sig_t sigr) {
            return !circuit.contains(sigl) && circuit.contains(sigr);
        });
        std::sort(begin(order), input_end, std::greater<>());

        sigmap<bool> values;
        for (size_t input{0}; input < combinations; input++) {
            size_t input_ordinal = input;
            for (size_t bit{0}; bit < input_count; bit++) {
                auto sig_val{static_cast<bool>(input_ordinal % 2)};
                values[order[bit]] = sig_val;
                input_ordinal /= 2;
            }
            print_circuit_output(circuit, values, order, input_count);
        }
    }
}

int main() {
    gate_graph circuit;
    std::string gate_info;
    bool error_occurred = false;

    for (uint64_t line{1}; std::getline(std::cin, gate_info); line++) {
        if (is_valid_input(gate_info)) {
            auto [name, signals]{split_by_name(gate_info)};
            auto [input, output]{parse_signals(signals)};

            if (!circuit.contains(output)) {
                circuit[output] = {logic::operator_of(name), input};
            } else {
                error::print_repetitive_output_message(line, output);
                error_occurred |= true;
            }
        } else {
            error::print_invalid_parsing_message(line, gate_info);
            error_occurred |= true;
        }
    }

    if (!error_occurred) {
        print_all_circuit_outputs(circuit);
    }

    return (error_occurred ? EXIT_FAILURE : EXIT_SUCCESS);
}
