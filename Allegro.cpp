#include "C_Lesh.hpp"

namespace Codeloader {

  /**
   * Creates and initializes the Allegro subsystem.
   */
  cAllegro::cAllegro() : cUtility() {
    this->display = NULL;
    this->screen = NULL;
    this->font = NULL;
    this->button_index = 0;
    this->button_map_loaded = false;
    this->button_count = sizeof(sInput) - BUTTONS_START;
    this->event_queue = NULL;
    bool allegro_ok = al_init();
    if (!allegro_ok) {
      throw std::string("Allegro could not be initialized.");
    }
    bool audio_ok = al_install_audio();
    if (!audio_ok) {
      throw std::string("Could not install audio.");
    }
    bool codec_ok = al_init_acodec_addon();
    if (codec_ok) {
      throw std::string("Could not initialize codec.");
    }
    bool font_ok = al_init_font_addon();
    if (!font_ok) {
      throw std::string("Could not initialize font.");
    }
    bool ttf_font_ok = al_init_ttf_addon();
    if (!ttf_font_ok) {
      throw std::string("Could not initialize TTF font.");
    }
    bool image_ok = al_init_image_addon();
    if (!image_ok) {
      throw std::string("Could not initialize image loader.");
    }
    bool primitives_ok = al_init_primitives_addon();
    if (!primitives_ok) {
      throw std::string("Could not initialize primitives.");
    }
    // Initialize input.
    bool keyboard_ok = al_install_keyboard();
    if (!keyboard_ok) {
      throw std::string("Keyboard was not initialized.");
    }
    bool gamepad_ok = al_install_joystick();
    if (!gamepad_ok) {
      throw std::string("Could not initialize gamepad.");
    }
    // Create the display.
    al_set_new_display_flags(ALLEGRO_WINDOWED);
    al_set_new_display_option(ALLEGRO_VSYNC, 1, ALLEGRO_SUGGEST);
    this->display = al_create_display(WINDOW_W, WINDOW_H);
    if (!this->display) {
      throw std::string("Could not initialize display.");
    }
  }
  
  /**
   * Does cleanup for the Allegro subsystem.
   */
  cAllegro::~cAllegro() {
    al_uninstall_audio();
    if (this->display) {
      al_destroy_display(this->display);
    }
    if (this->screen) {
      al_destroy_bitmap(this->screen);
    }
    for (std::map<std::string, ALLEGRO_BITMAP*>::iterator i = this->images.begin(); i != this->images.end(); ++i) {
      al_destroy_bitmap(i->second);
    }
    for (std::map<std::string, ALLEGRO_SAMPLE*>::iterator i = this->sounds.begin(); i != this->sounds.end(); ++i) {
      al_destroy_sample(i->second);
    }
    /*
    for (std::map<std::string, ALLEGRO_SAMPLE_ID*>::iterator i = this->sound_ids.begin(); i != this->sound_ids.end(); ++i) {
      al_destroy_sample_instance(i->second);
    }
    */
    for (std::map<std::string, ALLEGRO_MIXER*>::iterator i = this->mixers.begin(); i != this->mixers.end(); ++i) {
      al_destroy_mixer(i->second);
    }
    if (this->font) {
      al_destroy_font(this->font);
    }
    if (this->event_queue) {
      // Clean up event queue.
      al_destroy_event_queue(event_queue);
    }
  }
  
  /**
   * Creates or recreates the screen if it is not present.
   * @param width The width of the screen.
   * @param height The height of the screen.
   */
  void cAllegro::Create_Screen(int width, int height) {
    if (this->screen) {
      al_destroy_bitmap(this->screen);
    }
    al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
    this->screen = al_create_bitmap(width, height);
    this->Clear_Screen();
  }
  
  /**
   * Renders images from a vector.
   * @param images The list of image objects to render.
   */
  void cAllegro::Render_Images(std::vector<sImage>& images) {
    this->Clear_Screen();
    this->Render_Layer(images, "background");
    this->Render_Layer(images, "platform");
    this->Render_Layer(images, "character");
    this->Render_Layer(images, "foreground");
    this->Render_Layer(images, "overlay");
    images.clear(); // Clear out image stack.
    // Draw the screen.
    this->Render_Screen();
  }
  
