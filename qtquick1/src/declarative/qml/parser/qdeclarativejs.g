----------------------------------------------------------------------------
--
-- Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
-- Contact: http://www.qt-project.org/legal
--
-- This file is part of the QtDeclarative module of the Qt Toolkit.
--
-- $QT_BEGIN_LICENSE:LGPL-ONLY$
-- GNU Lesser General Public License Usage
-- This file may be used under the terms of the GNU Lesser
-- General Public License version 2.1 as published by the Free Software
-- Foundation and appearing in the file LICENSE.LGPL included in the
-- packaging of this file.  Please review the following information to
-- ensure the GNU Lesser General Public License version 2.1 requirements
-- will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
--
-- If you have questions regarding the use of this file, please contact
-- us via http://www.qt-project.org/.
--
-- $QT_END_LICENSE$
--
----------------------------------------------------------------------------

%parser         QDeclarativeJSGrammar
%decl           qdeclarativejsparser_p.h
%impl           qdeclarativejsparser.cpp
%expect         2
%expect-rr      2

%token T_AND "&"                T_AND_AND "&&"              T_AND_EQ "&="
%token T_BREAK "break"          T_CASE "case"               T_CATCH "catch"
%token T_COLON ":"              T_COMMA ";"                 T_CONTINUE "continue"
%token T_DEFAULT "default"      T_DELETE "delete"           T_DIVIDE_ "/"
%token T_DIVIDE_EQ "/="         T_DO "do"                   T_DOT "."
%token T_ELSE "else"            T_EQ "="                    T_EQ_EQ "=="
%token T_EQ_EQ_EQ "==="         T_FINALLY "finally"         T_FOR "for"
%token T_FUNCTION "function"    T_GE ">="                   T_GT ">"
%token T_GT_GT ">>"             T_GT_GT_EQ ">>="            T_GT_GT_GT ">>>"
%token T_GT_GT_GT_EQ ">>>="     T_IDENTIFIER "identifier"   T_IF "if"
%token T_IN "in"                T_INSTANCEOF "instanceof"   T_LBRACE "{"
%token T_LBRACKET "["           T_LE "<="                   T_LPAREN "("
%token T_LT "<"                 T_LT_LT "<<"                T_LT_LT_EQ "<<="
%token T_MINUS "-"              T_MINUS_EQ "-="             T_MINUS_MINUS "--"
%token T_NEW "new"              T_NOT "!"                   T_NOT_EQ "!="
%token T_NOT_EQ_EQ "!=="        T_NUMERIC_LITERAL "numeric literal"     T_OR "|"
%token T_OR_EQ "|="             T_OR_OR "||"                T_PLUS "+"
%token T_PLUS_EQ "+="           T_PLUS_PLUS "++"            T_QUESTION "?"
%token T_RBRACE "}"             T_RBRACKET "]"              T_REMAINDER "%"
%token T_REMAINDER_EQ "%="      T_RETURN "return"           T_RPAREN ")"
%token T_SEMICOLON ";"          T_AUTOMATIC_SEMICOLON       T_STAR "*"
%token T_STAR_EQ "*="           T_STRING_LITERAL "string literal"
%token T_PROPERTY "property"    T_SIGNAL "signal"           T_READONLY "readonly"
%token T_SWITCH "switch"        T_THIS "this"               T_THROW "throw"
%token T_TILDE "~"              T_TRY "try"                 T_TYPEOF "typeof"
%token T_VAR "var"              T_VOID "void"               T_WHILE "while"
%token T_WITH "with"            T_XOR "^"                   T_XOR_EQ "^="
%token T_NULL "null"            T_TRUE "true"               T_FALSE "false"
%token T_CONST "const"
%token T_DEBUGGER "debugger"
%token T_RESERVED_WORD "reserved word"
%token T_MULTILINE_STRING_LITERAL "multiline string literal"
%token T_COMMENT "comment"
%token T_COMPATIBILITY_SEMICOLON

--- context keywords.
%token T_PUBLIC "public"
%token T_IMPORT "import"
%token T_AS "as"
%token T_ON "on"

--- feed tokens
%token T_FEED_UI_PROGRAM
%token T_FEED_UI_OBJECT_MEMBER
%token T_FEED_JS_STATEMENT
%token T_FEED_JS_EXPRESSION
%token T_FEED_JS_SOURCE_ELEMENT
%token T_FEED_JS_PROGRAM

%nonassoc SHIFT_THERE
%nonassoc T_IDENTIFIER T_COLON T_SIGNAL T_PROPERTY T_READONLY
%nonassoc REDUCE_HERE

%start TopLevel

