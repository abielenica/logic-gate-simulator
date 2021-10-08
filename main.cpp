#include <vector>
#include <unordered_set>
#include <unordered_map>

/* Type representing an ordinal of a signal. */
using signal_t = int32_t;

/* Type representing a gate name and a collection of input signals. */
using gate_input = std::pair<std::string, std::vector<signal_t>>;

/* Type representing a structured logical gate system. */
using gate_graph = std::unordered_map<signal_t, gate_input>; // TODO: Czy na pewno unordered?

/* Type representing a mapping of input to output of a gate system. */
using truth_table = std::vector<std::vector<bool>>; // TODO: Nie wiem, czy taki typ dla ciebie jest ok?


int main() {
    return EXIT_SUCCESS;
}