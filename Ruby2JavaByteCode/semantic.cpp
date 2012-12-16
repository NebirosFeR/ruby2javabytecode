
#include "semantic.h"
#include "structures.h"
#include "test.h"
#include <QFile>
#include <QDataStream>
#include "CodeCommands.h"

SemanticAnalyzer::SemanticAnalyzer(ProgramNode* root)
{
    this->root = AttrProgram::fromParserNode(root);
}

void SemanticAnalyzer::doSemantics()
{
    //�������� ������ ������
    SemanticClass* commonClass = new SemanticClass();
    commonClass->id = NAME_COMMON_CLASS;
    commonClass->parentId = NAME_JAVA_OBJECT;
    commonClass->constClass = commonClass->addConstantClass(QString(NAME_COMMON_CLASS));
    commonClass->constParent = commonClass->addConstantClass(QString(NAME_JAVA_OBJECT));
    commonClass->addRTLConstants();
    //SemanticMethod* constr = new SemanticMethod();
    //constr->id = NAME_DEFAULT_CONSTRUCTOR;
    //commonClass->methods.insert(constr->id, constr);

    //constr->constName = commonClass->addConstantUtf8(QString(NAME_DEFAULT_CONSTRUCTOR));
    //constr->constDesc = commonClass->addConstantUtf8(QString("()V"));


    classTable.insert(commonClass->id, commonClass);
    root->doFirstSemantics(classTable, NULL, NULL, errors, NULL);
    root->doSemantics(classTable, NULL, NULL, errors);
}


void SemanticAnalyzer::dotPrint(QTextStream & out)
{
    out << QString("digraph G{ node[shape=\"rectangle\",style=\"rounded, filled\",fillcolor=\"white\"] \n");
    root->dotPrint(out);
    out << QString("}");
}

void SemanticAnalyzer::generate()
{
    foreach(SemanticClass* semClass, classTable)
        semClass->generate();
}

void AttributedNode::fillStmtList(StmtSeqNode* seq, QLinkedList<AttrStmt*> & list)
{
    if(seq)
    {
        StmtNode* current = seq->first;
        while(current)
        {
            list << AttrStmt::fromParserNode(current);
            current = current->next;
        }
    }
}

void AttributedNode::doFirstSemantics(QHash<QString, SemanticClass *> &classTable, SemanticClass *curClass, SemanticMethod *curMethod, QList<QString> &errors, AttrStmt *parentStmt)
{
}

void AttributedNode::transform()
{
}

void AttributedNode::dotPrintStmtSeq(QLinkedList<AttrStmt*> seq, QTextStream & out)
{
    out << QString::number((int)this + 1) + "[label = \"stmt seq\"]" + QString("\n");
    out << QString::number((int)this) + "->" + QString::number((int)this + 1) + QString("\n");
    foreach(AttrStmt* stmt, seq)
    {
        stmt->dotPrint(out);
        out << QString::number((int)this + 1) + "->" + QString::number((int)stmt) + QString("\n");
    }
}

void AttributedNode::dotPrintExprSeq(QLinkedList<AttrExpr*> seq, QTextStream & out)
{
    out << QString::number((int)this + 1) + "[label = \"expr seq\"]" + QString("\n");
    out << QString::number((int)this) + "->" + QString::number((int)this + 1) + QString("\n");
    foreach(AttrExpr* expr, seq)
    {
        expr->dotPrint(out);
        out << QString::number((int)this + 1) + "->" + QString::number((int)expr) + QString("\n");
    }
}

void AttrProgram::doSemantics(QHash<QString, SemanticClass*> & classTable, SemanticClass* curClass, SemanticMethod* curMethod, QList<QString> & errors)
{

    body.first()->doSemantics(classTable, NULL, NULL, errors);
}

void AttrProgram::doFirstSemantics(QHash<QString, SemanticClass *> &classTable, SemanticClass *curClass, SemanticMethod *curMethod, QList<QString> &errors, AttrStmt* parentStmt)
{
    transform();
    body.first()->doFirstSemantics(classTable, NULL, NULL, errors, parentStmt);
}

AttrProgram* AttrProgram::fromParserNode(ProgramNode* node)
{
    AttrProgram* result = new AttrProgram();
    AttributedNode::fillStmtList(node->body, result->body);
    return result;
}

