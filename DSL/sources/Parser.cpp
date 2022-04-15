#include "../headers/Parser.h"

Parser::Parser(V *tokenList) : tokenList(*tokenList), listIt(this->tokenList.begin()),
                               curLineNum(1){}

Parser::~Parser()
{
    for (size_t it = 0; it < tokenList.size(); ++it)
    {
        delete tokenList[it];
    }

    tokenList.clear();
}

void Parser::syntaxCheck()
{   
    while (listIt != tokenList.end())
    {
        try
        {
            if ((*listIt)->type == "FUNCTION")
            {
                this->functionCheck();
            }
            else if ((*listIt)->type == "L_BRACE")
            {
                this->blockCheck();
            }
            else if ((*listIt)->type == "VARIABLE")
            {
                this->assignmentCheck();
            }
            else { break; }
        }
        catch(const std::string &exception)
        {
            this->handleException(exception);
            break;
        }
    }
}

/* ----------------------EXCEPTIONS----------------------- */

void Parser::generateException(std::string expected, bool endl=false)
{
    if (listIt!=tokenList.end()) { curLineNum = (*listIt)->line; }
    else { curLineNum = (*(tokenList.end()--))->line; }

    if (endl)
    {
        throw "in line " + std::to_string(curLineNum) + "\nexpected: " + expected + 
        "; \\n was provided";
    }

    throw "in line " + std::to_string(curLineNum) + "\nexpected: " + expected + "; " + 
    ((listIt==tokenList.end())? "nothing" :  (*listIt)->type) + " was provided";
}

void Parser::handleException(const std::string &exception)  
{ 
    std::cerr<<exception<<"\nsyntax analysis has been stopped\n"; 
}

/* ----------------------EXPRESSIONS----------------------- */

void Parser::functionCheck()
{
    this->listIt++;

    if (listIt!=tokenList.end()) 
    { 
        if ((*listIt)->type == "L_BRACKET") { listIt++; } 
    }
    else { this->generateException("("); }

    if (listIt!=tokenList.end()) { this->argsCheck(); }
}

void Parser::argsCheck()
{
    if (listIt==tokenList.end()) { this->generateException(") or argument"); }

    else if ((*listIt)->type == "R_BRACKET") { listIt++; }
    else if (((*listIt)->type == "VARIABLE" or (*listIt)->type == "INTEGER" or
             (*listIt)->type == "STRING"))
    { 
        listIt++;

        if (listIt==tokenList.end()) { this->generateException(") or ,"); }

        while (true)
        {
            if (listIt==tokenList.end()) { this->generateException(") or argument"); }
            else if ((*listIt)->type == "R_BRACKET") 
            {
                listIt++;
                break;
            }

            if ((*listIt)->type == "ARG_SEPARATOR")
            {
                listIt++;

                if (listIt==tokenList.end()) { this->generateException(") or argument"); }
                else if ((*listIt)->type == "VARIABLE" or (*listIt)->type == "INTEGER" or
                         (*listIt)->type == "STRING")
                {
                    listIt++;
                }
                else {  this->generateException(") or argument");   }
            }
        }
    }
}

void Parser::blockCheck()
{
    this->listIt++;

    if (listIt==tokenList.end()) { this->generateException("} or expression"); }
    else if ((*listIt)->type == "R_BRACE")
    { 
        listIt++; 
        return;
    }

    while (true)
    {
        if (listIt==tokenList.end()) { this->generateException("} or expression"); }
        else if ((*listIt)->type == "R_BRACE")
        {
            listIt++;
            break;
        }
        else 
        {
            if      ((*listIt)->type == "RETURN")   { listIt++; }
            else if ((*listIt)->type == "VARIABLE") { this->assignmentCheck(); }
            else if ((*listIt)->type == "FUNCTION") { this->functionCheck(); }
            else if ((*listIt)->type == "L_BRACE")  { this->blockCheck(); }
        }
    }
}

void Parser::assignmentCheck()
{
    if (listIt==tokenList.end()) { this->generateException("variable"); }

    if ((*listIt)->type == "VARIABLE") { listIt++; }
    else { this->generateException("variable"); }


    if (listIt==tokenList.end()) { this->generateException("variable"); }

    if ((*listIt)->type == "ASSIGN_OPERATOR") { listIt++; }
    else { this->generateException("assignment operator");}

    this->expressionCheck();
}

void Parser::expressionCheck()
{
    if (listIt==tokenList.end()) { this->generateException("value"); }
    else if ((*listIt)->type == "VARIABLE" or (*listIt)->type == "INTEGER" or
             (*listIt)->type == "STRING")
    {   
        curLineNum = (*listIt)->line;
        listIt++;
    }
    else { this->generateException("value"); }

    while (true)
    {
        if (listIt==tokenList.end()) { break; }
        if ((*listIt)->type=="MATH_OPERATOR")
        {
            curLineNum = (*listIt)->line;
            listIt++;

            if (listIt==tokenList.end())                   { this->generateException("value"); }
            
            if ((*listIt)->type == "VARIABLE" or (*listIt)->type == "INTEGER" or
                (*listIt)->type == "STRING") 
            { 
                if (curLineNum < (*listIt)->line) 
                {   
                    listIt--;
                    this->generateException("value", true); 
                }

                curLineNum = (*listIt)->line;
                listIt++; 
            }

            if (listIt==tokenList.end())  { this->generateException("math. operator or \\n"); }
            else if ((*listIt)->type!="MATH_OPERATOR")
            {
                listIt--;
                if ((*listIt)->type=="MATH_OPERATOR")      { this->generateException("value"); }
                else { listIt++; }

                if (curLineNum < (*listIt)->line)
                {
                    curLineNum = (*listIt)->line;
                    if ((*listIt)->type == "VARIABLE") { this->assignmentCheck(); }
                    break;
                }
                else { this->generateException("math. operator or \\n"); }
            }
            else 
            {
                listIt--;
                if ((*listIt)->type=="MATH_OPERATOR")      { this->generateException("value"); }
                else { listIt++; }

                listIt++; 
                if (listIt==tokenList.end())               { this->generateException("value"); }
                if ((*listIt)->type == "VARIABLE" or (*listIt)->type == "INTEGER" or
                    (*listIt)->type == "STRING")
                {
                    if (listIt==tokenList.end())           { this->generateException("value"); }
                    else if (curLineNum < (*listIt)->line) { this->generateException("value"); }
                    else { listIt++; }
                }
                else { this->generateException("value"); }
            }
        }
        else
        {
            if (curLineNum < (*listIt)->line)
            {
                curLineNum = (*listIt)->line;
                if ((*listIt)->type == "VARIABLE") { this->assignmentCheck(); }
                break;
            }
            else { this->generateException("math. operator or \\n"); }
        }
    }
}