#include "C_Lesh.hpp"

namespace Codeloader {

  /**
   * Constructs a new C-Lesh compiler/interpreter. Also instantiates the command table.
   * @param memory_size The number of blocks to allocate.
   * @param allegro The Allegro library.
   */
  cC_Lesh::cC_Lesh(int memory_size, cAllegro* allegro) :
  code_table({
    { "eq", COND_EQ },
    { "ne", COND_NE },
    { "lt", COND_LT },
    { "gt", COND_GT },
    { "le", COND_LE },
    { "ge", COND_GE },
    { "+", OPER_ADD },
    { "-", OPER_SUBTRACT },
    { "*", OPER_MULTIPLY },
    { "/", OPER_DIVIDE },
    { "rem", OPER_REMAINDER },
    { "cat", OPER_CONCAT },
    { "rand", OPER_RANDOM },
    { "cos", OPER_COSINE },
    { "sin", OPER_SINE },
    { "e", TYPE_EMPTY },
    { "n", TYPE_NUMBER },
    { "s", TYPE_STRING },
    { "a", TYPE_VALUE },
    { "f", TYPE_FIELD },
    { "l", TYPE_LIST },
    { "h", TYPE_HASH },
    { "and", LOGIC_AND },
    { "or", LOGIC_OR }
  }),
  parse_table({
    { "test", { CMD_TEST, "<c>" } },
    { "move", { CMD_MOVE, "<e>" } },
    { "call", { CMD_CALL, "<e>" } },
    { "return", { CMD_RETURN, "" } },
    { "stop", { CMD_STOP, "" } },
    { "set", { CMD_SET, "<e> to <e>" } },
    { "output", { CMD_OUTPUT, "<e> at <e> <e> color <e> <e> <e>" } },
    { "load", { CMD_LOAD, "<e> from <e>" } },
    { "save", { CMD_SAVE, "<e> count <e> to <e> to <e>" } },
    { "draw", { CMD_DRAW, "<e> at <e> <e> scale <e> angle <e> layer <e> flip-x <e> flip-y <e>" } },
    { "play", { CMD_PLAY, "<e> mode <e>" } },
    { "music", { CMD_MUSIC, "<e> mode <e>" } },
    { "input", { CMD_INPUT, "<e> player <e>" } },
    { "collision", { CMD_COLLISION, "<e> other <e> results <e>" } },
    { "focus", { CMD_FOCUS, "<e> sprite <e>" } },
    { "update", { CMD_UPDATE, "" } },
    { "timeout", { CMD_TIMEOUT, "<e>" } },
    { "resource", { CMD_RESOURCE, "<e>" } },
    { "upload", { CMD_UPLOAD, "" } }
  }),
  cConsole(allegro) {
    // Initialize blocks.
    this->memory = new sBlock[memory_size];
    this->memory_size = memory_size; // Record size of memory.
    for (int block_index = 0; block_index < memory_size; block_index++) {
      sBlock& block = this->memory[block_index];
      block.code = 0;
      block.value.number = 0;
      block.value.type = this->code_table["n"];
    }
    this->compiled = false;
    this->prgm_counter = 0;
    this->time = 0;
    this->allegro = allegro;
    this->done = false;
    // Generate a random number.
    std::srand(std::time(NULL));
  }

  /**
   * Frees up the C-Lesh object.
   */
  cC_Lesh::~cC_Lesh() {
    if (this->memory) {
      delete[] this->memory;
    }
  }

  /**
   * Compiles the source file with C-Lesh code.
   * @param name The of the source file.
   */
  void cC_Lesh::Compile(std::string name) {
    // Do some cleanup.
    this->symtab.clear();
    this->tokens.clear();
    this->prgm_counter = 0;
    // Run preprocessor.
    this->Preprocess(name);
    try {
      // Now compile the source.
      this->Parse_Tokens(name);
      // Do command parse.
      while (this->tokens.size() > 0) {
        // Now parse the command.
        this->Parse_Command();
      }
      // Resolve symbol references.
      this->Replace_Symbols();
      this->compiled = true;
    }
    catch (std::string error) {
      std::cout << error.c_str() << std::endl;
    }
  }

  /**
   * Parses tokens given the name of a file where they are.
   * @param name The name of the source file.
   */
  void cC_Lesh::Parse_Tokens(std::string name) {
    std::vector<std::string> lines = this->Split_File(this->root + "/" + name);
    int line_count = lines.size();
    for (int line_index = 0; line_index < line_count; line_index++) {
      std::string line = lines[line_index];
      std::vector<std::string> tokens = this->Split_Line(line);
      // Build out the tokens.
      int tok_count = tokens.size();
      for (int tok_index = 0; tok_index < tok_count; tok_index++) {
        std::string token = tokens[tok_index];
        sToken tok_obj;
        tok_obj.token = token;
        tok_obj.line_no = line_index + 1;
        tok_obj.line = line;
        this->tokens.push_back(tok_obj);
      }
    }
  }

  /**
   * Parses an expression.
   * @return An expression object.
   */
  std::vector<sOperand> cC_Lesh::Parse_Expression() {
    std::vector<sOperand> expression;
    sOperand operand = this->Parse_Operand();
    expression.push_back(operand);
    while (this->Is_Operator()) {
      sToken oper = this->Parse_Token();
      int oper_code = this->code_table[oper.token];
      sOperand op;
      op.code = oper_code;
      expression.push_back(op);
      operand = this->Parse_Operand();
      expression.push_back(operand);
    }
    return expression;
  }