  /**
   * Renders a layer given the name of the layer.
   * @param images The list of images to render in the layer.
   * @param layer The layer to render.
   */
  void cAllegro::Render_Layer(std::vector<sImage>& images, std::string layer) {
    int image_count = images.size();
    for (int image_index = 0; image_index < image_count; image_index++) {
      sImage& image = images[image_index];
      if ((this->images.find(image.name) != this->images.end()) && (image.layer == layer)) {
        ALLEGRO_BITMAP* bitmap = this->images[image.name];
        int flags = 0;
        if (image.flip_x) {
          flags |= ALLEGRO_FLIP_HORIZONTAL;
        }
        if (image.flip_y) {
          flags |= ALLEGRO_FLIP_VERTICAL;
        }
        if (image.angle > 0) {
          if (image.scale > 1) {
            al_draw_scaled_rotated_bitmap(bitmap, image.width / 2, image.height / 2, image.x, image.y, image.scale, image.scale, (float)image.angle * cAllegro::RADIAN, flags);
          }
          else {
            al_draw_rotated_bitmap(bitmap, image.width / 2, image.height / 2, image.x, image.y, image.angle * cAllegro::RADIAN, flags);
          }
        }
        else {
          if (image.scale > 1) {
            al_draw_scaled_bitmap(bitmap, 0, 0, image.width, image.height, image.x, image.y, image.width * image.scale, image.height * image.scale, flags);
          }
          else {
            al_draw_bitmap(bitmap, image.x, image.y, flags);
          }
        }
      }
    }
  }
  
  /**
   * Outputs sounds from the stack.
   * @param sounds The list of sounds to output.
   */
  void cAllegro::Output_Sounds(std::vector<sSound>& sounds) {
    int sound_count = sounds.size();
    for (int sound_index = 0; sound_index < sound_count; sound_index++) {
      sSound& sound = sounds[sound_index];
      if (this->sounds.find(sound.name) != this->sounds.end()) {
        ALLEGRO_SAMPLE* sample = this->sounds[sound.name];
        ALLEGRO_SAMPLE_ID* sample_id = NULL;
        // Check for old sound ID and delete.
        if (this->sound_ids.find(sound.name) != this->sound_ids.end()) {
          // al_destroy_sample_instance(this->sound_ids[sound.name]);
          // Delete record.
          this->sound_ids.erase(sound.name);
        }
        if (sound.mode == "loop") {
          bool sound_ok = al_play_sample(sample, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, sample_id);
          if (sound_ok) {
            this->sound_ids[sound.name] = sample_id; // Replace this.
          }
        }
        else if (sound.mode == "play") {
          bool sound_ok = al_play_sample(sample, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, sample_id);
          if (sound_ok) {
            this->sound_ids[sound.name] = sample_id; // Replace this.
          }
        }
        else if (sound.mode == "stop") {
          if (this->sound_ids.find(sound.name) != this->sound_ids.end()) {
            al_stop_sample(this->sound_ids[sound.name]);
          }
        }
      }
    }
    sounds.clear();
  }
  
  /**
   * Outputs tracks from the stack. There are usually not too many.
   * @param tracks The list of tracks to output.
   */
  void cAllegro::Output_Tracks(std::vector<sSound>& tracks) {
    int track_count = tracks.size();
    for (int track_index = 0; track_index < track_count; track_index++) {
      sSound& track = tracks[track_index];
      if (this->tracks.find(track.name) != this->tracks.end()) {
        ALLEGRO_AUDIO_STREAM* audio = this->tracks[track.name];
        ALLEGRO_MIXER* mixer = this->mixers[track.name];
        al_rewind_audio_stream(audio);
        if (track.mode == "loop") {
          al_set_audio_stream_playmode(audio, ALLEGRO_PLAYMODE_LOOP);
          al_set_mixer_playing(mixer, true);
        }
        else if (track.mode == "play") {
          al_set_mixer_playing(mixer, true);
        }
        else if (track.mode == "stop") {
          al_set_mixer_playing(mixer, false);
        }
      }
    }
  }
  
  /**
   * Outputs texts that were sent to the stack.
   * @param texts All of the texts stored in the stacks.
   */
  void cAllegro::Output_Texts(std::vector<sText>& texts) {
    al_set_target_bitmap(this->screen);
    int text_count = texts.size();
    for (int text_index = 0; text_index < text_count; text_index++) {
      sText& text = texts[text_index];
      al_draw_text(this->font, al_map_rgb(text.color.red, text.color.green, text.color.blue), text.x, text.y, 0, text.string.c_str());
    }
  }
  
