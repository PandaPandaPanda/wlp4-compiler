#include <fstream>
#include <iostream>
#include <list>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "merl.h"

using namespace std;

/*
Starter code for the linker you are to write for CS241 Assignment 10 Problem 5.
The code uses the MERL library defined in "merl.h". This library depends on the
object file "merl.o" and this program must be compiled with those object files.

The starter code includes functionality for processing command-line arguments,
reading the input MERL files, calling the linker, and producing output on
standard output.

You need to implement the "linking constructor" for MERL files, which takes two
MERL file objects as input and produces a new MERL file object representing the
result of linking the files:

MERL::MERL(MERL& m1, MERL& m2)

A definition of this function is at the end of the file. The default
implementation creates a dummy MERL file to demonstrate the use of the MERL
library. You should replace it with code that links the two input files.

You are free to modify the existing code as needed, but it should not be
necessary. You are free to add your own helper functions to this file.

The functions in the MERL library will throw std::runtime_error if an error
occurs. The starter code is set up to catch these exceptions and output an error
message to standard error. You may wish to use this functionality for your own
error handling.
*/

// Links all the MERL objects in the given list, by recursively
// calling the "linking constructor" that links two MERL objects.
// You should not need to modify this function.
MERL link(std::list<MERL>& merls) {
  if (merls.size() == 1) {
    return merls.front();
  } else if (merls.size() == 2) {
    return MERL(merls.front(), merls.back());
  }
  MERL first = merls.front();
  merls.pop_front();
  MERL linkedRest = link(merls);
  return MERL(first, linkedRest);
}

// Main function, which reads the MERL files and passes them into
// the link function, then outputs the linked MERL file.
// You should not need to modify this function.
int main(int argc, char* argv[]) {
  if (argc == 1) {
    std::cerr << "Usage: " << argv[0] << " <file1.merl> <file2.merl> ..."
              << std::endl;
    return 1;
  }
  std::list<MERL> merls;
  try {
    for (int i = 1; i < argc; i++) {
      std::ifstream file;
      file.open(argv[i]);
      merls.emplace_back(file);
      file.close();
    }
    // Link all the MERL files read from the command line.
    MERL linked = link(merls);
    // Print a readable representation of the linked MERL file to standard error
    // for debugging.
    linked.print(std::cerr);
    // Write the binary representation of the linked MERL file to standard
    // output.
    std::cout << linked;
  } catch (std::runtime_error& re) {
    std::cerr << "ERROR: " << re.what() << std::endl;
    return 1;
  }
  return 0;
}

// Linking constructor for MERL objects.
// Implement this, which constructs a new MERL object by linking the two given
// MERL objects. The function is allowed to modify the inputs m1 and m2. It does
// not need to leave them in the same state as when the function was called. The
// default implementation creates a dummy MERL file that does not depend on
// either input, to demonstrate the use of the MERL library. You should not
// output anything to standard output here; the main function handles output.
MERL::MERL(MERL& m1, MERL& m2) {
  // === global ===
  const int wordSize = 4;
  // ==============

  /* Task 1: Check for duplicate exports. */
  unordered_map<string, Entry*> ESD1;
  unordered_map<string, Entry*> ESD2;

  for (auto& entry1 : m1.table) {
    if (entry1.type == Entry::Type::ESD) {
      ESD1[entry1.name] = &entry1;
    }
  }

  for (auto& entry2 : m2.table) {
    if (entry2.type == Entry::Type::ESD) {
      ESD2[entry2.name] = &entry2;
      if (ESD1[entry2.name] != nullptr) {
        std::cerr << "ERROR: duplicate export " << entry2.name << std::endl;
      }
    }
  }

  /* Task 2: Combine the code segments (not including tables) */
  // concatenate m1.code and m2.code without modifying either code segment
  code.insert(code.end(), m1.code.begin(), m1.code.end());
  code.insert(code.end(), m2.code.begin(), m2.code.end());

  /* Task 3: Relocate m2.table */
  // m1's code segment ends minus the size of the header (12)
  int offset = m1.endCode - 12;

  // every table entry in m2 must now be updated by adding this relocation
  // offset to the address that the entry contains.
  for (auto& entry : m2.table) {
    entry.location = entry.location + offset;
  }

  /* Task 4: Relocate m2.code using REL entries */
  // go through each REL entry in the modified m2.table, and update the
  // corresponding lines of m2.code by adding the relocation offset computed in
  // Task 3 to each such line.
  for (auto& entry : m2.table) {
    if (entry.type == Entry::Type::REL) {
      // index corresponding to code (remove header)
      int index = (entry.location - 12) / wordSize;
      code[index] = code[index] + offset;
    }
  }

  /* Task 5: Resolve imports for m1 */
  for (auto& entry1 : m1.table) {
    if (entry1.type == Entry::Type::ESR) {
      // if there is an ESD in m2.table with a matching name
      if (ESD2[entry1.name] != nullptr) {
        int index = (entry1.location - 12) / wordSize;
        code[index] = ESD2[entry1.name]->location;
        entry1.type = Entry::Type::REL;
      }
    }
  }

  /* Task 6: Resolve imports for m2 */
  for (auto& entry2 : m2.table) {
    if (entry2.type == Entry::Type::ESR) {
      // if there is an ESD in m1.table with a matching name
      if (ESD1[entry2.name] != nullptr) {
        int index = (entry2.location - 12) / wordSize;
        code[index] = ESD1[entry2.name]->location;
        entry2.type = Entry::Type::REL;
      }
    }
  }

  /* Task 7: Combine the tables for the linked file */
  table.insert(table.end(), m1.table.begin(), m1.table.end());
  table.insert(table.end(), m2.table.begin(), m2.table.end());

  /* Task 8: Compute the header information */
  // endCode = 12 + linked_code size in bytes
  endCode = 12 + code.size() * wordSize;
  // endModule = endCode + linked_table size in bytes
  int tableSize = 0;
  for (auto& entry : table) {
    tableSize = tableSize + entry.size();
  }
  endModule = endCode + tableSize;

  /* Task 9: Output the MERL file */
  // Handled in mian

  // output merl cookie
  // output endModule
  // output endCode
  // output linked_code
  // output linked_table
}