void AttrProgram::dotPrint(QTextStream & out)
{
    out << QString::number((int)this) + QString("[label = \"Program\"]") + QString("\n");
    dotPrintStmtSeq(body, out);
}

void AttrProgram::transform()
{
    AttrClassDef* mainClass = new AttrClassDef();
    mainClass->id = NAME_MAIN_CLASS;
    mainClass->parentId = NAME_COMMON_CLASS;

    AttrMethodDef* mainMethod = new AttrMethodDef();
    mainMethod->id = NAME_MAIN_CLASS_METHOD;
    mainMethod->params << new AttrMethodDefParam();
    mainMethod->params.first()->id = "argv";
    mainMethod->isStatic = true;
    mainClass->body << mainMethod;
    mainMethod->body << body;

    body.clear();
    body << mainClass;
}

AttrStmt* AttrStmt::fromParserNode(StmtNode* node)
{
    switch(node->type)
    {
    case eExpr:
        return AttrExprStmt::fromParserNode(node);
    case eWhile:
    case eUntil:
        return AttrCycleStmt::fromParserNode(node);
    case eClassDef:
        return AttrClassDef::fromParserNode(node);
    case eMethodDef:
        return AttrMethodDef::fromParserNode(node);
    case eReturn:
        return AttrReturnStmt::fromParserNode(node);
    }
}

void AttrStmt::generate(QDataStream &out, SemanticClass *curClass, SemanticMethod *curMethod)
{
}

AttrClassDef* AttrClassDef::fromParserNode(StmtNode* node)
{
    AttrClassDef* result = new AttrClassDef();
    result->id = node->id;
    result->parentId = node->secondId;
    result->type = node->type;
    AttributedNode::fillStmtList(node->block, result->body);
    return result;
}

void AttrClassDef::doSemantics(QHash<QString, SemanticClass *> &classTable, SemanticClass *curClass, SemanticMethod *curMethod, QList<QString> & errors)
{

    if(!id[0].isUpper() && id[0].isLetter())
    {
        errors << "Incorrect class name: " + id + ".";
        return;
    }
    //if(curClass->id != NAME_MAIN_CLASS && curMethod->id != NAME_MAIN_CLASS_METHOD)
    //{
    //    errors << "Class definition in wrong place: " + id + ".";
    //    return;
   // }

    transform();

    SemanticClass* semClass = classTable.value(id);

    semClass->addRTLConstants();

    //semClass->id = id;
    semClass->parentId = parentId;
    semClass->constClass = semClass->addConstantClass(id);



    // �������� ������������� ��������
    if(parentId != NAME_COMMON_CLASS && parentId != NAME_JAVA_OBJECT)
    {
        if(!classTable.contains(parentId))
        {
            errors << "Parent class " + parentId + " of " + id + " not found.";
            return;
        }
    }
    semClass->constParent = semClass->addConstantClass(parentId);

    foreach(AttrStmt* stmt, body)
        stmt->doSemantics(classTable, semClass, NULL, errors);


}

void AttrClassDef::doFirstSemantics(QHash<QString, SemanticClass *> &classTable, SemanticClass *curClass, SemanticMethod *curMethod, QList<QString> &errors, AttrStmt *parentStmt)
{
    // �������� ���������������
    if(classTable.contains(id))
    {
        errors << "Class redefinition: " + id + ".";
        return;
    }

    SemanticClass* semClass = new SemanticClass();
    semClass->id = id;
    semClass->classDef = this;
    classTable.insert(id, semClass);

    // ����������� ����������� ������ � ���� ������
    if(parentStmt)
    {
        QLinkedList<AttrStmt*>* parentBody = parentStmt->getBody();
        parentBody->removeOne(this);
        curClass->classDef->getBody()->append(this);
    }

    foreach(AttrStmt* stmt, body)
        stmt->doFirstSemantics(classTable, semClass, NULL, errors, this);

    // �������� ������������� ������������
    if(!semClass->methods.contains(id) && id != NAME_MAIN_CLASS)
    {
        AttrMethodDef* defaultConstructor = new AttrMethodDef();
        defaultConstructor->id = NAME_DEFAULT_CONSTRUCTOR;
        defaultConstructor->isConstructor = true;

        AttrExprStmt* stmt = new AttrExprStmt();
        AttrMethodCall* superCall = new AttrMethodCall();
        superCall->id = "super";
        superCall->left = NULL;
        stmt->expr = superCall;

        defaultConstructor->body << stmt;

        defaultConstructor->doFirstSemantics(classTable, semClass, NULL, errors, this);
    }
}