/./****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/QtDebug>
#include <QtWidgets/QApplication>

#include <string.h>

#include "private/qdeclarativejsengine_p.h"
#include "private/qdeclarativejslexer_p.h"
#include "private/qdeclarativejsast_p.h"
#include "private/qdeclarativejsnodepool_p.h"

./

/:/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

//
//  W A R N I N G
//  -------------
//
// This file is automatically generated from qdeclarativejs.g.
// Changes should be made to that file, not here. Any change to this file will
// be lost!
//
// To regenerate this file, run:
//    qlalr --no-debug --no-lines --qt qdeclarativejs.g
//

#ifndef QDECLARATIVEJSPARSER_P_H
#define QDECLARATIVEJSPARSER_P_H

#include "private/qdeclarativejsglobal_p.h"
#include "private/qdeclarativejsgrammar_p.h"
#include "private/qdeclarativejsast_p.h"
#include "private/qdeclarativejsengine_p.h"

#include <QtCore/QList>
#include <QtCore/QString>

QT_QML_BEGIN_NAMESPACE

namespace QDeclarativeJS {

class Engine;
class NameId;

class QML_PARSER_EXPORT Parser: protected $table
{
public:
    union Value {
      int ival;
      double dval;
      NameId *sval;
      AST::ArgumentList *ArgumentList;
      AST::CaseBlock *CaseBlock;
      AST::CaseClause *CaseClause;
      AST::CaseClauses *CaseClauses;
      AST::Catch *Catch;
      AST::DefaultClause *DefaultClause;
      AST::ElementList *ElementList;
      AST::Elision *Elision;
      AST::ExpressionNode *Expression;
      AST::Finally *Finally;
      AST::FormalParameterList *FormalParameterList;
      AST::FunctionBody *FunctionBody;
      AST::FunctionDeclaration *FunctionDeclaration;
      AST::Node *Node;
      AST::PropertyName *PropertyName;
      AST::PropertyNameAndValueList *PropertyNameAndValueList;
      AST::SourceElement *SourceElement;
      AST::SourceElements *SourceElements;
      AST::Statement *Statement;
      AST::StatementList *StatementList;
      AST::Block *Block;
      AST::VariableDeclaration *VariableDeclaration;
      AST::VariableDeclarationList *VariableDeclarationList;

      AST::UiProgram *UiProgram;
      AST::UiImportList *UiImportList;
      AST::UiImport *UiImport;
      AST::UiParameterList *UiParameterList;
      AST::UiPublicMember *UiPublicMember;
      AST::UiObjectDefinition *UiObjectDefinition;
      AST::UiObjectInitializer *UiObjectInitializer;
      AST::UiObjectBinding *UiObjectBinding;
      AST::UiScriptBinding *UiScriptBinding;
      AST::UiArrayBinding *UiArrayBinding;
      AST::UiObjectMember *UiObjectMember;
      AST::UiObjectMemberList *UiObjectMemberList;
      AST::UiArrayMemberList *UiArrayMemberList;
      AST::UiQualifiedId *UiQualifiedId;
      AST::UiSignature *UiSignature;
      AST::UiFormalList *UiFormalList;
      AST::UiFormal *UiFormal;
    };

public:
    Parser(Engine *engine);
    ~Parser();

    // parse a UI program
    bool parse() { return parse(T_FEED_UI_PROGRAM); }
    bool parseStatement() { return parse(T_FEED_JS_STATEMENT); }
    bool parseExpression() { return parse(T_FEED_JS_EXPRESSION); }
    bool parseSourceElement() { return parse(T_FEED_JS_SOURCE_ELEMENT); }
    bool parseUiObjectMember() { return parse(T_FEED_UI_OBJECT_MEMBER); }
    bool parseProgram() { return parse(T_FEED_JS_PROGRAM); }

    AST::UiProgram *ast() const
    { return AST::cast<AST::UiProgram *>(program); }

    AST::Statement *statement() const
    {
        if (! program)
            return 0;

        return program->statementCast();
    }

    AST::ExpressionNode *expression() const
    {
        if (! program)
            return 0;

        return program->expressionCast();
    }

    AST::UiObjectMember *uiObjectMember() const
    {
        if (! program)
            return 0;

        return program->uiObjectMemberCast();
    }

    AST::Node *rootNode() const
    { return program; }

    QList<DiagnosticMessage> diagnosticMessages() const
    { return diagnostic_messages; }

    inline DiagnosticMessage diagnosticMessage() const
    {
        foreach (const DiagnosticMessage &d, diagnostic_messages) {
            if (d.kind != DiagnosticMessage::Warning)
                return d;
        }

        return DiagnosticMessage();
    }

    inline QString errorMessage() const
    { return diagnosticMessage().message; }

    inline int errorLineNumber() const
    { return diagnosticMessage().loc.startLine; }

    inline int errorColumnNumber() const
    { return diagnosticMessage().loc.startColumn; }

protected:
    bool parse(int startToken);

    void reallocateStack();

    inline Value &sym(int index)
    { return sym_stack [tos + index - 1]; }

    inline AST::SourceLocation &loc(int index)
    { return location_stack [tos + index - 1]; }

    AST::UiQualifiedId *reparseAsQualifiedId(AST::ExpressionNode *expr);

protected:
    Engine *driver;
    int tos;
    int stack_size;
    Value *sym_stack;
    int *state_stack;
    AST::SourceLocation *location_stack;

    AST::Node *program;

    // error recovery
    enum { TOKEN_BUFFER_SIZE = 3 };

    struct SavedToken {
       int token;
       double dval;
       AST::SourceLocation loc;
    };

    double yylval;
    AST::SourceLocation yylloc;
    AST::SourceLocation yyprevlloc;

    SavedToken token_buffer[TOKEN_BUFFER_SIZE];
    SavedToken *first_token;
    SavedToken *last_token;

    QList<DiagnosticMessage> diagnostic_messages;
};

} // end of namespace QDeclarativeJS


:/


/.

//
//  W A R N I N G
//  -------------
//
// This file is automatically generated from qdeclarativejs.g.
// Changes should be made to that file, not here. Any change to this file will
// be lost!
//
// To regenerate this file, run:
//    qlalr --no-debug --no-lines --qt qdeclarativejs.g
//

#include "private/qdeclarativejsparser_p.h"
#include <QVarLengthArray>

#include <stdlib.h>

using namespace QDeclarativeJS;

QT_QML_BEGIN_NAMESPACE

void Parser::reallocateStack()
{
    if (! stack_size)
        stack_size = 128;
    else
        stack_size <<= 1;

    sym_stack = reinterpret_cast<Value*> (realloc(sym_stack, stack_size * sizeof(Value)));
    state_stack = reinterpret_cast<int*> (realloc(state_stack, stack_size * sizeof(int)));
    location_stack = reinterpret_cast<AST::SourceLocation*> (realloc(location_stack, stack_size * sizeof(AST::SourceLocation)));
}

inline static bool automatic(Engine *driver, int token)
{
    return token == $table::T_RBRACE
        || token == 0
        || driver->lexer()->prevTerminator();
}


Parser::Parser(Engine *engine):
    driver(engine),
    tos(0),
    stack_size(0),
    sym_stack(0),
    state_stack(0),
    location_stack(0),
    first_token(0),
    last_token(0)
{
}

Parser::~Parser()
{
    if (stack_size) {
        free(sym_stack);
        free(state_stack);
        free(location_stack);
    }
}

static inline AST::SourceLocation location(Lexer *lexer)
{
    AST::SourceLocation loc;
    loc.offset = lexer->tokenOffset();
    loc.length = lexer->tokenLength();
    loc.startLine = lexer->startLineNo();
    loc.startColumn = lexer->startColumnNo();
    return loc;
}

AST::UiQualifiedId *Parser::reparseAsQualifiedId(AST::ExpressionNode *expr)
{
    QVarLengthArray<NameId *, 4> nameIds;
    QVarLengthArray<AST::SourceLocation, 4> locations;

    AST::ExpressionNode *it = expr;
    while (AST::FieldMemberExpression *m = AST::cast<AST::FieldMemberExpression *>(it)) {
        nameIds.append(m->name);
        locations.append(m->identifierToken);
        it = m->base;
    }

    if (AST::IdentifierExpression *idExpr = AST::cast<AST::IdentifierExpression *>(it)) {
        AST::UiQualifiedId *q = makeAstNode<AST::UiQualifiedId>(driver->nodePool(), idExpr->name);
        q->identifierToken = idExpr->identifierToken;

        AST::UiQualifiedId *currentId = q;
        for (int i = nameIds.size() - 1; i != -1; --i) {
            currentId = makeAstNode<AST::UiQualifiedId>(driver->nodePool(), currentId, nameIds[i]);
            currentId->identifierToken = locations[i];
        }

        return currentId->finish();
    }

    return 0;
}

bool Parser::parse(int startToken)
{
    Lexer *lexer = driver->lexer();
    bool hadErrors = false;
    int yytoken = -1;
    int action = 0;

    token_buffer[0].token = startToken;
    first_token = &token_buffer[0];
    last_token = &token_buffer[1];

    tos = -1;
    program = 0;

    do {
        if (++tos == stack_size)
            reallocateStack();

        state_stack[tos] = action;

    _Lcheck_token:
        if (yytoken == -1 && -TERMINAL_COUNT != action_index[action]) {
            yyprevlloc = yylloc;

            if (first_token == last_token) {
                yytoken = lexer->lex();
                yylval = lexer->dval();
                yylloc = location(lexer);
            } else {
                yytoken = first_token->token;
                yylval = first_token->dval;
                yylloc = first_token->loc;
                ++first_token;
            }
        }

        action = t_action(action, yytoken);
        if (action > 0) {
            if (action != ACCEPT_STATE) {
                yytoken = -1;
                sym(1).dval = yylval;
                loc(1) = yylloc;
            } else {
              --tos;
              return ! hadErrors;
            }
        } else if (action < 0) {
          const int r = -action - 1;
          tos -= rhs[r];

          switch (r) {
./

--------------------------------------------------------------------------------------------------------
-- Declarative UI
--------------------------------------------------------------------------------------------------------

TopLevel: T_FEED_UI_PROGRAM UiProgram ;
/.
case $rule_number: {
  sym(1).Node = sym(2).Node;
  program = sym(1).Node;
} break;
./

TopLevel: T_FEED_JS_STATEMENT Statement ;
/.
case $rule_number: {
  sym(1).Node = sym(2).Node;
  program = sym(1).Node;
} break;
./

TopLevel: T_FEED_JS_EXPRESSION Expression ;
/.
case $rule_number: {
  sym(1).Node = sym(2).Node;
  program = sym(1).Node;
} break;
./

TopLevel: T_FEED_JS_SOURCE_ELEMENT SourceElement ;
/.
case $rule_number: {
  sym(1).Node = sym(2).Node;
  program = sym(1).Node;
} break;
./

TopLevel: T_FEED_UI_OBJECT_MEMBER UiObjectMember ;
/.
case $rule_number: {
  sym(1).Node = sym(2).Node;
  program = sym(1).Node;
} break;
./

TopLevel: T_FEED_JS_PROGRAM Program ;
/.
case $rule_number: {
  sym(1).Node = sym(2).Node;
  program = sym(1).Node;
} break;
./

UiProgram: UiImportListOpt UiRootMember ;
/.
case $rule_number: {
  sym(1).UiProgram = makeAstNode<AST::UiProgram> (driver->nodePool(), sym(1).UiImportList,
        sym(2).UiObjectMemberList->finish());
} break;
./

UiImportListOpt: Empty ;
UiImportListOpt: UiImportList ;
/.
case $rule_number: {
    sym(1).Node = sym(1).UiImportList->finish();
} break;
./

UiImportList: UiImport ;
/.
case $rule_number: {
    sym(1).Node = makeAstNode<AST::UiImportList> (driver->nodePool(), sym(1).UiImport);
} break;
./

UiImportList: UiImportList UiImport ;
/.
case $rule_number: {
    sym(1).Node = makeAstNode<AST::UiImportList> (driver->nodePool(),
        sym(1).UiImportList, sym(2).UiImport);
} break;
./

ImportId: MemberExpression ;

UiImport: UiImportHead T_AUTOMATIC_SEMICOLON ;
UiImport: UiImportHead T_SEMICOLON ;
/.
case $rule_number: {
    sym(1).UiImport->semicolonToken = loc(2);
} break;
./

UiImport: UiImportHead T_NUMERIC_LITERAL T_AUTOMATIC_SEMICOLON ;
UiImport: UiImportHead T_NUMERIC_LITERAL T_SEMICOLON ;
/.
case $rule_number: {
    sym(1).UiImport->versionToken = loc(2);
    sym(1).UiImport->semicolonToken = loc(3);
} break;
./

UiImport: UiImportHead T_NUMERIC_LITERAL T_AS JsIdentifier T_AUTOMATIC_SEMICOLON ;
UiImport: UiImportHead T_NUMERIC_LITERAL T_AS JsIdentifier T_SEMICOLON ;
/.
case $rule_number: {
    sym(1).UiImport->versionToken = loc(2);
    sym(1).UiImport->asToken = loc(3);
    sym(1).UiImport->importIdToken = loc(4);
    sym(1).UiImport->importId = sym(4).sval;
    sym(1).UiImport->semicolonToken = loc(5);
} break;
./

UiImport: UiImportHead T_AS JsIdentifier T_AUTOMATIC_SEMICOLON ;
UiImport: UiImportHead T_AS JsIdentifier T_SEMICOLON ;
/.
case $rule_number: {
    sym(1).UiImport->asToken = loc(2);
    sym(1).UiImport->importIdToken = loc(3);
    sym(1).UiImport->importId = sym(3).sval;
    sym(1).UiImport->semicolonToken = loc(4);
} break;
./


UiImportHead: T_IMPORT ImportId ;
/.
case $rule_number: {
    AST::UiImport *node = 0;

    if (AST::StringLiteral *importIdLiteral = AST::cast<AST::StringLiteral *>(sym(2).Expression)) {
        node = makeAstNode<AST::UiImport>(driver->nodePool(), importIdLiteral->value);
        node->fileNameToken = loc(2);
    } else if (AST::UiQualifiedId *qualifiedId = reparseAsQualifiedId(sym(2).Expression)) {
        node = makeAstNode<AST::UiImport>(driver->nodePool(), qualifiedId);
        node->fileNameToken = loc(2);
    }

    sym(1).Node = node;

    if (node) {
        node->importToken = loc(1);
    } else {
       diagnostic_messages.append(DiagnosticMessage(DiagnosticMessage::Error, loc(1),
         QLatin1String("Expected a qualified name id or a string literal")));

        return false; // ### remove me
    }
} break;
./

Empty: ;
/.
case $rule_number: {
    sym(1).Node = 0;
} break;
./

UiRootMember: UiObjectDefinition ;
/.
case $rule_number: {
    sym(1).Node = makeAstNode<AST::UiObjectMemberList> (driver->nodePool(), sym(1).UiObjectMember);
} break;
./

UiObjectMemberList: UiObjectMember ;
/.
case $rule_number: {
    sym(1).Node = makeAstNode<AST::UiObjectMemberList> (driver->nodePool(), sym(1).UiObjectMember);
} break;
./

UiObjectMemberList: UiObjectMemberList UiObjectMember ;
/.
case $rule_number: {
    AST::UiObjectMemberList *node = makeAstNode<AST:: UiObjectMemberList> (driver->nodePool(),
        sym(1).UiObjectMemberList, sym(2).UiObjectMember);
    sym(1).Node = node;
} break;
./

UiArrayMemberList: UiObjectDefinition ;
/.
case $rule_number: {
    sym(1).Node = makeAstNode<AST::UiArrayMemberList> (driver->nodePool(), sym(1).UiObjectMember);
} break;
./

UiArrayMemberList: UiArrayMemberList T_COMMA UiObjectDefinition ;
/.
case $rule_number: {
    AST::UiArrayMemberList *node = makeAstNode<AST::UiArrayMemberList> (driver->nodePool(),
        sym(1).UiArrayMemberList, sym(3).UiObjectMember);
    node->commaToken = loc(2);
    sym(1).Node = node;
} break;
./

UiObjectInitializer: T_LBRACE T_RBRACE ;
/.
case $rule_number: {
    AST::UiObjectInitializer *node = makeAstNode<AST::UiObjectInitializer> (driver->nodePool(), (AST::UiObjectMemberList*)0);
    node->lbraceToken = loc(1);
    node->rbraceToken = loc(2);
    sym(1).Node = node;
}   break;
./

UiObjectInitializer: T_LBRACE UiObjectMemberList T_RBRACE ;
/.
case $rule_number: {
    AST::UiObjectInitializer *node = makeAstNode<AST::UiObjectInitializer> (driver->nodePool(), sym(2).UiObjectMemberList->finish());
    node->lbraceToken = loc(1);
    node->rbraceToken = loc(3);
    sym(1).Node = node;
}   break;
./

UiObjectDefinition: UiQualifiedId UiObjectInitializer ;
/.
case $rule_number: {
    AST::UiObjectDefinition *node = makeAstNode<AST::UiObjectDefinition> (driver->nodePool(), sym(1).UiQualifiedId,
        sym(2).UiObjectInitializer);
    sym(1).Node = node;
}   break;
./

UiObjectMember: UiObjectDefinition ;

UiObjectMember: UiQualifiedId T_COLON T_LBRACKET UiArrayMemberList T_RBRACKET ;
/.
case $rule_number: {
    AST::UiArrayBinding *node = makeAstNode<AST::UiArrayBinding> (driver->nodePool(),
        sym(1).UiQualifiedId, sym(4).UiArrayMemberList->finish());
    node->colonToken = loc(2);
    node->lbracketToken = loc(3);
    node->rbracketToken = loc(5);
    sym(1).Node = node;
}   break;
./

UiObjectMember: UiQualifiedId             T_COLON UiQualifiedId  UiObjectInitializer ;
/.
case $rule_number: {
    AST::UiObjectBinding *node = makeAstNode<AST::UiObjectBinding> (driver->nodePool(),
      sym(1).UiQualifiedId, sym(3).UiQualifiedId, sym(4).UiObjectInitializer);
    node->colonToken = loc(2);
    sym(1).Node = node;
} break;
./

UiObjectMember: UiQualifiedId             T_ON UiQualifiedId  UiObjectInitializer ;
/.
case $rule_number: {
    AST::UiObjectBinding *node = makeAstNode<AST::UiObjectBinding> (driver->nodePool(),
      sym(3).UiQualifiedId, sym(1).UiQualifiedId, sym(4).UiObjectInitializer);
    node->colonToken = loc(2);
    node->hasOnToken = true;
    sym(1).Node = node;
} break;
./

UiObjectMember: UiQualifiedId T_COLON Block ;
/.case $rule_number:./

UiObjectMember: UiQualifiedId T_COLON EmptyStatement ;
/.case $rule_number:./

UiObjectMember: UiQualifiedId T_COLON ExpressionStatement ;
/.case $rule_number:./

UiObjectMember: UiQualifiedId T_COLON IfStatement ; --- ### do we really want if statement in a binding?
/.case $rule_number:./

/.
{
    AST::UiScriptBinding *node = makeAstNode<AST::UiScriptBinding> (driver->nodePool(),
        sym(1).UiQualifiedId, sym(3).Statement);
    node->colonToken = loc(2);
    sym(1).Node = node;
}   break;
./

UiPropertyType: T_VAR ;
/.
case $rule_number:
./
UiPropertyType: T_RESERVED_WORD ;
/.
case $rule_number: {
    sym(1).sval = driver->intern(lexer->characterBuffer(), lexer->characterCount());
    break;
}
./

UiPropertyType: T_IDENTIFIER ;

UiParameterListOpt: ;
/.
case $rule_number: {
  sym(1).Node = 0;
} break;
./

UiParameterListOpt: UiParameterList ;
/.
case $rule_number: {
  sym(1).Node = sym(1).UiParameterList->finish ();
} break;
./

UiParameterList: UiPropertyType JsIdentifier ;
/.
case $rule_number: {
  AST::UiParameterList *node = makeAstNode<AST::UiParameterList> (driver->nodePool(), sym(1).sval, sym(2).sval);
  node->identifierToken = loc(2);
  sym(1).Node = node;
} break;
./

UiParameterList: UiParameterList T_COMMA UiPropertyType JsIdentifier ;
/.
case $rule_number: {
  AST::UiParameterList *node = makeAstNode<AST::UiParameterList> (driver->nodePool(), sym(1).UiParameterList, sym(3).sval, sym(4).sval);
  node->commaToken = loc(2);
  node->identifierToken = loc(4);
  sym(1).Node = node;
} break;
./

UiObjectMember: T_SIGNAL T_IDENTIFIER T_LPAREN UiParameterListOpt T_RPAREN T_AUTOMATIC_SEMICOLON ;
UiObjectMember: T_SIGNAL T_IDENTIFIER T_LPAREN UiParameterListOpt T_RPAREN T_SEMICOLON ;
/.
case $rule_number: {
    AST::UiPublicMember *node = makeAstNode<AST::UiPublicMember> (driver->nodePool(), (NameId *)0, sym(2).sval);
    node->type = AST::UiPublicMember::Signal;
    node->propertyToken = loc(1);
    node->typeToken = loc(2);
    node->identifierToken = loc(2);
    node->parameters = sym(4).UiParameterList;
    node->semicolonToken = loc(6);
    sym(1).Node = node;
}   break;
./

UiObjectMember: T_SIGNAL T_IDENTIFIER T_AUTOMATIC_SEMICOLON ;
UiObjectMember: T_SIGNAL T_IDENTIFIER T_SEMICOLON ;
/.
case $rule_number: {
    AST::UiPublicMember *node = makeAstNode<AST::UiPublicMember> (driver->nodePool(), (NameId *)0, sym(2).sval);
    node->type = AST::UiPublicMember::Signal;
    node->propertyToken = loc(1);
    node->typeToken = loc(2);
    node->identifierToken = loc(2);
    node->semicolonToken = loc(3);
    sym(1).Node = node;
}   break;
./

UiObjectMember: T_PROPERTY T_IDENTIFIER T_LT UiPropertyType T_GT JsIdentifier T_AUTOMATIC_SEMICOLON ;
UiObjectMember: T_PROPERTY T_IDENTIFIER T_LT UiPropertyType T_GT JsIdentifier T_SEMICOLON ;
/.
case $rule_number: {
    AST::UiPublicMember *node = makeAstNode<AST::UiPublicMember> (driver->nodePool(), sym(4).sval, sym(6).sval);
    node->typeModifier = sym(2).sval;
    node->propertyToken = loc(1);
    node->typeModifierToken = loc(2);
    node->typeToken = loc(4);
    node->identifierToken = loc(6);
    node->semicolonToken = loc(7);
    sym(1).Node = node;
}   break;
./

UiObjectMember: T_PROPERTY UiPropertyType JsIdentifier T_AUTOMATIC_SEMICOLON ;
UiObjectMember: T_PROPERTY UiPropertyType JsIdentifier T_SEMICOLON ;
/.
case $rule_number: {
    AST::UiPublicMember *node = makeAstNode<AST::UiPublicMember> (driver->nodePool(), sym(2).sval, sym(3).sval);
    node->propertyToken = loc(1);
    node->typeToken = loc(2);
    node->identifierToken = loc(3);
    node->semicolonToken = loc(4);
    sym(1).Node = node;
}   break;
./

UiObjectMember: T_DEFAULT T_PROPERTY UiPropertyType JsIdentifier T_AUTOMATIC_SEMICOLON ;
UiObjectMember: T_DEFAULT T_PROPERTY UiPropertyType JsIdentifier T_SEMICOLON ;
/.
case $rule_number: {
    AST::UiPublicMember *node = makeAstNode<AST::UiPublicMember> (driver->nodePool(), sym(3).sval, sym(4).sval);
    node->isDefaultMember = true;
    node->defaultToken = loc(1);
    node->propertyToken = loc(2);
    node->typeToken = loc(3);
    node->identifierToken = loc(4);
    node->semicolonToken = loc(5);
    sym(1).Node = node;
}   break;
./

UiObjectMember: T_PROPERTY UiPropertyType JsIdentifier T_COLON Expression T_AUTOMATIC_SEMICOLON ;
UiObjectMember: T_PROPERTY UiPropertyType JsIdentifier T_COLON Expression T_SEMICOLON ;
/.
case $rule_number: {
    AST::UiPublicMember *node = makeAstNode<AST::UiPublicMember> (driver->nodePool(), sym(2).sval, sym(3).sval,
        sym(5).Expression);
    node->propertyToken = loc(1);
    node->typeToken = loc(2);
    node->identifierToken = loc(3);
    node->colonToken = loc(4);
    node->semicolonToken = loc(6);
    sym(1).Node = node;
}   break;
./

UiObjectMember: T_READONLY T_PROPERTY UiPropertyType JsIdentifier T_COLON Expression T_AUTOMATIC_SEMICOLON ;
UiObjectMember: T_READONLY T_PROPERTY UiPropertyType JsIdentifier T_COLON Expression T_SEMICOLON ;
/.
case $rule_number: {
    AST::UiPublicMember *node = makeAstNode<AST::UiPublicMember> (driver->nodePool(), sym(3).sval, sym(4).sval,
        sym(6).Expression);
    node->isReadonlyMember = true;
    node->readonlyToken = loc(1);
    node->propertyToken = loc(2);
    node->typeToken = loc(3);
    node->identifierToken = loc(4);
    node->colonToken = loc(5);
    node->semicolonToken = loc(7);
    sym(1).Node = node;
}   break;
./

UiObjectMember: T_DEFAULT T_PROPERTY UiPropertyType JsIdentifier T_COLON Expression T_AUTOMATIC_SEMICOLON ;
UiObjectMember: T_DEFAULT T_PROPERTY UiPropertyType JsIdentifier T_COLON Expression T_SEMICOLON ;
/.
case $rule_number: {
    AST::UiPublicMember *node = makeAstNode<AST::UiPublicMember> (driver->nodePool(), sym(3).sval, sym(4).sval,
        sym(6).Expression);
    node->isDefaultMember = true;
    node->defaultToken = loc(1);
    node->propertyToken = loc(2);
    node->typeToken = loc(3);
    node->identifierToken = loc(4);
    node->colonToken = loc(5);
    node->semicolonToken = loc(7);
    sym(1).Node = node;
}   break;
./

UiObjectMember: T_PROPERTY T_IDENTIFIER T_LT UiPropertyType T_GT JsIdentifier T_COLON T_LBRACKET UiArrayMemberList T_RBRACKET ;
/.
case $rule_number: {
    AST::UiPublicMember *node = makeAstNode<AST::UiPublicMember> (driver->nodePool(), sym(4).sval, sym(6).sval);
    node->typeModifier = sym(2).sval;
    node->propertyToken = loc(1);
    node->typeModifierToken = loc(2);
    node->typeToken = loc(4);
    node->identifierToken = loc(6);
    node->semicolonToken = loc(7); // insert a fake ';' before ':'

    AST::UiQualifiedId *propertyName = makeAstNode<AST::UiQualifiedId>(driver->nodePool(), sym(6).sval);
    propertyName->identifierToken = loc(6);
    propertyName->next = 0;

    AST::UiArrayBinding *binding = makeAstNode<AST::UiArrayBinding> (driver->nodePool(),
        propertyName, sym(9).UiArrayMemberList->finish());
    binding->colonToken = loc(7);
    binding->lbracketToken = loc(8);
    binding->rbracketToken = loc(10);

    node->binding = binding;

    sym(1).Node = node;
}   break;
./

UiObjectMember: T_PROPERTY UiPropertyType JsIdentifier T_COLON UiQualifiedId UiObjectInitializer ;
/.
case $rule_number: {
    AST::UiPublicMember *node = makeAstNode<AST::UiPublicMember> (driver->nodePool(), sym(2).sval, sym(3).sval);
    node->propertyToken = loc(1);
    node->typeToken = loc(2);
    node->identifierToken = loc(3);
    node->semicolonToken = loc(4); // insert a fake ';' before ':'

    AST::UiQualifiedId *propertyName = makeAstNode<AST::UiQualifiedId>(driver->nodePool(), sym(3).sval);
    propertyName->identifierToken = loc(3);
    propertyName->next = 0;

    AST::UiObjectBinding *binding = makeAstNode<AST::UiObjectBinding> (driver->nodePool(),
      propertyName, sym(5).UiQualifiedId, sym(6).UiObjectInitializer);
    binding->colonToken = loc(4);

    node->binding = binding;

    sym(1).Node = node;
}   break;
./

UiObjectMember: FunctionDeclaration ;
/.
case $rule_number: {
    sym(1).Node = makeAstNode<AST::UiSourceElement>(driver->nodePool(), sym(1).Node);
}   break;
./

UiObjectMember: VariableStatement ;
/.
case $rule_number: {
    sym(1).Node = makeAstNode<AST::UiSourceElement>(driver->nodePool(), sym(1).Node);
}   break;
./

JsIdentifier: T_IDENTIFIER;

JsIdentifier: T_PROPERTY ;
/.
case $rule_number: {
    QString s = QLatin1String(QDeclarativeJSGrammar::spell[T_PROPERTY]);
    sym(1).sval = driver->intern(s.constData(), s.length());
    break;
}
./

JsIdentifier: T_SIGNAL ;
/.
case $rule_number: {
    QString s = QLatin1String(QDeclarativeJSGrammar::spell[T_SIGNAL]);
    sym(1).sval = driver->intern(s.constData(), s.length());
    break;
}
./

JsIdentifier: T_READONLY ;
/.
case $rule_number: {
    QString s = QLatin1String(QDeclarativeJSGrammar::spell[T_READONLY]);
    sym(1).sval = driver->intern(s.constData(), s.length());
    break;
}
./

JsIdentifier: T_ON ;
/.
case $rule_number: {
    QString s = QLatin1String(QDeclarativeJSGrammar::spell[T_ON]);
    sym(1).sval = driver->intern(s.constData(), s.length());
    break;
}
./

--------------------------------------------------------------------------------------------------------
-- Expressions
--------------------------------------------------------------------------------------------------------

PrimaryExpression: T_THIS ;
/.
case $rule_number: {
  AST::ThisExpression *node = makeAstNode<AST::ThisExpression> (driver->nodePool());
  node->thisToken = loc(1);
  sym(1).Node = node;
} break;
./

PrimaryExpression: JsIdentifier ;
/.
case $rule_number: {
  AST::IdentifierExpression *node = makeAstNode<AST::IdentifierExpression> (driver->nodePool(), sym(1).sval);
  node->identifierToken = loc(1);
  sym(1).Node = node;
} break;
./

PrimaryExpression: T_NULL ;
/.
case $rule_number: {
  AST::NullExpression *node = makeAstNode<AST::NullExpression> (driver->nodePool());
  node->nullToken = loc(1);
  sym(1).Node = node;
} break;
./

PrimaryExpression: T_TRUE ;
/.
case $rule_number: {
  AST::TrueLiteral *node = makeAstNode<AST::TrueLiteral> (driver->nodePool());
  node->trueToken = loc(1);
  sym(1).Node = node;
} break;
./

PrimaryExpression: T_FALSE ;
/.
case $rule_number: {
  AST::FalseLiteral *node = makeAstNode<AST::FalseLiteral> (driver->nodePool());
  node->falseToken = loc(1);
  sym(1).Node = node;
} break;
./

PrimaryExpression: T_NUMERIC_LITERAL ;
/.
case $rule_number: {
  AST::NumericLiteral *node = makeAstNode<AST::NumericLiteral> (driver->nodePool(), sym(1).dval);
  node->literalToken = loc(1);
  sym(1).Node = node;
} break;
./

PrimaryExpression: T_MULTILINE_STRING_LITERAL ;
/.case $rule_number:./

PrimaryExpression: T_STRING_LITERAL ;
/.
case $rule_number: {
  AST::StringLiteral *node = makeAstNode<AST::StringLiteral> (driver->nodePool(), sym(1).sval);
  node->literalToken = loc(1);
  sym(1).Node = node;
} break;
./

PrimaryExpression: T_DIVIDE_ ;
/:
#define J_SCRIPT_REGEXPLITERAL_RULE1 $rule_number
:/
/.
case $rule_number: {
  bool rx = lexer->scanRegExp(Lexer::NoPrefix);
  if (!rx) {
    diagnostic_messages.append(DiagnosticMessage(DiagnosticMessage::Error, location(lexer), lexer->errorMessage()));
    return false; // ### remove me
  }

  loc(1).length = lexer->tokenLength();

  AST::RegExpLiteral *node = makeAstNode<AST::RegExpLiteral> (driver->nodePool(), lexer->pattern, lexer->flags);
  node->literalToken = loc(1);
  sym(1).Node = node;
} break;
./

PrimaryExpression: T_DIVIDE_EQ ;
/:
#define J_SCRIPT_REGEXPLITERAL_RULE2 $rule_number
:/
/.
case $rule_number: {
  bool rx = lexer->scanRegExp(Lexer::EqualPrefix);
  if (!rx) {
    diagnostic_messages.append(DiagnosticMessage(DiagnosticMessage::Error, location(lexer), lexer->errorMessage()));
    return false;
  }

  loc(1).length = lexer->tokenLength();

  AST::RegExpLiteral *node = makeAstNode<AST::RegExpLiteral> (driver->nodePool(), lexer->pattern, lexer->flags);
  node->literalToken = loc(1);
  sym(1).Node = node;
} break;
./

PrimaryExpression: T_LBRACKET T_RBRACKET ;
/.
case $rule_number: {
  AST::ArrayLiteral *node = makeAstNode<AST::ArrayLiteral> (driver->nodePool(), (AST::Elision *) 0);
  node->lbracketToken = loc(1);
  node->rbracketToken = loc(2);
  sym(1).Node = node;
} break;
./

PrimaryExpression: T_LBRACKET Elision T_RBRACKET ;
/.
case $rule_number: {
  AST::ArrayLiteral *node = makeAstNode<AST::ArrayLiteral> (driver->nodePool(), sym(2).Elision->finish());
  node->lbracketToken = loc(1);
  node->rbracketToken = loc(3);
  sym(1).Node = node;
} break;
./

PrimaryExpression: T_LBRACKET ElementList T_RBRACKET ;
/.
case $rule_number: {
  AST::ArrayLiteral *node = makeAstNode<AST::ArrayLiteral> (driver->nodePool(), sym(2).ElementList->finish ());
  node->lbracketToken = loc(1);
  node->rbracketToken = loc(3);
  sym(1).Node = node;
} break;
./

PrimaryExpression: T_LBRACKET ElementList T_COMMA T_RBRACKET ;
/.
case $rule_number: {
  AST::ArrayLiteral *node = makeAstNode<AST::ArrayLiteral> (driver->nodePool(), sym(2).ElementList->finish (),
    (AST::Elision *) 0);
  node->lbracketToken = loc(1);
  node->commaToken = loc(3);
  node->rbracketToken = loc(4);
  sym(1).Node = node;
} break;
./

PrimaryExpression: T_LBRACKET ElementList T_COMMA Elision T_RBRACKET ;
/.
case $rule_number: {
  AST::ArrayLiteral *node = makeAstNode<AST::ArrayLiteral> (driver->nodePool(), sym(2).ElementList->finish (),
    sym(4).Elision->finish());
  node->lbracketToken = loc(1);
  node->commaToken = loc(3);
  node->rbracketToken = loc(5);
  sym(1).Node = node;
} break;
./

-- PrimaryExpression: T_LBRACE T_RBRACE ;
-- /.
-- case $rule_number: {
--   sym(1).Node = makeAstNode<AST::ObjectLiteral> (driver->nodePool());
-- } break;
-- ./

PrimaryExpression: T_LBRACE PropertyNameAndValueListOpt T_RBRACE ;
/.
case $rule_number: {
  AST::ObjectLiteral *node = 0;
  if (sym(2).Node)
    node = makeAstNode<AST::ObjectLiteral> (driver->nodePool(),
        sym(2).PropertyNameAndValueList->finish ());
  else
    node = makeAstNode<AST::ObjectLiteral> (driver->nodePool());
  node->lbraceToken = loc(1);
  node->rbraceToken = loc(3);
  sym(1).Node = node;
} break;
./

PrimaryExpression: T_LBRACE PropertyNameAndValueList T_COMMA T_RBRACE ;
/.
case $rule_number: {
  AST::ObjectLiteral *node = makeAstNode<AST::ObjectLiteral> (driver->nodePool(),
    sym(2).PropertyNameAndValueList->finish ());
  node->lbraceToken = loc(1);
  node->rbraceToken = loc(4);
  sym(1).Node = node;
} break;
./

PrimaryExpression: T_LPAREN Expression T_RPAREN ;
/.
case $rule_number: {
  AST::NestedExpression *node = makeAstNode<AST::NestedExpression>(driver->nodePool(), sym(2).Expression);
  node->lparenToken = loc(1);
  node->rparenToken = loc(3);
  sym(1).Node = node;
} break;
./

UiQualifiedId: MemberExpression ;
/.
case $rule_number: {
  if (AST::ArrayMemberExpression *mem = AST::cast<AST::ArrayMemberExpression *>(sym(1).Expression)) {
    diagnostic_messages.append(DiagnosticMessage(DiagnosticMessage::Warning, mem->lbracketToken,
      QLatin1String("Ignored annotation")));

    sym(1).Expression = mem->base;
  }

  if (AST::UiQualifiedId *qualifiedId = reparseAsQualifiedId(sym(1).Expression)) {
    sym(1).UiQualifiedId = qualifiedId;
  } else {
    sym(1).UiQualifiedId = 0;

    diagnostic_messages.append(DiagnosticMessage(DiagnosticMessage::Error, loc(1),
      QLatin1String("Expected a qualified name id")));

    return false; // ### recover
  }
} break;
./

ElementList: AssignmentExpression ;
/.
case $rule_number: {
  sym(1).Node = makeAstNode<AST::ElementList> (driver->nodePool(), (AST::Elision *) 0, sym(1).Expression);
} break;
./

ElementList: Elision AssignmentExpression ;
/.
case $rule_number: {
  sym(1).Node = makeAstNode<AST::ElementList> (driver->nodePool(), sym(1).Elision->finish(), sym(2).Expression);
} break;
./

ElementList: ElementList T_COMMA AssignmentExpression ;
/.
case $rule_number: {
  AST::ElementList *node = makeAstNode<AST::ElementList> (driver->nodePool(), sym(1).ElementList,
    (AST::Elision *) 0, sym(3).Expression);
  node->commaToken = loc(2);
  sym(1).Node = node;
} break;
./

ElementList: ElementList T_COMMA Elision AssignmentExpression ;
/.
case $rule_number: {
  AST::ElementList *node = makeAstNode<AST::ElementList> (driver->nodePool(), sym(1).ElementList, sym(3).Elision->finish(),
    sym(4).Expression);
  node->commaToken = loc(2);
  sym(1).Node = node;
} break;
./

Elision: T_COMMA ;
/.
case $rule_number: {
  AST::Elision *node = makeAstNode<AST::Elision> (driver->nodePool());
  node->commaToken = loc(1);
  sym(1).Node = node;
} break;
./

Elision: Elision T_COMMA ;
/.
case $rule_number: {
  AST::Elision *node = makeAstNode<AST::Elision> (driver->nodePool(), sym(1).Elision);
  node->commaToken = loc(2);
  sym(1).Node = node;
} break;
./

PropertyNameAndValueList: PropertyName T_COLON AssignmentExpression ;
/.
case $rule_number: {
  AST::PropertyNameAndValueList *node = makeAstNode<AST::PropertyNameAndValueList> (driver->nodePool(),
      sym(1).PropertyName, sym(3).Expression);
  node->colonToken = loc(2);
  sym(1).Node = node;
} break;
./

PropertyNameAndValueList: PropertyNameAndValueList T_COMMA PropertyName T_COLON AssignmentExpression ;
/.
case $rule_number: {
  AST::PropertyNameAndValueList *node = makeAstNode<AST::PropertyNameAndValueList> (driver->nodePool(),
      sym(1).PropertyNameAndValueList, sym(3).PropertyName, sym(5).Expression);
  node->commaToken = loc(2);
  node->colonToken = loc(4);
  sym(1).Node = node;
} break;
./

PropertyName: T_IDENTIFIER %prec SHIFT_THERE ;
/.
case $rule_number: {
  AST::IdentifierPropertyName *node = makeAstNode<AST::IdentifierPropertyName> (driver->nodePool(), sym(1).sval);
  node->propertyNameToken = loc(1);
  sym(1).Node = node;
} break;
./

PropertyName: T_SIGNAL ;
/.case $rule_number:./

PropertyName: T_PROPERTY ;
/.
case $rule_number: {
  AST::IdentifierPropertyName *node = makeAstNode<AST::IdentifierPropertyName> (driver->nodePool(), driver->intern(lexer->characterBuffer(), lexer->characterCount()));
  node->propertyNameToken = loc(1);
  sym(1).Node = node;
} break;
./

PropertyName: T_STRING_LITERAL ;
/.
case $rule_number: {
  AST::StringLiteralPropertyName *node = makeAstNode<AST::StringLiteralPropertyName> (driver->nodePool(), sym(1).sval);
  node->propertyNameToken = loc(1);
  sym(1).Node = node;
} break;
./

PropertyName: T_NUMERIC_LITERAL ;
/.
case $rule_number: {
  AST::NumericLiteralPropertyName *node = makeAstNode<AST::NumericLiteralPropertyName> (driver->nodePool(), sym(1).dval);
  node->propertyNameToken = loc(1);
  sym(1).Node = node;
} break;
./

PropertyName: ReservedIdentifier ;
/.
case $rule_number: {
  AST::IdentifierPropertyName *node = makeAstNode<AST::IdentifierPropertyName> (driver->nodePool(), sym(1).sval);
  node->propertyNameToken = loc(1);
  sym(1).Node = node;
} break;
./

ReservedIdentifier: T_BREAK ;
/.
case $rule_number:
./
ReservedIdentifier: T_CASE ;
/.
case $rule_number:
./
ReservedIdentifier: T_CATCH ;
/.
case $rule_number:
./
ReservedIdentifier: T_CONTINUE ;
/.
case $rule_number:
./
ReservedIdentifier: T_DEFAULT ;
/.
case $rule_number:
./
ReservedIdentifier: T_DELETE ;
/.
case $rule_number:
./
ReservedIdentifier: T_DO ;
/.
case $rule_number:
./
ReservedIdentifier: T_ELSE ;
/.
case $rule_number:
./
ReservedIdentifier: T_FALSE ;
/.
case $rule_number:
./
ReservedIdentifier: T_FINALLY ;
/.
case $rule_number:
./
ReservedIdentifier: T_FOR ;
/.
case $rule_number:
./
ReservedIdentifier: T_FUNCTION ;
/.
case $rule_number:
./
ReservedIdentifier: T_IF ;
/.
case $rule_number:
./
ReservedIdentifier: T_IN ;
/.
case $rule_number:
./
ReservedIdentifier: T_INSTANCEOF ;
/.
case $rule_number:
./
ReservedIdentifier: T_NEW ;
/.
case $rule_number:
./
ReservedIdentifier: T_NULL ;
/.
case $rule_number:
./
ReservedIdentifier: T_RETURN ;
/.
case $rule_number:
./
ReservedIdentifier: T_SWITCH ;
/.
case $rule_number:
./
ReservedIdentifier: T_THIS ;
/.
case $rule_number:
./
ReservedIdentifier: T_THROW ;
/.
case $rule_number:
./
ReservedIdentifier: T_TRUE ;
/.
case $rule_number:
./
ReservedIdentifier: T_TRY ;
/.
case $rule_number:
./
ReservedIdentifier: T_TYPEOF ;
/.
case $rule_number:
./
ReservedIdentifier: T_VAR ;
/.
case $rule_number:
./
ReservedIdentifier: T_VOID ;
/.
case $rule_number:
./
ReservedIdentifier: T_WHILE ;
/.
case $rule_number:
./
ReservedIdentifier: T_CONST ;
/.
case $rule_number:
./
ReservedIdentifier: T_DEBUGGER ;
/.
case $rule_number:
./
ReservedIdentifier: T_RESERVED_WORD ;
/.
case $rule_number:
./
ReservedIdentifier: T_WITH ;
/.
case $rule_number:
{
  sym(1).sval = driver->intern(lexer->characterBuffer(), lexer->characterCount());
} break;
./

PropertyIdentifier: JsIdentifier ;
PropertyIdentifier: ReservedIdentifier ;

MemberExpression: PrimaryExpression ;
MemberExpression: FunctionExpression ;

MemberExpression: MemberExpression T_LBRACKET Expression T_RBRACKET ;
/.
case $rule_number: {
  AST::ArrayMemberExpression *node = makeAstNode<AST::ArrayMemberExpression> (driver->nodePool(), sym(1).Expression, sym(3).Expression);
  node->lbracketToken = loc(2);
  node->rbracketToken = loc(4);
  sym(1).Node = node;
} break;
./

MemberExpression: MemberExpression T_DOT PropertyIdentifier ;
/.
case $rule_number: {
  AST::FieldMemberExpression *node = makeAstNode<AST::FieldMemberExpression> (driver->nodePool(), sym(1).Expression, sym(3).sval);
  node->dotToken = loc(2);
  node->identifierToken = loc(3);
  sym(1).Node = node;
} break;
./

MemberExpression: T_NEW MemberExpression T_LPAREN ArgumentListOpt T_RPAREN ;
/.
case $rule_number: {
  AST::NewMemberExpression *node = makeAstNode<AST::NewMemberExpression> (driver->nodePool(), sym(2).Expression, sym(4).ArgumentList);
  node->newToken = loc(1);
  node->lparenToken = loc(3);
  node->rparenToken = loc(5);
  sym(1).Node = node;
} break;
./

NewExpression: MemberExpression ;

NewExpression: T_NEW NewExpression ;
/.
case $rule_number: {
  AST::NewExpression *node = makeAstNode<AST::NewExpression> (driver->nodePool(), sym(2).Expression);
  node->newToken = loc(1);
  sym(1).Node = node;
} break;
./

CallExpression: MemberExpression T_LPAREN ArgumentListOpt T_RPAREN ;
/.
case $rule_number: {
  AST::CallExpression *node = makeAstNode<AST::CallExpression> (driver->nodePool(), sym(1).Expression, sym(3).ArgumentList);
  node->lparenToken = loc(2);
  node->rparenToken = loc(4);
  sym(1).Node = node;
} break;
./

CallExpression: CallExpression T_LPAREN ArgumentListOpt T_RPAREN ;
/.
case $rule_number: {
  AST::CallExpression *node = makeAstNode<AST::CallExpression> (driver->nodePool(), sym(1).Expression, sym(3).ArgumentList);
  node->lparenToken = loc(2);
  node->rparenToken = loc(4);
  sym(1).Node = node;
} break;
./

CallExpression: CallExpression T_LBRACKET Expression T_RBRACKET ;
/.
case $rule_number: {
  AST::ArrayMemberExpression *node = makeAstNode<AST::ArrayMemberExpression> (driver->nodePool(), sym(1).Expression, sym(3).Expression);
  node->lbracketToken = loc(2);
  node->rbracketToken = loc(4);
  sym(1).Node = node;
} break;
./

CallExpression: CallExpression T_DOT PropertyIdentifier ;
/.
case $rule_number: {
  AST::FieldMemberExpression *node = makeAstNode<AST::FieldMemberExpression> (driver->nodePool(), sym(1).Expression, sym(3).sval);
  node->dotToken = loc(2);
  node->identifierToken = loc(3);
  sym(1).Node = node;
} break;
./

ArgumentListOpt: ;
/.
case $rule_number: {
  sym(1).Node = 0;
} break;
./

ArgumentListOpt: ArgumentList ;
/.
case $rule_number: {
  sym(1).Node = sym(1).ArgumentList->finish();
} break;
./

ArgumentList: AssignmentExpression ;
/.
case $rule_number: {
  sym(1).Node = makeAstNode<AST::ArgumentList> (driver->nodePool(), sym(1).Expression);
} break;
./

ArgumentList: ArgumentList T_COMMA AssignmentExpression ;
/.
case $rule_number: {
  AST::ArgumentList *node = makeAstNode<AST::ArgumentList> (driver->nodePool(), sym(1).ArgumentList, sym(3).Expression);
  node->commaToken = loc(2);
  sym(1).Node = node;
} break;
./

LeftHandSideExpression: NewExpression ;
LeftHandSideExpression: CallExpression ;
PostfixExpression: LeftHandSideExpression ;

PostfixExpression: LeftHandSideExpression T_PLUS_PLUS ;
/.
case $rule_number: {
  AST::PostIncrementExpression *node = makeAstNode<AST::PostIncrementExpression> (driver->nodePool(), sym(1).Expression);
  node->incrementToken = loc(2);
  sym(1).Node = node;
} break;
./

PostfixExpression: LeftHandSideExpression T_MINUS_MINUS ;
/.
case $rule_number: {
  AST::PostDecrementExpression *node = makeAstNode<AST::PostDecrementExpression> (driver->nodePool(), sym(1).Expression);
  node->decrementToken = loc(2);
  sym(1).Node = node;
} break;
./

UnaryExpression: PostfixExpression ;

UnaryExpression: T_DELETE UnaryExpression ;
/.
case $rule_number: {
  AST::DeleteExpression *node = makeAstNode<AST::DeleteExpression> (driver->nodePool(), sym(2).Expression);
  node->deleteToken = loc(1);
  sym(1).Node = node;
} break;
./

UnaryExpression: T_VOID UnaryExpression ;
/.
case $rule_number: {
  AST::VoidExpression *node = makeAstNode<AST::VoidExpression> (driver->nodePool(), sym(2).Expression);
  node->voidToken = loc(1);
  sym(1).Node = node;
} break;
./

UnaryExpression: T_TYPEOF UnaryExpression ;
/.
case $rule_number: {
  AST::TypeOfExpression *node = makeAstNode<AST::TypeOfExpression> (driver->nodePool(), sym(2).Expression);
  node->typeofToken = loc(1);
  sym(1).Node = node;
} break;
./

UnaryExpression: T_PLUS_PLUS UnaryExpression ;
/.
case $rule_number: {
  AST::PreIncrementExpression *node = makeAstNode<AST::PreIncrementExpression> (driver->nodePool(), sym(2).Expression);
  node->incrementToken = loc(1);
  sym(1).Node = node;
} break;
./

UnaryExpression: T_MINUS_MINUS UnaryExpression ;
/.
case $rule_number: {
  AST::PreDecrementExpression *node = makeAstNode<AST::PreDecrementExpression> (driver->nodePool(), sym(2).Expression);
  node->decrementToken = loc(1);
  sym(1).Node = node;
} break;
./

UnaryExpression: T_PLUS UnaryExpression ;
/.
case $rule_number: {
  AST::UnaryPlusExpression *node = makeAstNode<AST::UnaryPlusExpression> (driver->nodePool(), sym(2).Expression);
  node->plusToken = loc(1);
  sym(1).Node = node;
} break;
./

UnaryExpression: T_MINUS UnaryExpression ;
/.
case $rule_number: {
  AST::UnaryMinusExpression *node = makeAstNode<AST::UnaryMinusExpression> (driver->nodePool(), sym(2).Expression);
  node->minusToken = loc(1);
  sym(1).Node = node;
} break;
./

UnaryExpression: T_TILDE UnaryExpression ;
/.
case $rule_number: {
  AST::TildeExpression *node = makeAstNode<AST::TildeExpression> (driver->nodePool(), sym(2).Expression);
  node->tildeToken = loc(1);
  sym(1).Node = node;
} break;
./

UnaryExpression: T_NOT UnaryExpression ;
/.
case $rule_number: {
  AST::NotExpression *node = makeAstNode<AST::NotExpression> (driver->nodePool(), sym(2).Expression);
  node->notToken = loc(1);
  sym(1).Node = node;
} break;
./

MultiplicativeExpression: UnaryExpression ;

MultiplicativeExpression: MultiplicativeExpression T_STAR UnaryExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::Mul, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

MultiplicativeExpression: MultiplicativeExpression T_DIVIDE_ UnaryExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::Div, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

MultiplicativeExpression: MultiplicativeExpression T_REMAINDER UnaryExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::Mod, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

AdditiveExpression: MultiplicativeExpression ;

AdditiveExpression: AdditiveExpression T_PLUS MultiplicativeExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::Add, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

AdditiveExpression: AdditiveExpression T_MINUS MultiplicativeExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::Sub, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

ShiftExpression: AdditiveExpression ;

ShiftExpression: ShiftExpression T_LT_LT AdditiveExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::LShift, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

ShiftExpression: ShiftExpression T_GT_GT AdditiveExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::RShift, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

ShiftExpression: ShiftExpression T_GT_GT_GT AdditiveExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::URShift, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

RelationalExpression: ShiftExpression ;

RelationalExpression: RelationalExpression T_LT ShiftExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::Lt, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

RelationalExpression: RelationalExpression T_GT ShiftExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::Gt, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

RelationalExpression: RelationalExpression T_LE ShiftExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::Le, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

RelationalExpression: RelationalExpression T_GE ShiftExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::Ge, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

RelationalExpression: RelationalExpression T_INSTANCEOF ShiftExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::InstanceOf, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

RelationalExpression: RelationalExpression T_IN ShiftExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::In, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

RelationalExpressionNotIn: ShiftExpression ;

RelationalExpressionNotIn: RelationalExpressionNotIn T_LT ShiftExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::Lt, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

RelationalExpressionNotIn: RelationalExpressionNotIn T_GT ShiftExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::Gt, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

RelationalExpressionNotIn: RelationalExpressionNotIn T_LE ShiftExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::Le, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

RelationalExpressionNotIn: RelationalExpressionNotIn T_GE ShiftExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
   QSOperator::Ge, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

RelationalExpressionNotIn: RelationalExpressionNotIn T_INSTANCEOF ShiftExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::InstanceOf, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

EqualityExpression: RelationalExpression ;

EqualityExpression: EqualityExpression T_EQ_EQ RelationalExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::Equal, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

EqualityExpression: EqualityExpression T_NOT_EQ RelationalExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::NotEqual, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

EqualityExpression: EqualityExpression T_EQ_EQ_EQ RelationalExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::StrictEqual, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

EqualityExpression: EqualityExpression T_NOT_EQ_EQ RelationalExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::StrictNotEqual, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

EqualityExpressionNotIn: RelationalExpressionNotIn ;

EqualityExpressionNotIn: EqualityExpressionNotIn T_EQ_EQ RelationalExpressionNotIn ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::Equal, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

EqualityExpressionNotIn: EqualityExpressionNotIn T_NOT_EQ RelationalExpressionNotIn;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::NotEqual, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

EqualityExpressionNotIn: EqualityExpressionNotIn T_EQ_EQ_EQ RelationalExpressionNotIn ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::StrictEqual, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

EqualityExpressionNotIn: EqualityExpressionNotIn T_NOT_EQ_EQ RelationalExpressionNotIn ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::StrictNotEqual, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

BitwiseANDExpression: EqualityExpression ;

BitwiseANDExpression: BitwiseANDExpression T_AND EqualityExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::BitAnd, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

BitwiseANDExpressionNotIn: EqualityExpressionNotIn ;

BitwiseANDExpressionNotIn: BitwiseANDExpressionNotIn T_AND EqualityExpressionNotIn ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::BitAnd, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

BitwiseXORExpression: BitwiseANDExpression ;

BitwiseXORExpression: BitwiseXORExpression T_XOR BitwiseANDExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::BitXor, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

BitwiseXORExpressionNotIn: BitwiseANDExpressionNotIn ;

BitwiseXORExpressionNotIn: BitwiseXORExpressionNotIn T_XOR BitwiseANDExpressionNotIn ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::BitXor, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

BitwiseORExpression: BitwiseXORExpression ;

BitwiseORExpression: BitwiseORExpression T_OR BitwiseXORExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::BitOr, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

BitwiseORExpressionNotIn: BitwiseXORExpressionNotIn ;

BitwiseORExpressionNotIn: BitwiseORExpressionNotIn T_OR BitwiseXORExpressionNotIn ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::BitOr, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

LogicalANDExpression: BitwiseORExpression ;

LogicalANDExpression: LogicalANDExpression T_AND_AND BitwiseORExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::And, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

LogicalANDExpressionNotIn: BitwiseORExpressionNotIn ;

LogicalANDExpressionNotIn: LogicalANDExpressionNotIn T_AND_AND BitwiseORExpressionNotIn ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::And, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

LogicalORExpression: LogicalANDExpression ;

LogicalORExpression: LogicalORExpression T_OR_OR LogicalANDExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::Or, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

LogicalORExpressionNotIn: LogicalANDExpressionNotIn ;

LogicalORExpressionNotIn: LogicalORExpressionNotIn T_OR_OR LogicalANDExpressionNotIn ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    QSOperator::Or, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

ConditionalExpression: LogicalORExpression ;

ConditionalExpression: LogicalORExpression T_QUESTION AssignmentExpression T_COLON AssignmentExpression ;
/.
case $rule_number: {
  AST::ConditionalExpression *node = makeAstNode<AST::ConditionalExpression> (driver->nodePool(), sym(1).Expression,
    sym(3).Expression, sym(5).Expression);
  node->questionToken = loc(2);
  node->colonToken = loc(4);
  sym(1).Node = node;
} break;
./

ConditionalExpressionNotIn: LogicalORExpressionNotIn ;

ConditionalExpressionNotIn: LogicalORExpressionNotIn T_QUESTION AssignmentExpressionNotIn T_COLON AssignmentExpressionNotIn ;
/.
case $rule_number: {
  AST::ConditionalExpression *node = makeAstNode<AST::ConditionalExpression> (driver->nodePool(), sym(1).Expression,
    sym(3).Expression, sym(5).Expression);
  node->questionToken = loc(2);
  node->colonToken = loc(4);
  sym(1).Node = node;
} break;
./

AssignmentExpression: ConditionalExpression ;

AssignmentExpression: LeftHandSideExpression AssignmentOperator AssignmentExpression ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    sym(2).ival, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

AssignmentExpressionNotIn: ConditionalExpressionNotIn ;

AssignmentExpressionNotIn: LeftHandSideExpression AssignmentOperator AssignmentExpressionNotIn ;
/.
case $rule_number: {
  AST::BinaryExpression *node = makeAstNode<AST::BinaryExpression> (driver->nodePool(), sym(1).Expression,
    sym(2).ival, sym(3).Expression);
  node->operatorToken = loc(2);
  sym(1).Node = node;
} break;
./

AssignmentOperator: T_EQ ;
/.
case $rule_number: {
  sym(1).ival = QSOperator::Assign;
} break;
./

AssignmentOperator: T_STAR_EQ ;
/.
case $rule_number: {
  sym(1).ival = QSOperator::InplaceMul;
} break;
./

AssignmentOperator: T_DIVIDE_EQ ;
/.
case $rule_number: {
  sym(1).ival = QSOperator::InplaceDiv;
} break;
./

AssignmentOperator: T_REMAINDER_EQ ;
/.
case $rule_number: {
  sym(1).ival = QSOperator::InplaceMod;
} break;
./

AssignmentOperator: T_PLUS_EQ ;
/.
case $rule_number: {
  sym(1).ival = QSOperator::InplaceAdd;
} break;
./

AssignmentOperator: T_MINUS_EQ ;
/.
case $rule_number: {
  sym(1).ival = QSOperator::InplaceSub;
} break;
./

AssignmentOperator: T_LT_LT_EQ ;
/.
case $rule_number: {
  sym(1).ival = QSOperator::InplaceLeftShift;
} break;
./

AssignmentOperator: T_GT_GT_EQ ;
/.
case $rule_number: {
  sym(1).ival = QSOperator::InplaceRightShift;
} break;
./

AssignmentOperator: T_GT_GT_GT_EQ ;
/.
case $rule_number: {
  sym(1).ival = QSOperator::InplaceURightShift;
} break;
./

AssignmentOperator: T_AND_EQ ;
/.
case $rule_number: {
  sym(1).ival = QSOperator::InplaceAnd;
} break;
./

AssignmentOperator: T_XOR_EQ ;
/.
case $rule_number: {
  sym(1).ival = QSOperator::InplaceXor;
} break;
./

AssignmentOperator: T_OR_EQ ;
/.
case $rule_number: {
  sym(1).ival = QSOperator::InplaceOr;
} break;
./

Expression: AssignmentExpression ;

Expression: Expression T_COMMA AssignmentExpression ;
/.
case $rule_number: {
  AST::Expression *node = makeAstNode<AST::Expression> (driver->nodePool(), sym(1).Expression, sym(3).Expression);
  node->commaToken = loc(2);
  sym(1).Node = node;
} break;
./

ExpressionOpt: ;
/.
case $rule_number: {
  sym(1).Node = 0;
} break;
./

ExpressionOpt: Expression ;

ExpressionNotIn: AssignmentExpressionNotIn ;

ExpressionNotIn: ExpressionNotIn T_COMMA AssignmentExpressionNotIn ;
/.
case $rule_number: {
  AST::Expression *node = makeAstNode<AST::Expression> (driver->nodePool(), sym(1).Expression, sym(3).Expression);
  node->commaToken = loc(2);
  sym(1).Node = node;
} break;
./

ExpressionNotInOpt: ;
/.
case $rule_number: {
  sym(1).Node = 0;
} break;
./

ExpressionNotInOpt: ExpressionNotIn ;

Statement: Block ;
Statement: VariableStatement ;
Statement: EmptyStatement ;
Statement: ExpressionStatement ;
Statement: IfStatement ;
Statement: IterationStatement ;
Statement: ContinueStatement ;
Statement: BreakStatement ;
Statement: ReturnStatement ;
Statement: WithStatement ;
Statement: LabelledStatement ;
Statement: SwitchStatement ;
Statement: ThrowStatement ;
Statement: TryStatement ;
Statement: DebuggerStatement ;


Block: T_LBRACE StatementListOpt T_RBRACE ;
/.
case $rule_number: {
  AST::Block *node = makeAstNode<AST::Block> (driver->nodePool(), sym(2).StatementList);
  node->lbraceToken = loc(1);
  node->rbraceToken = loc(3);
  sym(1).Node = node;
} break;
./

StatementList: Statement ;
/.
case $rule_number: {
  sym(1).Node = makeAstNode<AST::StatementList> (driver->nodePool(), sym(1).Statement);
} break;
./

StatementList: StatementList Statement ;
/.
case $rule_number: {
  sym(1).Node = makeAstNode<AST::StatementList> (driver->nodePool(), sym(1).StatementList, sym(2).Statement);
} break;
./

StatementListOpt: ;
/.
case $rule_number: {
  sym(1).Node = 0;
} break;
./

StatementListOpt: StatementList ;
/.
case $rule_number: {
  sym(1).Node = sym(1).StatementList->finish ();
} break;
./

VariableStatement: VariableDeclarationKind VariableDeclarationList T_AUTOMATIC_SEMICOLON ;  -- automatic semicolon
VariableStatement: VariableDeclarationKind VariableDeclarationList T_SEMICOLON ;
/.
case $rule_number: {
  AST::VariableStatement *node = makeAstNode<AST::VariableStatement> (driver->nodePool(),
     sym(2).VariableDeclarationList->finish (/*readOnly=*/sym(1).ival == T_CONST));
  node->declarationKindToken = loc(1);
  node->semicolonToken = loc(3);
  sym(1).Node = node;
} break;
./

