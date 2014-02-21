#pragma once

#include "./ticpp/ticpp.h"
#include "./../Constants.h"
#include "./rand/RandomNums.h"
#include "boost/lexical_cast.hpp"
class XMLUtil
{
public :


	//adds a child element to the end of the children list, the child must
	//  have at most 1 value and no attributes
	template <class T>
	static void addSimpleXMLLeaf(ticpp::Element *_elem, string _childTag, T _value)
	{
		ticpp::Element child(_childTag);
		child.SetText(_value);
		_elem->InsertEndChild(child);
	}

	static void getDistFromXMLNode(ticpp::Element *_distContainingNode, NormalDist &_normalDist)
	{
		ticpp::Element *distParams = _distContainingNode->FirstChildElement("Distrib");
		distParams->FirstChildElement("mean")->GetText<double>(&_normalDist.mean);
		distParams->FirstChildElement("stdDev")->GetText<double>(&_normalDist.stddev);
	}

	static void getLogNormalDistFromXMLNode(ticpp::Element *_distContainingNode, LogNormalDist &_logNormalDist,
	                                        bool useCoeffVar = false, double coeffVar = 0.0)
	{
		ticpp::Element *distParams = _distContainingNode->FirstChildElement("Distrib");

		try
		{
			distParams->FirstChildElement("mu")->GetText<double>(&_logNormalDist.mu);
			distParams->FirstChildElement("sigma")->GetText<double>(&_logNormalDist.sigma);
		}
		catch(ticpp::Exception e)
		{
			double mean;
			double stddev;
			distParams->FirstChildElement("mean")->GetText<double>(&mean);

			if(mean <= 0)
			{
				_logNormalDist.mu = 0;
				_logNormalDist.sigma = 0;
				_logNormalDist.isZeroDistrib = true;
				return;
			}

			if(useCoeffVar)
			{
				stddev = mean * coeffVar;
			}
			else
			{
				distParams->FirstChildElement("stdDev")->GetText<double>(&stddev);
			}

			_logNormalDist.mu = log(mean) - 0.5 * log(1 + (stddev * stddev) / (mean * mean));
			_logNormalDist.sigma = sqrt(log(1 + (stddev * stddev) / (mean * mean)));
		}
	}

	static void getShiftedLogNormalDistFromXMLNode(ticpp::Element *_distContainingNode,
	        ShiftedLogNormalDist &_shiftedLogNormalDist)
	{
		ticpp::Element *distParams = _distContainingNode->FirstChildElement("Distrib");

		try
		{
			distParams->FirstChildElement("mu")->GetText<double>(&_shiftedLogNormalDist.mu);
			distParams->FirstChildElement("sigma")->GetText<double>(&_shiftedLogNormalDist.sigma);
			distParams->FirstChildElement("shift")->GetText<double>(&_shiftedLogNormalDist.shift);
		}
		catch(ticpp::Exception e)
		{
		    double mean = 0;
		    double stddev = 0;
		    double shift = 0;
			distParams->FirstChildElement("mean")->GetText<double>(&mean);

			if(mean <= 0)
			{
				_shiftedLogNormalDist.mu = 0;
				_shiftedLogNormalDist.sigma = 0;
				_shiftedLogNormalDist.shift = 0;
				_shiftedLogNormalDist.isZeroDistrib = true;
				return;
			}

			distParams->FirstChildElement("stdDev")->GetText<double>(&stddev);
			distParams->FirstChildElement("shift")->GetText<double>(&shift);
			_shiftedLogNormalDist.mu = log(mean - shift) - 0.5 * log(1 + (stddev * stddev) / ((mean - shift) * (mean - shift)));
			_shiftedLogNormalDist.sigma = sqrt(log(1 + (stddev * stddev) / ((mean - shift) * (mean - shift))));
			_shiftedLogNormalDist.shift = shift;
		}
	}

	static void getBetaDistFromXMLNode(ticpp::Element *_distContainingNode, BetaDist &_betaDist, bool useCoeffVar = false,
	                                   double coeffVar = 0.0)
	{
		ticpp::Element *distParams = _distContainingNode->FirstChildElement("Distrib");

		try
		{
		        distParams->FirstChildElement("alpha")->GetText<double>(&_betaDist.alpha);
			distParams->FirstChildElement("beta")->GetText<double>(&_betaDist.beta);
		}
		catch(ticpp::Exception e)
		{
			double mean;
			double stddev;

			if(useCoeffVar)
			{
				distParams->FirstChildElement("mean")->GetText<double>(&mean);
				stddev = mean * coeffVar;
			}
			else
			{
				distParams->FirstChildElement("mean")->GetText<double>(&mean);
				distParams->FirstChildElement("stdDev")->GetText<double>(&stddev);
			}

			double sampleSize = mean * (1 - mean) / (stddev * stddev) - 1;
			_betaDist.alpha = mean * sampleSize;
			_betaDist.beta = (1 - mean) * sampleSize;
		}
	}


	static double getExpDistMeanFromXMLNode(ticpp::Element *_distContainingNode)
	{
		ticpp::Element *distParams = _distContainingNode->FirstChildElement("Distrib");
		double mean = 0;
		distParams->FirstChildElement("mean")->GetText<double>(&mean);
		return mean;
	}

	static double getPoissonDistMeanFromXMLNode(ticpp::Element *_distContainingNode)
	{
		ticpp::Element *distParams = _distContainingNode->FirstChildElement("Distrib");
		double mean = 0;
		distParams->FirstChildElement("mean")->GetText<double>(&mean);
		return mean;
	}


	/**
		parses a "valsByHVL" element, which basically has tab delimited string of values
		and generates a vector of the tokenized values (not including tabs)
	**/
	static void getTabDelimitedNode(ticpp::Element *_tabDelimitedNode, vector<double> &_vals)
	{
		//get the transmission coefficients
		double token;
		stringstream ss(_tabDelimitedNode->GetText());

		while(ss >> token)
		{
			_vals.push_back(token);
		}
	}


	template <class T>
	static string fromArray(T *_array, int _size, string _delim = Constants::TAB)
	{
		std::ostringstream result;

		for(int i = 0; i < _size; i++)
		{
			result << _array[i];
			result << Constants::TAB;
		}

		return result.str();
	}

	template <class T>
	static string fromVector(vector<T> &_vector, string _delim = Constants::TAB)
	{
		std::ostringstream result;

		for(int i = 0; i < _vector.size(); i++)
		{
			result << _vector.at(i);
			result << Constants::TAB;
		}

		return result.str();
	}

	static void printNormalValues(NormalDist _normalDist)
	{
		cout << "{" << _normalDist.mean << "," << _normalDist.stddev << "}";
	}

	static void printParam(string _prefix, string _paramName, double _param, EventParams &_eventParams)
	{
		_eventParams.displayOut(_prefix.c_str());
		_eventParams.displayOut(_paramName.c_str());
		_eventParams.displayOut(" : ");
		_eventParams.displayOut(boost::lexical_cast<std::string>(_param).c_str());
		_eventParams.displayOut("\n");
	}

	static void printParam(string _prefix, string _paramName, NormalDist _normalDist)
	{
		cout << _prefix << _paramName << " : ";
		printNormalValues(_normalDist);
		cout << endl;
	}

};
