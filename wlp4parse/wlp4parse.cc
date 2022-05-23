#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>
#include <unordered_map>
#include <vector>

using namespace std;

//=========
bool debug = false;
//=========

unordered_map<string, bool> terminal;
unordered_map<string, bool> nonTerminal;
string start;
vector<pair<string, vector<string>>> rule;
unordered_map<int, unordered_map<string, pair<string, int>>> action;

struct TreeNode {
  string val;
  string lexeme;  // TreeNode should have either lexeme or children
  vector<TreeNode *> children;
  TreeNode() : val(0), children({}) {}
  TreeNode(string x) : val(x), children({}) {}
  TreeNode(string x, int length) : val(x), children(length, nullptr) {}
  TreeNode(string x, string l) : val(x), lexeme(l) {}
};

void deleteTree(TreeNode *root) {
  if (!root) {
    return;
  }

  for (int i = 0; i < root->children.size(); i++) {
    deleteTree(root->children[i]);
  }

  delete root;
}

// Skip the grammar part of the input.
void skipLine(fstream &in) {
  string s;
  getline(in, s);
}

void recordGrammar(fstream &in) {
  int i, numTerm, numNonTerm, numRule, numState, numAction;

  // read the number of terminals and move to the next line
  in >> numTerm;
  skipLine(in);

  // record terminals
  for (int i = 0; i < numTerm; i++) {
    string t;
    in >> t;
    terminal[t] = true;
    skipLine(in);
  }

  // read the number of non-terminals and move to the next line
  in >> numNonTerm;
  skipLine(in);

  // record non-terminals
  for (i = 0; i < numNonTerm; i++) {
    string n;
    in >> n;
    nonTerminal[n] = true;
    skipLine(in);
  }

  // skip the line containing the start symbol
  in >> start;
  skipLine(in);

  // read the number of rules and move to the next line
  in >> numRule;
  skipLine(in);

  // read production rules
  for (i = 0; i < numRule; i++) {
    string token;
    string line;
    getline(in, line);
    stringstream ss(line);

    ss >> token;
    rule.push_back({token, {}});

    while (ss >> token) {
      rule.back().second.push_back(token);
    }
  }

  in >> numState;
  skipLine(in);
  in >> numAction;
  skipLine(in);
  // If the DFA is in state, and terminal is first in the unread input, reduce
  // by rule number rule

  // If the DFA is in state1, shift symbol and transition to state2
  for (int i = 0; i < numAction; i++) {
    int state;
    string item;
    string reduceOrShift;
    int ruleOrState;
    in >> state;
    in >> item;
    in >> reduceOrShift;
    in >> ruleOrState;

    action[state][item] = {reduceOrShift, ruleOrState};
  }
}

vector<TreeNode *> readCode() {
  string token;
  string lexeme;

  vector<TreeNode *> s;
  s.push_back(new TreeNode("BOF", "BOF"));

  while (cin >> token) {
    cin >> lexeme;
    s.push_back(new TreeNode(token, lexeme));
  }

  s.push_back(new TreeNode("EOF", "EOF"));

  return s;
}

void printDerivation(TreeNode *root) {
  if (!root) {
    return;
  }

  for (int i = 0; i < root->children.size(); i++) {
    printDerivation(root->children[i]);
  }

  if (nonTerminal[root->val]) {
    cout << root->val << " ";
    for (int i = 0; i < root->children.size(); i++) {
      cout << root->children[i]->val << " ";
    }
    cout << endl;
  } else {
    cout << root->val << " " << root->lexeme << endl;
  }
}

TreeNode *buildFromLR(vector<TreeNode *> sequence) {
  stack<int> stateStack;
  stack<TreeNode *> treeStack;

  // push start state
  stateStack.push(0);
  for (int i = 0; i < sequence.size(); i++) {
    while (action[stateStack.top()][sequence[i]->val].first == "reduce") {
      pair<string, vector<string>> r =
          rule[action[stateStack.top()][sequence[i]->val].second];

      TreeNode *newNode = new TreeNode(r.first);
      stack<TreeNode *> temp;
      for (int j = 0; j < r.second.size(); j++) {
        temp.push(treeStack.top());
        treeStack.pop();
        stateStack.pop();
      }
      while (!temp.empty()) {  // invert stack output
        newNode->children.push_back(temp.top());
        temp.pop();
      }
      if (debug) {
        cout << stateStack.top() << " " << sequence[i]->val << " reduce "
             << action[stateStack.top()][sequence[i]->val].first << " "
             << action[stateStack.top()][sequence[i]->val].second << endl;
      }
      treeStack.push(newNode);
      stateStack.push(action[stateStack.top()][treeStack.top()->val].second);
    }
    treeStack.push(sequence[i]);

    // Reject if no suitable transition
    if (action[stateStack.top()][sequence[i]->val].first != "shift") {
      // Since BOF is not present in the input, it is not counted as a token
      // when determining the length of the longest correct prefix.
      cerr << "ERROR at " << i << endl;

      // cleanup stateStack
      while (!treeStack.empty()) {
        deleteTree(treeStack.top());
        treeStack.pop();
      }

      // cleanup sequence
      for (int j = i + 1; j < sequence.size(); j++) {
        deleteTree(sequence[j]);
      }
      sequence.clear();

      return nullptr;
    }

    if (debug) {
      cout << stateStack.top() << " " << sequence[i]->val << " shift "
           << action[stateStack.top()][sequence[i]->val].first << " "
           << action[stateStack.top()][sequence[i]->val].second << endl;
    }
    stateStack.push(action[stateStack.top()][sequence[i]->val].second);
  }

  // accept
  // Note that there is no reduce action for rule 0 in the LR(1) DFA, even
  // though you are required to output the rule corresponding to this final
  // reduction in your parser. Rule 0 will always be the unique rule which has
  // the start symbol on the LHS.
  pair<string, vector<string>> r = rule[0];

  TreeNode *newNode = new TreeNode(r.first);
  stack<TreeNode *> temp;
  for (int j = 0; j < r.second.size(); j++) {
    temp.push(treeStack.top());
    treeStack.pop();
    stateStack.pop();
  }
  while (!temp.empty()) {  // invert stack output
    newNode->children.push_back(temp.top());
    temp.pop();
  }
  treeStack.push(newNode);

  return treeStack.top();
}

void printToPreOrder(TreeNode *root) {
  if (!root) {
    return;
  }

  if (nonTerminal[root->val]) {
    cout << root->val << " ";
    for (int i = 0; i < root->children.size(); i++) {
      cout << root->children[i]->val << " ";
    }
    cout << endl;
  } else {
    cout << root->val << " " << root->lexeme << endl;
  }

  for (int i = 0; i < root->children.size(); i++) {
    printToPreOrder(root->children[i]);
  }
}

int main() {
  fstream wlp4grammar;
  wlp4grammar.open("WLP4.lr1");
  recordGrammar(wlp4grammar);

  vector<TreeNode *> sequence;
  sequence = readCode();

  TreeNode *top = buildFromLR(sequence);
  printToPreOrder(top);
  deleteTree(top);
}