VariableDeclarationKind: T_CONST ;
/.
case $rule_number: {
  sym(1).ival = T_CONST;
} break;
./

VariableDeclarationKind: T_VAR ;
/.
case $rule_number: {
  sym(1).ival = T_VAR;
} break;
./

VariableDeclarationList: VariableDeclaration ;
/.
case $rule_number: {
  sym(1).Node = makeAstNode<AST::VariableDeclarationList> (driver->nodePool(), sym(1).VariableDeclaration);
} break;
./

VariableDeclarationList: VariableDeclarationList T_COMMA VariableDeclaration ;
/.
case $rule_number: {
  AST::VariableDeclarationList *node = makeAstNode<AST::VariableDeclarationList> (driver->nodePool(),
    sym(1).VariableDeclarationList, sym(3).VariableDeclaration);
  node->commaToken = loc(2);
  sym(1).Node = node;
} break;
./

VariableDeclarationListNotIn: VariableDeclarationNotIn ;
/.
case $rule_number: {
  sym(1).Node = makeAstNode<AST::VariableDeclarationList> (driver->nodePool(), sym(1).VariableDeclaration);
} break;
./

VariableDeclarationListNotIn: VariableDeclarationListNotIn T_COMMA VariableDeclarationNotIn ;
/.
case $rule_number: {
  sym(1).Node = makeAstNode<AST::VariableDeclarationList> (driver->nodePool(), sym(1).VariableDeclarationList, sym(3).VariableDeclaration);
} break;
./

