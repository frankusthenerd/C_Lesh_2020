#ifndef C_LESH
#define C_LESH 1

#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <stack>

#include <boost/regex.hpp>
#include <boost/algorithm/string/join.hpp>

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_ttf.h>

namespace Codeloader {

  struct sOperand;
  struct sCondition;
  struct sValue;
  struct sBlock;
  struct sToken;
  struct sParse_Obj;
  struct sField;
  struct sList;
  struct sHash;
  struct sText;
  struct sImage;
  struct sSound;
  struct sInput;
  struct sPoint;
  struct sBox;
  struct sColor;
  class cUtility;
  class cC_Lesh;
  class cConsole;
  class cAllegro;
  
  struct sColor {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
  };

  struct sOperand {
    std::string string;
    int number;
    int address;
    int index;
    std::string field;
    int type;
    int code;
    int key;
    std::string num_placeholder;
    std::string addr_placeholder;
    std::string index_placeholder;
    std::string key_placeholder;
  };

  struct sCondition {
    int left;
    int test;
    int right;
    int logic;
  };

  struct sValue {
    std::string string;
    int number;
    int type;
  };

  struct sBlock {
    int code;
    std::vector< std::vector<sOperand> > expressions;
    std::vector<sCondition> conditional;
    std::map<std::string, sValue> fields;
    sValue value;
    std::vector<std::string> strings;
  };

  struct sToken {
    std::string token;
    int line_no;
    std::string line;
  };

  struct sParse_Obj {
    int code;
    std::string pattern;
  };

  struct sField {
    std::string name;
    int value;
    std::string vplaceholder;
  };

  struct sList {
    int address;
    int index;
    std::string field;
    std::string addr_placeholder;
    std::string index_placeholder;
  };

  struct sHash {
    int address;
    int key;
    std::string addr_placeholder;
    std::string key_placeholder;
  };

  struct sText {
    std::string string;
    int x;
    int y;
    sColor color;
  };

  struct sImage {
    std::string name;
    int x;
    int y;
    int angle;
    int scale;
    int opacity;
    std::string layer;
    int width;
    int height;
    bool flip_x;
    bool flip_y;
  };

  struct sSound {
    std::string name;
    std::string mode;
  };

  struct sInput {
    bool left;
    bool right;
    bool up;
    bool down;
    bool action;
    bool fire_1;
    bool fire_2;
    bool fire_3;
    bool start;
    bool select;
    bool l_button;
    bool r_button;
  };

  struct sPoint {
    int x;
    int y;
  };

  struct sBox {
    int left;
    int top;
    int right;
    int bottom;
  };

  class cUtility {

    public:
      enum Types {
        TYPE_EMPTY,
        TYPE_NUMBER,
        TYPE_STRING,
        TYPE_VALUE,
        TYPE_FIELD,
        TYPE_LIST,
        TYPE_HASH
      };
      
      double pi;
      std::string root;

      cUtility();
      std::vector<std::string> Split_File(std::string name);
      std::vector<std::string> Split_Line(std::string line);
      std::string Replace_Token(std::string token, std::string replacement, std::string line);
      std::string Replace_All(std::string token, std::string replacement, std::string line);
      std::vector<std::string> Split_String(std::string token, std::string string);
      bool Is_Identifier(std::string token);
      bool Is_Positive_Number(std::string token);
      std::string To_String(int number);
      bool Match(std::string pattern, std::string string);
      void Set_Number(sValue& value, int number);
      void Set_String(sValue& value, std::string string);
      void Set_Field_Number(std::map<std::string, sValue>& object, std::string field, int number);
      void Set_Field_String(std::map<std::string, sValue>& object, std::string field, std::string string);
      bool Does_Field_Exist(std::map<std::string, sValue>& object, std::string field);
      std::string Trim(std::string string);
      void Set_Root(std::string root);
      void Timeout(int timeout);
      std::string Write_Object(std::map<std::string, sValue>& object);

  };

  class cConsole: public cUtility {

    public:
      std::map<int, sInput> inputs;
      std::vector<sText> texts;
      std::vector<sImage> images;
      std::vector<sSound> sounds;
      std::vector<sSound> tracks;
      std::vector<std::string> resources;
      bool ready;
      int screen_w;
      int screen_h;
      cAllegro* allegro;

      cConsole(cAllegro* allegro);
      void Output_Text(std::string text, int x, int y, sColor color);
      void Load_File(std::string file, sBlock* memory, int memory_size, int offset);
      void Save_File(std::string name, sBlock* memory, int memory_size, int offset, int count);
      void Draw_Image(std::string name, int x, int y, int scale, int angle, bool flip_x, bool flip_y, std::string layer);
      void Play_Sound(std::string name, std::string mode);
      void Play_Track(std::string name, std::string mode);
      void Detect_Collision(std::map<std::string, sValue>& sprite, std::map<std::string, sValue>& other, std::map<std::string, sValue>& results);
      void Focus_Camera(std::map<std::string, sValue>& camera, std::map<std::string, sValue>& sprite);
      void Update_Output();
      void Load_Resource(std::string resource);
      void Clear_Input(sInput& input);
      bool Point_In_Box(sPoint point, sBox box);
      void Upload_Resources();
      void Read_Input(int input, sBlock* memory, int memory_size, int offset);

  };

  class cC_Lesh: public cConsole {

