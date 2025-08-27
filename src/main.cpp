#include "simulation.h"


int main()
{
    SIM simulator;
    simulator.initialize();
    simulator.run_main(); // main-loop
    return 0;
}
