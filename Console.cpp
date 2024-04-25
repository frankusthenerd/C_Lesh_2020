#include "C_Lesh.hpp"

namespace Codeloader {

  /**
   * Initializes the console component of C-Lesh.
   * @param allegro The allegro object.
   */
  cConsole::cConsole(cAllegro* allegro) : cUtility() {
    this->ready = false;
    this->screen_w = 400;
    this->screen_h = 300;
    this->allegro = allegro;
    this->allegro->Create_Inputs(this);
    this->allegro->Create_Screen(this->screen_w, this->screen_h);
  }

  /**
   * Draws an image to the image stack.
   * @param name The name of the image.
   * @param x The x coordinate of the image.
   * @param y The y coordinate of the image.
   * @param scale The scale of the image.
   * @param angle The angle of rotation in degrees.
   * @param layer The image layer. Possible values are "overlay", "foreground", "character", "platform", or "background".
   */
  void cConsole::Draw_Image(std::string name, int x, int y, int scale, int angle, bool flip_x, bool flip_y, std::string layer) {
    sImage image;
    image.name = name;
    image.x = x;
    image.y = y;
    image.scale = scale;
    image.angle = angle;
    image.layer = layer;
    image.flip_x = flip_x;
    image.flip_y = flip_y;
    this->images.push_back(image);
  }

  /**
   * Plays a sound on the stack.
   * @param name The name of the sound.
   * @param mode Values can be "loop", "stop", or "play".
   */
  void cConsole::Play_Sound(std::string name, std::string mode) {
    sSound sound;
    sound.name = name;
    sound.mode = mode;
    this->sounds.push_back(sound);
  }
  
  /**
   * Plays a sound track.
   * @param name The name of the track.
   * @param mode Values can be "loop", "stop", or "play".
   */
  void cConsole::Play_Track(std::string name, std::string mode) {
    sSound sound;
    sound.name = name;
    sound.mode = mode;
    this->tracks.push_back(sound);
  }

  /**
   * Outputs text to the text stack.
   * @param text The text to output.
   * @param x The x coordinate of the text in pixels.
   * @param y The y coordinate of the text in pixels.
   * @param color The color of the text.
   */
  void cConsole::Output_Text(std::string text, int x, int y, sColor color) {
    sText t;
    t.string = text;
    t.x = x;
    t.y = y;
    t.color = color;
    this->texts.push_back(t);
  }

  /**
   * Clears the input state.
   * @param input The input to clear.
   */
  void cConsole::Clear_Input(sInput& input) {
    input.left = false;
    input.right = false;
    input.up = false;
    input.down = false;
    input.action = false;
    input.fire_1 = false;
    input.fire_2 = false;
    input.fire_3 = false;
    input.start = false;
    input.select = false;
    input.l_button = false;
    input.r_button = false;
  }

  /**
   * Determines if a point is in a box.
   * @param point The point object of x, y.
   * @param box The box object of left, top, right, bottom.
   * @return True if the point is in the box.
   */
  bool cConsole::Point_In_Box(sPoint point, sBox box) {
    return ((point.x >= box.left) && (point.x <= box.right) && (point.y >= box.top) && (point.y <= box.bottom));
  }