  /**
   * Loads resources into the corresponding resource hashes.
   * @param resources The list of resources to load. These are processed via extensions.
   * @throws An error if a resource could not be loaded.
   */
  void cAllegro::Load_Resources(std::vector<std::string>& resources) {
    int resource_count = resources.size();
    for (int res_index = 0; res_index < resource_count; res_index++) {
      std::string resource = resources[res_index];
      std::string ext = this->Replace_Token("^\\w+\\.", "", resource);
      std::string name = this->Replace_Token("\\.\\w+$", "", resource);
      if (ext == "png") {
        ALLEGRO_BITMAP* bitmap = al_load_bitmap(std::string(this->root + "/" + resource).c_str());
        if (bitmap) {
          this->images[name] = bitmap;
        }
        else {
          throw std::string("Could not load image " + name + ".");
        }
      }
      else if (ext == "wav") {
        ALLEGRO_SAMPLE* sound = al_load_sample(std::string(this->root + "/" + resource).c_str());
        if (sound) {
          this->sounds[name] = sound;
        }
        else {
          throw std::string("Could not load sound " + name + ".");
        }
      }
      else if (ext == "mp3") {
        ALLEGRO_AUDIO_STREAM* track = al_load_audio_stream(std::string(this->root + "/" + resource).c_str(), 50, 50);
        if (track) {
          ALLEGRO_MIXER* mixer = al_create_mixer(48000, ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_2);
          if (mixer) {
            al_attach_audio_stream_to_mixer(track, mixer);
            this->mixers[name] = mixer;
          }
          else {
            throw std::string("Could not create mixer for " + name + ".");
          }
        }
        else {
          throw std::string("Could not load track " + name + ".");
        }
      }
    }
  }
  
  /**
   * Loads the font for the game.
   * @param name The name of the font to load.
   * @throws An error if the font could not be loaded.
   */
  void cAllegro::Load_Font(std::string name) {
    ALLEGRO_FONT* font = al_load_font(std::string(this->root + "/" + name).c_str(), FONT_SIZE, 0);
    if (font) {
      this->font = font;
    }
    else {
      throw std::string("Could not load font.");
    }
  }
  
  /**
   * Clears the screen to white.
   */
  void cAllegro::Clear_Screen() {
    al_set_target_bitmap(this->screen);
    al_clear_to_color(al_map_rgb(255, 255, 255));
  }
  
