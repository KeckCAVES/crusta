#ifndef _StatsManager_H_
#define _StatsManager_H_

#include <fstream>

#include <crusta/basics.h>
#include <crusta/Timer.h>

#define CRUSTA_RECORD_STATS 0


BEGIN_CRUSTA


class QuadNodeMainData;

class StatsManager
{
protected:
    static const int     numTimers = 5;
    static Timer         timers[numTimers];
    static std::ofstream file;

    static int numTiles;
    static int numSegments;
    static int maxSegmentsPerTile;
    static int numData;
    static int numDataUpdated;

public:
    enum Stat
    {
        INHERITSHAPECOVERAGE=1,
        UPDATELINEDATA,
        PROCESSVERTICALSCALE,
        EDITLINE
    };

    ~StatsManager();

    static void newFrame();

    static void start(Stat stat);
    static void stop(Stat stat);

    static void extractTileStats(std::vector<QuadNodeMainData*>& nodes);
    static void incrementDataUpdated();
};


extern StatsManager statsMan;


END_CRUSTA


#endif //_StatsManager_H_
