#include "codeGenerator.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>
#include <unordered_map>
#include <vector>

#include "typeChecker.h"

using namespace std;

bool debug = false;

CodeGenerator::CodeGenerator(TreeNode *root, TypeChecker *TC)
    : typeChecker(TC), offset(0), labelCounter(0) {
  genPrologue();
  genCode(root, "");
};
CodeGenerator::~CodeGenerator() {}

void CodeGenerator::genPrologue() {
  cout << "; begin Prologue" << endl;
  cout << ".import init" << endl;
  cout << ".import new" << endl;
  cout << ".import delete" << endl;
  cout << ".import print" << endl;
  cout << "lis $4 ; $4 will always hold 4" << endl;
  cout << ".word 4" << endl;
  cout << "lis $10 ; $10 will always hold address for print" << endl;
  cout << ".word print" << endl;
  cout << "lis $11 ; $11 will always hold 1" << endl;
  cout << ".word 1" << endl;
  cout << "sub $29 , $30 , $4 ; setup frame pointer" << endl;
  cout << ";end Prologue and begin Body" << endl;
  cout << endl;

  // Jump to wain
  // cout << "; reserve space for variables" << endl;
  // cout << "; translated WLP4 code" << endl;
  // cout << "; end Body and begin Epilogue" << endl;
  // cout << "; deallocate parameters and local variables of wain" << endl;
  push(31);
  cout << "lis $5" << endl;
  cout << ".word wain" << endl;
  cout << "jalr $5" << endl;
  pop(31);

  cout << "jr $31" << endl;
}

// by convention we push from $3
void CodeGenerator::push(int reg) {
  cout << "; push $" << reg << " to stack" << endl;
  cout << "sw $" << reg << " , -4($30)" << endl;
  cout << "sub $30, $30, $4" << endl;
  offset -= 4;
}

// by conention we pop to $5
void CodeGenerator::pop(int reg) {
  cout << "; pop to $" << reg << " from stack" << endl;
  cout << "add $30, $30, $4" << endl;
  cout << "lw $" << reg << " , -4($30)" << endl;
  offset += 4;
}