  /**
   * Detects a collision between a sprite and another sprite.
   *
   * Sprites should contain the following properties in order to get a good
   * collision detection:
   * %
   * {
   *   left: 0, // These are the dimensions of the hit box.
   *   top: 0,
   *   right: 10,
   *   bottom: 10,
   *   cdelta_x: 25, // The amount of padding for the hit box. This is percent.
   *   cdelta_y: 25,
   *   x: 0, // The sprite coordinates.
   *   y: 0,
   *   width: 10, // Dimensions of sprite.
   *   height: 10,
   *   size_x: 1, // Number of repeating units.
   *   size_y: 1,
   *   scale: 1
   * }
   * %
   * The results object is as follows:
   * %
   * {
   *   left: false, // Which face was hit relative to the sprite.
   *   top: false,
   *   right: false,
   *   bottom: false,
   *   center: false,
   *   left_corner: false,
   *   right_corner: false,
   *   x: 0, // The new suggested coordinates.
   *   y: 0
   * }
   * %
   * @param sprite The sprite that is colliding with the other object.
   * @param other The other sprite.
   * @param results The results object.
   * @throws An error if an object is incomplete.
   */
  void cConsole::Detect_Collision(std::map<std::string, sValue>& sprite, std::map<std::string, sValue>& other, std::map<std::string, sValue>& results) {
    this->Set_Field_Number(results, "left", 0);
    this->Set_Field_Number(results, "top", 0);
    this->Set_Field_Number(results, "right", 0);
    this->Set_Field_Number(results, "bottom", 0);
    this->Set_Field_Number(results, "center", 0);
    this->Set_Field_Number(results, "left_corner", 0);
    this->Set_Field_Number(results, "right_corner", 0);
    this->Set_Field_Number(results, "x", 0);
    this->Set_Field_Number(results, "y", 0);
    if (!this->Does_Field_Exist(sprite, "left") ||
        !this->Does_Field_Exist(sprite, "top") ||
        !this->Does_Field_Exist(sprite, "right") ||
        !this->Does_Field_Exist(sprite, "bottom") ||
        !this->Does_Field_Exist(sprite, "cdelta_x") ||
        !this->Does_Field_Exist(sprite, "cdelta_y") ||
        !this->Does_Field_Exist(sprite, "size_x") || 
        !this->Does_Field_Exist(sprite, "size_y") ||
        !this->Does_Field_Exist(sprite, "scale")) {
      throw std::string("Sprite object missing field in collision.");
    }
    if (!this->Does_Field_Exist(other, "left") ||
        !this->Does_Field_Exist(other, "top") ||
        !this->Does_Field_Exist(other, "right") ||
        !this->Does_Field_Exist(other, "bottom") ||
        !this->Does_Field_Exist(other, "x") ||
        !this->Does_Field_Exist(other, "y") ||
        !this->Does_Field_Exist(other, "size_x") || 
        !this->Does_Field_Exist(other, "size_y") ||
        !this->Does_Field_Exist(other, "scale")) {
      throw std::string("Other sprite object missing field in collision.");
    }
    int hmap_width = sprite["right"].number - sprite["left"].number + 1;
    int hmap_height = sprite["bottom"].number - sprite["top"].number + 1;
    int delta_x = (int)((float)hmap_width * ((float)sprite["cdelta_x"].number / 100.0));
    int delta_y = (int)((float)hmap_height * ((float)sprite["cdelta_y"].number / 100.0));
    // Create 12 collision points. The middle collision point is important.
    sPoint t1 = { sprite["left"].number + delta_x, sprite["top"].number };
    sPoint t2 = { sprite["right"].number - delta_x, sprite["top"].number };
    sPoint tc = { sprite["left"].number + (int)((float)hmap_width / 2.0), sprite["top"].number };
    sPoint l1 = { sprite["left"].number, sprite["top"].number + delta_y };
    sPoint l2 = { sprite["left"].number, sprite["bottom"].number - delta_y };
    sPoint lc = { sprite["left"].number, sprite["top"].number + (int)((float)hmap_height / 2.0) };
    sPoint r1 = { sprite["right"].number, sprite["top"].number + delta_y };
    sPoint r2 = { sprite["right"].number, sprite["bottom"].number - delta_y };
    sPoint rc = { sprite["right"].number, sprite["top"].number + (int)((float)hmap_height / 2.0) };
    sPoint b1 = { sprite["left"].number + delta_x, sprite["bottom"].number };
    sPoint b2 = { sprite["right"].number - delta_x, sprite["bottom"].number };
    sPoint bc = { sprite["left"].number + (int)((float)hmap_width / 2.0), sprite["bottom"].number };
    sPoint bl = { sprite["left"].number, sprite["bottom"].number };
    sPoint br = { sprite["right"].number, sprite["bottom"].number };
    // Determine which face was hit.
    sBox other_hmap;
    other_hmap.left = other["left"].number;
    other_hmap.top = other["top"].number;
    other_hmap.right = other["right"].number;
    other_hmap.bottom = other["bottom"].number;
    if (this->Point_In_Box(t1, other_hmap) || this->Point_In_Box(t2, other_hmap) || this->Point_In_Box(tc, other_hmap)) {
      results["top"].number = 1;
      results["center"].number = (int)this->Point_In_Box(tc, other_hmap);
      results["y"].number = other["y"].number + (other["height"].number * other["size_y"].number * other["scale"].number);
    }
    if (this->Point_In_Box(l1, other_hmap) || this->Point_In_Box(l2, other_hmap) || this->Point_In_Box(lc, other_hmap)) {
      results["left"].number = 1;
      results["center"].number = (int)this->Point_In_Box(lc, other_hmap);
      results["x"].number = other["x"].number + (other["width"].number * other["size_x"].number * other["scale"].number);
    }
    if (this->Point_In_Box(r1, other_hmap) || this->Point_In_Box(r2, other_hmap) || this->Point_In_Box(rc, other_hmap)) {
      results["right"].number = 1;
      results["center"].number = (int)this->Point_In_Box(rc, other_hmap);
      results["x"].number = other["x"].number - (sprite["width"].number * sprite["size_x"].number * sprite["scale"].number);
    }
    if (this->Point_In_Box(b1, other_hmap) || this->Point_In_Box(b2, other_hmap) || this->Point_In_Box(bc, other_hmap)) {
      results["bottom"].number = 1;
      results["center"].number = (int)this->Point_In_Box(bc, other_hmap);
      results["y"].number = other["y"].number - (sprite["height"].number * sprite["size_y"].number * sprite["scale"].number);
      // Also detect bottom right and bottom left hit.
      results["left_corner"].number = (int)this->Point_In_Box(bl, other_hmap);
      results["right_corner"].number = (int)this->Point_In_Box(br, other_hmap);
    }
  }

