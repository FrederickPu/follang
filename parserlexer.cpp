//
// Created by pufre on 2024-09-17.
// based on [llvm make your first frontend](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl02.html)
//

#include "parserlexer.h"

int ParserLexer::gettok() {
    // Skip any whitespace.
    while (isspace(LastChar))
        LastChar = getchar();

    if (isalpha(LastChar)) { // identifier: [a-zA-Z][a-zA-Z0-9]*
        IdentifierStr = LastChar;
        while (isalnum((LastChar = getchar())))
            IdentifierStr += LastChar;

        if (IdentifierStr == "forall") {
            quantifier = forall;
            return tok_quantifier;
        }
        if (IdentifierStr == "exists") {
            quantifier = exists;
            return tok_quantifier;
        }
        if (IdentifierStr == "def")
            return tok_def;
                return tok_identifier;
    }

    // Check for end of file.  Don't eat the EOF.
    if (LastChar == EOF)
        return tok_eof;


    // Otherwise, just return the character as its ascii value.
    int ThisChar = LastChar;
    LastChar = getchar();
    if (ThisChar == '/' && LastChar == '\\')
    {
        binop = op_and;
        LastChar = ' ';
        return tok_binop;
    }
    if (ThisChar == '\\' && LastChar == '/')
    {
        binop = op_or;
        LastChar = ' ';
        return tok_binop;
    }

    return ThisChar;
}

int ParserLexer::getNextToken() {
    return CurTok = gettok();
}

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
int ParserLexer::GetTokPrecedence() {
    if (CurTok == tok_binop)
        return BinopPrecedence[binop];
    else
        return -1;
}

/// LogError* - These are little helper functions for error handling.
std::unique_ptr<ExprAST> LogError(const char *Str) {
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
    LogError(Str);
    return nullptr;
}

/// parenexpr ::= '(' expression ')'
std::unique_ptr<ExprAST> ParserLexer::ParseParenExpr() {
    getNextToken(); // eat (.
    auto V = ParseExpression();
    if (!V)
        return nullptr;

    if (CurTok != ')')
        return LogError("expected ')'");
    getNextToken(); // eat ).
    return V;
}


// A x, (expression)
std::unique_ptr<QuantifierAST> ParserLexer::ParseQuantifierExpr() {
    getNextToken();
    if (CurTok != tok_identifier)
        return nullptr;
    std::string IdName = IdentifierStr;
    getNextToken();
    if (CurTok != ',')
        return nullptr;
    getNextToken(); // eat comma
    auto RHS = ParseExpression();
    if (!RHS)
        return nullptr;
    return std::make_unique<QuantifierAST>(quantifier, std::move(RHS));
}

/// identifierexpr
///   ::= identifier
///   ::=  identifier '(' expression* ')'
std::unique_ptr<ExprAST> ParserLexer::ParseIdentifierExpr() {
    std::string IdName = IdentifierStr;

    getNextToken();  // eat identifier.

    if (CurTok != '(') // Simple variable ref.
        return std::make_unique<VariableExprAST>(IdName);

    // Call.
    getNextToken();  // eat (
    std::vector<std::unique_ptr<ExprAST>> Args;
    if (CurTok != ')') {
        while (true) {
            if (auto Arg = ParseExpression())
                Args.push_back(std::move(Arg));
            else
                return nullptr;

            if (CurTok == ')')
                break;

            if (CurTok != ',')
                return LogError("Expected ')' or ',' in argument list");
            getNextToken();
        }
    }

    // Eat the ')'.
    getNextToken();

    return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
std::unique_ptr<ExprAST> ParserLexer::ParsePrimary() {
    switch (CurTok) {
        default:
            return LogError("unknown token when expecting an expression");
        case tok_quantifier:
            return ParseQuantifierExpr();
        case tok_identifier:
            return ParseIdentifierExpr();
        case '(':
            return ParseParenExpr();
    }
}

std::unique_ptr<ExprAST> ParserLexer::ParseExpression() {
    auto LHS = ParsePrimary();
    if (!LHS)
        return nullptr;

    return ParseBinOpRHS(0, std::move(LHS));
}

/// binoprhs
///   ::= ('+' primary)*
std::unique_ptr<ExprAST> ParserLexer::ParseBinOpRHS(int ExprPrec,
                                              std::unique_ptr<ExprAST> LHS) {
    // If this is a binop, find its precedence.
    while (true) {
        int TokPrec = GetTokPrecedence();

        // If this is a binop that binds at least as tightly as the current binop,
        // consume it, otherwise we are done.
        if (TokPrec < ExprPrec)
            return LHS;

        auto prevBinop = binop;
        getNextToken();  // eat binop

        // Parse the primary expression after the binary operator.
        auto RHS = ParsePrimary();
        if (!RHS)
            return nullptr;

        // If BinOp binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        int NextPrec = GetTokPrecedence();
        if (TokPrec < NextPrec) {
            RHS = ParseBinOpRHS(TokPrec+1, std::move(RHS));
            if (!RHS)
                return nullptr;
        }

        // Merge LHS/RHS.
        LHS = std::make_unique<BinaryExprAST>(prevBinop, std::move(LHS),
                                              std::move(RHS));
    }  // loop around to the top of the while loop.
}

/// prototype
///   ::= id '(' id* ')'
std::unique_ptr<PrototypeAST> ParserLexer::ParsePrototype() {
    if (CurTok != tok_identifier)
        return LogErrorP("Expected function name in prototype");

    std::string FnName = IdentifierStr;
    getNextToken();

    if (CurTok != '(')
        return LogErrorP("Expected '(' in prototype");

    // Read the list of argument names.
    std::vector<std::string> ArgNames;
    while (getNextToken() == tok_identifier)
        ArgNames.push_back(IdentifierStr);
    if (CurTok != ')')
        return LogErrorP("Expected ')' in prototype");

    // success.
    getNextToken();  // eat ')'.

    return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}


/// definition ::= 'def' prototype expression
std::unique_ptr<FunctionAST> ParserLexer::ParseDefinition() {
    getNextToken(); // eat def.
    auto Proto = ParsePrototype();
    if (!Proto)
        return nullptr;

    if (auto E = ParseExpression()) {
        auto temp =  std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
        return temp;
    }
    return nullptr;
}

void ParserLexer::HandleDefinition() {
    if (ParseDefinition()) {
        fprintf(stderr, "Parsed a function definition.\n");
    } else {
        // Skip token for error recovery.
        getNextToken();
    }
}

/// top ::= definition | external | expression | ';'
void ParserLexer::MainLoop() {
    while (true) {
        fprintf(stderr, "ready> ");
        switch (CurTok) {
            case tok_eof:
                return;
            case ';': // ignore top-level semicolons.
                getNextToken();
                break;
            case tok_def:
                HandleDefinition();
                break;
            default:
                getNextToken();
                fprintf(stderr, "Warning: uncaught parse error!\n");
//            case tok_extern:
//                HandleExtern();
//                break;
//            default:
//                HandleTopLevelExpression();
//                break;
        }
    }
}

// note functions declared using def cannot be passed in as arguments
// only predicates, synthetic functions, or expressions (those which cannot be executed and only can things proven about them) can be passed as arguments