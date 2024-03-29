/* File: ast_stmt.cc
 * -----------------
 * Implementation of statement node classes.
 */
#include "ast_stmt.h"
#include "ast_decl.h"
#include "ast_expr.h"
#include "ast_type.h"
#include "hashtable.h"
#include <set>
#include <string>
using std::set;
using std::string;
Program::Program(List<Decl *> *d) {
    Assert(d != NULL);
    (decls = d)->SetParentAll(this);
}

class Context;
void Program::Check() {
    /* pp3: here is where the semantic analyzer is kicked off.
     *      The general idea is perform a tree traversal of the
     *      entire program, examining all constructs for compliance
     *      with the semantic rules.  Each node can have its own way of
     *      checking itself, which makes for a great use of inheritance
     *      and polymorphism in the node classes.
     */

    StackNode::root = new StackNode(NULL);
    StackNode::namedTypeTable = new StackNode(NULL);
    // Add global decls to locate the classes and interfaces
    this->AddGlobal();

    // Patch all wrong type to unknown type and check extends type of class
    this->CheckType();

    // visited is only for resolving circular extend in classes
    set<Node *> *visited = new set<Node *>();
    // mainly in declarations and stmtblock
    this->ResolveConflict(visited);

    //argument for context passing (in loop? rettype?)
    Context ctx;

    /* //recursive approach */

    /* //mark duplicate node as error node. */
    //this->resolve_conflict_decl(ctx);

    //annotate source node
    //report error for undeclared identifiers and marked as error type.
    //except for field access right side, T_This and

    for (auto &i : children()) {
        if (i) {
            i->Check(ctx);
        }
    }
    return;
}

void Program::Emit() {
    // first locations
    int cnt = 0;
    for (int i = 0; i < decls->NumElements(); i++) {
        Decl *decl = decls->Nth(i);
        VarDecl *var = dynamic_cast<VarDecl *>(decl);
        if (var) {
            var->location =
                new Location(gpRelative, (cnt++) * 4, var->GetName());
            continue;
        }
        // generate labels for functions and generate locations for classes members
        decl->PrePareForEmit();
        // check if exists main
    }
    if (!CodeGenerator::instance->ifMain) {
        ReportError::NoMainFound();
        return;
    }
    // local vars locations are all at the beginning so we can combine
    // this with emit
    for (auto &i : children()) {
        VarDecl *var = dynamic_cast<VarDecl *>(i);
        // Fn or Class
        if (var == NULL) {
            i->Emit();
        }
    }
    CodeGenerator::instance->DoFinalCodeGen();
}

vector<Node *> Program::children() {
    vector<Node *> result = Node::children();
    result.assign(decls->elems.begin(), decls->elems.end());
    return result;
}

StmtBlock::StmtBlock(List<VarDecl *> *d, List<Stmt *> *s) {
    Assert(d != NULL && s != NULL);
    (decls = d)->SetParentAll(this);
    (stmts = s)->SetParentAll(this);
}

vector<Node *> StmtBlock::children() {
    vector<Node *> result = Stmt::children();
    result.assign(decls->elems.begin(), decls->elems.end());
    result.insert(result.end(), stmts->elems.begin(), stmts->elems.end());
    return result;
}

void StmtBlock::Emit() {
    for (auto &i : children()) {
        i->Emit();
    }
}

void StmtBlock::ResolveConflict(set<Node *> *visited) {
    if (scope == NULL) {
        scope = new StackNode(parent->scope);
        for (int i = 0; i < decls->NumElements(); i++) {
            VarDecl *decl = decls->Nth(i);
            scope->AddSymbol(decl->GetName(), decl);
        }
    }

    for (int i = 0; i < stmts->NumElements(); i++) {
        Stmt *stmt = stmts->Nth(i);
        stmt->ResolveConflict(visited);
    }
    /* Node::ResolveConflict(visited); */
}

ConditionalStmt::ConditionalStmt(Expr *t, Stmt *b) {
    Assert(t != NULL && b != NULL);
    (test = t)->SetParent(this);
    (body = b)->SetParent(this);
}

vector<Node *> ConditionalStmt::children() {
    vector<Node *> result = Stmt::children();
    result.push_back(test);
    result.push_back(body);
    return result;
}

ForStmt::ForStmt(Expr *i, Expr *t, Expr *s, Stmt *b) : LoopStmt(t, b) {
    Assert(i != NULL && t != NULL && s != NULL && b != NULL);
    (init = i)->SetParent(this);
    (step = s)->SetParent(this);
}

vector<Node *> ForStmt::children() {
    vector<Node *> result = LoopStmt::children();
    result.push_back(init);
    result.push_back(step);
    return result;
}

void ForStmt::Emit() {
    init->Emit();
    const char *beginLabel = CodeGenerator::instance->NewLabel();
    const char *breakLabel = CodeGenerator::instance->NewLabel();
    CodeGenerator::instance->GenLabel(beginLabel);
    Location *testLocation = test->cgen();
    CodeGenerator::instance->GenIfZ(testLocation, breakLabel);
    CodeGenerator::instance->loopEndLabels->push(
        breakLabel); // labels recorded for break
    body->Emit();
    step->Emit();
    CodeGenerator::instance->GenGoto(beginLabel);
    Assert(CodeGenerator::instance->loopEndLabels->size());
    const char *tmp = CodeGenerator::instance->loopEndLabels->top();
    Assert(strcmp(tmp, breakLabel) == 0);
    CodeGenerator::instance->loopEndLabels->pop();
    CodeGenerator::instance->GenLabel(breakLabel);
}