VariableDeclaration: JsIdentifier InitialiserOpt ;
/.
case $rule_number: {
  AST::VariableDeclaration *node = makeAstNode<AST::VariableDeclaration> (driver->nodePool(), sym(1).sval, sym(2).Expression);
  node->identifierToken = loc(1);
  sym(1).Node = node;
} break;
./

VariableDeclarationNotIn: JsIdentifier InitialiserNotInOpt ;
/.
case $rule_number: {
  AST::VariableDeclaration *node = makeAstNode<AST::VariableDeclaration> (driver->nodePool(), sym(1).sval, sym(2).Expression);
  node->identifierToken = loc(1);
  sym(1).Node = node;
} break;
./

Initialiser: T_EQ AssignmentExpression ;
/.
case $rule_number: {
  // ### TODO: AST for initializer
  sym(1) = sym(2);
} break;
./

InitialiserOpt: ;
/.
case $rule_number: {
  sym(1).Node = 0;
} break;
./

InitialiserOpt: Initialiser ;

InitialiserNotIn: T_EQ AssignmentExpressionNotIn ;
/.
case $rule_number: {
  // ### TODO: AST for initializer
  sym(1) = sym(2);
} break;
./

InitialiserNotInOpt: ;
/.
case $rule_number: {
  sym(1).Node = 0;
} break;
./

