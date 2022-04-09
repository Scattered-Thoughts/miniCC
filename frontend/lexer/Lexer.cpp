#include "Lexer.hpp"
#include <cassert>
#include <cctype>

std::unordered_map<std::string, Token::TokenKind> Lexer::Keywords =
    std::unordered_map<std::string, Token::TokenKind>{
        {"int", Token::Int},     {"double", Token::Double},
        {"void", Token::Void},   {"if", Token::If},
        {"else", Token::Else},   {"for", Token::For},
        {"while", Token::While}, {"return", Token::Return}};

Lexer::Lexer(std::vector<std::string> &s) {
  Source = std::move(s);
  TokenBuffer = std::vector<Token>();
  LineIndex = 0;
  ColumnIndex = 0;

  LookAhead(1);
}

void Lexer::ConsumeCurrentToken() {
  assert(TokenBuffer.size() > 0 && "TokenBuffer is empty.");
  TokenBuffer.erase(TokenBuffer.begin());
}

int Lexer::GetNextChar() {
  // If its an empty line then move forward by calling EatNextChar() which will
  // advance to the next line's first character.
  if (LineIndex < Source.size() && Source[LineIndex].length() == 0)
    EatNextChar();
  if (LineIndex >= Source.size() || (LineIndex == Source.size() - 1 &&
                                     ColumnIndex == Source[LineIndex].length()))
    return EOF;
  return Source[LineIndex][ColumnIndex];
}

int Lexer::GetNextNthCharOnSameLine(unsigned n) {
  if (LineIndex >= Source.size() ||
      (ColumnIndex + n >= Source[LineIndex].length()))
    return EOF;
  return Source[LineIndex][ColumnIndex + n];
}

void Lexer::EatNextChar() {
  if (LineIndex < Source.size()) {
    if (Source[LineIndex].size() == 0 ||
        ColumnIndex >= Source[LineIndex].size() - 1) {
      ColumnIndex = 0;
      LineIndex++;
    } else {
      ColumnIndex++;
    }
  }
}

std::optional<Token> Lexer::LexNumber() {
  unsigned StartLineIndex = LineIndex;
  unsigned StartColumnIndex = ColumnIndex;
  unsigned Length = 0;
  auto TokenKind = Token::Integer;

  while (isdigit(GetNextChar())) {
    Length++;
    EatNextChar();
  }

  // if its a real value like 3.14
  if (GetNextChar() == '.') {
    Length++;
    EatNextChar();
    TokenKind = Token::Real;

    if (!isdigit(GetNextChar()))
      return std::nullopt; // TODO it might be better to make Invalid token

    while (isdigit(GetNextChar())) {
      Length++;
      EatNextChar();
    }
  }

  if (Length == 0)
    return std::nullopt;

  std::string_view StringValue{&Source[StartLineIndex][StartColumnIndex],
                               Length};
  return Token(TokenKind, StringValue, StartLineIndex, StartColumnIndex);
}

std::optional<Token> Lexer::LexIdentifier() {
  unsigned StartLineIndex = LineIndex;
  unsigned StartColumnIndex = ColumnIndex;
  unsigned Length = 0;

  while (isalpha(GetNextChar()) || GetNextChar() == '_') {
    Length++;
    EatNextChar();
  }

  if (Length == 0)
    return std::nullopt;

  std::string_view StringValue{&Source[StartLineIndex][StartColumnIndex],
                               Length};
  return Token(Token::Identifier, StringValue, StartLineIndex,
               StartColumnIndex);
}

std::optional<Token> Lexer::LexKeyword() {
  std::size_t WordEnd =
      Source[LineIndex].substr(ColumnIndex).find_first_of("\t\n\v\f\r ");

  auto Word = Source[LineIndex].substr(ColumnIndex, WordEnd);

  if (!Keywords.count(Word))
    return std::nullopt;

  unsigned StartLineIndex = LineIndex;
  unsigned StartColumnIndex = ColumnIndex;

  for (int i = Word.length(); i > 0; i--)
    EatNextChar();

  std::string_view StringValue{&Source[StartLineIndex][StartColumnIndex],
                               Word.length()};
  return Token(Lexer::Keywords[Word], StringValue, StartLineIndex,
               StartColumnIndex);
}

