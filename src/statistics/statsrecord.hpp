#ifndef STATSRECORD_HPP
#define STATSRECORD_HPP

#include <assert.h>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>

#include "utility/enum.hpp"

namespace transm {

/**
 * This class keeps a record of statistics for any class. The templates make it easy to
 * use in any class.
 *
 * It contains two categories of stats
 * 1) single values - these are values of type double
 * 2) stratified values - these are vector<double>
 *	ex.  New infections stratified by CD4 strata
 *		or New infections by age bucket

 *  All stats are stored in an internal data structure, but we can access them via enum id's
 	typename PointStatIDs for single value stats
	typename StratifiedStatIDs for stratified value stats

This class was created so that statistics could be contained inside an object
Otherwise, the number of fields in say a Population object or a Person would be a lot

***/
template<typename PointStatIDs, typename StratifiedStatIDs>
class StatsRecord
{
	//any stats that can be represented as a single value
	std::vector<double> singleValStats;
	EnumCls<PointStatIDs> *statIDEnumCls;

public:
	/**
		Saves the an instance of EnumCls that wraps the enumerated stats that we are using

		Also initializes the data structures needed to store the statistics

		@param _singleValStatsLabels labels for single value stats
		@param _stratifiedStatLabels labels for the aggregate stats
		@param _stratifiedStatIndicesLabels labels for each individual strata within each aggregate stat
	**/
	StatsRecord();
	StatsRecord(EnumCls<PointStatIDs> *statIDEnumCls);
	//StatsRecord( EnumCls<PointStatIDs> *statIDEnumCls, EnumCls<StratifiedStatIDs> *stratifiedStatID, size_t _stratifiedStatDims[]);
	~StatsRecord();

	void print(std::ostream &_outStream);

	//get internal value that corresponds with _statID
	double getStat(PointStatIDs _statID) const;

	/*
		add _value to the internal value that corresponds to _statID label
		@param _statID statistic identifier
		@param _value the number we will add to the current value at _index
	*/
	void incrStat(PointStatIDs _statID, double _value);

	/**

	**/
	void init(EnumCls<PointStatIDs> *statIDEnumCls);

	/*
		multiply _value to the internal value that corresponds to _statID label
		@param _statID statistic identifier
		@param _value the number we will multiply with the current value at _index
	*/
	void multStat(PointStatIDs _statID, double _value);


	/*
		set the value of the statistic that corresponds with _statID to _value
	*/
	void setStat(PointStatIDs _statID, double _value);

	//returns true if _statID is valid for this instance of StatsRecord
	bool validStatID(PointStatIDs _statID) const;

};

template<typename PointStatIDs, typename StratifiedStatIDs>
StatsRecord<PointStatIDs, StratifiedStatIDs>::StatsRecord()
{
}

template<typename PointStatIDs, typename StratifiedStatIDs>
StatsRecord<PointStatIDs, StratifiedStatIDs>::StatsRecord(EnumCls<PointStatIDs> *_statIDs)
{
	init(_statIDs);
}

template<typename PointStatIDs, typename StratifiedStatIDs>
StatsRecord<PointStatIDs, StratifiedStatIDs>::~StatsRecord()
{
	//delete statIDEnumCls;
}

template<typename PointStatIDs, typename StratifiedStatIDs>
void StatsRecord<PointStatIDs, StratifiedStatIDs>::init(EnumCls<PointStatIDs> *_statIDEnumCls)
{
	assert(typeid(StratifiedStatIDs) == typeid(BaseEnumCls::NULL_ENUM));
	statIDEnumCls = _statIDEnumCls;
	//make room internally to store numSingleStats values
	singleValStats.resize(_statIDEnumCls->getNumEnums(), 0.0);
}

template<typename PointStatIDs, typename StratifiedStatIDs>
double StatsRecord<PointStatIDs, StratifiedStatIDs>::getStat(PointStatIDs _statID)  const
{
	assert(validStatID(_statID));
    return singleValStats.at((std::size_t)_statID);
}

template<typename PointStatIDs, typename StratifiedStatIDs>
void StatsRecord<PointStatIDs, StratifiedStatIDs>::incrStat(PointStatIDs _statID, double _value)
{
	assert(validStatID(_statID));
    singleValStats.at((std::size_t)_statID) += _value;
}

template<typename PointStatIDs, typename StratifiedStatIDs>
void StatsRecord<PointStatIDs, StratifiedStatIDs>::multStat(PointStatIDs _statID, double _value)
{
	assert(validStatID(_statID));
    singleValStats.at((std::size_t)_statID) *= _value;
}

template<typename PointStatIDs, typename StratifiedStatIDs>
void StatsRecord<PointStatIDs, StratifiedStatIDs>::setStat(PointStatIDs _statID, double _value)
{
	assert(validStatID(_statID));
    singleValStats.at((std::size_t)_statID) = _value;
}

template<typename PointStatIDs, typename StratifiedStatIDs>
bool StatsRecord<PointStatIDs, StratifiedStatIDs>::validStatID(PointStatIDs _statID) const
{
    return (statIDEnumCls->isValidNonWildCard((std::size_t)_statID));
}

} // namespace transm

#endif /* STATSRECORD_HPP */