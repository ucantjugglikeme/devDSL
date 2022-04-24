#include "../headers/Parser.h"

Parser::Parser(V *tokenList) : tokenList(*tokenList), listIt(this->tokenList.begin()),
                               curLineNum(1), lines((tokenList->back())->line){}

Parser::~Parser()
{
    for (size_t it = 0; it < tokenList.size(); ++it) { delete tokenList[it]; }
    tokenList.clear();
}

// lang -> (expr)*

int Parser::lang()
{   
    std::cout<<"in lang!\n";
    try {  while (listIt != tokenList.end()) { this->expr(); }  }
    catch (ParsingException &e) { return 1; }
    return 0;
}

// expr -> (functionDef | functionCall | assignment | opIf | opWhile){1}

void Parser::expr()
{
    const FuncV expressions =
    {
        {"functionDef", &Parser::functionDef}, {"functionCall", &Parser::functionCall},
        {"assignment", &Parser::assignment}, {"opIf", &Parser::opIf},
        {"opWhile", &Parser::opWhile}, {"failure", &Parser::generateFailure}
    };
    VecIt fixedIt = listIt;

    std::cout<<"in expr!\n";
    for (KeyFunc keyFuncIt : expressions)
    {
        try
        {
            std::cout<<"trying "<<(&keyFuncIt)->first<<"\n";
            (this->*((&keyFuncIt)->second))();
            std::cout<<"matched\n";
            fixedIt = listIt;
            break;
        }
        catch (ParsingException & e)
        {
            std::cerr<<e.what()<<"\n";
            if (e.getWhy() == "failed to parse") { throw e; }
            listIt = fixedIt;
        }
    }
}

/* --------------------------------------------EXCEPTIONS----------------------------------------- */

string Parser::generateException(string expected, string provided)
{
    curLineNum = provided == "nothing" ? lines : (*listIt)->line;
    return "in line " + std::to_string(curLineNum) + "\nexpected: " + expected + "; " +
            provided + " was provided";
}

void Parser::generateFailure() { throw ParsingException(std::string("failed to parse")); }

/* ------------------------------------------EXPRESSIONS------------------------------------------ */

void Parser::functionDef()
{
    // functionCall -> function{1}lBracket{1}(variable{1}((,){1}variable{1})*)?rBracket{1}block{1}
    this->keyword("def");
    this->function();
    this->lBracket();

    try { this->variable(); }
    catch (ParsingException & e) 
    { 
        this->rBracket();
        this->block();
        return;
    }
    while (listIt!=tokenList.end())
    {
        try { this->argsSeparator(); }
        catch (ParsingException &e) { break; }
        this->variable();
    }
    
    this->rBracket();
    this->block();
}

void Parser::functionCall()
{
    // functionCall -> function{1}lBracket{1}(value{1}((,){1}
    // (arithmeticExpression{1}|conditionalExpression{1}))*)?rBracket{1}separator?
    this->function();
    this->lBracket();

    try { this->value(); }
    catch (ParsingException &e) 
    { 
        this->rBracket();
        return;
    }
    while (listIt!=tokenList.end())
    {
        try { this->argsSeparator(); }
        catch (ParsingException &e) { break; }
        try { this->arithmeticExpression(); }
        catch (ParsingException &e) { this->conditionalExpression();}
    }

    this->rBracket();

    try { this->separator(); }
    catch (ParsingException &e){}
}

void Parser::block()
{
    // block -> (\{){1}
    // (functionDef|functionCall|assignment|opIf|opReturn|opWhile|opBreak|opContinue)?
    // (\}){1}
    this->lBrace();

    const FuncV blockExpressions
    {
        {"functionDef", &Parser::functionDef}, {"functionCall", &Parser::functionCall},
        {"assignment", &Parser::assignment}, {"opIf", &Parser::opIf},
        {"opReturn", &Parser::opReturn}, {"opWhile", &Parser::opWhile},
        {"opBreak", &Parser::opBreak}, {"opContinue", &Parser::opContinue},
        {"failure", &Parser::generateFailure}
    };
    VecIt fixedIt = listIt;    

    std::cout<<"in block!\n";
    while (listIt!=tokenList.end())
    {
        try
        {
            for (KeyFunc keyFuncIt : blockExpressions)
            {
                try 
                {
                    std::cout<<"trying "<<(&keyFuncIt)->first<<"\n";
                    (this->*((&keyFuncIt)->second))();
                    std::cout<<"matched\n";
                    fixedIt = listIt;
                    break;
                }
                catch (ParsingException & e) 
                {
                    std::cerr<<e.what()<<"\n";
                    if (e.getWhy() == "failed to parse") { throw e; }
                    listIt = fixedIt;
                }
            }
        }
        catch (ParsingException & e) { this->rBrace(); break; }
    }
}