std::optional<Token> Lexer::LexSymbol() {
  auto TokenKind = Token::Invalid;
  unsigned Size = 1;

  switch (GetNextChar()) {
  case ',':
    TokenKind = Token::Comma;
    break;
  case '+':
    TokenKind = Token::Plus;
    break;
  case '-':
    TokenKind = Token::Minus;
    break;
  case '*':
    TokenKind = Token::Astrix;
    break;
  case '/':
    TokenKind = Token::ForwardSlash;
    break;
  case '%':
    TokenKind = Token::Percent;
    break;
  case '=':
    if (GetNextNthCharOnSameLine(1) == '=') {
      TokenKind = Token::DoubleEqual;
      Size = 2;
    } else
      TokenKind = Token::Equal;
    break;
  case '<':
    TokenKind = Token::LessThan;
    break;
  case '>':
    TokenKind = Token::GreaterThan;
    break;
  case '!':
    if (GetNextNthCharOnSameLine(1) == '=') {
      TokenKind = Token::BangEqual;
      Size = 2;
    } else
      TokenKind = Token::Bang;
    break;
  case '&':
    if (GetNextNthCharOnSameLine(1) == '&') {
      TokenKind = Token::DoubleAnd;
      Size = 2;
    } else
      TokenKind = Token::And;
    break;
  case ';':
    TokenKind = Token::SemiColon;
    break;
  case '(':
    TokenKind = Token::LeftParen;
    break;
  case ')':
    TokenKind = Token::RightParen;
    break;
  case '[':
    TokenKind = Token::LeftBracet;
    break;
  case ']':
    TokenKind = Token::RightBracet;
    break;
  case '{':
    TokenKind = Token::LeftCurly;
    break;
  case '}':
    TokenKind = Token::RightCurly;
    break;
  default:
    return std::nullopt;
    break;
  }

  std::string_view StringValue{&Source[LineIndex][ColumnIndex], Size};
  auto Result = Token(TokenKind, StringValue, LineIndex, ColumnIndex);

  EatNextChar();
  if (Size == 2)
    EatNextChar();

  return Result;
}

Token Lexer::LookAhead(unsigned n) {
  // fill in the TokenBuffer to have at least n element
  for (int i = TokenBuffer.size(); i < n; i++)
    TokenBuffer.push_back(Lex());

  return TokenBuffer[n - 1];
}

bool Lexer::Is(Token::TokenKind tk) {
  // fill in the buffer with one token if it is empty
  if (TokenBuffer.size() == 0)
    LookAhead(1);

  return GetCurrentToken().GetKind() == tk;
}

bool Lexer::IsNot(Token::TokenKind tk) { return !Is(tk); }

Token Lexer::Lex() {
  // if the TokenBuffer not empty then return the Token from there
  // and remove it from the stack
  if (TokenBuffer.size() > 0) {
    auto CurrentToken = GetCurrentToken();
    ConsumeCurrentToken();
    return CurrentToken;
  }

  int CurrentCharacter = GetNextChar();
  std::string WhiteSpaceChars("\t\n\v\f\r ");

  // consume white space characters
  while (WhiteSpaceChars.find(CurrentCharacter) != std::string::npos ||
         CurrentCharacter == '\0') {
    EatNextChar();
    CurrentCharacter = GetNextChar();
  }

  if (CurrentCharacter == EOF) {
    return Token(Token::EndOfFile);
  }

  auto Result = LexKeyword();

  if (!Result)
    Result = LexNumber();
  if (!Result)
    Result = LexIdentifier();
  if (!Result)
    Result = LexSymbol();

  if (Result)
    return Result.value();

  return Token(Token::Invalid);
}
