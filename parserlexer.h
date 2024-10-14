//
// Created by pufre on 2024-09-17.
//

#include <memory>
#include <string>
#include <map>
#include <vector>

#ifndef FOLLANG_PARSERLEXER_H
#define FOLLANG_PARSERLEXER_H

enum Token {
    tok_eof = -1,

    // commands
    tok_def = -2,

    // primary
    tok_quantifier = -3,
    tok_binop = -4,
    tok_identifier = -5
};

enum Quantifier {
    forall = -1,
    exists = -2
};

enum Binop {
    op_and = 0,
    op_or = 1
};

class ExprAST {
public:
    virtual ~ExprAST() = default;
};


/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
    std::string Name;
    std::vector<std::string> Args;

public:
    PrototypeAST(const std::string &Name, std::vector<std::string> Args)
            : Name(Name), Args(std::move(Args)) {}

    const std::string &getName() const { return Name; }
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;

public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                std::unique_ptr<ExprAST> Body)
            : Proto(std::move(Proto)), Body(std::move(Body)) {}
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;

public:
    CallExprAST(const std::string &Callee,
                std::vector<std::unique_ptr<ExprAST>> Args)
            : Callee(Callee), Args(std::move(Args)) {}
};

class BinaryExprAST : public ExprAST {
    Binop Op;
    std::unique_ptr<ExprAST> LHS, RHS;

public:
    BinaryExprAST(Binop Op, std::unique_ptr<ExprAST> LHS,
                  std::unique_ptr<ExprAST> RHS)
            : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
};

class VariableExprAST : public ExprAST {
    std::string Name;

public:
    VariableExprAST(const std::string &Name) : Name(Name) {}
};

// \forall x, p(x)
// in this case x is the var
// p(x) is the body
class QuantifierAST : public ExprAST {
    Quantifier quantifier;
    std::unique_ptr<ExprAST> Body;
public:
    QuantifierAST(Quantifier quantifier, std::unique_ptr<ExprAST> Body) : quantifier(quantifier), Body(std::move(Body)) {}
};


class ParserLexer {
    std::string IdentifierStr;
    Quantifier quantifier;
    Binop binop;

    std::map<Binop, int> BinopPrecedence;
    char LastChar;
    int CurTok;


public:
    explicit ParserLexer(std::map<Binop, int> binopPrecedence) {
        BinopPrecedence = std::move(binopPrecedence);
        CurTok = ' ';
        IdentifierStr = ' ';
        LastChar = ' ';
    };

    int getNextToken();
    int gettok();
    int GetTokPrecedence();

    std::unique_ptr<ExprAST> ParseParenExpr();
    std::unique_ptr<QuantifierAST> ParseQuantifierExpr();
    std::unique_ptr<ExprAST> ParseIdentifierExpr();
    std::unique_ptr<ExprAST> ParsePrimary();
    std::unique_ptr<ExprAST> ParseExpression();
    std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS);

    std::unique_ptr<PrototypeAST> ParsePrototype();
    std::unique_ptr<FunctionAST> ParseDefinition();
    void HandleDefinition();

    void MainLoop();
};

#endif //FOLLANG_PARSERLEXER_H
