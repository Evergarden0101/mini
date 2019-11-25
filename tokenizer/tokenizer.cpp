#include "tokenizer/tokenizer.h"

#include <cctype>
#include <sstream>

namespace miniplc0 {

    std::pair<std::optional<Token>, std::optional<CompilationError>> Tokenizer::NextToken() {
        if (!_initialized)
            readAll();
        if (_rdr.bad())
            return std::make_pair(std::optional<Token>(),
                                  std::make_optional<CompilationError>(0, 0, ErrorCode::ErrStreamError));
        if (isEOF())
            return std::make_pair(std::optional<Token>(),
                                  std::make_optional<CompilationError>(0, 0, ErrorCode::ErrEOF));
        auto p = nextToken();
        if (p.second.has_value())
            return std::make_pair(p.first, p.second);
        auto err = checkToken(p.first.value());
        if (err.has_value())
            return std::make_pair(p.first, err.value());
        return std::make_pair(p.first, std::optional<CompilationError>());
    }

    std::pair<std::vector<Token>, std::optional<CompilationError>> Tokenizer::AllTokens() {
        std::vector<Token> result;
        while (true) {
            auto p = NextToken();
            if (p.second.has_value()) {
                if (p.second.value().GetCode() == ErrorCode::ErrEOF)
                    return std::make_pair(result, std::optional<CompilationError>());
                else
                    return std::make_pair(std::vector<Token>(), p.second);
            }
            result.emplace_back(p.first.value());
        }
    }