  /**
   * Parses an operand.
   * @return An operand object.
   * @throws An error if the parse did not complete.
   */
  sOperand cC_Lesh::Parse_Operand() {
    sOperand operand;
    operand.string = "";
    operand.number = 0;
    operand.address = 0;
    operand.index = 0;
    operand.field = "";
    operand.type = this->code_table["n"];
    // Now proceed with the parsing.
    if (this->Is_Number()) {
      operand.type = this->code_table["n"];
      operand.number = this->Parse_Number();
    }
    else if (this->Is_Address()) {
      operand.type = this->code_table["a"];
      operand.address = this->Parse_Address();
    }
    else if (this->Is_Field()) {
      operand.type = this->code_table["f"];
      sField field = this->Parse_Field();
      operand.address = field.value;
      operand.field = field.name;
    }
    else if (this->Is_List()) {
      operand.type = this->code_table["l"];
      sList list = this->Parse_List();
      operand.address = list.address;
      operand.index = list.index;
      operand.field = list.field;
    }
    else if (this->Is_Hash()) {
      operand.type = this->code_table["h"];
      sHash hash = this->Parse_Hash();
      operand.address = hash.address;
      operand.key = hash.key;
    }
    else if (this->Is_String()) {
      operand.type = this->code_table["s"];
      operand.string = this->Parse_String();
    }
    else if (this->Is_Num_Placeholder()) {
      operand.type = this->code_table["n"];
      operand.num_placeholder = this->Parse_Token().token;
    }
    else if (this->Is_Addr_Placeholder()) {
      operand.type = this->code_table["a"];
      operand.addr_placeholder = this->Parse_Addr_Placeholder();
    }
    else if (this->Is_Field_Placeholder()) {
      operand.type = this->code_table["f"];
      sField field = this->Parse_Field_Placeholder();
      operand.addr_placeholder = field.vplaceholder;
      operand.field = field.name;
    }
    else if (this->Is_List_Placeholder()) {
      operand.type = this->code_table["l"];
      sList list = this->Parse_List_Placeholder();
      operand.addr_placeholder = list.addr_placeholder;
      operand.index_placeholder = list.index_placeholder;
      operand.field = list.field;
    }
    else if (this->Is_Hash_Placeholder()) {
      operand.type = this->code_table["h"];
      sHash hash = this->Parse_Hash_Placeholder();
      operand.addr_placeholder = hash.addr_placeholder;
      operand.key_placeholder = hash.key_placeholder;
    }
    else {
      this->Generate_Error("Operand is invalid. (" + this->Peek_Token().token + ")");
    }
    return operand;
  }

  /**
   * Parses a condition and fills a blocks with it.
   * @param block The block to fill with the condition.
   * @return The condition expression.
   */
  sCondition cC_Lesh::Parse_Condition(sBlock& block) {
    sCondition condition;
    condition.left = 0;
    condition.right = 0;
    condition.test = 0;
    // The expression is stored in the expression list.
    std::vector<sOperand> left_exp = this->Parse_Expression();
    block.expressions.push_back(left_exp);
    condition.left = block.expressions.size() - 1; // Just point to expression.
    std::string test = this->Parse_Test();
    condition.test = this->code_table[test];
    std::vector<sOperand> right_exp = this->Parse_Expression();
    block.expressions.push_back(right_exp);
    condition.right = block.expressions.size() - 1;
    return condition;
  }

  /**
   * Parses a test.
   * @return The test token.
   * @throws An error if the test is invalid.
   */
  std::string cC_Lesh::Parse_Test() {
    sToken token = this->Parse_Token();
    std::string test = "";
    if ((token.token == "eq") ||
        (token.token == "ne") ||
        (token.token == "lt") ||
        (token.token == "gt") ||
        (token.token == "le") ||
        (token.token == "ge")) {
      test = token.token;
    }
    else {
      this->Generate_Error(token.token + " is not a valid test.");
    }
    return test;
  }

  /**
   * Parses a conditional.
   * @param block The block to populate with the conditional.
   * @return The conditional expression.
   */
  std::vector<sCondition> cC_Lesh::Parse_Conditional(sBlock& block) {
    std::vector<sCondition> conditional;
    sCondition condition = this->Parse_Condition(block);
    conditional.push_back(condition);
    while (this->Is_Logic()) {
      sToken logic = this->Parse_Token();
      sCondition logic_code;
      logic_code.logic = this->code_table[logic.token];
      conditional.push_back(logic_code);
      condition = this->Parse_Condition(block);
      conditional.push_back(condition);
    }
    return conditional;
  }

  /**
   * Parses a command.
   * @throws An error if the command is invalid.
   */
  void cC_Lesh::Parse_Command() {
    if (this->prgm_counter < this->memory_size) {
      sToken code = this->Parse_Token();
      if (code.token == "remark") { // Comment
        // Parse all the way to end.
        sToken end_tok = this->Peek_Token();
        while (end_tok.token != "end") {
          this->Parse_Token(); // Remove comment token.
          end_tok = this->Peek_Token();
        }
        // Remove end command.
        this->Parse_Token();
      }
      else if (code.token == "define") { // define <name> as <number>
        sToken name = this->Parse_Token();
        this->Parse_Keyword("as");
        int num = this->Parse_Number();
        sValue value;
        this->Set_Number(value, num);
        this->symtab[name.token] = value;
      }
      else if (code.token == "label") { // label <name>
        sToken name = this->Parse_Token();
        sValue value;
        this->Set_Number(value, this->prgm_counter);
        this->symtab[name.token] = value; // Update with current address.
      }
      else if (code.token == "var") { // var <name>
        sToken name = this->Parse_Token();
        sValue value;
        this->Set_Number(value, this->prgm_counter++);
        this->symtab[name.token] = value; // Skip one block.
      }
      else if (code.token == "list") { // list <name> alloc <blocks>
        sToken name = this->Parse_Token();
        sValue value;
        this->Set_Number(value, this->prgm_counter);
        this->symtab[name.token] = value;
        this->Parse_Keyword("alloc");
        int count = this->Parse_Number();
        // Allocate free blocks for the list.
        this->prgm_counter += count;
      }
      else if (code.token == "screen") { // screen <width> <height>
        this->screen_w = this->Parse_Number();
        this->screen_h = this->Parse_Number();
        this->allegro->Create_Screen(this->screen_w, this->screen_h);
      }
      else { // Possible commands.
        if (this->parse_table.find(code.token) != this->parse_table.end()) {
          sParse_Obj command = this->parse_table[code.token];
          sBlock block = this->memory[this->prgm_counter++];
          // Clear out the block.
          this->Clear_Block(block);
          // Assign block code.
          block.code = command.code;
          if (command.pattern.length() > 0) {
            std::vector<std::string> entries = this->Split_Line(command.pattern);
            int entry_count = entries.size();
            for (int entry_index = 0; entry_index < entry_count; entry_index++) {
              std::string entry = entries[entry_index];
              if (entry == "<c>") {
                block.conditional = this->Parse_Conditional(block);
              }
              else if (entry == "<e>") {
                block.expressions.push_back(this->Parse_Expression());
              }
              else if (entry == "<s>") {
                block.strings.push_back(this->Parse_String());
              }
              else { // This will be treated as keyword.
                this->Parse_Keyword(entry);
              }
            }
          }
          // Record debug info.
          this->debug_symbols.push_back(code.token);
        }
        else {
          this->Generate_Error("Invalid command " + code.token + ".");
        }
      }
    }
    else {
      // Should probably not be called.
      this->Generate_Error("Program too big for memory.");
    }
  }

