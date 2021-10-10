#include <regex>
#include <vector>
#include <unordered_map>

/* Type representing an ordinal of a signal. */
using sig_t = int32_t;

/* Type representing a gate name and a collection of input signals. */
using gate_input = std::pair<std::string, std::vector<sig_t>>;

/* Type representing a graph-structured logic gate system. */
using gate_graph = std::unordered_map<sig_t, gate_input>; // TODO: Czy na pewno unordered?

/* Type representing a mapping of input to output of a gate system. */
using truth_table = std::vector<std::vector<bool>>; // TODO: Nie wiem, czy taki typ dla ciebie jest ok?

// TODO: Uogólnię potem to tak, żeby nie sprawdzać nazwę, tylko typ (unary, binary, multi)
std::regex get_signal_regex(const std::string& gate_name) {
    const std::string sig_pattern{" [1-9]\\d{0,8}"};
    if (gate_name == "NOT")
        return std::regex("(" + sig_pattern +"){2}");
    else if (gate_name == "XOR")
        return std::regex("(" + sig_pattern + "){3}");
    return std::regex("(" + sig_pattern + "){3,}");
}

static auto name_regex_matcher() {
    return [](const std::string& name) {
        return std::make_pair(name, get_signal_regex(name));
    };
}

int main() {
    const std::vector<std::string> gates = {
            "NOT", "AND", "NAND",
            "OR", "XOR", "NOR"
    };

    std::unordered_map<std::string, std::regex> sig_regex;
    std::transform(begin(gates), end(gates),
                   std::inserter(sig_regex, end(sig_regex)),
                   name_regex_matcher());

    return EXIT_SUCCESS;
}