  /**
   * Focuses the camera on a sprite. The camera and the sprite should have the
   * following fields:
   * %
   *   camera = {
   *     x: 0, // Camera left corner.
   *     y: 0,
   *     limit_x: 0, // The size of the level.
   *     limit_y: 0,
   *     upper_bound: 0, // The coordinate of the highest screen at its top.
   *     bkg_x1: 0, // Background coordinates.
   *     bkg_x2: 0,
   *     bkg_y1: 0,
   *     bkg_y2: 0,
   *     x_speed: 0, // The speeds of the background.
   *     y_speed: 0,
   *     x_direction: 0, // The directions of the background.
   *     y_direction: 0
   *   }
   * %
   * %
   *   sprite = {
   *     x: 0,
   *     y: 0,
   *     width: 10,
   *     height: 10
   *   }
   * %
   * @param camera The camera object.
   * @param sprite The sprite object.
   */
  void cConsole::Focus_Camera(std::map<std::string, sValue>& camera, std::map<std::string, sValue>& sprite) {
    if (!this->Does_Field_Exist(camera, "x") ||
        !this->Does_Field_Exist(camera, "y") ||
        !this->Does_Field_Exist(camera, "limit_x") ||
        !this->Does_Field_Exist(camera, "limit_y") ||
        !this->Does_Field_Exist(camera, "upper_bound") ||
        !this->Does_Field_Exist(camera, "bkg_x1") ||
        !this->Does_Field_Exist(camera, "bkg_x2") ||
        !this->Does_Field_Exist(camera, "bkg_y1") ||
        !this->Does_Field_Exist(camera, "bkg_y2") ||
        !this->Does_Field_Exist(camera, "x_speed") ||
        !this->Does_Field_Exist(camera, "y_speed") ||
        !this->Does_Field_Exist(camera, "x_direction") ||
        !this->Does_Field_Exist(camera, "y_direction")) {
      throw std::string("Camera is missing field in focus.");
    }
    if (!this->Does_Field_Exist(sprite, "x") ||
        !this->Does_Field_Exist(sprite, "y") ||
        !this->Does_Field_Exist(sprite, "width") ||
        !this->Does_Field_Exist(sprite, "height")) {
      throw std::string("Sprite is missing field in focus.");
    }
    // Focus on x.
    int screen_cx = (this->screen_w - sprite["width"].number) / 2;
    int screen_right = camera["limit_x"].number - screen_cx;
    int sprite_right = sprite["x"].number + sprite["width"].number - 1;
    int dx = screen_right - screen_cx;
    if (sprite["x"].number < screen_cx) { // Far left.
      camera["x"].number = 0;
    }
    else if (sprite_right > screen_right) { // Far right.
      camera["x"].number = camera["limit_x"].number - this->screen_w;
    }
    else if ((sprite["x"].number >= screen_cx) && (sprite_right <= screen_right) && (dx > sprite["width"].number)) {
      camera["x"].number = sprite["x"].number - screen_cx;
      // Scroll backdrop.
      camera["bkg_x1"].number += (camera["x_speed"].number * -camera["x_direction"].number);
      if ((camera["bkg_x1"].number > 0) && (camera["bkg_x1"].number < this->screen_w)) {
        camera["bkg_x2"].number = camera["bkg_x1"].number - this->screen_w;
      }
      else if (camera["bkg_x1"].number >= this->screen_w) {
        camera["bkg_x1"].number = 0;
        camera["bkg_x2"].number = 0;
      }
      else if ((camera["bkg_x1"].number < 0) && (camera["bkg_x1"].number > -this->screen_w)) {
        camera["bkg_x2"].number = camera["bkg_x1"].number + this->screen_w;
      }
      else if (camera["bkg_x1"].number <= -this->screen_w) {
        camera["bkg_x1"].number = 0;
        camera["bkg_x2"].number = 0;
      }
    }
    // Focus on y.
    int screen_cy = (this->screen_h - sprite["height"].number) / 2;
    int screen_top = camera["upper_bound"].number + screen_cy;
    int screen_bottom = camera["limit_y"].number - screen_cy;
    int sprite_bottom = sprite["y"].number + sprite["height"].number - 1;
    int dy = screen_bottom - screen_top;
    if (sprite["y"].number < screen_top) {
      camera["y"].number = camera["upper_bound"].number;
    }
    else if (sprite_bottom > screen_bottom) {
      camera["y"].number = camera["limit_y"].number - this->screen_h;
    }
    else if ((sprite["y"].number >= screen_top) && (sprite_bottom <= screen_bottom) && (dy > this->screen_h)) {
      camera["y"].number = sprite["y"].number - screen_cy;
      // Scroll backdrop.
      if ((screen_bottom - screen_top) > screen_cy) {
        camera["bkg_y1"].number += (camera["y_speed"].number * -camera["y_direction"].number);
      }
      if ((camera["bkg_y1"].number > 0) && (camera["bkg_y1"].number < this->screen_h)) {
        camera["bkg_y2"].number = camera["bkg_y1"].number - this->screen_h;
      }
      else if (camera["bkg_y1"].number >= this->screen_h) {
        camera["bkg_y1"].number = 0;
        camera["bkg_y2"].number = 0;
      }
      else if ((camera["bkg_y1"].number < 0) && (camera["bkg_y1"].number > -this->screen_h)) {
        camera["bkg_y2"].number = camera["bkg_y1"].number + this->screen_h;
      }
      else if (camera["bkg_y1"].number <= -this->screen_h) {
        camera["bkg_y1"].number = 0;
        camera["bkg_y2"].number = 0;
      }
    }
  }
  
