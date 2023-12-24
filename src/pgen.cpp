#include "pch.hpp"
#include "pgen.hpp"


namespace pgen
{


bool is_whitespace(uint32_t c)
{
	return
		c == ' ' ||
		c == '\t' ||
		c == '\r' ||
		c == '\n';
}

bool is_identifier(uint32_t c)
{
	return
		(c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		(c >= '0' && c <= '9') ||
		c == '_';
}

uint8_t hex2num(char c)
{
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	if (c >= '0' && c <= '9')
		return c - '0';

	return 0;
}

bool is_eof(const char *s, const char *e)
{
	return s >= e;
}

void skip_whitespace(const char *&s, const char *e)
{
	while (!is_eof(s, e) && is_whitespace(*s))
		++s;
}

bool parse_newline(const char *&s, const char *e)
{
	const char *sc = s;

	if (is_eof(sc, e))
		return false;

	if (*sc == '\r')
	{
		++sc;

		if (is_eof(sc, e))
			return false;

		if (*sc != '\n')
			return false;
	}
	else
	if (*sc != '\n')
	{
		return false;
	}

	++sc;

	s = sc;
	return true;
}

bool parse_two_newlines(const char *&s, const char *e)
{
	const char *sc = s;

	if (parse_newline(sc, e) && parse_newline(sc, e))
	{
		s = sc;
		return true;
	}

	return false;
}

bool parse_literal(const char *&s, const char *e, const std::string_view &lit)
{
	if (is_eof(s, e))
		return false;

	size_t left = e - s;

	if (left < lit.size())
		return false;

	for (size_t i = 0; i < lit.size(); ++i)
	{
		if (s[i] != lit[i])
			return false;
	}

	s = s + lit.size();

	return true;
}

std::optional<std::string> parse_identifier(const char *&s, const char *e)
{
	std::string result;

	while (!is_eof(s, e))
	{
		if (!is_identifier(*s))
			break;

		result += *s;
		++s;
	}

	if (result.empty())
		return std::nullopt;

	return result;
}

std::optional<std::string> parse_string(const char *&s, const char *e)
{
	std::string result;

	const char *sc = s;

	if (!parse_literal(sc, e, "\""))
		return std::nullopt;

	bool escape = false;

	while (!is_eof(sc, e))
	{
		if (escape)
		{
			escape = false;

			if (*sc == 'x')
			{
				++sc;

				if (sc + 2 > e)
					return std::nullopt;

				char c0 = sc[0];
				char c1 = sc[1];

				sc += 2;

				result += (uint32_t)hex2num(c0) << 8 | (uint32_t)hex2num(c1);
				continue;
			}
			if (*sc == '"' || *sc == '\\')
			{
				result += *sc;
				++sc;
				continue;
			}
			else
			if (*sc == 'a')
				result += "\a";
			else
			if (*sc == 'b')
				result += "\b";
			else
			if (*sc == 't')
				result += "\t";
			else
			if (*sc == 'n')
				result += "\n";
			else
			if (*sc == 'v')
				result += "\v";
			else
			if (*sc == 'f')
				result += "\f";
			else
			if (*sc == 'r')
				result += "\r";
			else
				result += "\\";

			++sc;
			continue;
		}
		else
		if (*sc == '"')
		{
			++sc;
			s = sc;
			return result;
		}
		else
		if (*sc == '\\')
		{
			++sc;
			escape = true;
			continue;
		}

		result += *sc;
		++sc;
	}

	return std::nullopt;
}

std::string escape_string(const std::string_view &str)
{
	std::string result;

	for (char c : str)
	{
		if (c == '"')
			result += "\\\"";
		else
		if (c == '\\')
			result += "\\\\";
		else
		if (c == '\a')
			result += "\\a";
		else
		if (c == '\b')
			result += "\\b";
		else
		if (c == '\t')
			result += "\\t";
		else
		if (c == '\n')
			result += "\\n";
		else
		if (c == '\v')
			result += "\\v";
		else
		if (c == '\f')
			result += "\\f";
		else
		if (c == '\r')
			result += "\\r";
		else
			result += c;
	}

	return result;
}

std::optional<RuleItem> parse_ruleitem(const char *&s, const char *e, const std::string_view &parent_group_prefix, size_t &parent_group_id);

std::optional<RuleItemGroup> parse_group(const char *&s, const char *e, const std::string_view &parent_group_prefix, size_t &parent_group_id)
{
	RuleItemGroup result;

	if (!parse_literal(s, e, "("))
		return std::nullopt;

	skip_whitespace(s, e);

	result.name = std::string(parent_group_prefix) + "_$g" + std::to_string(parent_group_id);
	size_t group_id = 0;

	while (!is_eof(s, e))
	{
		if (parse_literal(s, e, ")"))
			break;

		auto r = parse_ruleitem(s, e, result.name, group_id).value();

		if (r.type == RuleItemType::ZeroOrMore)
		{
			if (!result.seq.empty())
			{
				result.seq.back().optional = true;
				result.seq.back().multiple = true;
			}
		}
		else
		if (r.type == RuleItemType::OneOrMore)
		{
			if (!result.seq.empty())
			{
				result.seq.back().multiple = true;
			}
		}
		else
		if (r.type == RuleItemType::ZeroOrOne)
		{
			if (!result.seq.empty())
			{
				result.seq.back().optional = true;
			}
		}
		else
		if (r.type == RuleItemType::Negate)
		{
			if (!result.seq.empty())
			{
				if (result.seq.back().type != RuleItemType::Literal)
					throw 2;

				result.seq.back().negate = true;
			}
		}
		else
		{
			result.seq.push_back(r);
		}

		skip_whitespace(s, e);
	}

	++parent_group_id;

	return result;
}

std::optional<RuleItem> parse_ruleitem(const char *&s, const char *e, const std::string_view &parent_group_prefix, size_t &parent_group_id)
{
	RuleItem result;

	if (is_eof(s, e))
		return std::nullopt;

	if (auto v = parse_string(s, e))
	{
		result.type = RuleItemType::Literal;
		result.literal = v.value();
	}
	else
	if (auto v = parse_identifier(s, e))
	{
		result.type = RuleItemType::Identifier;
		result.identifier = v.value();
	}
	else
	if (auto v = parse_group(s, e, parent_group_prefix, parent_group_id))
	{
		result.type = RuleItemType::Group;
		result.group = v.value();
	}
	else
	if (parse_literal(s, e, "|"))
	{
		result.type = RuleItemType::Or;
	}
	else
	if (parse_literal(s, e, "*"))
	{
		result.type = RuleItemType::ZeroOrMore;
	}
	else
	if (parse_literal(s, e, "+"))
	{
		result.type = RuleItemType::OneOrMore;
	}
	else
	if (parse_literal(s, e, "?"))
	{
		result.type = RuleItemType::ZeroOrOne;
	}
	else
	if (parse_literal(s, e, "^"))
	{
		result.type = RuleItemType::Negate;
	}
	else
	{
		return std::nullopt;
	}

	return result;
}

std::optional<Rule> parse_rule(const char *&s, const char *e)
{
	Rule result;

	result.name = parse_identifier(s, e).value();

	skip_whitespace(s, e);

	if (!parse_literal(s, e, ":"))
		throw 2;

	skip_whitespace(s, e);

	size_t group_id = 0;

	while (!is_eof(s, e))
	{
		auto r = parse_ruleitem(s, e, result.name, group_id).value();

		if (r.type == RuleItemType::ZeroOrMore)
		{
			if (!result.seq.empty())
			{
				result.seq.back().optional = true;
				result.seq.back().multiple = true;
			}
		}
		else
		if (r.type == RuleItemType::OneOrMore)
		{
			if (!result.seq.empty())
			{
				result.seq.back().multiple = true;
			}
		}
		else
		if (r.type == RuleItemType::ZeroOrOne)
		{
			if (!result.seq.empty())
			{
				result.seq.back().optional = true;
			}
		}
		else
		if (r.type == RuleItemType::Negate)
		{
			if (!result.seq.empty())
			{
				if (result.seq.back().type != RuleItemType::Literal)
					throw 2;

				result.seq.back().negate = true;
			}
		}
		else
		{
			result.seq.push_back(r);
		}

		if (parse_two_newlines(s, e))
			break;

		skip_whitespace(s, e);
	}

	if (result.seq.empty())
		throw 2;

	return result;
}

std::vector<Rule> parse(const char *str, size_t size)
{
	std::vector<Rule> result;

	const char *s = str;
	const char *e = str + size;

	skip_whitespace(s, e);

	while (!is_eof(s, e))
	{
		if (parse_literal(s, e, "#"))
		{
			while (!is_eof(s, e))
			{
				if (parse_newline(s, e))
					break;

				++s;
			}

			skip_whitespace(s, e);
			continue;
		}

		result.push_back(parse_rule(s, e).value());
		skip_whitespace(s, e);
	}

	return result;
}


namespace helpers
{


std::string dump(const RuleItem &ruleitem);

std::string dump(const std::vector<RuleItem> &seq)
{
	std::string result;

	for (const auto &v : seq)
	{
		result += dump(v);

		if (v.multiple)
		{
			if (v.optional)
				result += "*";
			else
				result += "+";
		}
		else
		if (v.optional)
		{
			result += "?";
		}

		result += " ";
	}

	if (result.back() == ' ')
		result.resize(result.size() - 1);

	return result;
}

std::string dump(const RuleItemGroup &group)
{
	std::string result;

	result += "(";

	result += dump(group.seq);
	
	result += ")";

	return result;
}

std::string dump(const RuleItem &ruleitem)
{
	switch (ruleitem.type)
	{
		case RuleItemType::Literal:
			return std::string() + "\"" + escape_string(ruleitem.literal) + "\"";
		case RuleItemType::Identifier:
			return ruleitem.identifier;
		case RuleItemType::Group:
			return dump(ruleitem.group);
		case RuleItemType::Or:
			return "|";
		default:
			return "<error>";
	}
}

std::string dump(const Rule &rule)
{
	std::string result;

	result += rule.name;
	result += ": ";

	result += dump(rule.seq);

	return result;
}

std::string dump(const std::vector<Rule> &rules)
{
	std::string result;

	for (const auto &v : rules)
	{
		result += dump(v);
		result += "\n\n";
	}

	return result;
}


void collect_groups(std::vector<const RuleItemGroup *> &groups, const std::vector<RuleItem> &seq)
{
	for (const auto &v : seq)
	{
		if (v.type == RuleItemType::Group)
		{
			groups.push_back(&v.group);
			collect_groups(groups, v.group.seq);
		}
	}
}

std::string generate_rule(const std::vector<RuleItem> &seq, const std::string &name, const std::string &ptype)
{
	std::string result;

	result += "// Rule: ";
	result += dump(seq);
	result += "\n";

	result += "[[nodiscard]]\n";
	result += "std::optional<$Parsed> $parse_" + name + "(const char *&s, const char *e)\n";
	result += "{\n";
	result += "	$Parsed result;\n";
	result += "	result.type = $ParsedType::" + ptype + ";\n";
	result += "	result.identifier = $IdentifierType::$i_" + name + ";\n";
	result += "\n";

	// each 'or'
	for (size_t i = 0; i < seq.size(); ++i)
	{
		result += "	{\n";
		result += "		const char *sc = s;\n";
		result += "		result.group.clear();\n";

		size_t level = 0;

		for (; i < seq.size(); ++i)
		{
			if (seq[i].type == pgen::RuleItemType::Or)
				break;

			//  optional &&  multiple - . if while
			//  optional && !multiple - . if
			// !optional &&  multiple - if . while
			// !optional && !multiple - if .

			result += "\n";

			if (seq[i].type == RuleItemType::Literal)
			{
				if (seq[i].negate)
					result += std::string(level, '\t') + "		if (auto v = $parse_negate_literal(sc, e, \"" + escape_string(seq[i].literal) + "\"))\n";
				else
					result += std::string(level, '\t') + "		if (auto v = $parse_literal(sc, e, \"" + escape_string(seq[i].literal) + "\"))\n";
			}
			else
			if (seq[i].type == RuleItemType::Group)
				result += std::string(level, '\t') + "		if (auto v = $parse_" + seq[i].group.name + "(sc, e))\n";
			else
				result += std::string(level, '\t') + "		if (auto v = $parse_" + seq[i].identifier + "(sc, e))\n";

			result += std::string(level, '\t') + "		{\n";
			result += std::string(level, '\t') + "			result.group.push_back(std::move(v).value());\n";

			if (seq[i].multiple)
			{
				result += "\n";

				if (seq[i].type == RuleItemType::Literal)
				{
					if (seq[i].negate)
						result += std::string(level, '\t') + "			while (auto v = $parse_negate_literal(sc, e, \"" + escape_string(seq[i].literal) + "\"))\n";
					else
						result += std::string(level, '\t') + "			while (auto v = $parse_literal(sc, e, \"" + escape_string(seq[i].literal) + "\"))\n";
				}
				else
				if (seq[i].type == RuleItemType::Group)
					result += std::string(level, '\t') + "			while (auto v = $parse_" + seq[i].group.name + "(sc, e))\n";
				else
					result += std::string(level, '\t') + "			while (auto v = $parse_" + seq[i].identifier + "(sc, e))\n";

				result += std::string(level, '\t') + "			{\n";
				result += std::string(level, '\t') + "				result.group.push_back(std::move(v).value());\n";
				result += std::string(level, '\t') + "			}\n";
				result += "\n";
			}

			if (seq[i].optional)
				result += std::string(level, '\t') + "		}\n";
			else
				++level;
		}

		result += "\n";

		result += std::string(level, '\t') + "		s = sc;\n";
		result += std::string(level, '\t') + "		return result;\n";

		for (; level != 0; --level)
			result += std::string(level - 1, '\t') + "		}\n";

		result += "	}\n";
		result += "\n";
	}

	result += "	return std::nullopt;\n";
	result += "}\n";

	return result;
}

std::string generate_code(const std::vector<Rule> &rules, const GenerateCodeParams &params)
{
	std::string result;

	std::vector<const RuleItemGroup *> groups;
	
	for (const auto &rule : rules)
		collect_groups(groups, rule.seq);

	result += R"AAA(// This file is generated

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <unordered_map>
#include <cassert>

)AAA";

	if (!params.custom_namespace.empty())
		result += "namespace " + params.custom_namespace + "\n{\n\n";

	result += "enum class $IdentifierType\n";
	result += "{\n";
	result += "	None,\n";

	for (const auto &rule : rules)
	{
		result += "	$i_" + rule.name + ",\n";
	}

	result += "\n";

	for (const auto &group : groups)
	{
		result += "	$i_" + group->name + ",\n";
	}

	result += "};\n";
	result += "\n";

	result += "const std::string table_$IdentifierType[]\n";
	result += "{\n";
	result += "	\"\",\n";

	for (const auto &rule : rules)
	{
		result += "	\"" + rule.name + "\",\n";
	}

	result += "\n";

	for (const auto &group : groups)
	{
		result += "	\"" + group->name + "\",\n";
	}

	result += "};\n";
	result += "\n";

	result += R"AAA(
enum class $ParsedType
{
	Literal,
	Identifier,
	Group,
};

struct $ParsedCustomData
{
	virtual ~$ParsedCustomData() {}
};

struct $Parsed
{
	$ParsedType type;
	$IdentifierType identifier = $IdentifierType::None;
	std::string literal;
	std::vector<$Parsed> group;
	mutable std::unique_ptr<$ParsedCustomData> custom_data;

	constexpr const $Parsed *find($IdentifierType id) const
	{
		for (const auto &v : group)
			if (v.identifier == id)
				return &v;

		return nullptr;
	}

	constexpr size_t size() const
	{
		return group.size();
	}

	constexpr const $Parsed &get(size_t index) const
	{
		assert(index < group.size());
		return group[index];
	}

	constexpr const $Parsed &get(size_t index, [[maybe_unused]] $IdentifierType _debug_id) const
	{
		assert(index < group.size() && group[index].identifier == _debug_id);
		return group[index];
	}

	constexpr std::string flatten() const
	{
		switch (type)
		{
			case $ParsedType::Literal:
				return literal;
			case $ParsedType::Identifier:
			case $ParsedType::Group:
				{
					std::string result;

					for (const auto &v : group)
						result += v.flatten();

					return result;
				}
		}

		__assume(0);
	}
};

[[nodiscard]]
bool $is_eof(const char *s, const char *e)
{
	return s >= e;
}

[[nodiscard]]
std::optional<$Parsed> $parse_literal(const char *&s, const char *e, const std::string_view &lit)
{
	$Parsed result;
	result.type = $ParsedType::Literal;
	result.literal = lit;

	if ($is_eof(s, e))
		return std::nullopt;

	size_t left = e - s;

	if (left < lit.size())
		return std::nullopt;

	for (size_t i = 0; i < lit.size(); ++i)
	{
		if (s[i] != lit[i])
			return std::nullopt;
	}

	s = s + lit.size();

	return result;
}

[[nodiscard]]
std::optional<$Parsed> $parse_negate_literal(const char *&s, const char *e, const std::string_view &lit)
{
	$Parsed result;
	result.type = $ParsedType::Literal;

	if ($is_eof(s, e))
		return std::nullopt;

	size_t left = e - s;

	if (left >= lit.size())
	{
		bool eq = true;

		for (size_t i = 0; i < lit.size(); ++i)
		{
			if (s[i] != lit[i])
			{
				eq = false;
				break;
			}
		}

		if (eq)
			return std::nullopt;
	}

	result.literal = std::string(1, *s);
	++s;
	return result;
}

namespace helpers
{

std::string _generate_graphviz_literal(const $Parsed &p, const std::unordered_map<const $Parsed *, int> &idx)
{
	std::string result;

	switch (p.type)
	{
		case $ParsedType::Literal:
			{
				result += " a" + std::to_string(idx.at(&p));
				break;
			}
		case $ParsedType::Identifier:
			{
				for (const auto &v : p.group)
					result += _generate_graphviz_literal(v, idx);

				break;
			}
		case $ParsedType::Group:
			{
				for (const auto &v : p.group)
					result += _generate_graphviz_literal(v, idx);

				break;
			}
	}

	return result;
}

std::string _generate_graphviz_ids(const $Parsed &p, std::unordered_map<const $Parsed *, int> &idx, int &max_id)
{
	std::string result;

	int &id = idx[&p];
	if (id == 0)
		id = max_id++;

	switch (p.type)
	{
		case $ParsedType::Literal:
			{
				result += std::string("\ta") + std::to_string(id) + "[label=\"" + p.literal + "\" shape=ellipse];\n";
				break;
			}
		case $ParsedType::Identifier:
			{
				result += std::string("\ta") + std::to_string(id) + "[label=\"" + table_$IdentifierType[(int)p.identifier] + "\" shape=box];\n";

				for (const auto &v : p.group)
					result += _generate_graphviz_ids(v, idx, max_id);

				break;
			}
		case $ParsedType::Group:
			{
				result += std::string("\ta") + std::to_string(id) + "[label=\"" + table_$IdentifierType[(int)p.identifier] + "\" shape=hexagon];\n";

				for (const auto &v : p.group)
					result += _generate_graphviz_ids(v, idx, max_id);

				break;
			}
	}

	return result;
}

std::string _generate_graphviz(const $Parsed &p, const std::unordered_map<const $Parsed *, int> &idx)
{
	std::string result;

	int id = idx.at(&p);

	switch (p.type)
	{
		case $ParsedType::Identifier:
		case $ParsedType::Group:
			{
				for (const auto &v : p.group)
				{
					result += std::string() + "\ta" + std::to_string(id) + " -> " + "a" + std::to_string(idx.at(&v)) + "\n";
					result += _generate_graphviz(v, idx);
				}

				break;
			}
	}

	return result;
}

std::string generate_graphviz(const $Parsed &p)
{
	std::unordered_map<const $Parsed *, int> idx;
	int max_id = 1;

	std::string result;

	result += "digraph g {\n";

	result += _generate_graphviz_ids(p, idx, max_id);

	result += "\n";

	result += _generate_graphviz(p, idx);

	result += "\n";

	result += "\t{ rank=same;" + _generate_graphviz_literal(p, idx) + " }\n";

	result += "}\n";

	return result;
}

std::string generate_tree(const $Parsed &p, size_t align = 0)
{
	std::string result;

	if (p.type == $ParsedType::Literal)
		result += std::string(align, ' ') + "'" + p.literal + "'\n";
	else
		result += std::string(align, ' ') + table_$IdentifierType[(int)p.identifier] + "\n";

	if (p.type == $ParsedType::Identifier || p.type == $ParsedType::Group)
	{
		for (const auto &v : p.group)
			result += generate_tree(v, align + 1);
	}

	return result;
}

std::string ansii_colored(const $Parsed &v, const std::unordered_map<std::string, std::string> &colors, const std::string &prev_color)
{
	std::string result;

	std::string colored;

	if (auto it = colors.find(table_$IdentifierType[(int)v.identifier]); it != colors.end())
	{
		result += it->second;
		colored = it->second;
	}

	switch (v.type)
	{
		case $ParsedType::Literal:
			result += v.literal;
			break;

		case $ParsedType::Identifier:
		case $ParsedType::Group:
			for (const auto &g : v.group)
				result += ansii_colored(g, colors, colored.empty() ? prev_color : colored);

			break;
	}

	if (!colored.empty())
	{
		result += prev_color;
	}

	return result;
}

} // namespace helpers

)AAA";

	result += "\n";

	for (const auto &rule : rules)
	{
		result += "[[nodiscard]] std::optional<$Parsed> $parse_" + rule.name + "(const char *&s, const char *e);\n";
	}

	result += "\n";

	for (const auto &group : groups)
	{
		result += "[[nodiscard]] std::optional<$Parsed> $parse_" + group->name + "(const char *&s, const char *e);\n";
	}

	result += "\n";

	for (const auto &rule : rules)
	{
		result += generate_rule(rule.seq, rule.name, "Identifier");
		result += "\n";
	}

	result += "\n";

	for (const auto &group : groups)
	{
		result += generate_rule(group->seq, group->name, "Group");
		result += "\n";
	}

	if (!params.custom_namespace.empty())
		result += "\n\n} // namespace " + params.custom_namespace + "\n";

	return result;
}


} // namespace helpers


} // namespace pgen