void Parser::assignment()
{
    // assignment -> variable{1}(=){1}(arithmeticExpression|conditionalExpression){1}
    this->variable();

    this->assignOp();

    try { this->arithmeticExpression(); }
    catch (ParsingException & e) { this->conditionalExpression(); }
}

void Parser::arithmeticExpression()
{
    // arithmeticExpression -> ( (\(arithmeticExpression{1}\)mathOp{1})?notEndLine{1}
    // value{1}((mathOp{1}arithmeticExpression{1})|((;)?)){1}
    do 
    {
        try
        {
            try { this->lBracket(); }
            catch (ParsingException & e) { break; }
            this->arithmeticExpression();
            this->rBracket();
            try { this->mathOp(); }
            catch (ParsingException & e) { return; }
        }
        catch (ParsingException & e) { throw e; }
    } while (false);

    this->notEndLine();
    this->value();

    try { this->mathOp(); }
    catch (ParsingException & e)
    {
        try { this->separator(); }
        catch (ParsingException & e){}
        return;
    }
    this->arithmeticExpression();
}

void Parser::conditionalExpression()
{
    // conditionalExpression -> ( (\(conditionalExpression{1}\)comprOp{1})?notEndLine{1}
    // value{1}((comprOp{1}conditionalExpression{1})|((;)?)){1}
    do 
    {
        try
        {
            try {this->lBracket();}
            catch (ParsingException & e) { break; }
            this->conditionalExpression();
            this->rBracket();
            try { this->comprOp(); }
            catch (ParsingException & e) { return; }
        }
        catch (ParsingException & e) { throw e; }
    } while (false);

    this->notEndLine();
    this->value();

    try { this->comprOp(); }
    catch (ParsingException & e)
    {
        try { this->separator(); }
        catch (ParsingException & e){}
        return;
    }
    this->conditionalExpression();
}

void Parser::opReturn()
{
    // opReturn -> (return){1}arithmeticExpression?separator?
    this->keyword("return");

    try { this->arithmeticExpression(); }
    catch (ParsingException &e){}

    try { this->separator(); }
    catch (ParsingException &e){}
}

void Parser::opBreak()
{
    // opBreak -> (break){1}separator?
    this->keyword("break");

    try { this->separator(); }
    catch (ParsingException &e){}
}

void Parser::opContinue()
{
    // opContinue -> (continue){1}separator?
    this->keyword("continue");

    try { this->separator(); }
    catch (ParsingException &e){}
}

void Parser::opIf()
{
    // opIf -> (if){1}(\(){1}conditionalExpression{1}(\)){1}block{1}opElif?opElse?
    this->keyword("if");
    this->lBracket();
    this->conditionalExpression();
    this->rBracket();
    this->block();

    try { this->opElif(); }
    catch (ParsingException & e){}

    try { this->opElse(); }
    catch (ParsingException & e){}
}

void Parser::opElif()
{
    // opElif -> (elif){1}(\(){1}conditionalExpression{1}(\)){1}block{1}opElif?opElse?
    this->keyword("elif");
    this->lBracket();
    this->conditionalExpression();
    this->rBracket();
    this->block();

    try { this->opElif(); }
    catch (ParsingException & e){}

    try { this->opElse(); }
    catch (ParsingException & e){}
}

void Parser::opElse()
{
    // else -> (else){1}block{1}
    this->keyword("else");
    this->block();
}

void Parser::opWhile()
{
    // while -> (while){1}(\(){1}conditionalExpression{1}(\)){1}block{1}
    this->keyword("while");
    this->lBracket();
    this->conditionalExpression();
    this->rBracket();
    this->block();
}

/*----------------------------------------------------ATHOMS-----------------------------------------------*/