void CodeGenerator::genCode(TreeNode *root, string procedure) {
  /* Linking */
  if (checkRule(root, {"start", "BOF", "procedures", "EOF"}, true)) {
    cout << "; start BOF procedures EOF" << endl;

    genCode(root->children[1], procedure);
  }

  else if (checkRule(root, {"procedures", "procedure", "procedures"}, true)) {
    cout << "; procedures procedure procedures" << endl;

    genCode(root->children[0], procedure);
    genCode(root->children[1], procedure);
  }

  else if (checkRule(root, {"procedures", "main"}, true)) {
    cout << "; procedures main" << endl;

    genCode(root->children[0], procedure);
  }

  else if (checkRule(root, {"statements", "statements", "statement"}, true)) {
    cout << "; statements statements statement" << endl;

    genCode(root->children[0], procedure);
    genCode(root->children[1], procedure);
  }

  else if (checkRule(root, {"expr", "term"}, true)) {
    cout << "; expr term" << endl;

    genCode(root->children[0], procedure);
  }

  else if (checkRule(root, {"term", "factor"}, true)) {
    cout << "; term factor" << endl;

    genCode(root->children[0], procedure);
  }

  /* Parenthesized expressions */
  else if (checkRule(root, {"factor", "LPAREN", "expr", "RPAREN"}, true)) {
    cout << "; factor LAREN expr RPAREN" << endl;

    genCode(root->children[1], procedure);
  }

  else if (checkRule(root, {"lvalue", "LPAREN", "lvalue", "RPAREN"}, true)) {
    cout << "; factor LAREN lvalue RPAREN" << endl;

    genCode(root->children[1], procedure);
  }

  /* Literals and identifiers | Handled at upper level*/
  // if (checkRule(root, {"ID"}, false)) {
  // } else if (checkRule(root, {"NUM"}, false)) {
  // } else if (checkRule(root, {"NULL"}, false)) {
  // }

  /* declaration | Handled at upper level */
  // else if (checkRule(root, {"dcl", "type", "ID"}, true)) {
  // }

  /* Pointers */
  else if (checkRule(root, {"factor", "AMP", "lvalue"}, true)) {
    // not retreiving the value stored at the variable here yet
    // Two outcomes: lvalue STAR factor (AMP cancel with STAR)
    // lvalue ID (gets the address of ID)
    cout << "; factor AMP lvalue" << endl;

    genCode(root->children[1], procedure);
  }

  else if (checkRule(root, {"lvalue", "STAR", "factor"}, true)) {
    // factor AMP lvalue -> lvalue STAR factor cancels each other out
    cout << "; lvalue STAR factor" << endl;

    genCode(root->children[1], procedure);
  }

  else if (checkRule(root, {"factor", "STAR", "factor"}, true)) {
    cout << "; factor STAR factor" << endl;

    genCode(root->children[1], procedure);
    cout << "lw $3, 0($3) ; $3 contains the loaded value" << endl;
  }

  /* Addition */
  else if (checkRule(root, {"expr", "expr", "PLUS", "term"}, true)) {
    cout << "; expr expr PLUS term" << endl;

    genCode(root->children[0], procedure);  // expr
    push(3);
    genCode(root->children[2], procedure);  // term
    pop(5);

    string l = typeChecker->typeOf(root->children[0], procedure);
    string r = typeChecker->typeOf(root->children[2], procedure);

    // $3 term, $5 expr
    if (l == "int*" && r == "int") {
      cout << "; int* + int" << endl;
      cout << "mult $3, $4" << endl;
      cout << "mflo $3"
           << " ; $3 <- sizeof(int)" << endl;
    } else if (l == "int" && r == "int*") {
      cout << "; int + int*" << endl;
      cout << "mult $5, $4" << endl;
      cout << "mflo $5"
           << "; $5 <- sizeof(int)" << endl;
    }
    // l == "int" && r == "int"
    cout << "add $3, $5, $3" << endl;
  }

  /* Subtraction */
  else if (checkRule(root, {"expr", "expr", "MINUS", "term"}, true)) {
    cout << "; expr expr MINUS term" << endl;
    genCode(root->children[0], procedure);  // expr
    push(3);
    genCode(root->children[2], procedure);  // term
    pop(5);

    string l = typeChecker->typeOf(root->children[0], procedure);
    string r = typeChecker->typeOf(root->children[2], procedure);

    if (l == "int*" && r == "int") {
      cout << "; int* - int" << endl;
      cout << "mult $3, $4" << endl;
      cout << "mflo $3"
           << " ; sizeof(int)" << endl;
      cout << "sub $3, $5, $3" << endl;
    } else if (l == "int*" && r == "int*") {
      cout << "; int* - int*" << endl;
      cout << "sub $3, $5, $3" << endl;
      cout << "divu $3, $4"
           << " ; convert pointer address to int" << endl;
      cout << "mflo $3" << endl;
    } else {  // l == "int" && r == "int"
      cout << "sub $3, $5, $3" << endl;
    }
  }

  /* Multiplication and division and mod */
  else if (checkRule(root, {"term", "term", "STAR", "factor"}, true)) {
    cout << "; term term STAR factor" << endl;
    genCode(root->children[0], procedure);  // term
    push(3);
    genCode(root->children[2], procedure);  // factor
    pop(5);
    cout << "mult $5, $3" << endl;
    cout << "mflo $3" << endl;
  }

  else if (checkRule(root, {"term", "term", "SLASH", "factor"}, true)) {
    cout << "; term term SLASH factor" << endl;
    genCode(root->children[0], procedure);  // term
    push(3);
    genCode(root->children[2], procedure);  // factor
    pop(5);
    cout << "div $5, $3" << endl;
    cout << "mflo $3" << endl;
  }

  else if (checkRule(root, {"term", "term", "PCT", "factor"}, true)) {
    cout << "; term term PCT factor" << endl;
    genCode(root->children[0], procedure);  // term
    push(3);
    genCode(root->children[2], procedure);  // factor
    pop(5);
    cout << "div $5, $3" << endl;
    cout << "mfhi $3" << endl;
  }

  /* Procedure call */
  else if (checkRule(root, {"factor", "ID", "LPAREN", "RPAREN"}, true)) {
    cout << "; factor ID LPAREN RPAREN" << endl;

    string label = root->children[0]->lexeme;
    push(29);
    push(31);

    cout << "lis $5" << endl;
    cout << ".word " << label << endl;
    cout << "jalr $5 " << endl;

    pop(31);
    pop(29);
  }

  else if (checkRule(root, {"factor", "ID", "LPAREN", "arglist", "RPAREN"},
                     true)) {
    cout << "; factor ID LPAREN arglist RPAREN" << endl;

    string label = root->children[0]->lexeme;
    push(29);
    push(31);

    // push params to stack
    int paramNum = 0;
    TreeNode *arglist = root->children[2];
    while (true) {
      if (checkRule(arglist, {"arglist", "expr"}, true)) {
        genCode(arglist->children[0], procedure);  // expr
        push(3);
        paramNum++;
        break;
      } else {
        // more arglsit remaining
        genCode(arglist->children[0], procedure);  // expr
        push(3);
        paramNum++;

        arglist = arglist->children[2];
      }
    }

    cout << "lis $5" << endl;
    cout << ".word " << label << endl;
    cout << "jalr $5 " << endl;

    // free params from stack
    cout << "lis $5" << endl;
    cout << ".word " << paramNum * 4 << endl;
    cout << "add $30, $30, $5" << endl;

    pop(31);
    pop(29);
  }

  /* Comparisons */
  else if (checkRule(root, {"test", "expr", "LT", "expr"}, true)) {
    cout << "; test expr LT expr" << endl;

    genCode(root->children[0], procedure);
    push(3);
    genCode(root->children[2], procedure);
    pop(5);

    string slt = typeChecker->typeOf(root->children[0], procedure) == "int"
                     ? "slt"
                     : "sltu";
    cout << slt << " $3, $5, $3" << endl;
  }

  else if (checkRule(root, {"test", "expr", "EQ", "expr"}, true)) {
    cout << "; test expr EQ expr" << endl;

    genCode(root->children[0], procedure);
    push(3);
    genCode(root->children[2], procedure);
    pop(5);

    string slt = typeChecker->typeOf(root->children[0], procedure) == "int"
                     ? "slt"
                     : "sltu";
    cout << slt << " $6, $3, $5" << endl;
    cout << slt << " $7, $5, $3" << endl;
    cout << "add $3, $6, $7" << endl;
    cout << "sub $3, $11, $3" << endl;
  }

  else if (checkRule(root, {"test", "expr", "NE", "expr"}, true)) {
    cout << "; test expr NE expr" << endl;

    genCode(root->children[0], procedure);
    push(3);
    genCode(root->children[2], procedure);
    pop(5);

    string slt = typeChecker->typeOf(root->children[0], procedure) == "int"
                     ? "slt"
                     : "sltu";
    cout << slt << " $6, $3, $5" << endl;
    cout << slt << " $7, $5, $3" << endl;
    cout << "add $3, $6, $7" << endl;
  }

  else if (checkRule(root, {"test", "expr", "LE", "expr"}, true)) {
    cout << "; test expr LE expr" << endl;

    genCode(root->children[0], procedure);
    push(3);
    genCode(root->children[2], procedure);
    pop(5);

    string slt = typeChecker->typeOf(root->children[0], procedure) == "int"
                     ? "slt"
                     : "sltu";

    cout << slt << " $3, $3, $5" << endl;
    cout << "sub $3, $11, $3" << endl;
  }

  else if (checkRule(root, {"test", "expr", "GE", "expr"}, true)) {
    cout << "; test expr GE expr" << endl;

    genCode(root->children[0], procedure);
    push(3);
    genCode(root->children[2], procedure);
    pop(5);

    string slt = typeChecker->typeOf(root->children[0], procedure) == "int"
                     ? "slt"
                     : "sltu";

    cout << slt << " $3, $5, $3" << endl;
    cout << "sub $3, $11, $3" << endl;
  }

  else if (checkRule(root, {"test", "expr", "GT", "expr"}, true)) {
    cout << "; test expr GT expr" << endl;

    genCode(root->children[0], procedure);
    push(3);
    genCode(root->children[2], procedure);
    pop(5);

    string slt = typeChecker->typeOf(root->children[0], procedure) == "int"
                     ? "slt"
                     : "sltu";

    cout << slt << " $3, $3, $5" << endl;
  }

  /* Control flow */
  else if (checkRule(root,
                     {"statement", "IF", "LPAREN", "test", "RPAREN", "LBRACE",
                      "statements", "RBRACE", "ELSE", "LBRACE", "statements",
                      "RBRACE"},
                     true)) {
    cout << "; statement IF LPAREN test RPAREN LBRACE statements RBRACE ELSE "
            "LBRACE statements RBRACE"
         << endl;

    string endifLabel = procedure + "endif" + to_string(labelCounter);
    string elseLabel = procedure + "else" + to_string(labelCounter);
    labelCounter++;

    genCode(root->children[2], procedure);  // test
    cout << "bne $3, $11, " << elseLabel << endl;
    genCode(root->children[5], procedure);
    cout << "beq $0, $0, " << endifLabel << endl;
    cout << elseLabel << ":" << endl;
    genCode(root->children[9], procedure);
    // end
    cout << endifLabel << ":" << endl;
  }

  else if (checkRule(root,
                     {
                         "statement",
                         "WHILE",
                         "LPAREN",
                         "test",
                         "RPAREN",
                         "LBRACE",
                         "statements",
                         "RBRACE",
                     },
                     true)) {
    cout << "; statement WHILE LPAREN test RPAREN LBRACE statements RBRACE"
         << endl;

    string loopLabel = procedure + "loop" + to_string(labelCounter);
    string endWhileLabel = procedure + "endWhile" + to_string(labelCounter);
    labelCounter++;

    cout << loopLabel << ":" << endl;
    genCode(root->children[2], procedure);
    cout << "bne $3, $11, " << endWhileLabel << endl;  // test is false
    genCode(root->children[5], procedure);
    cout << "beq $0, $0, " << loopLabel << endl;
    cout << endWhileLabel << ":" << endl;
  }

  /* Allocation / Deallocation */
  else if (checkRule(root, {"factor", "NEW", "INT", "LBRACK", "expr", "RBRACK"},
                     true)) {
    cout << "; factor NEW INT LBRACK expr RBRACK" << endl;
    push(1);

    genCode(root->children[3], procedure);
    cout << "add $1, $3, $0 ; new procedure expects value in $1" << endl;
    push(31);
    cout << "lis $5" << endl;
    cout << ".word new" << endl;
    cout << "jalr $5" << endl;
    pop(31);
    cout << "bne $3, $0, 1; if call succeeded, skip next instruction" << endl;
    cout << "add $3, $11, $0 ; if allocation fails, set $3 to NULL" << endl;

    pop(1);
  }

  else if (checkRule(
               root,
               {"statement", "DELETE", "LBRACK", "RBRACK", "expr", "SEMI"},
               true)) {
    cout << "; statement DELETE LBRACK RBRACK expr SEMI" << endl;

    string skipDeleteLabel = procedure + "skipDelete" + to_string(labelCounter);
    labelCounter++;

    genCode(root->children[3], procedure);
    cout << "beq $3, $11, " + skipDeleteLabel + " ; do NOT call delete on NULL"
         << endl;
    cout << "add $1, $3, $0 ; delete expects the address in $1" << endl;
    push(31);
    cout << "lis $5" << endl;
    cout << ".word delete" << endl;
    cout << "jalr $5" << endl;
    pop(31);
    cout << skipDeleteLabel << ":" << endl;
  }

  /* Printing */
  if (checkRule(root,
                {"statement", "PRINTLN", "LPAREN", "expr", "RPAREN", "SEMI"},
                true)) {
    cout << "; statement PRINTLN LPAREN expr RPAREN SEMI" << endl;

    push(1);
    genCode(root->children[2], procedure);  // expr
    cout << "add $1 , $3 , $0" << endl;
    push(31);
    cout << "lis $5" << endl;
    cout << ".word print" << endl;
    cout << "jalr $5" << endl;
    pop(31);
    pop(1);
  }

  /* Assignment */
  else if (checkRule(root, {"statement", "lvalue", "BECOMES", "expr", "SEMI"},
                     true)) {
    cout << "; statement lvalue BECOMES expr SEMI" << endl;

    genCode(root->children[0], procedure);  // lvalue
    push(3);
    genCode(root->children[2], procedure);  // expr
    pop(5);

    // lvalue is the ID containing an address
    // lvalue: $5, expr: $3
    cout << "sw $3, 0($5)" << endl;
  }

  else if (checkRule(root, {"lvalue", "ID"}, true)) {
    cout << "; lvalue ID" << endl;

    string symbol = root->children[0]->lexeme;
    int offset = typeChecker->getSymbolOffset(symbol, procedure);

    cout << "; address of ID(" << symbol << ")" << endl;
    cout << "lis $3" << endl;
    cout << ".word " << offset << endl;
    cout << "add $3, $3, $29" << endl;
  }

  else if (checkRule(root, {"factor", "ID"}, true)) {
    cout << "; factor ID" << endl;

    string symbol = root->children[0]->lexeme;
    int offset = typeChecker->getSymbolOffset(symbol, procedure);

    cout << "lw $3, " << offset << "($29)"
         << " ; load " << symbol << endl;
  }

  else if (checkRule(root, {"factor", "NUM"}, true)) {
    cout << "; factor NUM" << endl;

    string val = root->children[0]->lexeme;

    cout << "lis $3" << endl;
    cout << ".word " << val << endl;
  }

  else if (checkRule(root, {"factor", "NULL"}, true)) {
    cout << "; factor NULL" << endl;

    cout << "add $3, $0, $11 ; $11 is always 1" << endl;
  }

  /* Decl’ns */
  else if (checkRule(root, {"dcls", "dcls", "dcl", "BECOMES", "NUM", "SEMI"},
                     true)) {
    cout << "; dcls dcls dcl BECOMES NUM SEMI" << endl;

    genCode(root->children[0], procedure);

    string symbol = root->children[1]->children[1]->lexeme;
    string val = root->children[3]->lexeme;

    cout << "; dcl " << symbol << " = " << val << endl;
    cout << "lis $5" << endl;
    cout << ".word " << val << endl;

    // push $5 onto the stack
    typeChecker->setSymbolOffset(symbol, procedure, offset);
    push(5);  // push auto decrement offset
  }

  else if (checkRule(root, {"dcls", "dcls", "dcl", "BECOMES", "NULL", "SEMI"},
                     true)) {
    cout << "; dcls dcls dcl BECOMES NULL SEMI" << endl;

    genCode(root->children[0], procedure);

    string symbol = root->children[1]->children[1]->lexeme;
    string val = root->children[3]->lexeme;

    cout << "; dcl pointer " << symbol << " = " << val << endl;
    cout << "lis $5" << endl;
    cout << ".word 1" << endl;

    // push $5 onto the stack
    typeChecker->setSymbolOffset(symbol, procedure, offset);
    push(5);  // push auto decrement offset
  }

  /* Procedures */
  if (checkRule(
          root,
          {"main", "INT", "WAIN", "LPAREN", "dcl", "COMMA", "dcl", "RPAREN",
           "LBRACE", "dcls", "statements", "RETURN", "expr", "SEMI", "RBRACE"},
          true)) {
    cout << "; main INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls "
            "statements RETURN expr SEMI RBRACE"
         << endl;
    string procName = "wain";
    string programType =
        typeChecker->typeOf(root->children[3]->children[1], procName);
    cout << "; Program type is: " << programType << endl;
    cout << "wain:" << endl;

    // init
    // If the first parameter to wain is of type int* (i.e., the generated code
    // will be passed an array), then the size of this array must be in register
    // $2 when init is called. Note that mips.array already puts the size of the
    // array in register $2, so you only need to make sure that your generated
    // code does not change $2 before it calls init.
    // If the first parameter to wain is of type int, then register $2 must
    // contain the value 0 (zero) when init is called.
    push(31);
    push(2);

    if (programType == "int") {
      cout << "add $2, $0, $0" << endl;
    }
    cout << "lis $5" << endl;
    cout << ".word init" << endl;
    cout << "jalr $5" << endl;
    pop(2);
    pop(31);

    cout << "; begin Prologue " << procName << endl;
    cout << "sub $29 , $30 , $4 ; setup frame pointer" << endl;

    offset = 0;
    typeChecker->setSymbolOffset(root->children[3]->children[1]->lexeme,
                                 procName, offset);
    // param 1
    push(1);  // push auto decrement offset

    typeChecker->setSymbolOffset(root->children[5]->children[1]->lexeme,
                                 procName, offset);
    // param 2
    push(2);  // push auto decrement offset

    genCode(root->children[8], procName);   // dcls
    genCode(root->children[9], procName);   // statements
    genCode(root->children[11], procName);  // expr (return)

    cout << "; Epilogue" << endl;
    cout << "; deallocate parameters and local variables of wain" << endl;
    offset = 0;
    cout << "add $30, $29, $4" << endl;
    cout << "jr $31" << endl;
    cout << endl;
  }

  else if (checkRule(root,
                     {"procedure", "INT", "ID", "LPAREN", "params", "RPAREN",
                      "LBRACE", "dcls", "statements", "RETURN", "expr", "SEMI",
                      "RBRACE"},
                     true)) {
    cout << "; procedure INT ID LPAREN params RPAREN LBRACE dcls "
            "statements RETURN expr SEMI RBRACE"
         << endl;
    string procName = root->children[1]->lexeme;
    cout << "; Function : " << procName << endl;
    cout << procName << ":" << endl;

    cout << "; begin Prologue" << endl;
    cout << "sub $29 , $30 , $4 ; assume caller-saves old frames" << endl;

    // Handle arguments index (they already pushed into the stack by caller)
    // Argument Index: 4, 8, ...
    // Local Index: 0, -4, -8, ...
    offset = typeChecker->getSignature(procName).size() * 4;
    TreeNode *params = root->children[3];
    while (true) {
      if (checkRule(params, {"params"}, true)) {
        // no params
        break;
      } else if (checkRule(params, {"params", "paramlist"}, true)) {
        // has param
        params = params->children[0];
      } else if (checkRule(params, {"paramlist", "dcl"}, true)) {
        // last param
        typeChecker->setSymbolOffset(params->children[0]->children[1]->lexeme,
                                     procName, offset);
        offset -= 4;
        break;
      } else if (checkRule(params, {"paramlist", "dcl", "COMMA", "paramlist"},
                           true)) {
        // more params remaining
        typeChecker->setSymbolOffset(params->children[0]->children[1]->lexeme,
                                     procName, offset);
        offset -= 4;

        params = params->children[2];
      }
    }

    // It should be that offset = 0 here;
    assert(offset == 0);

    genCode(root->children[6], procName);  // dcls (record the local variables)
    genCode(root->children[7], procName);  // statements
    genCode(root->children[9], procName);  // expr

    cout << "; Epilogue" << endl;
    cout << "; deallocate parameters and local variables of wain" << endl;
    offset = 0;  // resore offset
    cout << "add $30, $29, $4" << endl;
    cout << "jr $31" << endl;
    cout << endl;
  }
}
