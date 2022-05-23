#include "wlp4gen.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>
#include <unordered_map>
#include <vector>

#include "codeGenerator.h"
#include "typeChecker.h"

using namespace std;

unordered_map<string, bool> WLP4gen::terminal{
    {"BECOMES", true}, {"BOF", true},    {"COMMA", true},  {"ELSE", true},
    {"EOF", true},     {"EQ", true},     {"GE", true},     {"GT", true},
    {"ID", true},      {"IF", true},     {"INT", true},    {"LBRACE", true},
    {"LE", true},      {"LPAREN", true}, {"LT", true},     {"MINUS", true},
    {"NE", true},      {"NUM", true},    {"PCT", true},    {"PLUS", true},
    {"PRINTLN", true}, {"RBRACE", true}, {"RETURN", true}, {"RPAREN", true},
    {"SEMI", true},    {"SLASH", true},  {"STAR", true},   {"WAIN", true},
    {"WHILE", true},   {"AMP", true},    {"LBRACK", true}, {"RBRACK", true},
    {"NEW", true},     {"DELETE", true}, {"NULL", true}};

WLP4gen::WLP4gen() { root = buildFromPreOrder(); }
WLP4gen::~WLP4gen() {}

void WLP4gen::printToPreOrder() { printToPreOrderHelper(root); }

void WLP4gen::printToPreOrderHelper(TreeNode *node) {
  if (!node) {
    return;
  }

  if (terminal[node->val]) {
    cout << node->val << " " << node->lexeme << endl;
  } else {
    cout << node->val << " ";
    for (int i = 0; i < node->children.size(); i++) {
      cout << node->children[i]->val << " ";
    }
    cout << endl;
  }

  for (int i = 0; i < node->children.size(); i++) {
    printToPreOrderHelper(node->children[i]);
  }
}

void WLP4gen::deleteTree() { deleteTreeHelper(root); }

void WLP4gen::deleteTreeHelper(TreeNode *node) {
  if (!node) {
    return;
  }

  for (int i = 0; i < node->children.size(); i++) {
    deleteTreeHelper(node->children[i]);
  }

  delete node;
}

TreeNode *WLP4gen::buildFromPreOrder() {
  string name;
  string token;
  string line;
  getline(cin, line);
  stringstream ss(line);

  ss >> name;
  TreeNode *root = new TreeNode(name);

  if (terminal[name]) {
    ss >> token;
    root->lexeme = token;
  } else {
    while (ss >> token) {
      root->children.push_back(buildFromPreOrder());
    };
  }

  return root;
}

int main() {
  WLP4gen *wlp4g = new WLP4gen();

  try {
    TypeChecker *TC = new TypeChecker(wlp4g->root);
    // TC->print();
    CodeGenerator *CG = new CodeGenerator(wlp4g->root, TC);
    delete TC;
    delete CG;
  } catch (TypeError se) {
  }

  wlp4g->deleteTree();

  delete wlp4g;
}