  /**
   * Loads a file into memory. Files a formatted with name=value pairs separated
   * by commas. Each line represents a record.
   * @param name The name of the file.
   * @param memory The memory that the file will be loaded onto.
   * @param memory_size The number of blocks in the memory.
   * @param offset Which block to start loading the file to.
   * @throws An error if the file could not be opened or invalid memory address.
   */
  void cConsole::Load_File(std::string name, sBlock* memory, int memory_size, int offset) {
    std::vector<std::string> records = this->Split_File(this->root + "/" + name);
    int record_count = records.size();
    for (int record_index = 0; record_index < record_count; record_index++) {
      int address = offset + record_index;
      std::string record = records[record_index];
      if ((address > 0) && (address < memory_size)) {
        std::vector<std::string> pairs = this->Split_String("\\s*,\\s*", record);
        int pair_count = pairs.size();
        for (int pair_index = 0; pair_index < pair_count; pair_index++) {
          std::vector<std::string> pair = this->Split_String("\\s*=\\s*", pairs[pair_index]);
          if (pair.size() == 2) { // Is this a pair?
            std::string name = this->Trim(pair[0]);
            std::string value = this->Trim(pair[1]);
            if (this->Is_Positive_Number(value)) {
              this->Set_Field_Number(memory[address].fields, name, std::atoi(value.c_str()));
            }
            else {
              this->Set_Field_String(memory[address].fields, name, value);
            }
          }
          else {
            throw std::string("Invalid pair format in " + name + ".");
          }
        }
      }
      else {
        throw std::string(name + " is too big to fit into the memory.");
      }
    }
  }
  
