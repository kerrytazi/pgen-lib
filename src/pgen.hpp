#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <unordered_map>


namespace pgen
{


struct RuleItemGroup;
struct RuleItem;
struct Rule;

enum class RuleItemType
{
	Literal,
	Identifier,
	Group,
	Or,
	ZeroOrMore,
	OneOrMore,
	ZeroOrOne,
};

struct RuleItemGroup
{
	std::string name;
	std::vector<RuleItem> seq;
};

struct RuleItem
{
	RuleItemType type = RuleItemType::Literal;

	std::string literal;
	std::string identifier;
	RuleItemGroup group;

	bool optional = false;
	bool multiple = false;
};

struct Rule
{
	std::string name;
	std::vector<RuleItem> seq;
};

std::vector<Rule> parse(const char *str, size_t size);


namespace helpers
{


std::string dump(const std::vector<RuleItem> &seq);
std::string dump(const RuleItemGroup &group);
std::string dump(const RuleItem &ruleitem);
std::string dump(const Rule &rule);
std::string dump(const std::vector<Rule> &rules);

struct GenerateCodeParams
{
	std::string custom_namespace;
};

std::string generate_code(const std::vector<Rule> &rules, const GenerateCodeParams &params);


} // namespace helpers


} // namespace pgen

