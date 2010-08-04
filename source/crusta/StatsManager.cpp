#include <crusta/StatsManager.h>

#include <crusta/QuadNodeData.h>

using namespace std;


BEGIN_CRUSTA

StatsManager statsMan;

Timer    StatsManager::timers[5];
#if CRUSTA_RECORD_STATS
ofstream StatsManager::file("timings.txt");
#else
ofstream StatsManager::file;
#endif //CRUSTA_RECORD_STATS

int StatsManager::numTiles           = 0;
int StatsManager::numSegments        = 0;
int StatsManager::maxSegmentsPerTile = 0;
int StatsManager::numData            = 0;


StatsManager::
~StatsManager()
{
    file.close();
}

void StatsManager::
StatsManager::
newFrame()
{
#if CRUSTA_RECORD_STATS
    //dump times
    for (int i=0; i<numTimers; ++i)
    {
        timers[i].stop();
        file << timers[i].seconds() << ",";
        timers[i].reset();
    }
    //dump usage stats
    file << numTiles <<","<< numSegments <<","<< maxSegmentsPerTile <<","<<
            numData <<","<< endl;
    numTiles = numSegments = maxSegmentsPerTile = numData = 0;

    timers[0].resume();
#endif //CRUSTA_RECORD_STATS
}

void StatsManager::
start(Stat stat)
{
#if CRUSTA_RECORD_STATS
    timers[stat].resume();
#endif //CRUSTA_RECORD_STATS
}

void StatsManager::
stop(Stat stat)
{
#if CRUSTA_RECORD_STATS
    timers[stat].stop();
#endif //CRUSTA_RECORD_STATS
}

void StatsManager::
extractTileStats(std::vector<QuadNodeMainData*>& nodes)
{
#if CRUSTA_RECORD_STATS
    typedef std::vector<QuadNodeMainData*>                     Nodes;
    typedef QuadNodeMainData::ShapeCoverage                    Coverage;

    numTiles = static_cast<int>(nodes.size());

    //go through all the nodes provided
    for (Nodes::iterator nit=nodes.begin(); nit!=nodes.end(); ++nit)
    {
        QuadNodeMainData* node     = *nit;
        Coverage&         coverage = node->lineCoverage;

        if (coverage.empty())
            continue;

        ++numData;

        for (Coverage::iterator lit=coverage.begin();lit!=coverage.end();++lit)
        {
            int segsInTile     = static_cast<int>(lit->second.size());
            numSegments       += segsInTile;
            maxSegmentsPerTile = std::max(maxSegmentsPerTile, segsInTile);
        }
    }
#endif //CRUSTA_RECORD_STATS
}

END_CRUSTA
