#include "ui.h"

#include "SDL2/SDL.h"


namespace eng {

Ui::~Ui()
{
	Done();
}

bool Ui::Init()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return false;

	return true;
}

void Ui::Done()
{
	SDL_Quit();
}

}