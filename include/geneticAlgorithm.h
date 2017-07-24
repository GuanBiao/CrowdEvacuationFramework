//
// Reference code: https://github.com/Arkq/smart-sweepers-qt.
//

#ifndef __GENETICALGORITHM_H__
#define __GENETICALGORITHM_H__

#include <random>
#include <algorithm>
#include <numeric>

#include "container.h"

struct Genome {
	arrayNi mGenes;
	double mFitness;

	Genome() : mFitness(0.0) {}
	Genome(arrayNi genes, double fitness) : mGenes(genes), mFitness(fitness) {}

	friend bool operator<(const Genome &lhs, const Genome &rhs) {
		return lhs.mFitness < rhs.mFitness;
	}
};

class GeneticAlgorithm {
public:
	int mChromosomeSize;
	int mMaxGeneValue;
	int mNumGenerations;
	double mCrossoverRate;
	double mMutationRate;
	int mNumElites;
	int mNumCopiesElite;

	GeneticAlgorithm( int populationSize, int chromosomeSize );
	std::vector<Genome> epoch( std::vector<Genome> &oldPopulation );

private:
	std::vector<Genome> mPopulation;
	double mTotalFitness;
	std::mt19937 mRNG;
	std::uniform_real_distribution<> mDistribution_real;

	void crossover( const arrayNi &mom, const arrayNi &dad, arrayNi &baby1, arrayNi &baby2 );
	void mutate( arrayNi &chromosome );
	Genome getChromosomeRoulette();
	void grabElites( int numElites, int numCopiesElite, std::vector<Genome> &population );
};

#endif