InitialiserNotInOpt: InitialiserNotIn ;

EmptyStatement: T_SEMICOLON ;
/.
case $rule_number: {
  AST::EmptyStatement *node = makeAstNode<AST::EmptyStatement> (driver->nodePool());
  node->semicolonToken = loc(1);
  sym(1).Node = node;
} break;
./

ExpressionStatement: Expression T_AUTOMATIC_SEMICOLON ;  -- automatic semicolon
ExpressionStatement: Expression T_SEMICOLON ;
/.
case $rule_number: {
  AST::ExpressionStatement *node = makeAstNode<AST::ExpressionStatement> (driver->nodePool(), sym(1).Expression);
  node->semicolonToken = loc(2);
  sym(1).Node = node;
} break;
./

IfStatement: T_IF T_LPAREN Expression T_RPAREN Statement T_ELSE Statement ;
/.
case $rule_number: {
  AST::IfStatement *node = makeAstNode<AST::IfStatement> (driver->nodePool(), sym(3).Expression, sym(5).Statement, sym(7).Statement);
  node->ifToken = loc(1);
  node->lparenToken = loc(2);
  node->rparenToken = loc(4);
  node->elseToken = loc(5);
  sym(1).Node = node;
} break;
./

IfStatement: T_IF T_LPAREN Expression T_RPAREN Statement ;
/.
case $rule_number: {
  AST::IfStatement *node = makeAstNode<AST::IfStatement> (driver->nodePool(), sym(3).Expression, sym(5).Statement);
  node->ifToken = loc(1);
  node->lparenToken = loc(2);
  node->rparenToken = loc(4);
  sym(1).Node = node;
} break;
./