void AttrClassDef::dotPrint(QTextStream & out)
{
    out << QString::number((int)this) + "[label = \"class " + id + "\"]" + QString("\n");
    dotPrintStmtSeq(body, out);
}

void AttrClassDef::transform()
{
    if(parentId.isEmpty())
        parentId = NAME_COMMON_CLASS;
}

void AttrClassDef::generate(QDataStream &out, SemanticClass *curClass, SemanticMethod *curMethod)
{
}

QLinkedList<AttrStmt *> *AttrClassDef::getBody()
{
    return &body;
}

AttrMethodDef* AttrMethodDef::fromParserNode(StmtNode* node)
{
    AttrMethodDef* result = new AttrMethodDef();
    result->id = node->id;
    AttributedNode::fillStmtList(node->block, result->body);
    MethodDefParamNode* current = node->params->first;
    while(current)
    {
        result->params << AttrMethodDefParam::fromParserNode(current);
        current = current->next;
    }
    result->type = node->type;
    return result;
}

void AttrMethodDef::doSemantics(QHash<QString, SemanticClass *> &classTable, SemanticClass *curClass, SemanticMethod *curMethod, QList<QString> & errors)
{

    SemanticMethod* newMethod = curClass->methods.value(id);
    newMethod->methodDef = this;


    if(curClass->id == id)
        isConstructor = true;
    if(curClass->id == NAME_MAIN_CLASS)
        isStatic = true;

    QString desc;
    if(curClass->id == NAME_MAIN_CLASS && id == NAME_MAIN_CLASS_METHOD)
        desc = DESC_MAIN_CLASS_METHOD;
    else
    {
        desc = QString("(");
        int count = params.count();
        for(int  i = 0; i < count; i++)
            desc += DESC_COMMON_VALUE;
        desc += QString(")");
        if(isConstructor)
            desc += "V";
        else
            desc += DESC_COMMON_VALUE;
    }


    newMethod->constName = curClass->addConstantUtf8(id);
    newMethod->constDesc = curClass->addConstantUtf8(desc);
    newMethod->constCode = curClass->addConstantUtf8(QString(ATTR_CODE));

    // ���������� ������ � ������ ��������
    //if(curClass->id != NAME_MAIN_CLASS)
    //{
    SemanticMethod* newMethodC;
    if(!isStatic)
    {
        SemanticClass* commonClass = classTable.value(QString(NAME_COMMON_CLASS));

        newMethodC = new SemanticMethod();
        newMethodC->id = id;
        newMethodC->constName = commonClass->addConstantUtf8(id);
        newMethodC->constDesc = commonClass->addConstantUtf8(desc);
        newMethodC->constCode = commonClass->addConstantUtf8(QString(ATTR_CODE));
        newMethodC->methodDef = this;

        commonClass->methods.insert(id, newMethodC);
    }

    foreach(AttrMethodDefParam* param, params)
        param->doSemantics(classTable, curClass, newMethod, errors);

    // �������� return
    AttrStmt* lastStmt = body.last();

    if(lastStmt->type != eReturn)
    {
        AttrReturnStmt* newLast = new AttrReturnStmt();
        if(newMethod->methodDef->isConstructor || newMethod->id == NAME_MAIN_CLASS_METHOD && curClass->id == NAME_MAIN_CLASS)
        {// ��� ������������� � main ������ return
            newLast->expr = NULL;
        }
        else if(lastStmt->type == eExpr)
        {// ���� � ����� ��� ��������� - �������� ��� �� return
            newLast->expr = ((AttrExprStmt*)lastStmt)->expr;
            body.removeLast();
            delete lastStmt;
        }
        else
        {// ����� return 0
            // TODO nil
            AttrConstExpr* value = new AttrConstExpr();
            value->intValue = 0;
            newLast->expr = value;
        }
        body << newLast;
    }

    foreach(AttrStmt* stmt, body)
        stmt->doSemantics(classTable, curClass, newMethod, errors);

    if(!isStatic)
        newMethodC->locals = newMethod->locals;
}

