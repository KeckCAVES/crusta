#include <GlobalGrid.h>

BEGIN_CRUSTA

void GlobalGrid::
registerScopeCallback(gridProcessing::Phase phase,
                      const gridProcessing::ScopeCallback callback)
{
    switch (phase)
    {
        case gridProcessing::UPDATE_PHASE:
            updateCallbacks.push_back(callback);
            break;
        case gridProcessing::DISPLAY_PHASE:
            displayCallbacks.push_back(callback);
            break;

        default:
            break;
    }
}

END_CRUSTA