  /**
   * Clears out a block.
   * @param block The block to clear out.
   */
  void cC_Lesh::Clear_Block(sBlock& block) {
    block.code = 0;
    block.expressions.clear();
    block.conditional.clear();
    block.fields.clear();
    block.value.string = "";
    block.value.number = 0;
    block.value.type = this->code_table["n"];
    block.strings.clear();
  }

  /**
   * Generates an error.
   * @param message The message describing the error.
   * @throws An error as a string.
   */
  void cC_Lesh::Generate_Error(std::string message) {
    std::string error = "---SCRIPT ERROR---\n\n";
    if (!this->compiled) {
      error += std::string("Line: " + this->To_String(this->last_token.line_no) + "\nCode: " + this->last_token.line + "\n\n");
    }
    else {
      if (this->prgm_counter < this->debug_symbols.size()) {
        std::string symbol = this->debug_symbols[this->prgm_counter - 1];
        error += std::string("Command: " + symbol + "\n" +
                             "PC: " + this->To_String(this->prgm_counter - 1) + "\n\n");
      }
    }
    error += message;
    throw error;
  }

  /**
   * Parses a single token.
   * @return A token object.
   * @throws An error if the token could not be parsed.
   */
  sToken cC_Lesh::Parse_Token() {
    sToken token;
    if (this->tokens.size() > 0) {
      token = this->tokens.front();
      this->tokens.erase(this->tokens.begin()); // Remove from front.
      // Get line data also.
      this->last_token = token;
    }
    else {
      this->Generate_Error("Out of tokens.");
    }
    return token;
  }

  /**
   * Looks at a token without removing it.
   * @return The token object.
   */
  sToken cC_Lesh::Peek_Token() {
    sToken token;
    if (this->tokens.size() > 0) {
      token = this->tokens[0];
    }
    return token;
  }

  /**
   * Checks to see if a keyword exists and removes it.
   * @param keyword The keyword to check.
   * @throws An error if the keyword does not exist.
   */
  void cC_Lesh::Parse_Keyword(std::string keyword) {
    sToken token = this->Parse_Token();
    if (token.token != keyword) {
      this->Generate_Error("Missing keyword " + keyword + ".");
    }
  }

  /**
   * Tests if a token is a number.
   * @return True if the token is a number, false otherwise.
   */
  bool cC_Lesh::Is_Number() {
    sToken token = this->Peek_Token();
    return this->Match("^(0|\\-?[1-9][0-9]*)$", token.token);
  }

  /**
   * Parses a number and returns the number.
   * @return The integer value of the number.
   * @throws An error if the number is not a number.
   */
  int cC_Lesh::Parse_Number() {
    int number = 0;
    if (this->Is_Number()) {
      sToken token = this->Parse_Token();
      number = std::atoi(token.token.c_str());
    }
    else {
      this->Generate_Error("Not a valid number.");
    }
    return number;
  }

  /**
   * Determines if a logic token is logic.
   * @return True if the token is logic, false otherwise.
   */
  bool cC_Lesh::Is_Logic() {
    sToken token = this->Peek_Token();
    return this->Match("^(and|or)$", token.token);
  }

  /**
   * Determines if a token is an operator.
   * @return True if the token is an operator, false otherwise.
   */
  bool cC_Lesh::Is_Operator() {
    sToken token = this->Peek_Token();
    return this->Match("^(\\+|\\-|\\*|\\/|rem|cat|rand|cos|sin)$", token.token);
  }

  /**
   * Checks if a token is an address.
   * @return True if the token is an address, false otherwise.
   */
  bool cC_Lesh::Is_Address() {
    sToken token = this->Peek_Token();
    return this->Match("^#(0|[1-9][0-9]*)$", token.token);
  }

  /**
   * Parses an address.
   * @return An integer representing the address.
   * @throws An error if the address if not formatted correctly.
   */
  int cC_Lesh::Parse_Address() {
    int address = 0;
    if (this->Is_Address()) {
      sToken token = this->Parse_Token();
      address = std::atoi(token.token.substr(1).c_str());
    }
    else {
      this->Generate_Error("Not a valid address.");
    }
    return address;
  }

  /**
   * Determines if a token is a field.
   * @return True if the token is a field, false otherwise.
   */
  bool cC_Lesh::Is_Field() {
    sToken token = this->Peek_Token();
    return this->Match("^#(0|[1-9][0-9]*):\\w+$", token.token);
  }