    // ע�⣺����ķ���ֵ�� Token �� CompilationError ֻ�ܷ���һ��������ͬʱ���ء�
    std::pair<std::optional<Token>, std::optional<CompilationError>> Tokenizer::nextToken() {
        // ���ڴ洢�Ѿ���������ɵ�ǰtoken�ַ�
        std::stringstream ss;
        // ����token�Ľ������Ϊ�˺����ķ���ֵ
        std::pair<std::optional<Token>, std::optional<CompilationError>> result;
        // <�кţ��к�>����ʾ��ǰtoken�ĵ�һ���ַ���Դ�����е�λ��
        std::pair<int64_t, int64_t> pos;
        // ��¼��ǰ�Զ�����״̬������˺���ʱ�ǳ�ʼ״̬
        DFAState current_state = DFAState::INITIAL_STATE;
        // ����һ����ѭ����������������
        // ÿһ��ִ��while�ڵĴ��룬�����ܵ���״̬�ı��
        while (true) {
            // ��һ���ַ�����ע��auto�Ƶ��ó���������std::optional<char>
            // ������ʵ������д��
            // 1. ÿ��ѭ��ǰ��������һ�� char
            // 2. ֻ���ڿ��ܻ�ת�Ƶ�״̬����һ�� char
            // ��Ϊ����ʵ���� unread��Ϊ��ʡ������ѡ���һ��
            auto current_char = nextChar();
            // ��Ե�ǰ��״̬���в�ͬ�Ĳ���
            switch (current_state) {

                // ��ʼ״̬
                // ��� case ���Ǹ����˺����߼������Ǻ���� case �����հᡣ
                case INITIAL_STATE: {
                    // �Ѿ��������ļ�β
                    if (!current_char.has_value())
                        // ����һ���յ�token���ͱ������ErrEOF���������ļ�β
                        return std::make_pair(std::optional<Token>(),
                                              std::make_optional<CompilationError>(0, 0, ErrEOF));

                    // ��ȡ�������ַ���ֵ��ע��auto�Ƶ�����������char
                    auto ch = current_char.value();
                    // ����Ƿ�����˲��Ϸ����ַ�����ʼ��Ϊ��
                    auto invalid = false;

                    // ʹ�����Լ���װ���ж��ַ����͵ĺ����������� tokenizer/utils.hpp
                    // see https://en.cppreference.com/w/cpp/string/byte/isblank
                    if (miniplc0::isspace(ch)) // �������ַ��ǿհ��ַ����ո񡢻��С��Ʊ���ȣ�
                        current_state = DFAState::INITIAL_STATE; // ������ǰ״̬Ϊ��ʼ״̬���˴�ֱ��breakҲ�ǿ��Ե�
                    else if (!miniplc0::isprint(ch)) // control codes and backspace
                        invalid = true;
                    else if (miniplc0::isdigit(ch)) // �������ַ�������
                        current_state = DFAState::UNSIGNED_INTEGER_STATE; // �л����޷���������״̬
                    else if (miniplc0::isalpha(ch)) // �������ַ���Ӣ����ĸ
                        current_state = DFAState::IDENTIFIER_STATE; // �л�����ʶ����״̬
                    else {
                        switch (ch) {
                            case '=': // ����������ַ���`=`�����л������ںŵ�״̬
                                current_state = DFAState::EQUAL_SIGN_STATE;
                                break;
                            case '-':
                                // ����գ��л������ŵ�״̬
                                current_state = DFAState::MINUS_SIGN_STATE;
                                break;
                            case '+':
                                // ����գ��л����Ӻŵ�״̬
                                current_state = DFAState::PLUS_SIGN_STATE;
                                break;
                            case '*':
                                // ����գ��л�״̬
                                current_state = DFAState::MULTIPLICATION_SIGN_STATE;
                                break;
                            case '/':
                                // ����գ��л�״̬
                                current_state = DFAState::DIVISION_SIGN_STATE;
                                break;

                                ///// ����գ�
                                ///// ���������Ŀɽ����ַ�
                                ///// �л�����Ӧ��״̬
                            case '(':
                                current_state = DFAState::LEFTBRACKET_STATE;
                                break;
                            case ')':
                                current_state = DFAState::RIGHTBRACKET_STATE;
                                break;
                            case ';':
                                current_state = DFAState::SEMICOLON_STATE;
                                break;
                                // �����ܵ��ַ����µĲ��Ϸ���״̬
                            default:
                                invalid = true;
                                break;
                        }
                    }
                    // ����������ַ�������״̬��ת�ƣ�˵������һ��token�ĵ�һ���ַ�
                    if (current_state != DFAState::INITIAL_STATE)
                        pos = previousPos(); // ��¼���ַ��ĵ�λ��Ϊtoken�Ŀ�ʼλ��
                    // �����˲��Ϸ����ַ�
                    if (invalid) {
                        // ��������ַ�
                        unreadLast();
                        // ���ر�����󣺷Ƿ�������
                        return std::make_pair(std::optional<Token>(),
                                              std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
                    }
                    // ����������ַ�������״̬��ת�ƣ�˵������һ��token�ĵ�һ���ַ�
                    if (current_state != DFAState::INITIAL_STATE) // ignore white spaces
                        ss << ch; // �洢�������ַ�
                    break;
                }

                    // ��ǰ״̬���޷�������
                case UNSIGNED_INTEGER_STATE: {
                    // ����գ�
                    // �����ǰ�Ѿ��������ļ�β��������Ѿ��������ַ���Ϊ����
                    //     �����ɹ��򷵻��޷����������͵�token�����򷵻ر������
                    if (!current_char.has_value()) {
                        std::string s;
                        ss >> s;
                        int count=0;
                        count=s.find_first_not_of('0',0);
                        s.erase(0,count);
                        if(s.length()==0)
                            s="0";
                        if(s.length()>10||(s.length()==10&&s.compare("2147483647")>0)){
                            return std::make_pair(std::optional<Token>(),
                                                  std::make_optional<CompilationError>(pos, ErrorCode::ErrIntegerOverflow));
                        }else{
                            return std::make_pair(
                                    std::make_optional<Token>(TokenType::UNSIGNED_INTEGER, s, pos, currentPos()),
                                    std::optional<CompilationError>());
                        }
                    }
                        // ����������ַ������֣���洢�������ַ�
                    else if (isdigit(current_char.value())) {
                        ss << current_char.value();
                    }
                        // �������������ĸ����洢�������ַ������л�״̬����ʶ��
                    else if (isalpha(current_char.value())) {
                        ss << current_char.value();
                        current_state = DFAState::IDENTIFIER_STATE;
                    }
                        // ����������ַ������������֮һ������˶������ַ����������Ѿ��������ַ���Ϊ����
                        //     �����ɹ��򷵻��޷����������͵�token�����򷵻ر������
                    else {
                        unreadLast();
                        std::string s;
                        ss >> s;
                        int count=0;
                        count=s.find_first_not_of('0',0);
                        s.erase(0,count);
                        if(s.length()==0)
                            s="0";
                        if(s.length()>10||(s.length()==10&&s.compare("2147483647")>0)){
                            return std::make_pair(std::optional<Token>(),
                                                  std::make_optional<CompilationError>(pos, ErrorCode::ErrIntegerOverflow));
                        }else{
                            return std::make_pair(
                                    std::make_optional<Token>(TokenType::UNSIGNED_INTEGER, s, pos, currentPos()),
                                    std::optional<CompilationError>());
                        }
                    }
                    break;
                }
                case IDENTIFIER_STATE: {
                    // ����գ�
                    // �����ǰ�Ѿ��������ļ�β��������Ѿ��������ַ���
                    //     �����������ǹؼ��֣���ô���ض�Ӧ�ؼ��ֵ�token�����򷵻ر�ʶ����token
                    if (!current_char.has_value()) {
                        std::string s;
                        ss >> s;
                        //�ؼ���'begin' | 'end' | 'const' | 'var' | 'print'
                        if (s.compare("begin") == 0) {
                            return std::make_pair(
                                    std::make_optional<Token>(TokenType::BEGIN, s, pos, currentPos()),
                                    std::optional<CompilationError>());
                        } else if (s.compare("end") == 0) {
                            return std::make_pair(
                                    std::make_optional<Token>(TokenType::END, s, pos, currentPos()),
                                    std::optional<CompilationError>());
                        } else if (s.compare("const") == 0) {
                            return std::make_pair(
                                    std::make_optional<Token>(TokenType::CONST, s, pos, currentPos()),
                                    std::optional<CompilationError>());
                        } else if (s.compare("var") == 0) {
                            return std::make_pair(
                                    std::make_optional<Token>(TokenType::VAR, s, pos, currentPos()),
                                    std::optional<CompilationError>());
                        } else if (s.compare("print") == 0) {
                            return std::make_pair(
                                    std::make_optional<Token>(TokenType::PRINT, s, pos, currentPos()),
                                    std::optional<CompilationError>());
                        } else {
                            return std::make_pair(
                                    std::make_optional<Token>(TokenType::IDENTIFIER, s, pos, currentPos()),
                                    std::optional<CompilationError>());
                        }
                    }
                        // ������������ַ�����ĸ����洢�������ַ�
                    else if (isalnum(current_char.value())) {
                        ss << current_char.value();
                    }
                        // ����������ַ������������֮һ������˶������ַ����������Ѿ��������ַ���
                        //     �����������ǹؼ��֣���ô���ض�Ӧ�ؼ��ֵ�token�����򷵻ر�ʶ����token
                    else {
                        unreadLast();
                        std::string s;
                        ss >> s;
                        //�ؼ���'begin' | 'end' | 'const' | 'var' | 'print'
                        if (s.compare("begin") == 0) {
                            return std::make_pair(
                                    std::make_optional<Token>(TokenType::BEGIN, s, pos, currentPos()),
                                    std::optional<CompilationError>());
                        } else if (s.compare("end") == 0) {
                            return std::make_pair(
                                    std::make_optional<Token>(TokenType::END, s, pos, currentPos()),
                                    std::optional<CompilationError>());
                        } else if (s.compare("const") == 0) {
                            return std::make_pair(
                                    std::make_optional<Token>(TokenType::CONST, s, pos, currentPos()),
                                    std::optional<CompilationError>());
                        } else if (s.compare("var") == 0) {
                            return std::make_pair(
                                    std::make_optional<Token>(TokenType::VAR, s, pos, currentPos()),
                                    std::optional<CompilationError>());
                        } else if (s.compare("print") == 0) {
                            return std::make_pair(
                                    std::make_optional<Token>(TokenType::PRINT, s, pos, currentPos()),
                                    std::optional<CompilationError>());
                        } else {
                            return std::make_pair(
                                    std::make_optional<Token>(TokenType::IDENTIFIER, s, pos, currentPos()),
                                    std::optional<CompilationError>());
                        }
                    }
                    break;
                }

                    // �����ǰ״̬�ǼӺ�
                case PLUS_SIGN_STATE: {
                    // ��˼������ΪʲôҪ���ˣ��������ط��᲻����Ҫ
                    unreadLast(); // Yes, we unread last char even if it's an EOF.
                    return std::make_pair(std::make_optional<Token>(TokenType::PLUS_SIGN, '+', pos, currentPos()),
                                          std::optional<CompilationError>());
                }
                    // ��ǰ״̬Ϊ���ŵ�״̬
                case MINUS_SIGN_STATE: {
                    // ����գ����ˣ������ؼ���token
                    unreadLast(); // Yes, we unread last char even if it's an EOF.
                    return std::make_pair(std::make_optional<Token>(TokenType::MINUS_SIGN, '-', pos, currentPos()),
                                          std::optional<CompilationError>());
                }

                    // ����գ�
                    // ���������ĺϷ�״̬�����к��ʵĲ���
                    // ������н���������token�����ر������
                case LEFTBRACKET_STATE: {
                    unreadLast();
                    return std::make_pair(std::make_optional<Token>(TokenType::LEFT_BRACKET, '(', pos, currentPos()),
                                          std::optional<CompilationError>());
                }
                case RIGHTBRACKET_STATE: {
                    unreadLast();
                    return std::make_pair(std::make_optional<Token>(TokenType::RIGHT_BRACKET, ')', pos, currentPos()),
                                          std::optional<CompilationError>());
                }
                case SEMICOLON_STATE: {
                    unreadLast();
                    return std::make_pair(std::make_optional<Token>(TokenType::SEMICOLON, ';', pos, currentPos()),
                                          std::optional<CompilationError>());
                }
                case MULTIPLICATION_SIGN_STATE: {
                    unreadLast();
                    return std::make_pair(
                            std::make_optional<Token>(TokenType::MULTIPLICATION_SIGN, '*', pos, currentPos()),
                            std::optional<CompilationError>());
                }
                case DIVISION_SIGN_STATE: {
                    unreadLast();
                    return std::make_pair(std::make_optional<Token>(TokenType::DIVISION_SIGN, '/', pos, currentPos()),
                                          std::optional<CompilationError>());
                }
                case EQUAL_SIGN_STATE: {
                    unreadLast();
                    return std::make_pair(std::make_optional<Token>(TokenType::EQUAL_SIGN, '=', pos, currentPos()),
                                          std::optional<CompilationError>());
                }

                    // Ԥ��֮���״̬�����ִ�е������˵�������쳣
                default:
                    DieAndPrint("unhandled state.");
                    break;
            }
        }
        // Ԥ��֮���״̬�����ִ�е������˵�������쳣
        return std::make_pair(std::optional<Token>(), std::optional<CompilationError>());
    }

    std::optional<CompilationError> Tokenizer::checkToken(const Token &t) {
        switch (t.GetType()) {
            case IDENTIFIER: {
                auto val = t.GetValueString();
                if (miniplc0::isdigit(val[0]))
                    return std::make_optional<CompilationError>(t.GetStartPos().first, t.GetStartPos().second,
                                                                ErrorCode::ErrInvalidIdentifier);
                break;
            }
            default:
                break;
        }
        return {};
    }

    void Tokenizer::readAll() {
        if (_initialized)
            return;
        for (std::string tp; std::getline(_rdr, tp);)
            _lines_buffer.emplace_back(std::move(tp + "\n"));
        _initialized = true;
        _ptr = std::make_pair<int64_t, int64_t>(0, 0);
        return;
    }

    // Note: We allow this function to return a postion which is out of bound according to the design like std::vector::end().
    std::pair<uint64_t, uint64_t> Tokenizer::nextPos() {
        if (_ptr.first >= _lines_buffer.size())
            DieAndPrint("advance after EOF");
        if (_ptr.second == _lines_buffer[_ptr.first].size() - 1)
            return std::make_pair(_ptr.first + 1, 0);
        else
            return std::make_pair(_ptr.first, _ptr.second + 1);
    }

    std::pair<uint64_t, uint64_t> Tokenizer::currentPos() {
        return _ptr;
    }

    std::pair<uint64_t, uint64_t> Tokenizer::previousPos() {
        if (_ptr.first == 0 && _ptr.second == 0)
            DieAndPrint("previous position from beginning");
        if (_ptr.second == 0)
            return std::make_pair(_ptr.first - 1, _lines_buffer[_ptr.first - 1].size() - 1);
        else
            return std::make_pair(_ptr.first, _ptr.second - 1);
    }

    std::optional<char> Tokenizer::nextChar() {
        if (isEOF())
            return {}; // EOF
        auto result = _lines_buffer[_ptr.first][_ptr.second];
        _ptr = nextPos();
        return result;
    }

    bool Tokenizer::isEOF() {
        return _ptr.first >= _lines_buffer.size();
    }

    // Note: Is it evil to unread a buffer?
    void Tokenizer::unreadLast() {
        _ptr = previousPos();
    }
}