void AttrMethodDef::doFirstSemantics(QHash<QString, SemanticClass *> &classTable, SemanticClass *curClass, SemanticMethod *curMethod, QList<QString> &errors, AttrStmt *parentStmt)
{
    if(curClass->methods.contains(id))
    {
        errors << QString("Method already defined: " + id);
        return;
    }

    SemanticMethod* newMethod = new SemanticMethod();
    newMethod->paramCount = params.count();
    newMethod->id = id;
    curClass->methods.insert(id, newMethod);

    // ����������� ����������� ������� � ���� ������
    QLinkedList<AttrStmt*>* parentBody = parentStmt->getBody();
    parentBody->removeOne(this);
    curClass->classDef->getBody()->append(this);

    foreach(AttrStmt* stmt, body)
        stmt->doFirstSemantics(classTable, curClass, newMethod, errors, this);
}

void AttrMethodDef::dotPrint(QTextStream & out)
{
    out << QString::number((int)this) + "[label = \"method " + id + "\"]" + QString("\n");
    dotPrintStmtSeq(body, out);

    out << QString::number((int)this + 2) + "[label = \"param seq\"]" + QString("\n");
    out << QString::number((int)this) + "->" + QString::number((int)this + 2) + QString("\n");
    foreach(AttrMethodDefParam* param, params)
    {
        param->dotPrint(out);
        out << QString::number((int)this + 2) + "->" + QString::number((int)param) + QString("\n");
    }
}

void AttrMethodDef::generateCode(QDataStream &out, SemanticClass *curClass)
{ 
    SemanticMethod* semMethod = curClass->methods.value(id);

    out << (quint16)semMethod->constCode;

    QByteArray attribute;
    QDataStream attOut(&attribute, QIODevice::WriteOnly);

    attOut << (quint16)1000;
    int localsCount = semMethod->locals.count();
    if(!isStatic)
        localsCount++;
    attOut << (quint16)localsCount;

    QByteArray byteCode;
    QDataStream byteOut(&byteCode, QIODevice::WriteOnly);

    if(curClass->id != NAME_COMMON_CLASS)
        foreach(AttrStmt* stmt, body)
        {
            stmt->generate(byteOut, curClass, semMethod);
        }
    else if(id == NAME_MAIN_CLASS_METHOD)
        byteOut << RETURN;
    else
    {
        byteOut << NEW << (quint16)curClass->constants.value(curClass->constCommonValueClass)->number;
        byteOut << DUP;
        byteOut << BIPUSH << (qint8)0;
        byteOut << INVOKESPECIAL << (quint16)curClass->constants.value(curClass->constRTLInitIntRef)->number;
        byteOut << ARETURN;
        // TODO ������ ����������
    }


    attOut << (quint32)byteCode.length();
    attOut.writeRawData(byteCode.data(), byteCode.size());
    attOut << (quint16)0;
    attOut << (quint16)0;
    out << (quint32)attribute.size();
    out.writeRawData(attribute.data(), attribute.size());
}

QLinkedList<AttrStmt *> *AttrMethodDef::getBody()
{
    return &body;
}

AttrMethodDefParam* AttrMethodDefParam::fromParserNode(MethodDefParamNode* node)
{
    AttrMethodDefParam* result = new AttrMethodDefParam();
    result->id = node->id;
    return result;
}


void AttrMethodDefParam::doSemantics(QHash<QString, SemanticClass*> & classTable, SemanticClass* curClass, SemanticMethod* curMethod, QList<QString> & errors)
{
    curMethod->addLocalVar(id, curClass);
}

void AttrMethodDefParam::dotPrint(QTextStream & out)
{
    out << QString::number((int)this) + "[label=\"" + id + "\"]" + QString("\n");
}

AttrCycleStmt* AttrCycleStmt::fromParserNode(StmtNode* node)
{
    AttrCycleStmt* result = new AttrCycleStmt();
    result->expr = AttrExpr::fromParserNode(node->expr);
    AttributedNode::fillStmtList(node->block, result->block);
    if( node->type == eWhile)
        result->cycleType = cycleWhile;
    else
        result->cycleType = cycleUntil;
    result->type = node->type;
    return result;
}


