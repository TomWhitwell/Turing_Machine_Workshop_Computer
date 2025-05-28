#pragma once
#include "ComputerCard.h"
#include "Clock.h"
#include "UI.h"

class MainApp : public ComputerCard
{
public:
    MainApp();
    virtual void ProcessSample() override;

    void PulseLed1(bool status);
    void PulseLed2(bool status);

    Clock clk;
    UI ui;
};