void WhileStmt::Emit() {
    // same as for stmt without step and init
    const char *beginLabel = CodeGenerator::instance->NewLabel();
    const char *breakLabel = CodeGenerator::instance->NewLabel();
    CodeGenerator::instance->GenLabel(beginLabel);
    Location *testLocation = test->cgen();
    CodeGenerator::instance->GenIfZ(testLocation, breakLabel);
    CodeGenerator::instance->loopEndLabels->push(
        breakLabel); // labels recorded for break
    body->Emit();
    CodeGenerator::instance->GenGoto(beginLabel);
    Assert(CodeGenerator::instance->loopEndLabels->size());
    const char *tmp = CodeGenerator::instance->loopEndLabels->top();
    Assert(strcmp(tmp, breakLabel) == 0);
    CodeGenerator::instance->loopEndLabels->pop();
    CodeGenerator::instance->GenLabel(breakLabel);
}

void BreakStmt::Emit() {
    const char *label = CodeGenerator::instance->loopEndLabels->top();
    CodeGenerator::instance->GenGoto(label);
}

IfStmt::IfStmt(Expr *t, Stmt *tb, Stmt *eb) : ConditionalStmt(t, tb) {
    Assert(t != NULL && tb != NULL); // else can be NULL
    elseBody = eb;
    if (elseBody)
        elseBody->SetParent(this);
}

vector<Node *> IfStmt::children() {
    vector<Node *> result = ConditionalStmt::children();
    result.push_back(elseBody);

    return result;
}

void IfStmt::Emit() {
    const char *elseLabel = NULL;
    if (elseBody)
        elseLabel = CodeGenerator::instance->NewLabel();
    const char *endLabel = CodeGenerator::instance->NewLabel();
    Location *testLocation = test->cgen();
    if (elseBody) {
        CodeGenerator::instance->GenIfZ(testLocation, elseLabel);
        body->Emit();
        CodeGenerator::instance->GenGoto(endLabel);
        CodeGenerator::instance->GenLabel(elseLabel);
        elseBody->Emit();
    } else {
        CodeGenerator::instance->GenIfZ(testLocation, endLabel);
        body->Emit();
    }
    CodeGenerator::instance->GenLabel(endLabel);
}

ReturnStmt::ReturnStmt(yyltype loc, Expr *e) : Stmt(loc) {
    Assert(e != NULL);
    (expr = e)->SetParent(this);
}

vector<Node *> ReturnStmt::children() {
    vector<Node *> result = Stmt::children();
    result.push_back(expr);
    return result;
}

void ReturnStmt::Emit() {
    Location *returnLocation = NULL;
    if (expr) {
        returnLocation = expr->cgen();
    }
    CodeGenerator::instance->GenReturn(returnLocation);
    return;
}

PrintStmt::PrintStmt(List<Expr *> *a) {
    Assert(a != NULL);
    (args = a)->SetParentAll(this);
}

vector<Node *> PrintStmt::children() {
    vector<Node *> result = Stmt::children();
    result.insert(result.end(), args->elems.begin(), args->elems.end());

    return result;
}

void PrintStmt::Check(Context ctx) {

    Stmt::Check(ctx);
    int c = 0;
    for (auto &i : args->elems) {
        c++;
        //if(isErrorTypeName(i->c))
        if (isErrorTypeName(i->cachedType->GetName()))
            continue;
        if ((string) i->cachedType->GetName() == "int" ||
            (string) i->cachedType->GetName() == "bool" ||
            (string) i->cachedType->GetName() == "string")
            continue;

        ReportError::PrintArgMismatch(i, c, i->cachedType);
    }
}

void PrintStmt::Emit() {
    for (auto &i : args->elems) {
        // printf("%s\n", i->cachedType->GetName());
        if ((string) i->cachedType->GetName() == "int") {
            CodeGenerator::instance->GenBuiltInCall(PrintInt, i->cgen(), NULL);
        }
        if ((string) i->cachedType->GetName() == "bool") {
            CodeGenerator::instance->GenBuiltInCall(PrintBool, i->cgen(), NULL);
        }
        if ((string) i->cachedType->GetName() == "string") {
            CodeGenerator::instance->GenBuiltInCall(PrintString, i->cgen(),
                                                    NULL);
        }
    }
}

void ConditionalStmt::Check(Context ctx) {
    Stmt::Check(ctx);
    if (!(isErrorTypeName(test->cachedType->GetName()) ||
          (string) test->cachedType->GetName() == "bool")) {
        ReportError::TestNotBoolean(test);
    }
}

void ReturnStmt::Check(Context ctx) {
    Stmt::Check(ctx);
    if (expr->cachedType) {
        if (!typeMatchParent(expr->cachedType, ctx.rettype))
            ReportError::ReturnMismatch(this, expr->cachedType, ctx.rettype);
    } else {
        if ((string) ctx.rettype->GetName() != "void")
            ReportError::ReturnMismatch(this, Type::voidType, ctx.rettype);
    }
}