void AttrCycleStmt::doSemantics(QHash<QString, SemanticClass *> &classTable, SemanticClass *curClass, SemanticMethod *curMethod, QList<QString> &errors)
{
    expr->doSemantics(classTable, curClass, curMethod, errors);
    foreach(AttrStmt* stmt, block)
        stmt->doSemantics(classTable, curClass, curMethod, errors);
}

void AttrCycleStmt::doFirstSemantics(QHash<QString, SemanticClass *> &classTable, SemanticClass *curClass, SemanticMethod *curMethod, QList<QString> &errors, AttrStmt *parentStmt)
{
    foreach(AttrStmt* stmt, block)
        stmt->doFirstSemantics(classTable, curClass, curMethod, errors, this);
}

void AttrCycleStmt::dotPrint(QTextStream & out)
{
    QString label;
    if(cycleType == cycleWhile)
        label = "while";
    else
        label = "until";
    out << QString::number((int)this) + "[label = \"" + label +"\"]" + QString("\n");
    dotPrintStmtSeq(block, out);
    expr->dotPrint(out);
    out << QString::number((int)this) + "->" + QString::number((int)expr) + QString("\n");
}

QLinkedList<AttrStmt *> *AttrCycleStmt::getBody()
{
    return &block;
}


AttrReturnStmt* AttrReturnStmt::fromParserNode(StmtNode* node)
{
    AttrReturnStmt* result = new AttrReturnStmt();
    result->expr = AttrExpr::fromParserNode(node->expr);
    result->type = node->type;
    return result;
}


void AttrReturnStmt::doSemantics(QHash<QString, SemanticClass *> &classTable, SemanticClass *curClass, SemanticMethod *curMethod, QList<QString> &errors)
{
    if(expr)
        expr->doSemantics(classTable, curClass, curMethod, errors);
}

void AttrReturnStmt::dotPrint(QTextStream & out)
{
    out << QString::number((int)this) + "[label = \"return\"]" + QString("\n");
    if(expr)
    {
        expr->dotPrint(out);
        out << QString::number((int)this) + "->" + QString::number((int)expr) + QString("\n");
    }
}

void AttrReturnStmt::generate(QDataStream &out, SemanticClass *curClass, SemanticMethod *curMethod)
{
    if(expr)
    {
        expr->generate(out, curClass, curMethod);
        out << ARETURN;
    }
    else
        out << RETURN;
}

QLinkedList<AttrStmt *> *AttrReturnStmt::getBody()
{
    return NULL;
}

AttrExprStmt* AttrExprStmt::fromParserNode(StmtNode* node)
{
    AttrExprStmt* result = new AttrExprStmt();
    result->expr = AttrExpr::fromParserNode(node->expr);
    result->type = node->type;
    return result;
}


void AttrExprStmt::doSemantics(QHash<QString, SemanticClass *> &classTable, SemanticClass *curClass, SemanticMethod *curMethod, QList<QString> &errors)
{
    expr->doSemantics(classTable, curClass, curMethod, errors);
}

void AttrExprStmt::dotPrint(QTextStream & out)
{
    expr->dotPrint(out);
    out << QString::number((int)this) + "[label=\"expr stmt\"]" + QString("\n");
    out << QString::number((int)this) + "->" + QString::number((int)expr) + QString("\n");
}

void AttrExprStmt::generate(QDataStream &out, SemanticClass *curClass, SemanticMethod *curMethod)
{
    expr->generate(out, curClass, curMethod);
}

QLinkedList<AttrStmt *> *AttrExprStmt::getBody()
{
    return NULL;
}

AttrExpr* AttrExpr::fromParserNode(ExprNode* node)
{
    if(!node)
        return NULL;
    switch(node->type)
    {
    case eInt:
    case eBool:
    case eString:
        return AttrConstExpr::fromParserNode(node);
    case eFieldAcc:
        return AttrFieldAcc::fromParserNode(node);
    case eLocal:
        return AttrLocal::fromParserNode(node);
    case eMethodCall:
    case eSuper:
        return AttrMethodCall::fromParserNode(node);
    case eAssign:
    case ePlus:
    case eMinus:
    case eMul:
    case eDiv:
    case eLess:
    case eMore:
    case eOr:
    case eAnd:
    case eEqu:
    case eNEqu:
        return AttrBinExpr::fromParserNode(node);
    case eNot:
    case eUMinus:
    case eBrackets:
        return AttrUnExpr::fromParserNode(node);
    }
}

