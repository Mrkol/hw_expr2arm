#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <sstream>
#include "jit.hpp"

TEST_CASE("Tokenizer test 1", "[tokenizer]")
{
	std::stringstream dummy;
	dummy << "div(a + b, c)";
	Tokenizer tokenizer(dummy);
	REQUIRE(*tokenizer.Advance() == "div");
	REQUIRE(*tokenizer.Advance() == "(");
	REQUIRE(*tokenizer.Advance() == "a");
	REQUIRE(*tokenizer.Advance() == " ");
	REQUIRE(*tokenizer.Advance() == "+");
	REQUIRE(*tokenizer.AdvanceSkipSpace() == "b");
	REQUIRE(*tokenizer.Advance() == ",");
	REQUIRE(*tokenizer.Advance() == " ");
	REQUIRE(*tokenizer.Advance() == "c");
	REQUIRE(*tokenizer.Advance() == ")");
}

TEST_CASE("Tokenizer test 2", "[tokenizer]")
{
	std::stringstream dummy;
	dummy << "(1+a)*c + div(2+4,2)";
	Tokenizer tokenizer(dummy);
	REQUIRE(*tokenizer.Advance() == "(");
	REQUIRE(*tokenizer.Advance() == "1");
	REQUIRE(*tokenizer.Advance() == "+");
	REQUIRE(*tokenizer.Advance() == "a");
	REQUIRE(*tokenizer.Advance() == ")");
	REQUIRE(*tokenizer.Advance() == "*");
	REQUIRE(*tokenizer.Advance() == "c");
	REQUIRE(*tokenizer.Advance() == " ");
	REQUIRE(*tokenizer.Advance() == "+");
	REQUIRE(*tokenizer.Advance() == " ");
	REQUIRE(*tokenizer.Advance() == "div");
	REQUIRE(*tokenizer.Advance() == "(");
	REQUIRE(*tokenizer.Advance() == "2");
	REQUIRE(*tokenizer.Advance() == "+");
	REQUIRE(*tokenizer.Advance() == "4");
	REQUIRE(*tokenizer.Advance() == ",");
	REQUIRE(*tokenizer.Advance() == "2");
	REQUIRE(*tokenizer.Advance() == ")");
}

TEST_CASE("Parser test 1", "[parser]")
{
	std::stringstream dummy;
	dummy << "a*b";
	Tokenizer tokenizer(dummy);
	Parser parser(tokenizer);

	auto result = parser.Parse();
	ASTBinaryOperator* root = dynamic_cast<ASTBinaryOperator*>(result.get());
	REQUIRE(root);
	REQUIRE(root->operatorName == "*");
	ASTFunction* left = dynamic_cast<ASTFunction*>(root->left.get());
	ASTFunction* right = dynamic_cast<ASTFunction*>(root->right.get());
	REQUIRE(left);
	REQUIRE(left->symbolName == "a");
	REQUIRE(right);
	REQUIRE(right->symbolName == "b");
}

TEST_CASE("Parser test 2", "[parser]")
{
	std::stringstream dummy;
	dummy << "a+b-c";
	Tokenizer tokenizer(dummy);
	Parser parser(tokenizer);

	auto result = parser.Parse();
	ASTBinaryOperator* root = dynamic_cast<ASTBinaryOperator*>(result.get());
	REQUIRE(root);
	REQUIRE(root->operatorName == "-");
	
	ASTFunction* right = dynamic_cast<ASTFunction*>(root->right.get());
	REQUIRE(right);
	REQUIRE(right->symbolName == "c");

	ASTBinaryOperator* left = dynamic_cast<ASTBinaryOperator*>(root->left.get());
	REQUIRE(left);
	REQUIRE(left->operatorName == "+");

	ASTFunction* ll = dynamic_cast<ASTFunction*>(left->left.get());
	REQUIRE(ll);
	REQUIRE(ll->symbolName == "a");

	ASTFunction* lr = dynamic_cast<ASTFunction*>(left->right.get());
	REQUIRE(lr);
	REQUIRE(lr->symbolName == "b");
}

TEST_CASE("Parser test 3", "[parser]")
{
	std::stringstream dummy;
	dummy << "func(a, b + c, d)";
	Tokenizer tokenizer(dummy);
	Parser parser(tokenizer);

	auto result = parser.Parse();
	ASTFunction* root = dynamic_cast<ASTFunction*>(result.get());
	REQUIRE(root);
	REQUIRE(root->symbolName == "func");
	
	ASTFunction* a = dynamic_cast<ASTFunction*>(root->arguments[0].get());
	REQUIRE(a);
	REQUIRE(a->symbolName == "a");

	ASTBinaryOperator* bc = dynamic_cast<ASTBinaryOperator*>(root->arguments[1].get());
	REQUIRE(bc);
	REQUIRE(bc->operatorName == "+");

	ASTFunction* d = dynamic_cast<ASTFunction*>(root->arguments[2].get());
	REQUIRE(d);
	REQUIRE(d->symbolName == "d");
}

TEST_CASE("Parser test 4", "[parser]")
{
	std::stringstream dummy;
	dummy << "1337 - 42";
	Tokenizer tokenizer(dummy);
	Parser parser(tokenizer);

	auto result = parser.Parse();
	ASTBinaryOperator* root = dynamic_cast<ASTBinaryOperator*>(result.get());
	REQUIRE(root);
	REQUIRE(root->operatorName == "-");

	ASTLiteral* left = dynamic_cast<ASTLiteral*>(root->left.get());
	REQUIRE(left);
	REQUIRE(left->literal == "1337");

	ASTLiteral* right = dynamic_cast<ASTLiteral*>(root->right.get());
	REQUIRE(right);
	REQUIRE(right->literal == "42");
}