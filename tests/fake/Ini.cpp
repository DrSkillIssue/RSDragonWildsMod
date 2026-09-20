#include "UnrealAbi.hpp"
#include <charconv>
#include <stdexcept>
#include <string>

namespace RC::Ini
{
const std::wstring& Value::get_string_value() const { return m_ref->m_string_value; }
std::int64_t Value::get_int64_value() const { return m_ref->m_int64_value; }
float Value::get_float_value() const { return m_ref->m_float_value; }
bool Value::get_bool_value() const { return m_ref->m_bool_value; }
void Value::set_ref(const Value* reference) { m_ref = reference; }

void Value::add_string_value(std::wstring_view data)
{
    m_string_value = data;
    m_valid_types[1] = true;
}

void Value::add_int64_value(const std::wstring& data, int base)
{
    const std::string narrow(data.begin(), data.end());
    std::from_chars(narrow.data(), narrow.data() + narrow.size(), m_int64_value, base);
    m_valid_types[2] = true;
}

void Value::add_float_value(const std::wstring& data)
{
    const std::string narrow(data.begin(), data.end());
    std::from_chars(narrow.data(), narrow.data() + narrow.size(), m_float_value);
    m_valid_types[3] = true;
}

void Value::add_bool_value(bool data)
{
    m_bool_value = data;
    m_valid_types[4] = true;
}

void Parser::parse(std::wstring& input)
{
    Section* current = nullptr;
    std::size_t position = 0;
    while (position < input.size())
    {
        const auto end = input.find(L'\n', position);
        std::wstring_view line(input.data() + position, (end == std::wstring::npos ? input.size() : end) - position);
        position = end == std::wstring::npos ? input.size() : end + 1;
        if (!line.empty() && line.back() == L'\r') line.remove_suffix(1);
        const auto first = line.find_first_not_of(L' ');
        if (first == std::wstring_view::npos) continue;
        line.remove_prefix(first);
        if (line.front() == L';') continue;
        if (line.front() == L'[')
        {
            current = &m_sections[std::wstring(line.substr(1, line.find(L']') - 1))];
            continue;
        }
        const auto equals = line.find(L'=');
        if (equals == std::wstring_view::npos) throw std::runtime_error("Syntax error: Expected state CreateSectionKey, got NewLineStarted");
        if (!current) throw std::runtime_error("Syntax error: No section. Global variables not supported, please create a [Section]");
        std::wstring key(line.substr(0, equals));
        key.erase(key.find_last_not_of(L' ') + 1);
        std::wstring_view rest = line.substr(equals + 1);
        const auto start = rest.find_first_not_of(L' ');
        rest = start == std::wstring_view::npos ? std::wstring_view() : rest.substr(start);
        auto& value = current->key_value_pairs[key];
        value.set_ref(&value);
        value.add_string_value(rest);
        const std::wstring text(rest);
        bool digits = !text.empty();
        for (wchar_t c : text) digits = digits && c >= L'0' && c <= L'9';
        if (digits) value.add_int64_value(text, 10);
        bool number = !text.empty() && text[0] >= L'0' && text[0] <= L'9';
        for (wchar_t c : text) number = number && ((c >= L'0' && c <= L'9') || c == L'.');
        if (number) value.add_float_value(text);
        if (text == L"1" || text == L"true") value.add_bool_value(true);
        if (text == L"0" || text == L"false") value.add_bool_value(false);
    }
    m_parsing_is_complete = true;
}
}
