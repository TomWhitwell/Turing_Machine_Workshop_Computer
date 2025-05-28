

#include "ComputerCard.h"
#include "Clock.h"
#include "UI.h"
#include "MainApp.h"

int main()
{
    MainApp app;
    app.EnableNormalisationProbe();
    app.Run(); // Starts the main audio loop
}