IterationStatement: T_DO Statement T_WHILE T_LPAREN Expression T_RPAREN T_AUTOMATIC_SEMICOLON ;  -- automatic semicolon
IterationStatement: T_DO Statement T_WHILE T_LPAREN Expression T_RPAREN T_COMPATIBILITY_SEMICOLON ;  -- for JSC/V8 compatibility
IterationStatement: T_DO Statement T_WHILE T_LPAREN Expression T_RPAREN T_SEMICOLON ;
/.
case $rule_number: {
  AST::DoWhileStatement *node = makeAstNode<AST::DoWhileStatement> (driver->nodePool(), sym(2).Statement, sym(5).Expression);
  node->doToken = loc(1);
  node->whileToken = loc(3);
  node->lparenToken = loc(4);
  node->rparenToken = loc(6);
  node->semicolonToken = loc(7);
  sym(1).Node = node;
} break;
./

IterationStatement: T_WHILE T_LPAREN Expression T_RPAREN Statement ;
/.
case $rule_number: {
  AST::WhileStatement *node = makeAstNode<AST::WhileStatement> (driver->nodePool(), sym(3).Expression, sym(5).Statement);
  node->whileToken = loc(1);
  node->lparenToken = loc(2);
  node->rparenToken = loc(4);
  sym(1).Node = node;
} break;
./

