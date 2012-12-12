#ifndef TABLES_H
#define TABLES_H

#include <qmap.h>
#include <qhash.h>
#include <QDataStream>

class SemanticClass;
class SemanticConst;
class SemanticMethod;
class SemanticVar;

class AttrClassDef;
class AttrMethodDef;

#define EMPTY_CONST_NUMBER -1
#define NAMESPACE "Ruby/"

#define NAME_COMMON_CLASS NAMESPACE"<CommonClass>"
#define NAME_MAIN_CLASS NAMESPACE"<MainClass>"
#define NAME_MAIN_CLASS_METHOD "main"
#define NAME_DEFAULT_CONSTRUCTOR "<init>"
#define NAME_JAVA_OBJECT "Java/lang/Object"

#define ACC_SUPER 0x02
#define ACC_PUBLIC 0x01
#define ACC_PUBLIC_FIELD 0x001
#define ACC_ABSTRACT 0x0400

#define VALUE_MAX2BIT 65535

#define DESC_COMMON_CLASS "L"NAME_COMMON_CLASS";"

enum ConstType
{
    CONSTANT_Utf8 = 1,
    CONSTANT_Integer = 3,
    CONSTANT_String = 8,
    CONSTANT_NameAndType = 12,
    CONSTANT_Class = 7,
    CONSTANT_Fieldref = 9,
    CONSTANT_Methodref = 10
};

class SemanticClass
{
public:
    int constClass;
    int constParent;
    bool isAbstract;

    QString id;
    QString parentId;

    QHash<QString, SemanticMethod*> methods;
    QHash<int, SemanticConst*> constants;
    QHash<QString, SemanticVar*> fields;

    SemanticClass() { isAbstract = false; }
    AttrClassDef* classDef;

    void addField(QString & id);
   // void addMethod(QString & name, QString & desc);

    int addConstantUtf8(QString & value);
    int addConstantInteger(int value);
    int addConstantString(QString & value);
    int addConstantNameAndType(QString & name, QString & type);
    int addConstantClass(QString & name);
    int addConstantFieldRef(QString & className, QString & fieldName, QString & type);
    int addConstantMethodRef(QString & className, QString & methodName, QString & type);

    void generate();
};

class SemanticConst
{
public:
    SemanticConst(ConstType type, int number, QString & strValue = QString(), int intValue = 666, int ref1 = -1, int ref2 = -1);

    ConstType type;
    int number; // ����� ���������

    QString strValue;
    int intValue;

    int numberRef1;
    int numberRef2;

    QString typeToString();
    QString valueToString();
    QString numberToString();
};

QDataStream & operator<< (QDataStream& out, const SemanticConst *constant);

class SemanticMethod
{
public:
    QString id;
    int constName;
    int constDesc;
    bool abstract;
    QHash<QString, SemanticVar*> locals;
    int paramCount;
    AttrMethodDef* methodDef;

    void addLocalVar(QString & name, SemanticClass* currentClass);

    void generate(QDataStream & out);
};

class SemanticVar
{
public:
    int constName;
    int constType;
    int number;
};

QDataStream & operator<< (QDataStream& out, const SemanticVar *var);

#endif // TABLES_H
