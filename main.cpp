#include <iostream>
#include <numeric>
#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
  for (int i : gate.second) {
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
  std::sort(signals.begin(), signals.begin() + (long)input_size,
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
                                         (long)input_size);
  }
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