IterationStatement: T_FOR T_LPAREN ExpressionNotInOpt T_SEMICOLON ExpressionOpt T_SEMICOLON ExpressionOpt T_RPAREN Statement ;
/.
case $rule_number: {
  AST::ForStatement *node = makeAstNode<AST::ForStatement> (driver->nodePool(), sym(3).Expression,
    sym(5).Expression, sym(7).Expression, sym(9).Statement);
  node->forToken = loc(1);
  node->lparenToken = loc(2);
  node->firstSemicolonToken = loc(4);
  node->secondSemicolonToken = loc(6);
  node->rparenToken = loc(8);
  sym(1).Node = node;
} break;
./

IterationStatement: T_FOR T_LPAREN T_VAR VariableDeclarationListNotIn T_SEMICOLON ExpressionOpt T_SEMICOLON ExpressionOpt T_RPAREN Statement ;
/.
case $rule_number: {
  AST::LocalForStatement *node = makeAstNode<AST::LocalForStatement> (driver->nodePool(),
     sym(4).VariableDeclarationList->finish (/*readOnly=*/false), sym(6).Expression,
     sym(8).Expression, sym(10).Statement);
  node->forToken = loc(1);
  node->lparenToken = loc(2);
  node->varToken = loc(3);
  node->firstSemicolonToken = loc(5);
  node->secondSemicolonToken = loc(7);
  node->rparenToken = loc(9);
  sym(1).Node = node;
} break;
./

IterationStatement: T_FOR T_LPAREN LeftHandSideExpression T_IN Expression T_RPAREN Statement ;
/.
case $rule_number: {
  AST:: ForEachStatement *node = makeAstNode<AST::ForEachStatement> (driver->nodePool(), sym(3).Expression,
    sym(5).Expression, sym(7).Statement);
  node->forToken = loc(1);
  node->lparenToken = loc(2);
  node->inToken = loc(4);
  node->rparenToken = loc(6);
  sym(1).Node = node;
} break;
./

IterationStatement: T_FOR T_LPAREN T_VAR VariableDeclarationNotIn T_IN Expression T_RPAREN Statement ;
/.
case $rule_number: {
  AST::LocalForEachStatement *node = makeAstNode<AST::LocalForEachStatement> (driver->nodePool(),
    sym(4).VariableDeclaration, sym(6).Expression, sym(8).Statement);
  node->forToken = loc(1);
  node->lparenToken = loc(2);
  node->varToken = loc(3);
  node->inToken = loc(5);
  node->rparenToken = loc(7);
  sym(1).Node = node;
} break;
./

ContinueStatement: T_CONTINUE T_AUTOMATIC_SEMICOLON ;  -- automatic semicolon
ContinueStatement: T_CONTINUE T_SEMICOLON ;
/.
case $rule_number: {
  AST::ContinueStatement *node = makeAstNode<AST::ContinueStatement> (driver->nodePool());
  node->continueToken = loc(1);
  node->semicolonToken = loc(2);
  sym(1).Node = node;
} break;
./

ContinueStatement: T_CONTINUE JsIdentifier T_AUTOMATIC_SEMICOLON ;  -- automatic semicolon
ContinueStatement: T_CONTINUE JsIdentifier T_SEMICOLON ;
/.
case $rule_number: {
  AST::ContinueStatement *node = makeAstNode<AST::ContinueStatement> (driver->nodePool(), sym(2).sval);
  node->continueToken = loc(1);
  node->identifierToken = loc(2);
  node->semicolonToken = loc(3);
  sym(1).Node = node;
} break;
./

BreakStatement: T_BREAK T_AUTOMATIC_SEMICOLON ;  -- automatic semicolon
BreakStatement: T_BREAK T_SEMICOLON ;
/.
case $rule_number: {
  AST::BreakStatement *node = makeAstNode<AST::BreakStatement> (driver->nodePool());
  node->breakToken = loc(1);
  node->semicolonToken = loc(2);
  sym(1).Node = node;
} break;
./

BreakStatement: T_BREAK JsIdentifier T_AUTOMATIC_SEMICOLON ;  -- automatic semicolon
BreakStatement: T_BREAK JsIdentifier T_SEMICOLON ;
/.
case $rule_number: {
  AST::BreakStatement *node = makeAstNode<AST::BreakStatement> (driver->nodePool(), sym(2).sval);
  node->breakToken = loc(1);
  node->identifierToken = loc(2);
  node->semicolonToken = loc(3);
  sym(1).Node = node;
} break;
./

ReturnStatement: T_RETURN ExpressionOpt T_AUTOMATIC_SEMICOLON ;  -- automatic semicolon
ReturnStatement: T_RETURN ExpressionOpt T_SEMICOLON ;
/.
case $rule_number: {
  AST::ReturnStatement *node = makeAstNode<AST::ReturnStatement> (driver->nodePool(), sym(2).Expression);
  node->returnToken = loc(1);
  node->semicolonToken = loc(3);
  sym(1).Node = node;
} break;
./

WithStatement: T_WITH T_LPAREN Expression T_RPAREN Statement ;
/.
case $rule_number: {
  AST::WithStatement *node = makeAstNode<AST::WithStatement> (driver->nodePool(), sym(3).Expression, sym(5).Statement);
  node->withToken = loc(1);
  node->lparenToken = loc(2);
  node->rparenToken = loc(4);
  sym(1).Node = node;
} break;
./

SwitchStatement: T_SWITCH T_LPAREN Expression T_RPAREN CaseBlock ;
/.
case $rule_number: {
  AST::SwitchStatement *node = makeAstNode<AST::SwitchStatement> (driver->nodePool(), sym(3).Expression, sym(5).CaseBlock);
  node->switchToken = loc(1);
  node->lparenToken = loc(2);
  node->rparenToken = loc(4);
  sym(1).Node = node;
} break;
./

CaseBlock: T_LBRACE CaseClausesOpt T_RBRACE ;
/.
case $rule_number: {
  AST::CaseBlock *node = makeAstNode<AST::CaseBlock> (driver->nodePool(), sym(2).CaseClauses);
  node->lbraceToken = loc(1);
  node->rbraceToken = loc(3);
  sym(1).Node = node;
} break;
./

CaseBlock: T_LBRACE CaseClausesOpt DefaultClause CaseClausesOpt T_RBRACE ;
/.
case $rule_number: {
  AST::CaseBlock *node = makeAstNode<AST::CaseBlock> (driver->nodePool(), sym(2).CaseClauses, sym(3).DefaultClause, sym(4).CaseClauses);
  node->lbraceToken = loc(1);
  node->rbraceToken = loc(5);
  sym(1).Node = node;
} break;
./

CaseClauses: CaseClause ;
/.
case $rule_number: {
  sym(1).Node = makeAstNode<AST::CaseClauses> (driver->nodePool(), sym(1).CaseClause);
} break;
./

CaseClauses: CaseClauses CaseClause ;
/.
case $rule_number: {
  sym(1).Node = makeAstNode<AST::CaseClauses> (driver->nodePool(), sym(1).CaseClauses, sym(2).CaseClause);
} break;
./

CaseClausesOpt: ;
/.
case $rule_number: {
  sym(1).Node = 0;
} break;
./

CaseClausesOpt: CaseClauses ;
/.
case $rule_number: {
  sym(1).Node = sym(1).CaseClauses->finish ();
} break;
./

CaseClause: T_CASE Expression T_COLON StatementListOpt ;
/.
case $rule_number: {
  AST::CaseClause *node = makeAstNode<AST::CaseClause> (driver->nodePool(), sym(2).Expression, sym(4).StatementList);
  node->caseToken = loc(1);
  node->colonToken = loc(3);
  sym(1).Node = node;
} break;
./

DefaultClause: T_DEFAULT T_COLON StatementListOpt ;
/.
case $rule_number: {
  AST::DefaultClause *node = makeAstNode<AST::DefaultClause> (driver->nodePool(), sym(3).StatementList);
  node->defaultToken = loc(1);
  node->colonToken = loc(2);
  sym(1).Node = node;
} break;
./

LabelledStatement: T_SIGNAL T_COLON Statement ;
/.case $rule_number:./