  /**
   * Parses a field.
   * @return A field object.
   */
  sField cC_Lesh::Parse_Field() {
    sField field;
    if (this->Is_Field()) {
      sToken token = this->Parse_Token();
      std::vector<std::string> pair = this->Split_String(":", token.token.substr(1));
      field.name = pair[1];
      // Convert the address to a number.
      field.value = std::atoi(pair[0].c_str());
    }
    return field;
  }

  /**
   * Tests if a token is a hash.
   * @return True if the token is a hash.
   */
  bool cC_Lesh::Is_Hash() {
    sToken token = this->Peek_Token();
    return this->Match("^#(0|[1-9][0-9]*):(0|[1-9][0-9]*)$", token.token);
  }

  /**
   * Parses a hash.
   * @return A hash object.
   */
  sHash cC_Lesh::Parse_Hash() {
    sHash hash;
    if (this->Is_Hash()) {
      sToken token = this->Parse_Token();
      std::vector<std::string> pair = this->Split_String(":", token.token);
      // Convert both values to numbers.
      hash.address = std::atoi(pair[0].c_str());
      hash.key = std::atoi(pair[1].c_str());
    }
    return hash;
  }

  /**
   * Determines if a list token is a list.
   * @return True if the token is a list, false otherwise.
   */
  bool cC_Lesh::Is_List() {
    sToken token = this->Peek_Token();
    return this->Match("^#(0|[1-9][0-9]*):(0|[1-9][0-9]*):\\w+$", token.token);
  }

  /**
   * Parses a list.
   * @return A list object.
   */
  sList cC_Lesh::Parse_List() {
    sList list;
    if (this->Is_List()) {
      sToken token = this->Parse_Token();
      std::vector<std::string> tripplet = this->Split_String(":", token.token);
      // Convert address and index to number.
      list.address = std::atoi(tripplet[0].c_str()); // Address
      list.index = std::atoi(tripplet[1].c_str()); // Index
      list.field = tripplet[2];
    }
    return list;
  }

  /**
   * Determines if a token is a string.
   * @return True if the token is a string, false otherwise.
   */
  bool cC_Lesh::Is_String() {
    sToken token = this->Peek_Token();
    return this->Match("^\"[^\"]*\"$", token.token);
  }

  /**
   * Parses a string.
   * @return A string.
   * @throws An error if the string is invalid.
   */
  std::string cC_Lesh::Parse_String() {
    std::string string = "";
    if (this->Is_String()) {
      sToken token = this->Parse_Token();
      string = this->Replace_Token("^\"([^\"]*)\"$", "$1", token.token);
      // Also replace string entities.
      string = this->Replace_All("\\\\\"", "\"", string);
      string = this->Replace_All("\\\\s", " ", string);
    }
    else {
      this->Generate_Error("Not a valid string.");
    }
    return string;
  }

  /**
   * Determines if the token is a number placeholder.
   * @return True if the token is a placeholder, false otherwise.
   */
  bool cC_Lesh::Is_Num_Placeholder() {
    sToken token = this->Peek_Token();
    return this->Match("^\\[[^\\]]+\\]$", token.token);
  }

  /**
   * Determines if a program is an address placeholder.
   * @return True if the token is a placeholder, false otherwise.
   */
  bool cC_Lesh::Is_Addr_Placeholder() {
    sToken token = this->Peek_Token();
    return this->Match("^#\\[[^\\]]+\\]$", token.token);
  }

  /**
   * Parses an address placeholder.
   * @return A string representing the placeholder.
   * @throws An error if the placeholder is invalid.
   */
  std::string cC_Lesh::Parse_Addr_Placeholder() {
    std::string placeholder = "";
    if (this->Is_Addr_Placeholder()) {
      sToken token = this->Parse_Token();
      placeholder = token.token.substr(1);
    }
    else {
      this->Generate_Error("Not an address placeholder.");
    }
    return placeholder;
  }

  /**
   * Determines if the token is a field placeholder.
   * @return True if the token is a placeholder, false otherwise.
   */
  bool cC_Lesh::Is_Field_Placeholder() {
    sToken token = this->Peek_Token();
    return this->Match("^#\\[[^\\]]+\\]:\\w+$", token.token);
  }

  /**
   * Parses a field placeholder.
   * @return A field object.
   * @throws An error if the placeholder is invalid.
   */
  sField cC_Lesh::Parse_Field_Placeholder() {
    sField field;
    if (this->Is_Field_Placeholder()) {
      sToken token = this->Parse_Token();
      std::vector<std::string> placeholder = this->Split_String(":", token.token.substr(1));
      field.vplaceholder = placeholder[0];
      field.name = placeholder[1];
    }
    else {
      this->Generate_Error("Not a field placeholder.");
    }
    return field;
  }

  /**
   * Determines if the token is a hash placeholder.
   * @return True if the token is a placeholder, false otherwise.
   */
  bool cC_Lesh::Is_Hash_Placeholder() {
    sToken token = this->Peek_Token();
    return this->Match("^#\\[[^\\]]+\\]:\\[[^\\]]+\\]$", token.token);
  }

  /**
   * Parses a hash placeholder.
   * @return A hash object.
   */
  sHash cC_Lesh::Parse_Hash_Placeholder() {
    sHash hash;
    if (this->Is_Hash_Placeholder()) {
      sToken token = this->Parse_Token();
      std::vector<std::string> placeholder = this->Split_String(":", token.token.substr(1));
      hash.addr_placeholder = placeholder[0];
      hash.key_placeholder = placeholder[1];
    }
    return hash;
  }

  /**
   * Determines if a token is a list placeholder.
   * @return True if the token is a placeholder, false otherwise.
   */
  bool cC_Lesh::Is_List_Placeholder() {
    sToken token = this->Peek_Token();
    return this->Match("^#\\[[^\\]]+\\]:\\[[^\\]]+\\]:\\w+$", token.token);
  }

