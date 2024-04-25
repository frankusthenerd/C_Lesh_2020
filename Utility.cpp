#include "C_Lesh.hpp"

namespace Codeloader {

  /**
   * Creates a new utility object.
   */
  cUtility::cUtility() {
    this->pi = std::acos(-1);
  }

  /**
   * Splits a file into lines.
   * @param name The name of the file to split.
   * @return A list of strings representing the lines.
   */
  std::vector<std::string> cUtility::Split_File(std::string name) {
    std::ifstream file(name.c_str());
    std::vector<std::string> lines;
    if (file) {
      std::string line = "";
      while (file.good()) {
        char ch = 0;
        file.get(ch);
        if (file.good()) {
          if ((ch != '\r') && (ch != '\n')) {
            line += ch;
          }
          else {
            if (line.length() > 0) {
              lines.push_back(line);
              line = "";
            }
          }
        }
      }
    }
    return lines;
  }

  /**
   * Splits a line into tokens each separated by white space.
   * @param line The line to split into tokens.
   * @return A list of tokens.
   */
  std::vector<std::string> cUtility::Split_Line(std::string line) {
    std::vector<std::string> tokens;
    int ch_count = line.length();
    std::string token = "";
    for (int ch_index = 0; ch_index < ch_count; ch_index++) {
      char ch = line[ch_index];
      if (!std::isspace(ch)) {
        token += ch;
      }
      else {
        if (token.length() > 0) {
          tokens.push_back(token);
          token = "";
        }
      }
    }
    // Get last token.
    if (token.length() > 0) {
      tokens.push_back(token);
    }
    return tokens;
  }

  /**
   * Replaces a single token in a line.
   * @param token The token to replace.
   * @param replacement The replacement token.
   * @param line The line where the token is to be replaced.
   * @return The replaced line.
   */
  std::string cUtility::Replace_Token(std::string token, std::string replacement, std::string line) {
    boost::regex re(token);
    return boost::regex_replace(line, re, replacement, boost::format_first_only);
  }

  /**
   * Replaces all tokens in a line.
   * @param token The token to replace in the line.
   * @param replacement The replacement token.
   * @param line The line where the replacement is occuring.
   * @return The replaced line.
   */
  std::string cUtility::Replace_All(std::string token, std::string replacement, std::string line) {
    boost::regex re(token);
    return boost::regex_replace(line, re, replacement, boost::format_all);
  }

  /**
   * Splits a string based on a token.
   * @param token The token used for splitting. This is a regex string.
   * @param string The string to produce tokens from.
   * @return A list of tokens parsed from the string.
   */
  std::vector<std::string> cUtility::Split_String(std::string token, std::string string) {
    std::vector<std::string> tokens;
    boost::regex re(token);
    boost::sregex_token_iterator a(string.begin(), string.end(), re, -1);
    boost::sregex_token_iterator b;
    while (a != b) {
      std::string token = *a++;
      tokens.push_back(token);
    }
    return tokens;
  }

  /**
   * Determines if a token is an identifier.
   * @param token The token to match as an identifier.
   * @return True if the token is an identifier, false otherwise.
   */
  bool cUtility::Is_Identifier(std::string token) {
    return this->Match("^\\w+$", token);
  }

  /**
   * Determines if a token is a positive number.
   * @param token The token to match as a positive number.
   * @return True if the token is a positive number, false otherwise.
   */
  bool cUtility::Is_Positive_Number(std::string token) {
    return this->Match("^(0|\\-?[1-9][0-9]*)$", token);
  }

  /**
   * Converts a number to a string.
   * @param number The number to convert.
   * @return The string version of the number.
   */
  std::string cUtility::To_String(int number) {
    char buffer[256];
    std::sprintf(buffer, "%i", number);
    return std::string(buffer);
  }

  /**
   * Performs a regular expression match.
   * @param pattern The pattern to match.
   * @param string The string to find the pattern in.
   * @return True if the pattern was found, false otherwise.
   */
  bool cUtility::Match(std::string pattern, std::string string) {
    boost::regex re(pattern);
    return boost::regex_match(string, re);
  }

  /**
   * Sets a number to a value.
   * @param value The value object.
   * @param number The numeric value.
   */
  void cUtility::Set_Number(sValue& value, int number) {
    value.type = TYPE_NUMBER;
    value.number = number;
    value.string = "";
  }

  /**
   * Sets a string to a value.
   * @param value The value object.
   * @param string The string value.
   */
  void cUtility::Set_String(sValue& value, std::string string) {
    value.type = TYPE_STRING;
    value.string = string;
    value.number = 0;
  }

  /**
   * Sets a field number on an object.
   * @param object The object to set the number to.
   * @param field The object's field.
   * @param number The number to set.
   */
  void cUtility::Set_Field_Number(std::map<std::string, sValue>& object, std::string field, int number) {
    if (object.find(field) != object.end()) {
      object[field].number = number;
      object[field].string = "";
      object[field].type = TYPE_NUMBER;
    }
    else {
      sValue valuea;
      value.number = number;
      value.string = "";
      value.type = TYPE_NUMBER;
      object[field] = value;
    }
  }

  /**
   * Sets a field string on an object.
   * @param object The object to set the string on.
   * @param field The object's field.
   * @param string The string value to set.
   */
  void cUtility::Set_Field_String(std::map<std::string, sValue>& object, std::string field, std::string string) {
    if (object.find(field) != object.end()) {
      object[field].string = string;
      object[field].number = 0;
      object[field].type = TYPE_STRING;
    }
    else {
      sValue value;
      value.string = string;
      value.number = 0;
      value.type = TYPE_STRING;
      object[field] = value;
    }
  }

  /**
   * Determines if a field exists.
   * @param object The object with the field.
   * @param field The field to test.
   * @return True if the field exists, false otherwise.
   */
  bool cUtility::Does_Field_Exist(std::map<std::string, sValue>& object, std::string field) {
    return (object.find(field) != object.end());
  }
  
  /**
   * Trims a string.
   * @param string The string to trim.
   * @return The trimmed string.
   */
  std::string cUtility::Trim(std::string string) {
    string = this->Replace_Token("^\\s*", "", string);
    string = this->Replace_Token("\\s*$", "", string);
    return string;
  }
  
    /**
   * Writes out an object to a string.
   * @param object The object from the memory to write.
   * @return The serialized string. The string consists of key=value pairs separated by commas.
   */
  std::string cUtility::Write_Object(std::map<std::string, sValue>& object) {
    std::vector<std::string> pairs;
    for (std::map<std::string, sValue>::iterator i = object.begin(); i != object.end(); ++i) {
      std::string key = i->first;
      sValue value = i->second;
      if (value.type == TYPE_NUMBER) {
        pairs.push_back(key + "=" + this->To_String(value.number));
      }
      else if (value.type == TYPE_STRING) {
        pairs.push_back(key + "=" + value.string);
      }
    }
    return boost::algorithm::join(pairs, ",");
  }
  
  /**
   * Sets the root for the whole project.
   * @param root The project folder name.
   */
  void cUtility::Set_Root(std::string root) {
    this->root = root;
  }
  
    /**
   * Pauses execution for a specified number of milliseconds.
   * @param wait The time to pause for in milliseconds.
   */
  void cUtility::Timeout(int wait) {
    int time = (int)std::time(NULL) + wait;
    while ((int)std::time(NULL) < time) { // This will block!
      // Do nothing.
    }
  }

}
