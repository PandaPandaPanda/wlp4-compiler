#include <iostream>
#include <string>
#include <vector>

#include "scanner.h"
using namespace std;

/*
 * All code requires C++14, so if you're getting compile errors make sure to
 * use -std=c++14.
 */
int main() {
    string line;
    try {
        while (getline(cin, line)) {
            vector<Token> tokenLine = scan(line);

            for (auto &token : tokenLine) {
                cout << token << endl;
            }
        }
    } catch (ScanningFailure &f) {
        std::cerr << f.what() << std::endl;
        return 1;
    }
    return 0;
}