  /**
   * This processes the event loop. It blocks until the app is exited.
   * @param c_lesh The C-Lesh interpreter.
   * @throws An error is something went wrong.
   */
  void cAllegro::Process_Messages(cC_Lesh* c_lesh) {
    bool done = false;
    bool paused = false;
    // Create event queue.
    this->event_queue = al_create_event_queue();
    // Set up event handlers.
    ALLEGRO_EVENT_SOURCE* keyboard_event = al_get_keyboard_event_source();
    ALLEGRO_EVENT_SOURCE* gamepad_event = al_get_joystick_event_source();
    ALLEGRO_EVENT_SOURCE* display_event = al_get_display_event_source(this->display);
    al_register_event_source(event_queue, keyboard_event);
    al_register_event_source(event_queue, gamepad_event);
    al_register_event_source(event_queue, display_event);
    // Create the input registers.
    this->Create_Inputs(c_lesh);
    this->Create_Keyboard_Input(c_lesh);
    // Load button definitions.
    this->Load_Button_Defs("Button_Defs.txt");
    // Load the button map.
    this->Load_Button_Map("Buttons.txt");
    if (!this->button_map_loaded) {
      this->Update_Button_Disp();
    }
    // Do the event loop.
    while (!done) {
      ALLEGRO_EVENT event;
      bool got_event = al_get_next_event(event_queue, &event);
      if (got_event && !paused) {
        switch (event.type) {
          case ALLEGRO_EVENT_JOYSTICK_CONFIGURATION:
            al_reconfigure_joysticks();
            // Recreate inputs.
            this->Delete_Inputs(c_lesh);
            this->Create_Inputs(c_lesh);
            break;
          case ALLEGRO_EVENT_JOYSTICK_AXIS:
            if (this->button_map_loaded) {
              this->Process_Control_Pad(event, c_lesh);
            }
            break;
          case ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN:
            if (this->button_map_loaded) {
              this->Process_Gamepad(event, c_lesh, true);
            }
            else {
              this->Select_Gamepad_Button(event);
              // Update the display.
              this->Update_Button_Disp();
            }
            break;
          case ALLEGRO_EVENT_JOYSTICK_BUTTON_UP:
            if (this->button_map_loaded) {
              this->Process_Gamepad(event, c_lesh, false);
            }
            break;
          case ALLEGRO_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
              throw std::string("Program break.");
            }
            this->Process_Keyboard(event, c_lesh, true);
            break;
          case ALLEGRO_EVENT_KEY_UP:
            this->Process_Keyboard(event, c_lesh, false);
            break;
          case ALLEGRO_EVENT_DISPLAY_CLOSE:
            done = true;
            break;
          case ALLEGRO_EVENT_DISPLAY_SWITCH_IN:
            paused = false;
            break;
          case ALLEGRO_EVENT_DISPLAY_SWITCH_OUT:
            paused = true;
            break;
        }
      }
      // Process C-Lesh here.
      if (this->button_map_loaded) {
        try {
          c_lesh->Execute(); // This does not block. It uses a timer interrupt to exit after a specified time.
          if (c_lesh->done) {
            done = true;
          }
        }
        catch (std::string error) {
          throw error;
        }
      }
    }
  }
  
  /**
   * Creates the inputs for the console based on the number of gamepads connected.
   * @param console The console object reference.
   */
  void cAllegro::Create_Inputs(cConsole* console) {
    int gamepad_count = al_get_num_joysticks();
    // Proceed with gamepad loading.
    for (int gp_index = 0; gp_index < gamepad_count; gp_index++) {
      ALLEGRO_JOYSTICK* gamepad = al_get_joystick(gp_index);
      this->gamepads[gamepad] = gp_index;
      sInput input;
      console->Clear_Input(input);
      console->inputs[gp_index] = input;
    }
  }
  
  /**
   * Deletes the inputs both in the console and Allegro.
   * @param console The console object reference.
   */
  void cAllegro::Delete_Inputs(cConsole* console) {
    this->gamepads.clear();
    console->inputs.clear();
  }
  
  /**
   * Creates a keyboard input.
   * @param console The console object reference.
   */
  void cAllegro::Create_Keyboard_Input(cConsole* console) {
    sInput input;
    console->Clear_Input(input);
    console->inputs[KEYBOARD_CTRL] = input; // -1 Reserved for the keyboard.
  }
  
  /**
   * Processes the control pad.
   * @param event The associated Allegro event.
   * @param console The console object.
   */
  void cAllegro::Process_Control_Pad(ALLEGRO_EVENT& event, cConsole* console) {
    if (this->gamepads.find(event.joystick.id) != this->gamepads.end()) {
      int input_id = this->gamepads[event.joystick.id];
      sInput& input = console->inputs[input_id];
      if (event.joystick.axis == AXIS_X) {
        if (event.joystick.pos < 0) {
          input.left = true;
        }
        else if (event.joystick.pos > 0) {
          input.right = true;
        }
        else {
          input.left = false;
          input.right = false;
        }
      }
      else if (event.joystick.axis == AXIS_Y) {
        if (event.joystick.pos < 0) {
          input.up = true;
        }
        else if (event.joystick.pos > 0) {
          input.down = true;
        }
        else {
          input.up = false;
          input.down = false;
        }
      }
    }
  }
  
  /**
   * Processes the gamepad buttons.
   * @param event The associated Allegro event.
   * @param console The console object.
   * @param down If the button was pressed down.
   */
  void cAllegro::Process_Gamepad(ALLEGRO_EVENT& event, cConsole* console, bool down) {
    if (this->gamepads.find(event.joystick.id) != this->gamepads.end()) {
      int input_id = this->gamepads[event.joystick.id];
      sInput& input = console->inputs[input_id];
      int button_count = this->button_map.size();
      for (int button_index = 0; button_index < button_count; button_index++) {
        int button = this->button_map[button_index];
        if (event.joystick.button == button) {
          bool* buttons = (bool*)(&input + BUTTONS_START); // Assuming dealing with bytes.
          buttons[button_index] = down;
          break;
        }
      }
    }
  }
  
  /**
   * Processes the keyboard keys. You cannot play the game with a keyboard.
   * @param event The associated Allegro event.
   * @param console The console object.
   * @param down If a key was pressed down.
   */
  void cAllegro::Process_Keyboard(ALLEGRO_EVENT& event, cConsole* console, bool down) {
    sInput& input = console->inputs[KEYBOARD_CTRL];
    switch (event.keyboard.keycode) {
      case ALLEGRO_KEY_LEFT:
        input.left = down;
        break;
      case ALLEGRO_KEY_RIGHT:
        input.right = down;
        break;
      case ALLEGRO_KEY_UP:
        input.up = down;
        break;
      case ALLEGRO_KEY_DOWN:
        input.down = down;
        break;
      case ALLEGRO_KEY_Z:
        input.action = down;
        break;
      case ALLEGRO_KEY_X:
        input.fire_1 = down;
        break;
      case ALLEGRO_KEY_C:
        input.fire_2 = down;
        break;
      case ALLEGRO_KEY_V:
        input.fire_3 = down;
        break;
      case ALLEGRO_KEY_ENTER:
        input.start = down;
        break;
      case ALLEGRO_KEY_RSHIFT:
        input.select = down;
        break;
      case ALLEGRO_KEY_A:
        input.l_button = down;
        break;
      case ALLEGRO_KEY_S:
        input.r_button = down;
        break;
    }
  }
  
  /**
   * Loads a button map for processing.
   * @param name The name of the button map.
   * @throws An error if the button map could not be loaded.
   */
  void cAllegro::Load_Button_Map(std::string name) {
    std::vector<std::string> records = this->Split_File(name);
    int rec_count = records.size();
    if (rec_count > 0) {
      if (rec_count != this->button_count) {
        throw std::string("Button map does not have the correct amount of buttons.");
      }
      for (int rec_index = 0; rec_index < rec_count; rec_count++) {
        std::string record = records[rec_index];
        if (this->Is_Positive_Number(record)) {
          int button = std::atoi(record.c_str());
          this->button_map.push_back(button);
        }
        else {
          throw std::string("Button map has invalid number.");
        }
      }
      this->button_map_loaded = true;
    }
  }
  
  /**
   * Saves a button map to disk.
   * @param name The name of the button map.
   * @throws An error if the button map could not be saved.
   */
  void cAllegro::Save_Button_Map(std::string name) {
    std::ofstream file(name.c_str());
    if (file) {
      int button_count = this->button_map.size();
      for (int button_index = 0; button_index < button_count; button_index++) {
        int button = this->button_map[button_index];
        file << button << std::endl;
      }
    }
    else {
      throw std::string("Could not save button map.");
    }
  }
  
  /**
   * Loads button definitions, i.e. button names.
   * @param name The name of the file to load from.
   * @throws An error if the button defs could not be loaded.
   */
  void cAllegro::Load_Button_Defs(std::string name) {
    this->button_names = this->Split_File(name);
    if (this->button_names.size() != this->button_count) {
      throw std::string("There are not the correct amount of button names.");
    }
  }
  
  /**
   * Updates the display for the buttons selected for the gamepad. It should display black text on a white background.
   */
  void cAllegro::Update_Button_Disp() {
    this->Clear_Screen();
    std::string text = "Please select the " + this->button_names[this->button_index] + " button.";
    int width = al_get_text_width(this->font, text.c_str());
    int x = (this->screen_w - width) / 2;
    int y = this->screen_h / 2;
    al_draw_text(this->font, al_map_rgb(0, 0, 0), x, y, 0, text.c_str());
    this->Render_Screen();
  }
  
  /**
   * Renders the screen.
   */
  void cAllegro::Render_Screen() {
    ALLEGRO_BITMAP* backbuffer = al_get_backbuffer(this->display);
    al_set_target_bitmap(backbuffer);
    int width = al_get_bitmap_width(backbuffer);
    int height = al_get_bitmap_height(backbuffer);
    al_draw_scaled_bitmap(this->screen, 0, 0, this->screen_w, this->screen_h, 0, 0, width, height, 0);
    al_flip_display();
  }
  
  /**
   * Allows the user to select a gamepad button.
   * @param event The associated Allegro event.
   */
  void cAllegro::Select_Gamepad_Button(ALLEGRO_EVENT& event) {
    if (this->button_index < this->button_names.size()) {
      this->button_index++;
    }
    else {
      this->button_map_loaded = true;
      this->Save_Button_Map("Buttons.txt");
    }
    this->Timeout(200); // Wait a bit!
  }

}
