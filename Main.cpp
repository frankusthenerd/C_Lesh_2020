#include "C_Lesh.hpp"

int main(int argc, char** argv) {
  Codeloader::cAllegro* allegro = NULL;
  Codeloader::cC_Lesh* c_lesh = NULL;
  if (argc == 3) {
    std::string game = argv[1];
    int memory_size = std::atoi(argv[2]);
    try {
      allegro = new Codeloader::cAllegro();
      allegro->Set_Root(game);
      allegro->Load_Font("Game.ttf");
      c_lesh = new Codeloader::cC_Lesh(memory_size, allegro);
      c_lesh->Set_Root(game);
      c_lesh->Compile(game + ".clsh");
      allegro->Process_Messages(c_lesh); // Block here.
    }
    catch (std::string error) {
      std::cout << "Error: " << error.c_str() << std::endl;
    }
    if (allegro) {
      delete allegro;
    }
  }
  else {
    std::cout << "Usage: " << argv[0] << " <game> <memory>" << std::endl;
  }
  std::cout << "Done." << std::endl;
  return 0;
}
