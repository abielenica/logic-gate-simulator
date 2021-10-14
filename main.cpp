#include <iostream>
#include <regex>
#include <unordered_map>
#include <vector>
#include <numeric>

namespace logic {
/* Sequence of binary digits */
using binseq = std::vector<bool>;

/* Function with a sequence of binary digits as parameter */
using binfunc = std::function<bool(binseq)>;

/* Abstract functor for logical operations */
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

binfunc operator_of(const std::string& name) {
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

/* Sequence of signal indexes */
using sigvector = std::vector<sig_t>;

/* Sequence of binary digits */
template<typename T>
using sigmap = std::unordered_map<sig_t, T>;

/* Logical gate represented by its name and input type cardinality */
using gate_t = std::pair<std::string, in_type>;

/* Logical gate represented by its type and list of input signals */
using gate_input = std::pair<logic::binfunc, sigvector>;

/* Graph representing the circuit of all logical gates */
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

std::regex make_signal_regex(in_type type) {
  const std::string sig_pattern{("( [1-9]\\d{0,8})")};

  if (type == in_type::MULTI)
    return std::regex(sig_pattern + "{3,}");

  auto ordinal{static_cast<int8_t>(type)};
  auto repetitions{std::to_string(ordinal + 1)};

  return std::regex(sig_pattern + "{" + repetitions + "}");
}

auto regex_matcher() {
  return [](const gate_t &gate) {
    return std::make_pair(gate.first, make_signal_regex(gate.second));
  };
}

std::pair<std::string, std::string> split_by_first_space(std::string &src) {
  const auto delim_pos{src.find_first_of(' ')};

  if (delim_pos == std::string::npos)
    return {src, std::string()};

  auto first{src.substr(0, delim_pos)};
  auto second{src.substr(delim_pos, src.size() - delim_pos)};

  return {first, second};
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

/* Visits a node in graph representing the logical gates system in order to sort it topologically */
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

/* Produces order in which gates have to be evaluated */
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

/* Counts independent input signals in gate system */
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

/* Displays output for a single combination of input signals */
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

/* Displays complete program output */
void print_all_circuit_outputs(const gate_graph& circuit) {
  sigvector order{get_signal_evaluation_order(circuit)};
  const auto input_count{count_inputs(circuit, order)};
  const auto combinations{1L << input_count};
  auto input_end{std::next(begin(order), static_cast<int32_t>(input_count))};

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
  const std::vector<gate_t> gates{{
    {"NOT", in_type::UNARY}, {"XOR", in_type::BINARY},
    {"AND", in_type::MULTI}, {"NAND",in_type::MULTI},
    {"OR",  in_type::MULTI}, {"NOR", in_type::MULTI}
  }};

  std::unordered_map<std::string, std::regex> sig_regex;
  std::transform(begin(gates), end(gates),
                 std::inserter(sig_regex, end(sig_regex)),
                 regex_matcher());

  gate_graph circuit;
  std::string gate_info;

  for (uint64_t line{1}; std::getline(std::cin, gate_info); line++) {
    auto [name, signals]{split_by_first_space(gate_info)};

    if (!std::regex_match(signals, sig_regex[name])) {
      error::print_invalid_parsing_message(line, gate_info);
      return EXIT_FAILURE;
    }

    auto [input, output]{parse_signals(signals)};

    if (!circuit.contains(output)) {
      circuit[output] = {logic::operator_of(name), input};
    } else {
      error::print_repetitive_output_message(line, output);
      return EXIT_FAILURE;
    }
  }

  print_all_circuit_outputs(circuit);

  return EXIT_SUCCESS;
}