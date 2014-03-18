#pragma once

#include <assert.h>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>

#include "../data/Enum.h"

/**
This class keeps a record of statistics for any class. The templates make it easy to
use in any class.

It contains two categories of stats
1) single values - these are values of type double
2) stratified values - these are vector<double>
	ex.  New infections stratified by CD4 strata
		or New infections by age bucket

All stats are stored in an internal data structure, but we can access them via enum id's
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

	/*
	// container for all statified statistics, which are stored in array form
	vector<vector<double>> stratifiedStats;
	EnumCls<StratifiedStatIDs>* stratifiedStatIDEnumCls;

	*/
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
	//------------< Begin Single value stats methods >---------------------//


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
	//------------< End Single value stats methods >---------------------//



	/*
	//------------< Begin Stratified stats methods >---------------------//


	*
		multiply _value to index _index of the internal stats vector that corresponds with _statID
		@param _statID statistic identifier
		@param _index index within the Stratified that will be changed

	*
	double getStat(StratifiedStatIDs _statID, int _index)  const;

	*
		add _value to index _index of the internal array that corresponds with _statID
		function will check to see whether _index is larger than the size of the internal array
		@param _statID statistic identifier
		@param _index index within the internal stats vector that will be changed
		@param _value the number we will add to the current value at _index
	*
	void incrStat(StratifiedStatIDs _statID, int _index, double _value);

	*
		multiply _value to index _index of the internal array that corresponds with _statID
		@param _statID statistic identifier
		@param _index index within the internal stats vector that will be changed
		@param _value the number we will multiply to the current value at _index

	*
	void multStat(StratifiedStatIDs _statID, int _index, double _value);


	*
		set the internal value of the statistic that corresponds to _value
	*
	void setStat(StratifiedStatIDs _statID, int _index, double _value);

	**
	returns true if 1) _statID is valid for this instance of StatsRecord
					2) _index is valid for valid _statID
		we are basically checking for out-of-bounds of internal data structures
	*
	bool validStatID(StratifiedStatIDs _statID, int _index) const;

	//------------< End Stratified stats methods >---------------------//
	*/

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
	assert(typeid(StratifiedStatIDs) == typeid(BaseEnumCls::nullptr_ENUM));
	statIDEnumCls = _statIDEnumCls;
	//make room internally to store numSingleStats values
	singleValStats.resize(_statIDEnumCls->getNumEnums(), 0.0);
}


/*
template<typename PointStatIDs, typename StratifiedStatIDs>
StatsRecord<PointStatIDs,StratifiedStatIDs>::StatsRecord(  EnumCls<PointStatIDs>* _statIDs,  EnumCls<StratifiedStatIDs> * _stratifiedStatIDs, size_t _stratifiedStatDims[]) {
	assert( typeid(StratifiedStatIDs) != typeid(BaseEnumCls::nullptr_ENUM));

	statIDEnumCls = _statIDs;
	stratifiedStatIDEnumCls = _stratifiedStatIDs;

	//make room internally to store numSingleStats values
	singleValStats.resize(_statIDs->getNumEnums(), 0.0);

	//make room internally to store numArrayStatIDs vectors
	stratifiedStats.resize(_stratifiedStatIDs->getNumEnums());

	//iterate through all stratified stats
	for(size_t i = 0; i < _stratifiedStatIDs.getNumEnums(); i++) {
		//make room internally to store numStrata values
		stratifiedStats.at(i).resize( _stratifiedStatDims[i], 0.0);
	}
}
*/

template<typename PointStatIDs, typename StratifiedStatIDs>
void StatsRecord<PointStatIDs, StratifiedStatIDs>::print(std::ostream &_outStream)
{
	//print out single value stats
	for(PointStatIDs i = PointStatIDs(0); i < statIDEnumCls->getNumEnums(); ++i)
	{
		statIDEnumCls->appendEnumStr(_outStream, i);
		_outStream << "\t" << singleValStats.at(i) << std::endl;
	}

	//print out array stats
	for(StratifiedStatIDs i = StratifiedStatIDs(0); i < stratifiedStatIDEnumCls->getNumEnums(); ++i)
	{
		stratifiedStatIDEnumCls->appendEnumStr(_outStream, i);
		_outStream << ":\t(";

		for(size_t j = 0; j < stratifiedStats.at(i).size(); j++)
		{
			_outStream << stratifiedStats.at(i).at(j) << "\t";
		}

		_outStream << ")" << std::endl;
	}
}

//------------< Begin Single value stats methods >---------------------//

template<typename PointStatIDs, typename StratifiedStatIDs>
double StatsRecord<PointStatIDs, StratifiedStatIDs>::getStat(PointStatIDs _statID)  const
{
	assert(validStatID(_statID));
	return singleValStats.at(_statID);
}

template<typename PointStatIDs, typename StratifiedStatIDs>
void StatsRecord<PointStatIDs, StratifiedStatIDs>::incrStat(PointStatIDs _statID, double _value)
{
	assert(validStatID(_statID));
	singleValStats.at(_statID) += _value;
}

template<typename PointStatIDs, typename StratifiedStatIDs>
void StatsRecord<PointStatIDs, StratifiedStatIDs>::multStat(PointStatIDs _statID, double _value)
{
	assert(validStatID(_statID));
	singleValStats.at(_statID) *= _value;
}

template<typename PointStatIDs, typename StratifiedStatIDs>
void StatsRecord<PointStatIDs, StratifiedStatIDs>::setStat(PointStatIDs _statID, double _value)
{
	assert(validStatID(_statID));
	singleValStats.at(_statID) = _value;
}

template<typename PointStatIDs, typename StratifiedStatIDs>
bool StatsRecord<PointStatIDs, StratifiedStatIDs>::validStatID(PointStatIDs _statID) const
{
	return (statIDEnumCls->isValidNonWildCard(_statID));
}

//------------< End Single value stats methods >---------------------//

/*
//------------< Begin Stratified stats methods >---------------------//
template<typename PointStatIDs, typename StratifiedStatIDs>
double StatsRecord<PointStatIDs,StratifiedStatIDs>::getStat(StratifiedStatIDs _statID, int _index)  const{
	assert( validStatID(_statID, _index) );
	return stratifiedStats.at(_statID).at(_index);
}

template<typename PointStatIDs, typename StratifiedStatIDs>
void StatsRecord<PointStatIDs,StratifiedStatIDs>::incrStat(StratifiedStatIDs _statID, int _index, double _value){
	assert( validStatID(_statID, _index) );
	stratifiedStats.at(_statID).at(_index) += _value;
}

template<typename PointStatIDs, typename StratifiedStatIDs>
void StatsRecord<PointStatIDs,StratifiedStatIDs>::multStat(StratifiedStatIDs _statID, int _index, double _value){
	assert( validStatID(_statID, _index) );
	stratifiedStats.at(_statID).at(_index) *= _value;
}

template<typename PointStatIDs, typename StratifiedStatIDs>
void StatsRecord<PointStatIDs,StratifiedStatIDs>::setStat(StratifiedStatIDs _statID, int _index, double _value){
	assert( validStatID(_statID, _index) );
	stratifiedStats.at(_statID).at(_index) = _value;
}

template<typename PointStatIDs, typename StratifiedStatIDs>
bool StatsRecord<PointStatIDs,StratifiedStatIDs>::validStatID(StratifiedStatIDs _statID, int _index) const{
	return ( stratifiedStatIDEnumCls->isValidNonWildCard(_statID)) &&
			 (_index >= 0) &&
			 (_index < stratifiedStats.at(_statID).size())
		   );
}

//------------< End Stratified stats methods >---------------------//

*/