void AttrExpr::generate(QDataStream &out, SemanticClass *curClass, SemanticMethod* curMethod)
{
}

AttrBinExpr* AttrBinExpr::fromParserNode(ExprNode* node)
{
    AttrBinExpr* result = new AttrBinExpr();
    result->left = AttrExpr::fromParserNode(node->left);
    result->right = AttrExpr::fromParserNode(node->right);
    result->type = node->type;
    return result;
}

void AttrBinExpr::doSemantics(QHash<QString, SemanticClass *> &classTable, SemanticClass *curClass, SemanticMethod *curMethod, QList<QString> &errors)
{
    transform();

    if(type == eAssign && left->type != eLocalNewRef && left->type != eFieldAccRef)
    {
        errors << "Not lvalue left of \"=\"";
        return;
    }
    if(left)
        left->doSemantics(classTable, curClass, curMethod, errors);
    right->doSemantics(classTable, curClass, curMethod, errors);
}

void AttrBinExpr::dotPrint(QTextStream & out)
{
    transform();
    if(type == eAssign && left->type != eLocalRef && left->type != eFieldAccRef)
    {
        out << QString::number((int)this) + "[label=\" = \"]" + QString("\n");
    }
    else
    {
        char str[25];
        exprTypeToStr(type, str);
        out << QString::number((int)this) + "[label=\"" + str + "\"]" + QString("\n");
    }
    if(left)
        left->dotPrint(out);
    right->dotPrint(out);
    out << QString::number((int)this) + "->" + QString::number((int)left) + QString("\n");
    out << QString::number((int)this) + "->" + QString::number((int)right) + QString("\n");
}

void AttrBinExpr::transform()
{
    if(type == eAssign)
    {
        if(left->type == eFieldAcc)
        {
            //type = eFieldAccAssign;
            left->type = eFieldAccRef;
           // AttrExpr* newLeft;
          //  newLeft = ((AttrFieldAcc*)left)->left;
           // id = ((AttrFieldAcc*)left)->id;
           // delete left;
           // left = newLeft;
        }
        else if(left->type == eLocal)
        {
            //type = eLocalAssign;
            left->type = eLocalNewRef;
            //AttrExpr* newLeft;
            //newLeft = ((AttrLocal*)left)->left;
            //id = ((AttrFieldAcc*)left)->id;
            //delete left;
            //left = newLeft;
        }
        // TODO array
    }
}

void AttrBinExpr::generate(QDataStream &out, SemanticClass *curClass, SemanticMethod *curMethod)
{
    left->generate(out, curClass, curMethod);
    right->generate(out, curClass, curMethod);

    switch(type)
    {
        case eAssign:
        {
            out << INVOKEVIRTUAL << (quint16)curClass->constRTLAssignRef;
            break;
        }
        case ePlus:
        case eMinus:
        case eMul:
        case eDiv:
        case eLess:
        case eMore:
        case eOr:
        case eAnd:
        case eEqu:
        case eNEqu:
        break;
    }
}

AttrUnExpr* AttrUnExpr::fromParserNode(ExprNode* node)
{
    AttrUnExpr* result = new AttrUnExpr();
    result->expr = AttrExpr::fromParserNode(node->left);
    result->type = node->type;
    return result;
}

void AttrUnExpr::doSemantics(QHash<QString, SemanticClass *> &classTable, SemanticClass *curClass, SemanticMethod *curMethod, QList<QString> &errors)
{
    transform();
    if(expr->type == eBool || expr->type == eString)
    {
        errors << "Incorrect type with uMinus";
        return;
    }
    expr->doSemantics(classTable, curClass, curMethod, errors);
}

void AttrUnExpr::dotPrint(QTextStream & out)
{
    char str[25];
    exprTypeToStr(type, str);
    out << QString::number((int)this) + "[label=\"" + str + "\"]" + QString("\n");
    expr->dotPrint(out);
    out << QString::number((int)this) + "->" + QString::number((int)expr) + QString("\n");
}

void AttrUnExpr::generate(QDataStream &out, SemanticClass *curClass, SemanticMethod *curMethod)
{
}