  /**
   * Parses a list placeholder from a token.
   * @return A list object.
   * @throws An error if the list is invalid.
   */
  sList cC_Lesh::Parse_List_Placeholder() {
    sList list;
    if (this->Is_List_Placeholder()) {
      sToken token = this->Parse_Token();
      std::vector<std::string> placeholder = this->Split_String(":", token.token.substr(1));
      list.addr_placeholder = placeholder[0];
      list.index_placeholder = placeholder[1];
      list.field = placeholder[2];
    }
    else {
      this->Generate_Error("Not a list placeholder.");
    }
    return list;
  }

  /**
   * Replaces all symbols in the source file.
   */
  void cC_Lesh::Replace_Symbols() {
    for (int cmd_index = 0; cmd_index <= this->prgm_counter; cmd_index++) {
      sBlock& block = this->memory[cmd_index];
      // Replace symbols in expressions.
      int exp_count = block.expressions.size();
      for (int exp_index = 0; exp_index < exp_count; exp_index++) {
        std::vector<sOperand>& expression = block.expressions[exp_index];
        this->Replace_Expression(expression);
      }
    }
  }

  /**
   * Replaces an expression with placeholders.
   * @param expression The expression with placeholders.
   */
  void cC_Lesh::Replace_Expression(std::vector<sOperand>& expression) {
    int exp_size = expression.size();
    for (int part_index = 0; part_index < exp_size; part_index++) {
      sOperand& part = expression[part_index];
      if (((part_index + 1) % 2) == 1) { // An operand, they're odd numbers.
        part.number = (part.num_placeholder.length() > 0) ? this->Replace_Symbol(part.num_placeholder) : part.number;
        part.address = (part.addr_placeholder.length() > 0) ? this->Replace_Symbol(part.addr_placeholder) : part.address;
        part.index = (part.index_placeholder.length() > 0) ? this->Replace_Symbol(part.index_placeholder) : part.index;
        part.key = (part.key_placeholder.length() > 0) ? this->Replace_Symbol(part.key_placeholder) : part.key;
      }
    }
  }

  /**
   * Replaces a symbol with a number.
   * @param name The name of the symbol to replace.
   * @return A number associated with the symbol.
   */
  int cC_Lesh::Replace_Symbol(std::string name) {
    int value = 0;
    std::string sym_name = this->Replace_Token("^\\[([^\\]]+)\\]$", "$1", name); // Get symbol name.
    // Is it in the symbol table?
    if (this->symtab.find(sym_name) != this->symtab.end()) {
      value = this->symtab[sym_name].number;
    }
    else {
      this->Generate_Error("Label " + sym_name + " was not declared.");
    }
    return value;
  }

  /**
   * Preprocesses the source code.
   * @param name The source file.
   * @return The preprocessed code.
   */
  std::string cC_Lesh::Preprocess(std::string name) {
    // Anything that needs to be added to the symbol table
    // would go here.
    // Screen
    this->Set_Number(this->symtab["SCREEN_W"], this->screen_w);
    this->Set_Number(this->symtab["SCREEN_H"], this->screen_h);
    // Layers
    this->Set_Number(this->symtab["BACKGROUND"],  1);
    this->Set_Number(this->symtab["PLATFORM"], 2);
    this->Set_Number(this->symtab["CHARACTER"], 3);
    this->Set_Number(this->symtab["FOREGROUND"], 4);
    this->Set_Number(this->symtab["OVERLAY"], 5);
    // Input Signals
    this->Set_Number(this->symtab["NONE"], 0);
    this->Set_Number(this->symtab["PRESSED"], 1);
    // Language Constants
    this->Set_Number(this->symtab["TRUE"], 1);
    this->Set_Number(this->symtab["FALSE"], 0);
    // Constants
    this->Set_Number(this->symtab["KEYBOARD"], -1);
    // Replace imports with code.
    std::vector<std::string> lines = this->Split_File(this->root + "/" + name);
    return this->Preprocess_Lines(lines);
  }

  /**
   * Preprocesses all source lines to include imports.
   * @param lines The source code lines.
   * @return The preprocessed source code.
   */
  std::string cC_Lesh::Preprocess_Lines(std::vector<std::string> lines) {
    int line_count = lines.size();
    std::string code = "";
    for (int line_index = 0; line_index < line_count; line_index++) {
      std::string line = lines[line_index];
      if (this->Match("^\\s*import\\s+\"\\w*\".*$", line)) { // An import.
        std::string fname = this->root + "/" + this->Replace_Token("^\\s*import\\s+\"(\\w*)\".*$", "$1", line) + ".clsh";
        std::ifstream file(fname.c_str());
        if (file) {
          file.seekg(0, std::ios::end);
          int size = file.tellg();
          char* buffer = new char[size];
          file.read(buffer, size);
          std::string data = buffer;
          code += std::string(data + "\n");
          delete buffer;
        }
      }
      else { // Regular line.
        code += std::string(line + "\n");
      }
    }
    return code;
  }

  /**
   * Executes the specified program. It will allow the program to run for a
   * specified amount of time before relinqishing control of the CPU.
   * @throws An error if there was a problem with the execution.
   */
  void cC_Lesh::Execute() {
    try { // Check for errors.
      // Record initial time.
      this->time = (int)std::time(NULL) + TIMEOUT;
      int block_count = this->memory_size; // Cannot go past program memory.
      while (this->prgm_counter < block_count) {
        // Check the timer interrupt.
        if (this->time < (int)std::time(NULL)) {
          break; // We must exit the program. Our time is up!
        }
        // Interpret a command.
        this->Interpret();
      }
    }
    catch (std::string error) {
      std::string message = error;
      int symbol_index = this->prgm_counter - 1;
      if ((symbol_index >= 0) && (symbol_index < this->debug_symbols.size())) {
        std::string symbol = this->debug_symbols[symbol_index];
        message = "Command: " + symbol + "\n" +
                  "PC: " + this->To_String(symbol_index) + "\n" +
                  "Error: " + error;
      }
      throw message;
    }
  }