    /**
   * Saves a portion of memory to a file.
   * @param name The name of the file to write to.
   * @param memory The memory to write to the file.
   * @param memory_size The number of blocks in memory.
   * @param offset Where in the memory to write from.
   * @param count The number of blocks to write.
   * @throws An error if the file could not be written.
   */
  void cConsole::Save_File(std::string name, sBlock* memory, int memory_size, int offset, int count) {
    int limit = offset + count;
    std::string data = "";
    for (int index = offset; index < limit; index++) {
      if ((index > 0) && (index < memory_size)) {
        std::map<std::string, sValue>& object = memory[index].fields;
        data += std::string(this->Write_Object(object) + "\n");
      }
      else {
        throw std::string(name + " is accessing non-existant portions of memory.");
      }
    }
    std::ofstream file(std::string(this->root + "/" + name).c_str());
    if (file) {
      int length = data.length();
      for (int ch_index = 0; ch_index < length; ch_index++) {
        char ch = data[ch_index];
        file.put(ch);
      }
    }
    else {
      throw std::string("Could not write file " + name + ".");
    }
  }
  
  /**
   * Updates the output of the Allegro subsystem.
   */
  void cConsole::Update_Output() {
    this->allegro->Clear_Screen();
    this->allegro->Render_Images(this->images);
    this->allegro->Output_Sounds(this->sounds);
    this->allegro->Output_Texts(this->texts);
  }
  
  /**
   * Loads a resource to the stack for the loading to the client.
   * @param resource The name of the resource to load.
   */
  void cConsole::Load_Resource(std::string resource) {
    this->resources.push_back(resource);
  }
  
  /**
   * Uploads resources to the Allegro subsystem for processing.
   */
  void cConsole::Upload_Resources() {
    this->allegro->Load_Resources(this->resources);
  }

  /**
   * Reads input from input array.
   * @param input The input ID.
   * @param memory The memory where the input will be stored.
   * @param memory_size The size of the memory.
   * @param offset The offset where to store the input.
   * @throws An error if the memory is being stored in an invalid location.
   */
  void cConsole::Read_Input(int input, sBlock* memory, int memory_size, int offset) {
    if ((offset >= 0) && (offset < memory_size)) {
      sBlock& block = memory[offset];
      sInput& buttons = this->inputs[input];
      this->Set_Field_Number(block.fields, "left", buttons.left);
      this->Set_Field_Number(block.fields, "right", buttons.right);
      this->Set_Field_Number(block.fields, "up", buttons.up);
      this->Set_Field_Number(block.fields, "down", buttons.down);
      this->Set_Field_Number(block.fields, "action", buttons.action);
      this->Set_Field_Number(block.fields, "fire_1", buttons.fire_1);
      this->Set_Field_Number(block.fields, "fire_2", buttons.fire_2);
      this->Set_Field_Number(block.fields, "fire_3", buttons.fire_3);
      this->Set_Field_Number(block.fields, "start", buttons.start);
      this->Set_Field_Number(block.fields, "select", buttons.select);
      this->Set_Field_Number(block.fields, "l_button", buttons.l_button);
      this->Set_Field_Number(block.fields, "r_button", buttons.r_button);
    }
    else {
      throw std::string("Cannot store input in invalid memory location.");
    }
  }

}
