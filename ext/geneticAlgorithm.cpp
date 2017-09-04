#include "geneticAlgorithm.h"

GeneticAlgorithm::GeneticAlgorithm(int populationSize, int chromosomeSize) {
	mChromosomeSize = chromosomeSize;
	mNumGenerations = 0;

	mPopulation.resize(populationSize);
	std::for_each(mPopulation.begin(), mPopulation.end(), [=](Genome &i) { i.mGenes.resize(mChromosomeSize); });

	mRNG.seed(std::random_device{}());
	mDistribution_real = std::uniform_real_distribution<>(0.0, 1.0);
}

std::vector<Genome> GeneticAlgorithm::epoch(std::vector<Genome> &oldPopulation) {
	std::vector<Genome> newPopulation;
	newPopulation.reserve(oldPopulation.size());
	mPopulation = oldPopulation;

	std::sort(mPopulation.begin(), mPopulation.end());
	mTotalFitness = std::accumulate(mPopulation.begin(), mPopulation.end(), 0.0, [](double &sum, const Genome &i) { return sum + i.mFitness; });

	grabElites(mNumElites, mNumCopiesElite, newPopulation);

	/*
	 * Run generic/generational/simple Genetic Algorithm.
	 */
	while (newPopulation.size() < mPopulation.size()) {
		Genome mom = getChromosomeRoulette();
		Genome dad = getChromosomeRoulette();
		arrayNi baby1, baby2;

		crossover(mom.mGenes, dad.mGenes, baby1, baby2);

		mutate(baby1);
		mutate(baby2);

		newPopulation.push_back(Genome(baby1, 0.0));
		newPopulation.push_back(Genome(baby2, 0.0));
	}

	mNumGenerations++;

	return newPopulation;
}

void GeneticAlgorithm::crossover(const arrayNi &mom, const arrayNi &dad, arrayNi &baby1, arrayNi &baby2) {
	if (mDistribution_real(mRNG) > mCrossoverRate || mom == dad) {
		baby1 = mom;
		baby2 = dad;
		return;
	}

	std::uniform_int_distribution<> distribution_int(0, mChromosomeSize - 1);
	int crossoverPoint = distribution_int(mRNG);
	for (int i = 0; i < crossoverPoint; i++) {
		baby1.push_back(mom[i]);
		baby2.push_back(dad[i]);
	}
	for (int i = crossoverPoint; i < mChromosomeSize; i++) {
		baby1.push_back(dad[i]);
		baby2.push_back(mom[i]);
	}
}

void GeneticAlgorithm::mutate(arrayNi &chromosome) {
	std::uniform_int_distribution<> distribution_int(0, mMaxGeneValue);
	for (size_t i = 0; i < chromosome.size(); i++) {
		if (mDistribution_real(mRNG) < mMutationRate)
			chromosome[i] = distribution_int(mRNG);
	}
}

Genome GeneticAlgorithm::getChromosomeRoulette() {
	double slice = mDistribution_real(mRNG) * mTotalFitness;

	Genome chosenChromosome;
	double fitnessSoFar = 0.0;
	for (size_t i = 0; i < mPopulation.size(); i++) {
		fitnessSoFar += mPopulation[i].mFitness;
		if (fitnessSoFar >= slice) {
			chosenChromosome = mPopulation[i];
			break;
		}
	}

	return chosenChromosome;
}

void GeneticAlgorithm::grabElites(int numElites, int numCopiesElite, std::vector<Genome> &population) {
	while (numElites--) {
		for (int i = 0; i < numCopiesElite; i++)
			population.push_back(mPopulation[(mPopulation.size() - 1) - numElites]);
	}
}