  /**
   * This is the core interpreter. We should spend as much time in here as
   * possible for efficiency.
   */
  void cC_Lesh::Interpret() {
    sBlock& block = this->memory[this->prgm_counter++];
    if (block.code == CMD_DATA) { // Data
      // Do nothing.
    }
    else if (block.code == CMD_TEST) { // test <condition>
      bool result = this->Eval_Conditional(block);
      if (!result) { // Fail
        this->prgm_counter++; // Skip over next command.
      }
    }
    else if (block.code == CMD_MOVE) { // move <address>
      sValue move_addr = this->Eval_Expression(block, 0);
      this->prgm_counter = move_addr.number;
    }
    else if (block.code == CMD_CALL) { // call <address>
      sValue call_addr = this->Eval_Expression(block, 0);
      this->stack.push(this->prgm_counter); // Save the current location.
      this->prgm_counter = call_addr.number;
    }
    else if (block.code == CMD_RETURN) { // return
      if (this->stack.size() > 0) {
        this->prgm_counter = this->stack.top();
        this->stack.pop();
      }
      else {
        this->Generate_Error("Too many returns.");
      }
    }
    else if (block.code == CMD_STOP) { // stop
      this->prgm_counter = this->memory_size; // Will cause a stop.
      this->done = true; // Report that C-Lesh is done.
    }
    else if (block.code == CMD_SET) { // set <address> to <data>
      // The first expression is a variable or field so we won't evaluate it.
      // We'll just look at the first operand for addressing.
      if (block.expressions.size() == 0) {
        this->Generate_Error("Variable's address is not set.");
      }
      if (block.expressions[0].size() > 1) {
        this->Generate_Error("Expression has more than one entity for destination.");
      }
      sOperand& dest = block.expressions[0][0];
      sValue data = this->Eval_Expression(block, 1);
      if (data.type != TYPE_EMPTY) {
        // Save as field or value.
        if (dest.type == TYPE_VALUE) { // Value
          this->Read_Write_Memory(dest.address, "", data);
        }
        else if (dest.type == TYPE_FIELD) { // Field
          this->Read_Write_Memory(dest.address, dest.field, data);
        }
        else if (dest.type == TYPE_LIST) { // List
          sValue index_var;
          index_var.type = TYPE_EMPTY; // Read
          index_var.number = 0;
          index_var.string = "";
          // Read the iterator.
          this->Read_Write_Memory(dest.index, "", index_var);
          // Write the data.
          this->Read_Write_Memory(dest.address + index_var.number, dest.field, data);
        }
        else if (dest.type == TYPE_HASH) { // Hash
          sValue key_var;
          key_var.type = TYPE_EMPTY; // Read
          key_var.number = 0;
          key_var.string = "";
          // Read key address to locate the key variable.
          this->Read_Write_Memory(dest.key, "", key_var);
          // Now read the memory.
          this->Read_Write_Memory(dest.address, key_var.string, data);
        }
        else {
          this->Generate_Error("Variable needs to be of type address, field, hash, or list.");
        }
      }
    }
    else if (block.code == CMD_OUTPUT) { // output <data> at <number> <number> color <number> <number> <number>
      sValue data = this->Eval_Expression(block, 0);
      sValue x = this->Eval_Expression(block, 1);
      sValue y = this->Eval_Expression(block, 2);
      sValue red = this->Eval_Expression(block, 3);
      sValue green = this->Eval_Expression(block, 4);
      sValue blue = this->Eval_Expression(block, 5);
      sColor color;
      color.red = (unsigned char)red.number;
      color.green = (unsigned char)green.number;
      color.blue = (unsigned char)blue.number;
      std::string value = (data.type == TYPE_NUMBER) ? this->To_String(data.number) : data.string;
      this->Output_Text(value, x.number, y.number, color);
    }
    else if (block.code == CMD_LOAD) { // load <address> from <string>
      sValue offset = this->Eval_Expression(block, 0);
      sValue file = this->Eval_Expression(block, 1);
      this->Load_File(file.string, this->memory, this->memory_size, offset.number);
    }
    else if (block.code == CMD_SAVE) { // save <address> count <number> to <string>
      sValue offset = this->Eval_Expression(block, 0);
      sValue count = this->Eval_Expression(block, 1);
      sValue file = this->Eval_Expression(block, 2);
      this->Save_File(file.string, this->memory, this->memory_size, offset.number, count.number);
    }
    else if (block.code == CMD_DRAW) { // draw <string> at <number> <number> scale <number> angle <number> layer <number> flip-x <number> flip-y <number>
      sValue name = this->Eval_Expression(block, 0);
      sValue x = this->Eval_Expression(block, 1);
      sValue y = this->Eval_Expression(block, 2);
      sValue scale = this->Eval_Expression(block, 3);
      sValue angle = this->Eval_Expression(block, 4);
      sValue layer = this->Eval_Expression(block, 5);
      sValue flip_x = this->Eval_Expression(block, 6);
      sValue flip_y = this->Eval_Expression(block, 7);
      this->Draw_Image(name.string, x.number, y.number, scale.number, angle.number, (bool)flip_x.number, (bool)flip_y.number, layer.string);
    }
    else if (block.code == CMD_PLAY) { // play <string> mode <string>
      sValue name = this->Eval_Expression(block, 0);
      sValue mode = this->Eval_Expression(block, 1);
      this->Play_Sound(name.string, mode.string);
    }
    else if (block.code == CMD_MUSIC) { // music <string> mode <string>
      sValue name = this->Eval_Expression(block, 0);
      sValue mode = this->Eval_Expression(block, 1);
      this->Play_Track(name.string, mode.string);
    }
    else if (block.code == CMD_INPUT) { // input <address> player <number>
      sValue address = this->Eval_Expression(block, 0);
      sValue player = this->Eval_Expression(block, 1);
      if (this->Valid_Address(address.number)) {
        this->Generate_Error("Invalid memory read.");
      }
      if (this->inputs.find(player.number) != this->inputs.end()) {
        this->Generate_Error("Player number is out of bounds.");
      }
      this->Read_Input(player.number, this->memory, this->memory_size, address.number);
    }
    else if (block.code == CMD_COLLISION) { // collision <address> other <address> results <address>
      sValue sprite_addr = this->Eval_Expression(block, 0);
      sValue other_addr = this->Eval_Expression(block, 1);
      sValue results_addr = this->Eval_Expression(block, 2);
      if (!this->Valid_Address(sprite_addr.number) || !this->Valid_Address(other_addr.number) || !this->Valid_Address(results_addr.number)) {
        this->Generate_Error("Collision detection invalid memory access.");
      }
      std::map<std::string, sValue>& sprite = this->memory[sprite_addr.number].fields;
      std::map<std::string, sValue>& other = this->memory[other_addr.number].fields;
      std::map<std::string, sValue>& results = this->memory[results_addr.number].fields;
      this->Detect_Collision(sprite, other, results);
    }
    else if (block.code == CMD_FOCUS) { // focus <address> sprite <address>
      sValue sprite_addr = this->Eval_Expression(block, 0);
      sValue camera_addr = this->Eval_Expression(block, 1);
      if (!this->Valid_Address(sprite_addr.number) || !this->Valid_Address(camera_addr.number)) {
        this->Generate_Error("Camera invalid memory access.");
      }
      std::map<std::string, sValue>& sprite = this->memory[sprite_addr.number].fields;
      std::map<std::string, sValue>& camera = this->memory[camera_addr.number].fields;
      this->Focus_Camera(camera, sprite);
    }
    else if (block.code == CMD_UPDATE) { // update
      this->Update_Output();
    }
    else if (block.code == CMD_TIMEOUT) { // timeout <number>
      sValue timeout = this->Eval_Expression(block, 0);
      this->Timeout(timeout.number);
    }
    else if (block.code == CMD_RESOURCE) { // resource <string>
      sValue resource = this->Eval_Expression(block, 0);
      this->Load_Resource(resource.string);
    }
    else if (block.code == CMD_UPLOAD) { // upload
      this->Upload_Resources();
    }
    else {
      this->Generate_Error("Invalid code executed.");
    }
  }