AttrMethodCall* AttrMethodCall::fromParserNode(ExprNode* node)
{
    AttrMethodCall* result = new AttrMethodCall();
    result->type = node->type;
    result->id = node->id;

    if(node->list)
    {
        ExprNode* current = node->list->first;
        while(current)
        {
            result->arguments << AttrExpr::fromParserNode(current);
            current = current->next;
        }
    }
    result->left = AttrExpr::fromParserNode(node->left);
    return result;
}

void AttrMethodCall::doSemantics(QHash<QString, SemanticClass *> &classTable, SemanticClass *curClass, SemanticMethod *curMethod, QList<QString> &errors)
{
    bool methodFound = false;
    if(left == NULL)
    {
        if(id == NAME_SUPER_METHOD ||
                id == NAME_PRINTINT_METHOD)
            methodFound = true;
    }
    if(!methodFound)
    {
        foreach(SemanticClass* semClass, classTable)
            foreach(SemanticMethod* method, semClass->methods)
                if(method->id == id && method->paramCount == arguments.count())
                    methodFound = true;
    }
    if(!methodFound)
    {
        errors << "Method with " + QString::number(arguments.count()) + " arguments not exists: " + id;
        return;
    }

    QString desc("(");
    int count = arguments.count();
    for(int  i = 0; i < count; i++)
        desc += DESC_COMMON_VALUE;
    desc += QString(")");

    if(left)
    {
        // �������� �� �������� new
        QString leftId;
        if(left->type == eLocal)
        {
            leftId = ((AttrLocal*)left)->id;
            if(left->type == eFieldAcc && id == "new" && leftId[0].isUpper())
            {
                if(!classTable.contains(leftId))
                {
                    errors << "Class doesn't exist when creating object: " + id;
                    return;
                }
                isObjectCreating = true;
            }
        }
        else
        {
            isObjectCreating = false;
        }

        if(!isObjectCreating)
            desc += DESC_COMMON_VALUE;
        else
            desc += "V";

        if(isObjectCreating)
            constMethodRef = curClass->addConstantMethodRef(leftId, id, desc);
        else
            constMethodRef = curClass->addConstantMethodRef(QString(NAME_COMMON_CLASS), id, desc);

        left->doSemantics(classTable, curClass, curMethod, errors);
    }
    else
    {
        desc += DESC_COMMON_VALUE;
        if(curClass->id == NAME_MAIN_CLASS)
        // ����������� ����� ������ MainClass
            constMethodRef = curClass->addConstantMethodRef(QString(NAME_MAIN_CLASS), id, desc);
        else
        // ������� �����
            constMethodRef = curClass->addConstantMethodRef(QString(NAME_COMMON_CLASS), id, desc);
    }
    if(id == "super")
    {
        if(classTable.contains(curClass->parentId) && !classTable.value(curClass->parentId)->methods.contains(curMethod->id))
            errors << "Parent class doesn't have method:" + curMethod->id;
    }
}

void AttrMethodCall::dotPrint(QTextStream & out)
{
    out << QString::number((int)this) + "[label=\" method call: " + id + "\"]" + QString("\n");

    dotPrintExprSeq(arguments, out);
    if(left != NULL)
    {
        left->dotPrint(out);
        out << QString::number((int)this) + "->" + QString::number((int)left) + QString("\n");
    }
}

void AttrMethodCall::generate(QDataStream &out, SemanticClass *curClass, SemanticMethod *curMethod)
{
    //TODO



    if(id == NAME_PRINTINT_METHOD)
    {
        foreach(AttrExpr* argument, arguments)
            argument->generate(out, curClass, curMethod);
        out << INVOKESTATIC << (quint16)curClass->constants.value(curClass->constRTLConsolePrintIntRef)->number;
    }
    else if(id == NAME_SUPER_METHOD)
    {
       // TODO

    }
    else
    {
        if(left)
            left->generate(out, curClass, curMethod);
        //else
         //   if(!)
         //   out << (quint16)0; // ������ �� self

        foreach(AttrExpr* argument, arguments)
            argument->generate(out, curClass, curMethod);

        if(!left && curClass->id == NAME_MAIN_CLASS)
        {
            out << INVOKESTATIC << (quint16)constMethodRef;
        }
        //if()
        //out << INVOKEVIRTUAL << (quint16)curClass->constants.value(constMethodRef);
        //out << INVOKESTATIC << (quint16)curClass->constants.value(constMethodRef)->number;
    }
}