LabelledStatement: T_PROPERTY T_COLON Statement ;
/.
case $rule_number: {
  AST::LabelledStatement *node = makeAstNode<AST::LabelledStatement> (driver->nodePool(), driver->intern(lexer->characterBuffer(), lexer->characterCount()), sym(3).Statement);
  node->identifierToken = loc(1);
  node->colonToken = loc(2);
  sym(1).Node = node;
} break;
./

LabelledStatement: T_IDENTIFIER T_COLON Statement ;
/.
case $rule_number: {
  AST::LabelledStatement *node = makeAstNode<AST::LabelledStatement> (driver->nodePool(), sym(1).sval, sym(3).Statement);
  node->identifierToken = loc(1);
  node->colonToken = loc(2);
  sym(1).Node = node;
} break;
./

ThrowStatement: T_THROW Expression T_AUTOMATIC_SEMICOLON ;  -- automatic semicolon
ThrowStatement: T_THROW Expression T_SEMICOLON ;
/.
case $rule_number: {
  AST::ThrowStatement *node = makeAstNode<AST::ThrowStatement> (driver->nodePool(), sym(2).Expression);
  node->throwToken = loc(1);
  node->semicolonToken = loc(3);
  sym(1).Node = node;
} break;
./

TryStatement: T_TRY Block Catch ;
/.
case $rule_number: {
  AST::TryStatement *node = makeAstNode<AST::TryStatement> (driver->nodePool(), sym(2).Statement, sym(3).Catch);
  node->tryToken = loc(1);
  sym(1).Node = node;
} break;
./

TryStatement: T_TRY Block Finally ;
/.
case $rule_number: {
  AST::TryStatement *node = makeAstNode<AST::TryStatement> (driver->nodePool(), sym(2).Statement, sym(3).Finally);
  node->tryToken = loc(1);
  sym(1).Node = node;
} break;
./

TryStatement: T_TRY Block Catch Finally ;
/.
case $rule_number: {
  AST::TryStatement *node = makeAstNode<AST::TryStatement> (driver->nodePool(), sym(2).Statement, sym(3).Catch, sym(4).Finally);
  node->tryToken = loc(1);
  sym(1).Node = node;
} break;
./

Catch: T_CATCH T_LPAREN JsIdentifier T_RPAREN Block ;
/.
case $rule_number: {
  AST::Catch *node = makeAstNode<AST::Catch> (driver->nodePool(), sym(3).sval, sym(5).Block);
  node->catchToken = loc(1);
  node->lparenToken = loc(2);
  node->identifierToken = loc(3);
  node->rparenToken = loc(4);
  sym(1).Node = node;
} break;
./

Finally: T_FINALLY Block ;
/.
case $rule_number: {
  AST::Finally *node = makeAstNode<AST::Finally> (driver->nodePool(), sym(2).Block);
  node->finallyToken = loc(1);
  sym(1).Node = node;
} break;
./

DebuggerStatement: T_DEBUGGER T_AUTOMATIC_SEMICOLON ; -- automatic semicolon
DebuggerStatement: T_DEBUGGER T_SEMICOLON ;
/.
case $rule_number: {
  AST::DebuggerStatement *node = makeAstNode<AST::DebuggerStatement> (driver->nodePool());
  node->debuggerToken = loc(1);
  node->semicolonToken = loc(2);
  sym(1).Node = node;
} break;
./

FunctionDeclaration: T_FUNCTION JsIdentifier T_LPAREN FormalParameterListOpt T_RPAREN T_LBRACE FunctionBodyOpt T_RBRACE ;
/.
case $rule_number: {
  AST::FunctionDeclaration *node = makeAstNode<AST::FunctionDeclaration> (driver->nodePool(), sym(2).sval, sym(4).FormalParameterList, sym(7).FunctionBody);
  node->functionToken = loc(1);
  node->identifierToken = loc(2);
  node->lparenToken = loc(3);
  node->rparenToken = loc(5);
  node->lbraceToken = loc(6);
  node->rbraceToken = loc(8);
  sym(1).Node = node;
} break;
./

FunctionExpression: T_FUNCTION IdentifierOpt T_LPAREN FormalParameterListOpt T_RPAREN T_LBRACE FunctionBodyOpt T_RBRACE ;
/.
case $rule_number: {
  AST::FunctionExpression *node = makeAstNode<AST::FunctionExpression> (driver->nodePool(), sym(2).sval, sym(4).FormalParameterList, sym(7).FunctionBody);
  node->functionToken = loc(1);
  if (sym(2).sval)
      node->identifierToken = loc(2);
  node->lparenToken = loc(3);
  node->rparenToken = loc(5);
  node->lbraceToken = loc(6);
  node->rbraceToken = loc(8);
  sym(1).Node = node;
} break;
./

FormalParameterList: JsIdentifier ;
/.
case $rule_number: {
  AST::FormalParameterList *node = makeAstNode<AST::FormalParameterList> (driver->nodePool(), sym(1).sval);
  node->identifierToken = loc(1);
  sym(1).Node = node;
} break;
./

FormalParameterList: FormalParameterList T_COMMA JsIdentifier ;
/.
case $rule_number: {
  AST::FormalParameterList *node = makeAstNode<AST::FormalParameterList> (driver->nodePool(), sym(1).FormalParameterList, sym(3).sval);
  node->commaToken = loc(2);
  node->identifierToken = loc(3);
  sym(1).Node = node;
} break;
./

FormalParameterListOpt: ;
/.
case $rule_number: {
  sym(1).Node = 0;
} break;
./

FormalParameterListOpt: FormalParameterList ;
/.
case $rule_number: {
  sym(1).Node = sym(1).FormalParameterList->finish ();
} break;
./

FunctionBodyOpt: ;
/.
case $rule_number: {
  sym(1).Node = 0;
} break;
./

FunctionBodyOpt: FunctionBody ;

FunctionBody: SourceElements ;
/.
case $rule_number: {
  sym(1).Node = makeAstNode<AST::FunctionBody> (driver->nodePool(), sym(1).SourceElements->finish ());
} break;
./

Program: SourceElements ;
/.
case $rule_number: {
  sym(1).Node = makeAstNode<AST::Program> (driver->nodePool(), sym(1).SourceElements->finish ());
} break;
./

SourceElements: SourceElement ;
/.
case $rule_number: {
  sym(1).Node = makeAstNode<AST::SourceElements> (driver->nodePool(), sym(1).SourceElement);
} break;
./

SourceElements: SourceElements SourceElement ;
/.
case $rule_number: {
  sym(1).Node = makeAstNode<AST::SourceElements> (driver->nodePool(), sym(1).SourceElements, sym(2).SourceElement);
} break;
./

SourceElement: Statement ;
/.
case $rule_number: {
  sym(1).Node = makeAstNode<AST::StatementSourceElement> (driver->nodePool(), sym(1).Statement);
} break;
./

SourceElement: FunctionDeclaration ;
/.
case $rule_number: {
  sym(1).Node = makeAstNode<AST::FunctionSourceElement> (driver->nodePool(), sym(1).FunctionDeclaration);
} break;
./

IdentifierOpt: ;
/.
case $rule_number: {
  sym(1).sval = 0;
} break;
./

IdentifierOpt: JsIdentifier ;

PropertyNameAndValueListOpt: ;
/.
case $rule_number: {
  sym(1).Node = 0;
} break;
./

PropertyNameAndValueListOpt: PropertyNameAndValueList ;

/.
            } // switch
            action = nt_action(state_stack[tos], lhs[r] - TERMINAL_COUNT);
        } // if
    } while (action != 0);

    if (first_token == last_token) {
        const int errorState = state_stack[tos];

        // automatic insertion of `;'
        if (yytoken != -1 && ((t_action(errorState, T_AUTOMATIC_SEMICOLON) && automatic(driver, yytoken))
                              || t_action(errorState, T_COMPATIBILITY_SEMICOLON))) {
            SavedToken &tk = token_buffer[0];
            tk.token = yytoken;
            tk.dval = yylval;
            tk.loc = yylloc;

            yylloc = yyprevlloc;
            yylloc.offset += yylloc.length;
            yylloc.startColumn += yylloc.length;
            yylloc.length = 0;

            //const QString msg = qApp->translate("QDeclarativeParser", "Missing `;'");
            //diagnostic_messages.append(DiagnosticMessage(DiagnosticMessage::Warning, yylloc, msg));

            first_token = &token_buffer[0];
            last_token = &token_buffer[1];

            yytoken = T_SEMICOLON;
            yylval = 0;

            action = errorState;

            goto _Lcheck_token;
        }

        hadErrors = true;

        token_buffer[0].token = yytoken;
        token_buffer[0].dval = yylval;
        token_buffer[0].loc = yylloc;

        token_buffer[1].token = yytoken = lexer->lex();
        token_buffer[1].dval  = yylval  = lexer->dval();
        token_buffer[1].loc   = yylloc  = location(lexer);

        if (t_action(errorState, yytoken)) {
            QString msg;
            int token = token_buffer[0].token;
            if (token < 0 || token >= TERMINAL_COUNT)
                msg = qApp->translate("QDeclarativeParser", "Syntax error");
            else
                msg = qApp->translate("QDeclarativeParser", "Unexpected token `%1'").arg(QLatin1String(spell[token]));
            diagnostic_messages.append(DiagnosticMessage(DiagnosticMessage::Error, token_buffer[0].loc, msg));

            action = errorState;
            goto _Lcheck_token;
        }

        static int tokens[] = {
            T_PLUS,
            T_EQ,

            T_COMMA,
            T_COLON,
            T_SEMICOLON,

            T_RPAREN, T_RBRACKET, T_RBRACE,

            T_NUMERIC_LITERAL,
            T_IDENTIFIER,

            T_LPAREN, T_LBRACKET, T_LBRACE,

            EOF_SYMBOL
        };

        for (int *tk = tokens; *tk != EOF_SYMBOL; ++tk) {
            int a = t_action(errorState, *tk);
            if (a > 0 && t_action(a, yytoken)) {
                const QString msg = qApp->translate("QDeclarativeParser", "Expected token `%1'").arg(QLatin1String(spell[*tk]));
                diagnostic_messages.append(DiagnosticMessage(DiagnosticMessage::Error, token_buffer[0].loc, msg));

                yytoken = *tk;
                yylval = 0;
                yylloc = token_buffer[0].loc;
                yylloc.length = 0;

                first_token = &token_buffer[0];
                last_token = &token_buffer[2];

                action = errorState;
                goto _Lcheck_token;
            }
        }

        for (int tk = 1; tk < TERMINAL_COUNT; ++tk) {
            if (tk == T_AUTOMATIC_SEMICOLON || tk == T_FEED_UI_PROGRAM    ||
                tk == T_FEED_JS_STATEMENT   || tk == T_FEED_JS_EXPRESSION ||
                tk == T_FEED_JS_SOURCE_ELEMENT)
               continue;

            int a = t_action(errorState, tk);
            if (a > 0 && t_action(a, yytoken)) {
                const QString msg = qApp->translate("QDeclarativeParser", "Expected token `%1'").arg(QLatin1String(spell[tk]));
                diagnostic_messages.append(DiagnosticMessage(DiagnosticMessage::Error, token_buffer[0].loc, msg));

                yytoken = tk;
                yylval = 0;
                yylloc = token_buffer[0].loc;
                yylloc.length = 0;

                action = errorState;
                goto _Lcheck_token;
            }
        }

        const QString msg = qApp->translate("QDeclarativeParser", "Syntax error");
        diagnostic_messages.append(DiagnosticMessage(DiagnosticMessage::Error, token_buffer[0].loc, msg));
    }

    return false;
}

QT_QML_END_NAMESPACE


./
/:
QT_QML_END_NAMESPACE



#endif // QDECLARATIVEJSPARSER_P_H
:/