  /**
   * Determines if a memory address is valid.
   * @param address The memory address to test.
   * @return True if the memory address is valid, false otherwise.
   */
  bool cC_Lesh::Valid_Address(int address) {
    return ((address >= 0) && (address < this->memory_size));
  }

  /**
   * Reads or writes memory to an object.
   * @param address The memory address to read or write to.
   * @param field The field name being accessed.
   * @param data The data written or returned.
   */
  void cC_Lesh::Read_Write_Memory(int address, std::string field, sValue& data) {
    if (this->Valid_Address(address)) {
      sBlock& block = this->memory[address];
      if (data.type == TYPE_EMPTY) { // Read
        if (field.length() == 0) { // Read value.
          data.string = block.value.string;
          data.number = block.value.number;
          data.type = block.value.type;
        }
        else { // Read field.
          if (block.fields.find(field) != block.fields.end()) {
            data.string = block.fields[field].string;
            data.number = block.fields[field].number;
            data.type = block.fields[field].type;
          }
          else { // Field not defined.
            data.string = "null";
            data.number = 0;
            data.type = TYPE_STRING; // String
          }
        }
      }
      else { // Write
        if (field.length() == 0) { // Write value.
          block.value.string = data.string;
          block.value.number = data.number;
          block.value.type = data.type;
        }
        else { // Write field.
          if (block.fields.find(field) != block.fields.end()) {
            block.fields[field].string = data.string;
            block.fields[field].number = data.number;
            block.fields[field].type = data.type;
          }
          else {
            sValue value;
            value.type = data.type;
            value.string = data.string;
            value.number = data.number;
            block.fields[field] = value;
          }
        }
      }
    }
    else {
      this->Generate_Error("Invalid memory access from object read.");
    }
  }