AttrFieldAcc* AttrFieldAcc::fromParserNode(ExprNode* node)
{
    AttrFieldAcc* result = new AttrFieldAcc();
    result->id = node->id;
    result->type = node->type;
    result->left = AttrExpr::fromParserNode(node->left);
    return result;
}



void AttrFieldAcc::doSemantics(QHash<QString, SemanticClass *> &classTable, SemanticClass *curClass, SemanticMethod *curMethod, QList<QString> &errors)
{
    if(left != NULL)
    {
        left->doSemantics(classTable, curClass, curMethod, errors);
    }
    else
    {
        curClass->addField(id);
        constFieldRef = curClass->addConstantFieldRef(curClass->id, id, QString(DESC_COMMON_VALUE));
    }
}

void AttrFieldAcc::dotPrint(QTextStream & out)
{
    out << QString::number((int)this) + "[label=\" field access: " + id + "\"]" + QString("\n");
    if(left != NULL)
    {
        left->dotPrint(out);
        out << QString::number((int)this) + "->" + QString::number((int)left) + QString("\n");
    }
}

void AttrFieldAcc::generate(QDataStream &out, SemanticClass *curClass, SemanticMethod *curMethod)
{
    //out << ILOAD;
}

AttrConstExpr* AttrConstExpr::fromParserNode(ExprNode* node)
{
    AttrConstExpr* result = new AttrConstExpr();
    result->str = node->str;
    result->intValue = node->value;
    result->type = node->type;
    return result;
}

void AttrConstExpr::doSemantics(QHash<QString, SemanticClass *> &classTable, SemanticClass *curClass, SemanticMethod *curMethod, QList<QString> &errors)
{
    if(type == eInt && intValue > VALUE_MAX2BIT)
    {
        constValue = curClass->addConstantInteger(intValue);
    }
    else if(type == eString)
    {
        constValue = curClass->addConstantString(str);
    }
}

void AttrConstExpr::dotPrint(QTextStream & out)
{
    QString label;
    switch(type)
    {
    case eInt:
        label = QString::number(intValue);
        break;
    case eBool:
        if(intValue)
            label = QString("true");
        else
            label = QString("false");
        break;
    case eString:
        label = str;
        break;
    }
    out << QString::number((int)this) + "[label=\"" + label + "\"]" + QString("\n");
}

void AttrConstExpr::generate(QDataStream &out, SemanticClass *curClass, SemanticMethod *curMethod)
{
    if(type == eInt)
    {
        out << NEW << (quint16)curClass->constants.value(curClass->constCommonValueClass)->number;
        out << DUP;
        //TODO long int
        out << BIPUSH << (qint8)intValue;
        out << INVOKESPECIAL << (quint16)curClass->constants.value(curClass->constRTLInitIntRef)->number;
    }
}


AttrLocal *AttrLocal::fromParserNode(ExprNode *node)
{
    AttrLocal* result = new AttrLocal();
    result->id = node->id;
    result->type = eLocal;
    return result;
}

void AttrLocal::doSemantics(QHash<QString, SemanticClass *> &classTable, SemanticClass *curClass, SemanticMethod *curMethod, QList<QString> &errors)
{
    curMethod->addLocalVar(id, curClass);
}

void AttrLocal::dotPrint(QTextStream &out)
{
    out << QString::number((int)this) + "[label=\" local access: " + id + "\"]" + QString("\n");
}

void AttrLocal::generate(QDataStream &out, SemanticClass *curClass, SemanticMethod* curMethod)
{
    if(type == eLocal)
    {
        out << ALOAD << (quint8)curMethod->locals.value(id)->number;
    }
    else if(type == eLocalNewRef)
    {
        out << NEW << (quint16)curClass->constants.value(curClass->constCommonValueClass)->number;
        out << DUP;
        out << DUP;
        out << ASTORE << (quint8)curMethod->locals.value(id)->number;
        out << INVOKESPECIAL << (quint16)curClass->constants.value(curClass->constRTLInitUninitRef)->number;
    }
    else if(type == eLocalRef)
    {
        out << ALOAD << (quint8)curMethod->locals.value(id)->number;
    }
}
