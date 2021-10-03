#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

constexpr bool end_on_first_solution = false;
constexpr int progress_step = 5000000;

int solution_count = 0;

struct Ranger {
    char name;
    bool is_north; // true->north, false->south

    // statistics

    int north_count{0};
    int south_count{0};
    int moved_count{0};

    std::map<char,int> paired_count;

    Ranger(char _name, bool _is_north) :
        name(_name), is_north(_is_north)
    {
        // empty
    }

    void init_paired_count(char name) {
        paired_count.insert({name, 0});
    }

    void increment_paired_count(char n) {
        auto p = paired_count.find(n);
        assert(p != paired_count.end());
        p->second++;
    }

    bool operator<(const Ranger& rhs) const {
        return name < rhs.name;
    }

    bool operator==(const Ranger& rhs) const {
        return name == rhs.name;
    }

    bool is_end_state() const {
        if (north_count != south_count) {
            return false;
        }
        bool first = true;
        int count = -1;
        for (const auto& [n, c] : paired_count) {
            if (first) {
                first = false;
                count = c;
            } else {
                if (count != c) {
                    return false;
                }
            }
        }
        return true;
    }

    friend std::ostream& operator<<(std::ostream& out, const Ranger& r) {
        out << "name:" << r.name << ", station:" << (r.is_north ? 'N' : 'S') <<
            ", ncount:" << r.north_count << ", scount:" << r.south_count <<
            ", mcount:" << r.moved_count;
        for (const auto& [n,c] : r.paired_count) {
            out << ", with_" << n << ":" << c;
        }
        return out;
    }
};


struct State {
    // name -> ranger
    std::map<char,Ranger> rangers;

    // sequence of swaps that led to this state
    std::vector<std::pair<char,char>> swap_history;

    void add_ranger(char name, bool is_north) {
        Ranger new_r(name, is_north);
        for (auto& [n, r] : rangers) {
            r.init_paired_count(name);
            new_r.init_paired_count(n);
        }
        rangers.insert({name, new_r});
    }

    bool is_end_state(const std::set<char>& end_north) const {
        // make sure those who should be int he north are
        
        auto i = end_north.cbegin();
        const char north1_name = *i;
        const char north2_name = *(++i);

        const auto north1_i = rangers.find(north1_name);
        const auto north2_i = rangers.find(north2_name);

        if (!north1_i->second.is_north || !north2_i->second.is_north) {
            return false;
        }

        // other end-state checks
        
        bool first = true;
        int moved_count = -1;
        for (const auto& [n, r] : rangers) {
            // make sure that each ranger has matching north and south
            // shifts and matching shifts with each other player
            if (!r.is_end_state()) {
                return false;
            }

            // make sure each ranger has the same number of moves
            if (first) {
                first = false;
                moved_count = r.moved_count;
            } else {
                if (moved_count != r.moved_count) {
                    return false;
                }
            }
        }

        return true;
    }

    // swap rangers with the names passed in; return true if
    // successful, false if not
    
    bool swap(char n1, char n2) {
        // fail if both rangers are at the same station
        
        auto& r1 = rangers.find(n1)->second;
        auto& r2 = rangers.find(n2)->second;
        if (r1.is_north == r2.is_north) {
            return false;
        }

        // first account for shifts before move; we account for before
        // the move so when we reach the end state we have not
        // re-accounted for the initial state

        Ranger* norths[2];
        Ranger* souths[2];
        int north_index = 0;
        int south_index = 0;
        for (auto& [n,r] : rangers) {
            if (r.is_north) {
                norths[north_index++] = &r;
                r.north_count++;
            } else {
                souths[south_index++] = &r;
                r.south_count++;
            }
        }
        norths[0]->increment_paired_count(norths[1]->name);
        norths[1]->increment_paired_count(norths[0]->name);
        souths[0]->increment_paired_count(souths[1]->name);
        souths[1]->increment_paired_count(souths[0]->name);

        // record the swap
        
        swap_history.emplace_back(std::make_pair(n1, n2));

        // perform the swap

        r1.is_north = !r1.is_north;
        r2.is_north = !r2.is_north;
        r1.moved_count++;
        r2.moved_count++;

        // indicate swap successful
        return true;
    }

    friend std::ostream& operator<<(std::ostream& out, const State& s) {
        for (const auto& [n, r] : s.rangers) {
            out << r << std::endl;
        }
        for (const auto& p : s.swap_history) {
            out << "[" << p.first << "," << p.second << "], ";
        }
        out << std::endl;
        
        return out;
    }
};


int main() {
    srand(time(NULL));
    std::deque<State> breadth_first;
    const std::set<char> end_north = {'A', 'B'};
    {
        State s;
        s.add_ranger('A', true);
        s.add_ranger('B', true);
        s.add_ranger('C', false);
        s.add_ranger('D', false);

        // since there is symmetry, force the first swap to reduce
        // search space; there are four such first swaps and the
        // solution to each should be symmetrical
        s.swap('A', 'C');

        // prime the breadth-first queue
        breadth_first.push_back(s);
    }

    // lambda to try to do a swap; it will fail if both rangers are at
    // the same station; if it succeeds check for end state and add
    // this new state to the queuee
    auto try_swap = [&breadth_first, &end_north](const State& s, char n1, char n2) {
        State s_try = s;
        const bool correct = s_try.swap(n1, n2);
        if (!correct) {
            return;
        }

        if (s_try.is_end_state(end_north)) {
            std::cout << "Solution: " << ++solution_count << std::endl <<
                s_try << std::endl;
            if (end_on_first_solution) {
                exit(0);
            }
        } else {
            breadth_first.push_back(s_try);
        }
    };

    // a list of all posswible swaps
    const std::vector<std::pair<char,char>> swaps =
        {{'A', 'B'}, {'B', 'C'}, {'B', 'D'}, {'C', 'D'}, {'A', 'C'}, {'A', 'D'}};

    int count = 0;
    while(!breadth_first.empty()) {
        State s = breadth_first.front();
        breadth_first.pop_front();

        ++count;

        // show progress occasionally
        if (count % progress_step == 0) {
            std::cout << "count: " << count << ", deque_size: " <<
                breadth_first.size() << ", swaps: " <<
                s.swap_history.size() << std::endl;
        }

        // randomize the order of swaps tried to try to avoid leading
        // with a non-solution first (e.g., [A,B], [A,B], [A,B], ...);
        // so choose a random starting point in the sequence of
        // available swaps
        const int start_i = rand() % swaps.size();
        for (int i = start_i; i < swaps.size(); ++i) {
            const auto& swap = swaps.at(i);
            try_swap(s, swap.first, swap.second);
        }
        for (int i = 0; i < start_i; ++i) {
            const auto& swap = swaps.at(i);
            try_swap(s, swap.first, swap.second);
        };
    }
}
