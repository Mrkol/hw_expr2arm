#pragma once
#ifndef JIT_HPP
#define JIT_HPP

#include <memory>
#include <vector>
#include <sstream>
#include <map>

class AST
{
public:
	virtual ~AST() = default;
};

class ASTUnaryOperator : public AST
{
public:
	std::unique_ptr<AST> argument;

	std::string operatorName;

	~ASTUnaryOperator() override = default;
};


class ASTBinaryOperator : public AST
{
public:
	std::unique_ptr<AST> left;
	std::unique_ptr<AST> right;

	std::string operatorName;

	~ASTBinaryOperator() override = default;
};

class ASTFunction : public AST
{
public:
	std::vector<std::unique_ptr<AST>> arguments;
	std::string symbolName;

	~ASTFunction() override  = default;
};

class ASTLiteral : public AST
{
public:
	std::string literal;

	~ASTLiteral() override = default;
};

class Tokenizer
{
	std::istreambuf_iterator<char> iterator_;

	enum class State
	{
		Start,
		Word,
		Symbol,
		Number,
		Whitespace,
		Error
	};

	State currentState_;
	std::string nextToken_;
	std::string currentToken_;
	bool finished_;

	State transitionMap(State current, char next);
	bool isLetter(char c);
	bool isDigit(char c);
	bool isSymbol(char c);
	bool isWhitespace(char c);

public:
	Tokenizer(std::istream& stream);
	Tokenizer(std::istream&&) = delete;

	bool CurrentIsIdentifier();
	bool CurrentIsNumber();
	const std::string& operator*();
	Tokenizer& Advance();
	Tokenizer& AdvanceSkipSpace();
};


class Parser
{
	Tokenizer* tokenizer_;

	std::unique_ptr<AST> parseProduct();
	std::unique_ptr<AST> parseSum();
	std::unique_ptr<AST> parseUnaryMinus();
	std::unique_ptr<AST> parseSimpleExpression();


public:
	Parser(Tokenizer& tokenizer);

	std::unique_ptr<AST> Parse();
};

class Compiler
{
	AST* treeDependency_;
	std::ostream* streamDependency_;
	std::map<std::string, uint32_t>* symtableDependency_;

	void compileTree(AST* current);

	void writeWord(uint32_t word);

	void pop(uint8_t reg);
	void push(uint8_t reg);


	void sum(uint8_t first, uint8_t second);
	void sub(uint8_t first, uint8_t second);
	void mul(uint8_t first, uint8_t second);

	void blx(uint8_t adress);

	void constant(uint32_t constant, uint8_t reg);
	void loadConstant(uint32_t adress, uint8_t reg);

	static constexpr uint32_t ADD_MASK  = 0b1110'00'0'0100'0'0000'0000'000000000000;
	static constexpr uint32_t SUB_MASK  = 0b1110'00'0'0010'0'0000'0000'000000000000;
	static constexpr uint32_t MOV_MASK  = 0b1110'00'0'1101'0'0000'0000'000000000000;

	static constexpr uint32_t MUL_MASK  = 0b1110'000000'0'0'0000'0000'0000'1001'0000;

	static constexpr uint32_t LDR_MASK  = 0b1110'01'0'1'1'0'0'1'0000'0000'000000000000;
	static constexpr uint32_t STR_MASK  = 0b1110'01'0'0'0'0'0'0'0000'0000'000000000000;

	static constexpr uint32_t PUSH_MASK = 0b1110'01'0'1'0'0'1'0'1101'0000'000000000100;
	static constexpr uint32_t POP_MASK  = 0b1110'01'0'0'1'0'0'1'1101'0000'000000000100;

	static constexpr uint32_t BLX_MASK   = 0b1110'0001001011111111111100110000;


public:
	Compiler(AST& tree);

	void Compile(std::ostream& stream, std::map<std::string, uint32_t>& symtable);
};

extern "C"
{
	typedef struct
	{
		const char* name;
		void* pointer;
	} symbol_t;

	
	void jit_compile_expression_to_arm(
		const char* expression,
		const symbol_t* externs,
		void* out_buffer);
}

#endif // JIT_HPP
