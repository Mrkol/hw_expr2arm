#include "jit.hpp"


bool Tokenizer::isLetter(char c)
{
	return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

bool Tokenizer::isDigit(char c)
{
	return '0' <= c && c <= '9';
}

bool Tokenizer::isSymbol(char c)
{
	return c == '(' || c == ')' || c == '-' || c == '+' || c == '*' || c == ',';
}

bool Tokenizer::isWhitespace(char c)
{
	return c == ' ' || c == '\n' || c == '\t';
}

auto Tokenizer::transitionMap(State current, char next)
	-> State
{
	switch (current)
	{
		case State::Start:
			if (isLetter(next) || next == '_') return State::Word;
			if (isDigit(next)) return State::Number;
			if (isSymbol(next)) return State::Symbol;
			if (isWhitespace(next)) return State::Whitespace;
			return State::Error;

		case State::Word:
			if (isLetter(next) || isDigit(next) || next == '_') return State::Word;
			return State::Start;

		case State::Symbol:
			//potential place to implement digraph operators
			return State::Start;

		case State::Number:
			if (isDigit(next)) return State::Number;
			return State::Start;

		case State::Whitespace:
			if (isWhitespace(next)) return State::Whitespace;
			return State::Start;

		default:
			return State::Error;
	}
}

Tokenizer::Tokenizer(std::istream& stream)
	: iterator_(stream),
	currentState_(State::Start),
	nextToken_(""),
	currentToken_(""),
	finished_(false)
{

}

bool Tokenizer::CurrentIsIdentifier()
{
	return currentToken_.size() > 0 &&
		(isLetter(currentToken_[0]) || currentToken_[0] == '_');
}

bool Tokenizer::CurrentIsNumber()
{
	return currentToken_.size() > 0 && isDigit(currentToken_[0]);
}

Tokenizer& Tokenizer::Advance()
{
	if (finished_)
		return *this;

	currentToken_ = "";

	if (iterator_ == std::istreambuf_iterator<char>())
	{
		currentToken_ = std::move(nextToken_);
		finished_ = true;
		return *this;
	}

	while (currentToken_.size() == 0)
	{
		char c = *iterator_;
		++iterator_;

		State newState = transitionMap(currentState_, c);

		if (newState == State::Start)
		{
			currentToken_ = std::move(nextToken_);
			newState = transitionMap(newState, c);
		}

		nextToken_ += c;
		currentState_ = newState;
	}

	return *this;
}

Tokenizer& Tokenizer::AdvanceSkipSpace()
{
	if (finished_)
		return *this;

	Advance();
	while (isWhitespace(currentToken_[0]))
		Advance();
	return *this;
}

const std::string& Tokenizer::operator*()
{
	return currentToken_;
}

std::unique_ptr<AST> Parser::parseProduct()
{
	std::unique_ptr<AST> result = parseUnaryMinus();
	while (**tokenizer_ == "*")
	{
		tokenizer_->AdvanceSkipSpace();

		std::unique_ptr<ASTBinaryOperator> tmp
			= std::make_unique<ASTBinaryOperator>();
		tmp->left = std::move(result);
		tmp->right = parseUnaryMinus();
		tmp->operatorName = "*";

		result = std::move(tmp);
	}

	return result;
}

std::unique_ptr<AST> Parser::parseSum()
{
	std::unique_ptr<AST> result = parseProduct();
	while (**tokenizer_ == "-" || **tokenizer_ == "+")
	{
		std::string op = **tokenizer_;
		tokenizer_->AdvanceSkipSpace();

		std::unique_ptr<ASTBinaryOperator> tmp
			= std::make_unique<ASTBinaryOperator>();
		tmp->left = std::move(result);
		tmp->right = parseProduct();
		tmp->operatorName = op;

		result = std::move(tmp);
	}
	return result;
}

std::unique_ptr<AST> Parser::parseUnaryMinus()
{
	if (**tokenizer_ == "-")
	{
		std::unique_ptr<ASTUnaryOperator> result =
			std::make_unique<ASTUnaryOperator>();
		result->operatorName = **tokenizer_;
		tokenizer_->AdvanceSkipSpace();
		result->argument = parseUnaryMinus();

		return result;
	}
	else
	{
		return parseSimpleExpression();
	}
}

std::unique_ptr<AST> Parser::parseSimpleExpression()
{
	if (**tokenizer_ == "(")
	{
		tokenizer_->AdvanceSkipSpace();
		std::unique_ptr<AST> result = parseSum();
		if (**tokenizer_ != ")")
			throw 0;
		tokenizer_->AdvanceSkipSpace();
		return result;
	}
	else if(tokenizer_->CurrentIsIdentifier())
	{
		std::string identifier = **tokenizer_;
		tokenizer_->AdvanceSkipSpace();

		std::unique_ptr<ASTFunction> result =
			std::make_unique<ASTFunction>();
		result->symbolName = identifier;
		
		if (**tokenizer_ == "(")
		{
			do
			{
				tokenizer_->AdvanceSkipSpace();
				result->arguments.push_back(parseSum());
			}
			while (**tokenizer_ == ",");

			if (**tokenizer_ != ")")
				throw 0;

			tokenizer_->AdvanceSkipSpace();
		}

		return result;
	}
	else if (tokenizer_->CurrentIsNumber())
	{
		std::unique_ptr<ASTLiteral> result =
			std::make_unique<ASTLiteral>();

		result->literal = **tokenizer_;
		tokenizer_->AdvanceSkipSpace();

		return result;
	}


	throw 0;
}


Parser::Parser(Tokenizer& tokenizer)
	: tokenizer_(&tokenizer)
{

}

std::unique_ptr<AST> Parser::Parse()
{
	tokenizer_->AdvanceSkipSpace();
	return parseSum();
}





Compiler::Compiler(AST& tree)
	: treeDependency_(&tree)
{

}

void Compiler::writeWord(uint32_t word)
{
	streamDependency_->write((char*) &word, sizeof(uint32_t));
}

void Compiler::pop(uint8_t reg)
{
	writeWord(POP_MASK | ((reg & 0xf) << 12));
}

void Compiler::push(uint8_t reg)
{
	writeWord(PUSH_MASK | ((reg & 0xf) << 12));
}

void Compiler::sum(uint8_t first, uint8_t second)
{
	writeWord(ADD_MASK | ((first & 0xf) << 16) | ((first & 0xf) << 12) | (second & 0xf));
}

void Compiler::sub(uint8_t first, uint8_t second)
{
	writeWord(SUB_MASK | ((first & 0xf) << 16) | ((first & 0xf) << 12) | (second & 0xf));
}

void Compiler::mul(uint8_t first, uint8_t second)
{
	writeWord(MUL_MASK | ((first & 0xf) << 16) | ((first & 0xf) << 8) | (second & 0xf));
}


void Compiler::blx(uint8_t reg)
{
	writeWord(BLX_MASK | (reg & 0xf));
}

void Compiler::constant(uint32_t constant, uint8_t reg)
{
	writeWord(LDR_MASK | (0xf << 16) | ((reg & 0xf) << 12));
	writeWord(ADD_MASK | (0xf << 16) | (0xf << 12) | (0x1 << 25));
	writeWord(constant);
}

void Compiler::loadConstant(uint32_t adress, uint8_t reg)
{
	constant(adress, reg);
	writeWord(LDR_MASK | ((reg & 0xf) << 16) | ((reg & 0xf) << 12));
}


void Compiler::compileTree(AST* current)
{
	if (ASTBinaryOperator* casted = dynamic_cast<ASTBinaryOperator*>(current))
	{
		compileTree(casted->left.get());
		compileTree(casted->right.get());
		if (casted->operatorName == "+")
		{
			pop(1);
			pop(0);
			sum(0, 1);
			push(0);
		}
		else if (casted->operatorName == "-")
		{
			pop(1);
			pop(0);
			sub(0, 1);
			push(0);
		}
		else if (casted->operatorName == "*")
		{
			pop(1);
			pop(0);
			mul(0, 1);
			push(0);
		}
		else
		{
			throw 0;
		}
	}
	else if (ASTFunction* casted = dynamic_cast<ASTFunction*>(current))
	{
		auto it = symtableDependency_->find(casted->symbolName);
		
		if (it == symtableDependency_->end())
			throw 0;

		if (casted->arguments.size() == 0)
		{
			loadConstant(it->second, 0);
			push(0);
		}
		else
		{
			for (size_t i = 0; i < casted->arguments.size(); ++i)
			{
				compileTree(casted->arguments[i].get());
			}

			//will break if there are a lot of arguments
			for (size_t i = 0; i < casted->arguments.size(); ++i)
			{
				pop(casted->arguments.size() - 1 - i);
			}

			constant(it->second, 7);
			blx(7);
			push(0);
		}
	}
	else if (ASTUnaryOperator* casted = dynamic_cast<ASTUnaryOperator*>(current))
	{
		compileTree(casted->argument.get());

		if (casted->operatorName == "-")
		{

			pop(1);
			constant(0, 0);
			sub(0, 1);
			push(0);
		}
		else
		{
			throw 0;
		}
	}
	else if (ASTLiteral* casted = dynamic_cast<ASTLiteral*>(current))
	{
		constant(stoul(casted->literal), 0);
		push(0);
	}
	else
	{
		throw 0;
	}
}

void Compiler::Compile(std::ostream& stream, std::map<std::string, uint32_t>& symtable)
{
	streamDependency_ = &stream;
	symtableDependency_ = &symtable;
	// init code

	writeWord(0xe92d43f0); // push {r4-r9, lr}
	compileTree(treeDependency_);
	pop(0);
	writeWord(0xe8bd43f0); // pop {r4-r9, lr}
	writeWord(0xe12fff1e); // bx lr
}

extern "C" void jit_compile_expression_to_arm(
	const char* expression,
	const symbol_t* externs,
	void* out_buffer)
{
	std::stringstream in(expression);

	Tokenizer tokenizer(in);
	Parser parser(tokenizer);
	auto tree = parser.Parse();
	Compiler compiler(*tree);


	std::map<std::string, uint32_t> symtable;
	for (int i = 0; externs[i].name != 0 || externs[i].pointer != 0; ++i)
	{
		symtable[externs[i].name] = reinterpret_cast<uint32_t>(externs[i].pointer);
	}

	std::stringstream out;
	compiler.Compile(out, symtable);

	out.seekg(0, std::ios::end);
	int size = out.tellg();
	out.seekg(0, std::ios::beg);

	out.read(reinterpret_cast<char*>(out_buffer), size);
}

