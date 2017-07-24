#ifndef __CELLULARAUTOMATONMODEL_GA_H__
#define __CELLULARAUTOMATONMODEL_GA_H__

#include "cellularAutomatonModel.h"
#include "geneticAlgorithm.h"

class CellularAutomatonModel_GA : public CellularAutomatonModel {
public:
	CellularAutomatonModel_GA();
	~CellularAutomatonModel_GA() { delete mGA; }
	void update();
	void save();
	void draw();

private:
	GeneticAlgorithm *mGA;
	std::vector<Genome> mPopulation;
	bool mHasConverged;
	int mUnchangedBestCount, mTerminationCount;
	double mLastBestFitness, mImprovementThresh;

	array2i mBlocksDim;               // [0]: width, [1]: height
	arrayNi mBlocks;                  // store block ID of every cell
	arrayNi mEvacPlan;                // store exit ID for every block (have the same size as one chromosome)
	arrayNi mAgentGoals;              // store exit ID for every agent (grow or shrink as simStep() goes)
	std::vector<array3f> mExitColors; // store the color of every exit
	int mTotalTimesteps;              // accumulate every agent's evacuation time

	void read( const char *fileName );
	void print();
	void reset();
	void assignExitColors();
	void initPopulation();
	void evaluatePopulation();
	void generateGoalsFromPlan( const arrayNi &plan );
	void GAStep();
	void simStep_GA( std::vector<array2i> &agents, arrayNb &isAlive, int &numAliveAgents );
	void simStep();
	void moveAgents( array2i &agent, int goal, std::uniform_real_distribution<> &distribution );
};

#endif