    public:
      enum Settings {
        TIMEOUT = 20
      };
      enum Commands {
        CMD_DATA,
        CMD_TEST,
        CMD_MOVE,
        CMD_CALL,
        CMD_RETURN,
        CMD_STOP,
        CMD_SET,
        CMD_OUTPUT,
        CMD_LOAD,
        CMD_SAVE,
        CMD_DRAW,
        CMD_PLAY,
        CMD_MUSIC,
        CMD_INPUT,
        CMD_COLLISION,
        CMD_FOCUS,
        CMD_UPDATE,
        CMD_TIMEOUT,
        CMD_RESOURCE,
        CMD_UPLOAD
      };
      enum Operators {
        OPER_ADD = 1,
        OPER_SUBTRACT,
        OPER_MULTIPLY,
        OPER_DIVIDE,
        OPER_REMAINDER,
        OPER_CONCAT,
        OPER_RANDOM,
        OPER_COSINE,
        OPER_SINE
      };
      enum Conditions {
        COND_EQ = 1,
        COND_NE,
        COND_LT,
        COND_GT,
        COND_LE,
        COND_GE
      };
      enum Logics {
        LOGIC_AND = 1,
        LOGIC_OR
      };

      std::map<std::string, sValue> symtab;
      sBlock* memory;
      int memory_size;
      std::stack<int> stack;
      int prgm_counter;
      std::vector<sToken> tokens;
      std::map<std::string, sParse_Obj> parse_table;
      std::map<std::string, int> code_table;
      std::vector<std::string> debug_symbols;
      sToken last_token;
      bool compiled;
      int time;
      cAllegro* allegro;
      bool done;

      cC_Lesh(int memory_size, cAllegro* allegro);
      ~cC_Lesh();
      void Compile(std::string name);
      void Parse_Tokens(std::string name);
      std::vector<sOperand> Parse_Expression();
      sOperand Parse_Operand();
      sCondition Parse_Condition(sBlock& block);
      std::string Parse_Test();
      std::vector<sCondition> Parse_Conditional(sBlock& block);
      void Parse_Command();
      void Clear_Block(sBlock& block);
      void Generate_Error(std::string message);
      sToken Parse_Token();
      sToken Peek_Token();
      void Parse_Keyword(std::string keyword);
      bool Is_Number();
      int Parse_Number();
      bool Is_Logic();
      bool Is_Operator();
      bool Is_Address();
      int Parse_Address();
      bool Is_Field();
      sField Parse_Field();
      bool Is_Hash();
      sHash Parse_Hash();
      bool Is_List();
      sList Parse_List();
      bool Is_String();
      std::string Parse_String();
      bool Is_Num_Placeholder();
      bool Is_Addr_Placeholder();
      std::string Parse_Addr_Placeholder();
      bool Is_Field_Placeholder();
      sField Parse_Field_Placeholder();
      bool Is_Hash_Placeholder();
      sHash Parse_Hash_Placeholder();
      bool Is_List_Placeholder();
      sList Parse_List_Placeholder();
      void Replace_Symbols();
      void Replace_Expression(std::vector<sOperand>& expression);
      int Replace_Symbol(std::string name);
      std::string Preprocess(std::string name);
      std::string Preprocess_Lines(std::vector<std::string> lines);
      void Interpret();
      bool Valid_Address(int address);
      void Read_Write_Memory(int address, std::string field, sValue& data);
      sValue Eval_Expression(sBlock& block, int expression_id);
      void Eval_Operand(sOperand& operand, sValue& result);
      bool Eval_Condition(sCondition& condition, sBlock& block);
      bool Eval_Conditional(sBlock& block);
      void Execute();

  };
  
  class cAllegro: public cUtility {
  
    public:
      enum Settings {
        AXIS_X = 0,
        AXIS_Y = 1,
        TIMER_MAX = 200,
        BUTTONS_START = 4,
        KEYBOARD_CTRL = -1,
        WINDOW_W = 400,
        WINDOW_H = 300,
        FONT_SIZE = 24
      };
      const float RADIAN = 0.01745329;
      
      ALLEGRO_BITMAP* screen;
      ALLEGRO_DISPLAY* display;
      ALLEGRO_FONT* font;
      ALLEGRO_EVENT_QUEUE* event_queue;
      int screen_w;
      int screen_h;
      std::map<std::string, ALLEGRO_BITMAP*> images;
      std::map<std::string, ALLEGRO_SAMPLE*> sounds;
      std::map<std::string, ALLEGRO_SAMPLE_ID*> sound_ids;
      std::map<std::string, ALLEGRO_AUDIO_STREAM*> tracks;
      std::map<std::string, ALLEGRO_MIXER*> mixers;
      std::map<ALLEGRO_JOYSTICK*, int> gamepads;
      std::vector<int> button_map;
      std::vector<std::string> button_names;
      bool button_map_loaded;
      int button_index;
      int button_count;
    
      cAllegro();
      ~cAllegro();
      void Create_Screen(int width, int height);
      void Render_Images(std::vector<sImage>& images);
      void Render_Layer(std::vector<sImage>& images, std::string layer);
      void Output_Sounds(std::vector<sSound>& sounds);
      void Output_Tracks(std::vector<sSound>& tracks);
      void Output_Texts(std::vector<sText>& texts);
      void Load_Resources(std::vector<std::string>& resources);
      void Load_Font(std::string name);
      void Clear_Screen();
      void Render_Screen();
      void Create_Inputs(cConsole* console);
      void Delete_Inputs(cConsole* console);
      void Create_Keyboard_Input(cConsole* console);
      void Process_Messages(cC_Lesh* c_lesh);
      void Process_Control_Pad(ALLEGRO_EVENT& event, cConsole* console);
      void Process_Gamepad(ALLEGRO_EVENT& event, cConsole* console, bool down);
      void Process_Keyboard(ALLEGRO_EVENT& event, cConsole* console, bool down);
      void Load_Button_Map(std::string name);
      void Save_Button_Map(std::string name);
      void Load_Button_Defs(std::string name);
      void Update_Button_Disp();
      void Select_Gamepad_Button(ALLEGRO_EVENT& event);
  
  };

}

#endif
