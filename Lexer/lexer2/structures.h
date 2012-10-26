#ifndef STRUCTURES_H
#define STRUCTURES_H

// ��� ���������
enum ExprNodeType
{
	eInt,
	eBool,
	eString,
	eId,
	eAssign,
	ePlus,
	eMinus,
	eMul,
	eDiv,
	eLess,
	eMore,
	eOr,
	eAnd,
	eEqu,
	eNEqu,
	eNot,
	eUMinus,
	eBrackets,
	eQBrackets,
	eQBracketsInit,
	eFieldAcc,
	eSuper,
	eSelf,
	eNil,
};

// ��� ���������
enum StmtNodeType
{
	eExpr,
	eIf,
	eWhile,
	eUnless,
	eUntil,
	eClassDef,
	eMethodDef,
	eReturn
};

// ���������
struct ExprNode
{
	enum ExprNodeType type; // ���
	
	int value; // �������� �������� (��� ����������)
	char* id; // ���
	char* str; // ������ (��� ��������� ���������)
	
	struct ExprNode* left; // ����� �������
	struct ExprNode* right; // ������ �������
	struct ExprSeqNode* list; // ������ ��������� (��� ������ �������)
	
	struct ExprNode* next; // ��������� � ������
};

// ������������������ ���������
struct ExprSeqNode
{
	struct ExprNode* first; // ������ �������
	struct ExprNode* last; // ��������� �������
};

// ��������
struct StmtNode
{
	enum StmtNodeType type; // ���
	
	struct ExprNode* expr; // ���������
	struct StmtSeqNode* block; // ���� ����������
	
	char* id; // ������������� (��� ���������� ������� � �������)
	char* secondId; // ������ ������������� (��� ������������)
	struct MethodDefParamSeqNode* params; // ��������� ����������� �������
	
	struct StmtNode* next; // ��������� � ������
};

// ������������������ ����������
struct StmtSeqNode
{
	struct StmtNode* first; // ������
	struct StmtNode* last; // ���������
};

// ������������������ ���������� � ���������� �������
struct MethodDefParamSeqNode
{
	struct MethodDefParamNode* first; // ������
	struct MethodDefParamNode* last;// ���������
};

// �������� � ���������� �������
struct MethodDefParamNode
{
	char* id; // �������������
	struct MethodDefParamNode* next; // ��������� � ������
};

// ������ ������
struct ProgramNode
{
	struct StmtSeqNode* body; // ���� ���� ���������
};

#endif 