void Parser::keyword(string concrete)
{
    if (listIt == tokenList.end()) 
    {
        throw ParsingException(generateException(concrete, string("nothing")));
    }
    else if (std::strcmp((*listIt)->value.c_str(), concrete.c_str()) != 0) 
    {
        throw ParsingException(generateException(concrete, (*listIt)->type)); 
    }
    else { listIt++; }
}

void Parser::function()
{
    if (listIt == tokenList.end())
    {
        throw ParsingException(generateException(string("function"), string("nothing")));
    }
    else if ((*listIt)->type != "FUNCTION")
    {
        throw ParsingException(generateException(string("function"), (*listIt)->type));
    }
    else { listIt++; }
}

void Parser::lBracket()
{
    if (listIt == tokenList.end())
    {
        throw ParsingException(generateException(std::string("("), string("nothing")));
    }
    else if ((*listIt)->type != "L_BRACKET")
    {
        throw ParsingException(generateException(std::string("("), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::variable()
{
    if (listIt == tokenList.end())
    {
        throw ParsingException(generateException(std::string("variable"), string("nothing")));
    }
    else if ((*listIt)->type != "VARIABLE")
    {
        throw ParsingException(generateException(std::string("variable"), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::argsSeparator()
{
    if (listIt == tokenList.end())
    {
        throw ParsingException(generateException(std::string(","), string("nothing")));
    }
    else if ((*listIt)->type != "ARG_SEPARATOR")
    {
        throw ParsingException(generateException(std::string(","), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::rBracket()
{
    if (listIt == tokenList.end())
    {
        throw ParsingException(generateException(std::string(")"), string("nothing")));
    }
    else if ((*listIt)->type != "R_BRACKET")
    {
        throw ParsingException(generateException(std::string(")"), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::lBrace()
{
    if (listIt == tokenList.end())
    {
        throw ParsingException(generateException(std::string("{"), string("nothing")));
    }
    else if ((*listIt)->type != "L_BRACE")
    {
        throw ParsingException(generateException(std::string("{"), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::rBrace()
{
    if (listIt == tokenList.end())
    {
        throw ParsingException(generateException(std::string("}"), string("nothing")));
    }
    else if ((*listIt)->type != "R_BRACE")
    {
        throw ParsingException(generateException(std::string("}"), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::assignOp()
{
    if (listIt == tokenList.end())
    {
        throw ParsingException(generateException(std::string("="), string("nothing")));
    }
    else if ((*listIt)->type != "ASSIGN_OPERATOR")
    {
        throw ParsingException(generateException(std::string("="), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::value()
{
    if (listIt == tokenList.end())
    {
        throw ParsingException(generateException(std::string("value"), string("nothing")));
    }
    else if ((*listIt)->type != "INTEGER" and (*listIt)->type != "STRING" and
             (*listIt)->type != "VARIABLE")
    {
        throw ParsingException(generateException(std::string("value"), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::mathOp()
{
    if (listIt == tokenList.end())
    {
        throw ParsingException(generateException(std::string("math. operator"), string("nothing")));
    }
    else if ((*listIt)->type != "MATH_OPERATOR")
    {
        throw ParsingException(generateException(std::string("math. operator"), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::separator()
{
    if (listIt == tokenList.end())
    {
        throw ParsingException(generateException(std::string(";"), string("nothing")));
    }
    else if ((*listIt)->type != "SEPARATOR")
    {
        throw ParsingException(generateException(std::string(";"), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::comprOp()
{
    if (listIt == tokenList.end())
    {
        throw ParsingException(generateException(std::string("compr. operator"), string("nothing")));
    }
    else if ((*listIt)->type != "COMPRASION_OPERATOR")
    {
        throw ParsingException(generateException(std::string("compr. operator"), (*listIt)->type));
    }
    else { listIt++; }   
}

void Parser::notEndLine()
{
    if (listIt == tokenList.end())
    {
        throw ParsingException(generateException(string("value"), string("nothing")));
    }

    listIt--;
    int prevTokenLine = (*listIt)->line;

    listIt++;
        curLineNum    = (*listIt)->line;

    if (curLineNum > prevTokenLine)
    {
        throw ParsingException(generateException(std::string("value"), (*listIt)->type));
    }
}