  /**
   * Evaluates an expression and returns the result.
   * @param block The block containing the expression.
   * @param expression_id The index of the expression to process.
   * @return A value object.
   */
  sValue cC_Lesh::Eval_Expression(sBlock& block, int expression_id) {
    if ((expression_id < 0) || (expression_id >= block.expressions.size())) {
      this->Generate_Error("No expression to process.");
    }
    sValue result;
    result.type = TYPE_EMPTY;
    result.string = "";
    result.number = 0;
    sValue op_result;
    op_result.type = TYPE_EMPTY;
    op_result.number = 0;
    op_result.string = "";
    // Fill result with first operand.
    std::vector<sOperand>& expression = block.expressions[expression_id];
    sOperand operand = expression[0];
    this->Eval_Operand(operand, result);
    // Evaluate the rest of the expression.
    int part_count = expression.size();
    int part_index = 1;
    while (part_index < part_count) {
      sOperand oper = expression[part_index + 0];
      operand = expression[part_index + 1];
      op_result.type = TYPE_EMPTY; // Prepare for data input.
      this->Eval_Operand(operand, op_result);
      // Process operator.
      if (oper.code == OPER_ADD) {
        result.number = result.number + op_result.number; // Compound assignment is slow so not used.
        result.type = TYPE_NUMBER; // Number
      }
      else if (oper.code == OPER_SUBTRACT) {
        result.number = result.number - op_result.number;
        result.type = TYPE_NUMBER;
      }
      else if (oper.code == OPER_MULTIPLY) {
        result.number = result.number * op_result.number;
        result.type = TYPE_NUMBER;
      }
      else if (oper.code == OPER_DIVIDE) {
        // Divide by zero gives 0.
        result.number = (op_result.number == 0) ? 0 : (result.number / op_result.number);
        result.type = TYPE_NUMBER;
      }
      else if (oper.code == OPER_REMAINDER) { // Remainder
        result.number = result.number % op_result.number;
        result.type = TYPE_NUMBER;
      }
      else if (oper.code == OPER_CONCAT) { // Concatenate
        if (op_result.type == TYPE_NUMBER) {
          result.string = result.string + this->To_String(op_result.number);
        }
        else if (op_result.type == TYPE_STRING) {
          result.string = result.string + op_result.string;
        }
        result.type = TYPE_STRING;
      }
      else if (oper.code == OPER_RANDOM) { // Random Number
        result.number = result.number + ((std::rand() % op_result.number) + 1);
        result.type = TYPE_NUMBER;
      }
      else if (oper.code == OPER_COSINE) { // Cosine
        result.number = (int)((double)result.number * std::cos((double)op_result.number * (this->pi / 180.0)));
        result.type = TYPE_NUMBER;
      }
      else if (oper.code == OPER_SINE) { // Sine
        result.number = (int)((double)result.number * std::sin((double)op_result.number * (this->pi / 180.0)));
        result.type = TYPE_NUMBER;
      }
      part_index = part_index + 2;
    }
    return result;
  }

  /**
   * Evaluates an operand and populates the result.
   * @param operand The operand to evaluate.
   * @param result The result to populate.
   */
  void cC_Lesh::Eval_Operand(sOperand& operand, sValue& result) {
    if (operand.type == TYPE_NUMBER) { // Number
      result.number = operand.number;
      result.type = TYPE_NUMBER;
    }
    else if (operand.type == TYPE_STRING) { // String
      result.string = operand.string;
      result.type = TYPE_STRING;
    }
    else if (operand.type == TYPE_VALUE) { // Address
      // We'll do some mapping here. Memory addressing might
      // be a bit on the slow side.
      this->Read_Write_Memory(operand.address, "", result);
    }
    else if (operand.type == TYPE_FIELD) { // Field
      // We'll do more memory mapping here as well. Could be a
      // bit slow but that could be me being paranoid!
      this->Read_Write_Memory(operand.address, operand.field, result);
    }
    else if (operand.type == TYPE_LIST) { // List
      // We need to concatinate the memory address with the index to get the
      // list item. The index, however, is only a pointer to an index variable.
      sValue index_var;
      index_var.type = TYPE_EMPTY;
      index_var.string = "";
      index_var.number = 0;
      // Read the index in the iterator.
      this->Read_Write_Memory(operand.index, "", index_var);
      // Now read the memory.
      this->Read_Write_Memory(operand.address + index_var.number, operand.field, result);
    }
    else if (operand.type == TYPE_HASH) { // Hash
      sValue key_var;
      key_var.type = TYPE_EMPTY;
      key_var.string = "";
      key_var.number = 0;
      // Read key address to locate the key variable.
      this->Read_Write_Memory(operand.key, "", key_var);
      // Now read the memory.
      this->Read_Write_Memory(operand.address, key_var.string, result);
    }
  }

  /**
   * Evaluates a condition and returns the result.
   * @param condition The condition to evaluate.
   * @param block The block where the condition resides.
   * @return True if the condition passed, false otherwise.
   */
  bool cC_Lesh::Eval_Condition(sCondition& condition, sBlock& block) {
    sValue left_result = this->Eval_Expression(block, condition.left);
    sValue right_result = this->Eval_Expression(block, condition.right);
    bool result = false;
    // Process tests.
    if (condition.test == COND_EQ) { // EQ
      if (left_result.type == TYPE_STRING) {
        result = (left_result.string == right_result.string);
      }
      else if (left_result.type == TYPE_NUMBER) {
        result = (left_result.number == right_result.number);
      }
    }
    else if (condition.test == COND_NE) { // NE
      if (left_result.type == TYPE_STRING) {
        result = (left_result.string != right_result.string);
      }
      else if (left_result.type == TYPE_NUMBER) {
        result = (left_result.number != right_result.number);
      }
    }
    else if (condition.test == COND_LT) { // LT
      // Only works with numbers.
      result = (left_result.number < right_result.number);
    }
    else if (condition.test == COND_GT) { // GT
      result = (left_result.number > right_result.number);
    }
    else if (condition.test == COND_LE) { // LE
      result = (left_result.number <= right_result.number);
    }
    else if (condition.test == COND_GE) { // GE
      result = (left_result.number >= right_result.number);
    }
    return result;
  }

  /**
   * Evaluates the conditional given a block.
   * @param block The block containing a conditional.
   * @return True if the conditional evaluates to true, false otherwise.
   */
  bool cC_Lesh::Eval_Conditional(sBlock& block) {
    int cond_count = block.conditional.size();
    if ((cond_count % 2) == 0) {
      this->Generate_Error("Condition not formatted correctly.");
    }
    int result = (int)this->Eval_Condition(block.conditional[0], block); // Assign first condition.
    int cond_index = 1;
    while (cond_index < cond_count) {
      sCondition& logic = block.conditional[cond_index + 0];
      sCondition& condition = block.conditional[cond_index + 1];
      if (logic.logic == LOGIC_AND) { // AND
        // Multiply to get value of AND.
        result = result * (int)this->Eval_Condition(condition, block);
      }
      else if (logic.logic == LOGIC_OR) { // OR
        // Add to get value of OR.
        result = result + (int)this->Eval_Condition(condition, block);
      }
      cond_index = cond_index + 2;
    }
    return (result > 0);
  }

}
